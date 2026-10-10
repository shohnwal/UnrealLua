// Fill out your copyright notice in the Description page of Project Settings.

#include "LuaTypes/LuaScriptStruct.h"
#include "Utility/LuaLogMacros.h"
#include "LuaTypes/LuaInstancedStruct.h"
#include "LuaTypes/LuaUStruct.h"
#include "sol/sol.hpp"
#include "Reflection/PropertyHelper.h"
#include "Reflection/PropertyHelperTypes.h"

#include "LuaCoreDelegates.h"
#include "LuaTypes/LuaSharedStruct.h"
#include "UObjectRegistry/LuaUObjectItem.h"
#include "StructUtils/InstancedStruct.h"

static const FDelegateHandle fLuaScriptStructLuaTypeHandle = FLuaCoreDelegates::OnRegisterLuaUsertypes.AddStatic(&FLuaScriptStruct::RegisterUsertype);

FLuaScriptStructMemory* FLuaScriptStructMemory::Allocate(const UScriptStruct* InScriptStruct, const void* memToCopyFrom)
{
	// Align RequiredSize to InScriptStruct's alignment to effectively add padding in between ScriptStruct and
	// StructMemory. GetMemory will then round &StructMemory up past this 'padding' to the nearest aligned address.
	const int32 baseSize = static_cast<int32>(Align(sizeof(FLuaScriptStructMemory), InScriptStruct->GetMinAlignment()));
	const int32 RequiredSize = baseSize + InScriptStruct->GetStructureSize();
	// Code analysis is unable to understand correctly what we are doing here, so disabling the warning C6386: Buffer overrun while writing to...
	CA_SUPPRESS( 6386 )
	void* newMem = FMemory::Malloc(RequiredSize, InScriptStruct->GetMinAlignment());
	FLuaScriptStructMemory* StructMemory = new(newMem) FLuaScriptStructMemory(InScriptStruct, memToCopyFrom);
	return StructMemory;
}


FLuaScriptStructMemory::FLuaScriptStructMemory(const UScriptStruct* ss, const void* memToCopyFrom)
	: FLuaGCObject(), ScriptStruct(ss)
{
	ScriptStruct->InitializeStruct(this->GetMemory());
		
	if (memToCopyFrom)
	{
		ScriptStruct->CopyScriptStruct(this->GetMemory(), memToCopyFrom);
	}
}


FLuaScriptStructMemory::~FLuaScriptStructMemory()
{
	ScriptStruct->DestroyStruct(this->GetMemory());
}

void FLuaScriptStructMemory::AddReferencedObjects(FReferenceCollector& Collector)
{
	//LUA_LOG("Adding references in %s", *GetNameSafe(this->GetScriptStruct()));
	if(this->ScriptStruct && this->GetMemory())
	{
	
		Collector.AddReferencedObject(this->ScriptStruct);
		Collector.AddReferencedObjects(this->ScriptStruct, this->GetMemory(), nullptr);
		//Collector.AddPropertyReferencesWithStructARO(this->GetScriptStruct(), this->GetMemory());
	}
}

uint8* FLuaScriptStructMemory::GetMemory() const
{
	return Align(const_cast<uint8*>(this->Data), ScriptStruct->GetMinAlignment());
}

const UScriptStruct* FLuaScriptStructMemory::GetScriptStruct() const
{
	return this->ScriptStruct;
}

void FLuaScriptStructMemory::AddRef()
{
	this->RefCount++;
	verifyf(this->RefCount >= 0, TEXT("RefCount of FLuaScriptStructMemory %p is %d"), this, this->RefCount);
}

int32 FLuaScriptStructMemory::RemoveRef()
{
	this->RefCount--;
	verifyf(this->RefCount >= 0, TEXT("RefCount of FLuaScriptStructMemory %p is %d"), this, this->RefCount);
	return this->RefCount;
}

void FLuaScriptStruct::RegisterUsertype(sol::state_view& lua)
{
	lua.new_usertype<FLuaScriptStruct>(
		"FLuaScriptStruct",
		sol::base_classes, sol::bases<FLuaScriptStructBase>(),
		sol::call_constructor, sol::factories
		(
			[]() { return sol::nil; },
			[](const std::string& path, sol::this_state lua)
			{
				return FLuaScriptStruct::MakeFromPath(path, lua);
			},
			[](const FLuaScriptStruct& other) { return FLuaScriptStruct(other); },
			[](const FLuaUStruct& metaData, sol::variadic_args args) { return FLuaScriptStruct(&metaData, args);} ,
			[](const FLuaInstancedStruct& instanced) { return FLuaScriptStruct(instanced.GetScriptStruct(), static_cast<void*>(instanced.GetMemory()), false); }
		),
		"Get", &FLuaScriptStruct::GetPropertyValues,
		"Set", &FLuaScriptStruct::SetPropertyValues,
		"Copy", &FLuaScriptStruct::Lua_Copy,
		"MakeInstanced", &FLuaInstancedStruct::MakeFromDataStruct,
		"IsReference", &FLuaScriptStruct::IsReference,
		sol::meta_function::index, sol::c_call<decltype(&FLuaScriptStruct::__index), &FLuaScriptStruct::__index>,
		sol::meta_function::new_index, sol::c_call<decltype(&FLuaScriptStruct::__newindex), &FLuaScriptStruct::__newindex>,
		sol::meta_function::equal_to, &FLuaScriptStruct::__equals,
		sol::meta_function::less_than_or_equal_to, &FLuaScriptStructBase::__le,
		"is", []() { LUA_LOG("this is a FLuaScriptStruct") },
		sol::meta_function::to_string, [](FLuaScriptStruct* self){ return __toString(self); }
	);
}

FLuaScriptStruct::FLuaScriptStruct()
	: FLuaScriptStructBase(nullptr)
{
	checkNoEntry();
}

FLuaScriptStruct::FLuaScriptStruct(const UScriptStruct* metaStruct)
	: FLuaScriptStructBase(metaStruct)
{
	verify(this->PropertyMapping != nullptr);
	this->MemoryVariant.Emplace<FLuaScriptStructMemory*>(FLuaScriptStructMemory::Allocate(metaStruct, nullptr));
	this->AddRef();
}

extern const FName HasNativeMakeMetaDataKey;
extern const FName NativeBreakFuncMetaDataKey;

FLuaScriptStruct::FLuaScriptStruct(const FLuaUStruct* metaData, sol::variadic_args args)
	: FLuaScriptStructBase(Cast<UScriptStruct>(metaData->TryLoad())), MemoryVariant()
{
	verify(this->PropertyMapping != nullptr);
	const UScriptStruct* ss = metaData->TryLoad();
	this->MemoryVariant.Emplace<FLuaScriptStructMemory*>(FLuaScriptStructMemory::Allocate(ss, nullptr));
	this->AddRef();

	//Early out if there are no arguments, no need to loop over all properties,  instead initialize them to defaults
	if(!args.size())
	{
		return;
	}
	
	//Python-like assignment per key-value table
	if(args.size() == 1 && args[0].get_type() == sol::type::table)
	{
		sol::table argtbl = args[0].get<sol::table>();
		verify(argtbl.valid())
		UnrealLua::PropertyHelper::InitializeStructFromTable(ss, this->GetMemoryNonVirtual(), argtbl);
	}
	//Classic initialization list
	else
	{
		int32 index = 0;
		for(TFieldIterator<FProperty> propIt(ss); propIt; ++propIt)
		{
			sol::stack_object value = args[index];
			TSetPropertyValueParams params{*propIt, this->GetMemoryNonVirtual(), 0, value };
			UnrealLua::PropertyHelper::SetPropertyValue_InContainer(params);
			index++;
		}	
	}
}

//copy constructor
FLuaScriptStruct::FLuaScriptStruct(const FLuaScriptStruct& other)
	: FLuaScriptStructBase(other.GetScriptStruct()),  MemoryVariant(other.MemoryVariant)
{
	verify(this->PropertyMapping != nullptr);
	if(this->OwnsMemory())
	{
		//we own memory, so just increase ref counter
		this->AddRef();
	}
}

FLuaScriptStruct::FLuaScriptStruct(FLuaScriptStruct&& other) noexcept
	: FLuaScriptStructBase(other.GetScriptStruct()), MemoryVariant(other.MemoryVariant)
{
	if(this->OwnsMemory())
	{
		//must add ref BEFORE decreasing ref from other, to keep data alive
		this->AddRef();
	}
	verify(this->PropertyMapping != nullptr);
	other.Reset();
	verify(other.MemoryVariant.IsType<std::nullptr_t>());
	verify(other.PropertyMapping == nullptr);
}

//reference constructor
//used for
//	- pass-by-ref in function calls
//	- getting a class member property
FLuaScriptStruct::FLuaScriptStruct(const UScriptStruct* metaStruct, void* otherMemory, bool asReference, bool bIsConst)
	: FLuaScriptStructBase(metaStruct)
	, MemoryVariant()
{
	verify(this->PropertyMapping != nullptr);
	if(asReference)
	{
		this->MemoryVariant.Emplace<void*>(static_cast<uint8*>(const_cast<void*>(otherMemory)));
	}
	else
	{
		this->MemoryVariant.Emplace<FLuaScriptStructMemory*>(FLuaScriptStructMemory::Allocate(metaStruct, otherMemory));
		this->AddRef();
	}
}

FLuaScriptStruct::FLuaScriptStruct(const UScriptStruct* metaStruct, const void* otherMemory, bool asReference)
	: FLuaScriptStructBase(metaStruct)
	, MemoryVariant()
{
	verify(this->PropertyMapping != nullptr);
	if(asReference)
	{
		this->MemoryVariant.Emplace<void*>(static_cast<uint8*>(const_cast<void*>(otherMemory)));
	}
	else
	{
		this->MemoryVariant.Emplace<FLuaScriptStructMemory*>(FLuaScriptStructMemory::Allocate(metaStruct, otherMemory));
		this->AddRef();
	}
}

//Copy from a UProperty (class or function)
FLuaScriptStruct::FLuaScriptStruct(FStructProperty * prop, const void * sourcePtr)
	: FLuaScriptStructBase(prop->Struct)
	, MemoryVariant()
{
	verify(this->PropertyMapping != nullptr);
	const UScriptStruct* ss = prop->Struct;
	this->MemoryVariant.Emplace<FLuaScriptStructMemory*>(FLuaScriptStructMemory::Allocate(ss, sourcePtr));
	this->AddRef();
}

FLuaScriptStruct::~FLuaScriptStruct()
{
	this->Reset();
}

bool FLuaScriptStruct::IsInitialized() const
{
	return this->IsValid();
}

sol::object FLuaScriptStruct::MakeFromPath(const std::string& path, sol::this_state lua_)
{
	sol::state_view lua{lua_};
	sol::object ustruct_o = lua["UE"][path];
	if(!ustruct_o.is<FLuaUStruct>())
	{
		return sol::nil;
	}
	FLuaUStruct& ustruct = ustruct_o.as<FLuaUStruct&>();
	UScriptStruct* ss = Cast<UScriptStruct>(ustruct.TryLoad());
	if(!ss)
	{
		return sol::nil;
	}
	return sol::object(lua, sol::in_place_type<FLuaScriptStruct>, FLuaScriptStruct{ss});
}


void FLuaScriptStruct::Reset()
{
	this->RemoveRef();
	this->MemoryVariant.Emplace<std::nullptr_t>();
	this->PropertyMapping = nullptr;
}

int FLuaScriptStruct::__index(lua_State* lua)
{
	sol::stack_object self{lua,1};
	sol::stack_object key{lua,2};
	if(!key.valid())
	{
		return sol::stack::push(lua, sol::nil);
	}

	FLuaScriptStruct* strct = self.as<FLuaScriptStruct*>();
	if(!strct)
	{
		return sol::stack::push(lua, sol::nil);
	}
	return UnrealLua::PropertyHelper::GetValueFromScriptStructProperty(key, *strct);	
}

bool FLuaScriptStruct::__newindex(FLuaScriptStruct * strct, sol::stack_object key, sol::stack_object value, sol::this_state lua)
{
	if(!strct)
	{
		return false;
	}
	return UnrealLua::PropertyHelper::SetValueInScriptStructProperty(key, *strct, value);
}

bool FLuaScriptStruct::__equals(FLuaScriptStruct* me, FLuaScriptStruct* other)
{
	LUA_LOG("Comparing structs %p %hs and %p %hs", me->GetMemory(), me->__toString(me).data(), other->GetMemory(), other->__toString(other).data())
	return me == other || (me->GetScriptStruct() == other->GetScriptStruct() && me->GetScriptStruct()->CompareScriptStruct(me->GetMemory(), other->GetMemory(), 0));
}

std::string FLuaScriptStruct::__toString(FLuaScriptStruct* me)
{
	if(!me->GetMemory() || !me->GetScriptStruct())
	{
		return "Invalid struct data";
	}
	const UScriptStruct* ss = me->GetScriptStruct();

	FString Output = TEXT("");
	ss->ExportText(Output, me->GetMemory(), nullptr, nullptr, (PPF_ExportsNotFullyQualified | PPF_Copy | PPF_Delimited | PPF_IncludeTransient), nullptr);
	
	FString val = me->GetScriptStruct()->GetStructCPPName() + ":\n" + Output;
	return StringCast<char>(*val).Get();
}

FString FLuaScriptStruct::ToLuaSyntaxValueString() const
{
	if(!this->GetMemory() || !this->GetScriptStruct())
	{
		return "";
	}
	const UScriptStruct* ss = this->GetScriptStruct();

	FString Output = TEXT("");
	ss->ExportText(Output, this->GetMemory(), nullptr, nullptr, (PPF_ExportsNotFullyQualified | PPF_Copy | PPF_Delimited | PPF_IncludeTransient), nullptr);
	Output.ReplaceCharInline('(', '{');
	Output.ReplaceCharInline(')', '}');
	FString val = this->GetScriptStruct()->GetStructCPPName() + "{" + Output + "}";
	return val;
}

sol::object FLuaScriptStruct::Lua_Copy(sol::this_state lua) const
{
	return sol::object(lua, sol::in_place_type<FLuaScriptStruct>,this->GetScriptStruct(),this->GetMemory(), false, false);
}

FLuaScriptStruct FLuaScriptStruct::MakeCopy() const
{
	return FLuaScriptStruct{this->GetScriptStruct(), this->GetMemory(), false, false};
}

sol::variadic_results FLuaScriptStruct::GetPropertyValues(sol::variadic_args propNames)
{
	sol::variadic_results results;
	sol::this_state lua{propNames.lua_state()};
	if (!this->GetMemory() || !this->GetScriptStruct() || propNames.size() == 0) [[unlikely]]
	{
		return results;;
	}
	results.reserve(propNames.size());
	for(const sol::object& key : propNames)
	{
		results.emplace_back(UnrealLua::PropertyHelper::GetValueFromScriptStructProperty(key, *this, lua));
	}
	return results;
}

void FLuaScriptStruct::SetPropertyValues(sol::table tbl)
{
	if (!this->GetMemory() || !this->GetScriptStruct() || !tbl.valid()) [[unlikely]]
	{
		return;
	}
	UnrealLua::PropertyHelper::InitializeStructFromTable(this->GetScriptStruct(), this->GetMemory(), tbl);
}

void* FLuaScriptStruct::GetMemoryNonVirtual() const
{
	if(this->OwnsMemory())
	{
		return this->MemoryVariant.Get<FLuaScriptStructMemory*>()->GetMemory();
	}
	else if (this->MemoryVariant.IsType<void*>())
	{
		return this->MemoryVariant.Get<void*>();
	}
	return nullptr;
}

void* FLuaScriptStruct::GetMemory() const
{
	if(this->OwnsMemory())
	{
		return this->MemoryVariant.Get<FLuaScriptStructMemory*>()->GetMemory();
	}
	else if (this->MemoryVariant.IsType<void*>())
	{
		return this->MemoryVariant.Get<void*>();
	}
	return nullptr;
}

bool FLuaScriptStruct::IsReference() const
{
	return !this->OwnsMemory();
}

void FLuaScriptStruct::AddRef()
{
	verify(this->MemoryVariant.IsType<FLuaScriptStructMemory*>());
	this->MemoryVariant.Get<FLuaScriptStructMemory*>()->AddRef();
}

int32 FLuaScriptStruct::RemoveRef()
{
	if(this->OwnsMemory())
	{
		if(this->MemoryVariant.Get<FLuaScriptStructMemory*>()->RemoveRef() == 0)
		{
			//this->LuaMemory->~FLuaScriptStructMemory();
			//FMemory::Free(this->LuaMemory);
			delete this->MemoryVariant.Get<FLuaScriptStructMemory*>();
			this->MemoryVariant.Emplace<std::nullptr_t>(nullptr);
		}
	}
	return -1;
}

bool FLuaScriptStruct::OwnsMemory() const
{
	return this->MemoryVariant.IsType<FLuaScriptStructMemory*>();
}

const UScriptStruct* FLuaScriptStruct::GetScriptStruct() const
{
	return this->OwnsMemory() ? this->MemoryVariant.Get<FLuaScriptStructMemory*>()->GetScriptStruct() : this->PropertyMapping != nullptr ? Cast<UScriptStruct>(this->PropertyMapping->OwningField) : nullptr;
}
