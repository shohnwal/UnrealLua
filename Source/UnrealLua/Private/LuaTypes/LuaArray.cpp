// Fill out your copyright notice in the Description page of Project Settings.


#include "LuaTypes/LuaArray.h"
#include "Utility/UnrealVersion.h"
#include "LuaCoreDelegates.h"
#include "Utility/LuaLogMacros.h"
#include "PropertyTypeCompatibility.h"
#include "Config/UnrealLuaConstants.h"
#include "Config/UnrealLua_CompilerFlags.h"
#include "Reflection/PropertyHelper.h"
#include "Reflection/PropertyHelperTypes.h"
#include "utility/to_string.hpp"

/*
local myFac = self.EntityInfo.Faction
local isEnemy = function(entity) return entity.Faction:IsEnemyWith(myFac) end
local randomTarget = self:GetEntitiesInRadius():Filter(isEnemy):Any()
 */


static const FDelegateHandle fLuaArrayLuaTypeHandle = FLuaCoreDelegates::OnRegisterLuaUsertypes.AddStatic(&FLuaArray::RegisterUsertype);
// Create copy, used for passing tarray from Blueprint to Lua as a function argument / function return
//const auto FLuaArray_Usertype_Index = FLuaUsertypes::LuaUserTypesCallbacks.Add(&FLuaArray::AddLuaUsertype);

FLuaScriptArray::FLuaScriptArray(FProperty* inner)
{
	this->Inner.Reset(inner);
	TArray<const FStructProperty*> encounteredStructProps;
	this->bContainsObjectReferences = UnrealLua::PropertyHelper::CanPropertyContainObjectReferences(this->Inner.Get()) && this->Inner->ContainsObjectReference(encounteredStructProps);
	verify(this->IsEmpty());
}

FLuaScriptArray::~FLuaScriptArray()
{
	FScriptArrayHelper helper = FScriptArrayHelper::CreateHelperFormInnerProperty(this->Inner.Get(), static_cast<FScriptArray*>(this));
	helper.EmptyValues();
	this->RefCount = 0;
	this->Inner.Reset();
}

void FLuaScriptArray::AddReferencedObjects(FReferenceCollector& Collector)
{
	verify(this->Inner != nullptr);
	if(!bContainsObjectReferences)
	{
		return;
	}
	if (!this->IsEmpty())
	{
		return;
	}
	TArray<const FStructProperty*> encounteredStructProps;
#if UNREALLUA_UE_VERSION_OLDER_THAN(5,8,0)
	if(!this->Inner->ContainsObjectReference(encounteredStructProps, EPropertyObjectReferenceType::Any))
#else
	if(!this->Inner->ContainsObjectReference(encounteredStructProps, EPropertyObjectReferenceType_Any))
#endif
	{
		return;
	}

	FProperty* inner = this->Inner.Get();
	FScriptArrayHelper helper = FScriptArrayHelper::CreateHelperFormInnerProperty(inner, this);
	uint32 numReferenced = 0;
	for (int32 index = this->Num() - 1; index >= 0; index--)
	{
		void* ptr = helper.GetElementPtr(index);
		numReferenced += UnrealLua::PropertyHelper::AddRefByProperty(Collector, inner, ptr);
	}
}

void FLuaScriptArray::AddRef()
{
	this->RefCount++;
	verifyf(this->RefCount >= 0, TEXT("RefCount of FLuaArrayMemory %p is %d"), this, this->RefCount);
}

int32 FLuaScriptArray::RemoveRef()
{
	this->RefCount--;
	verifyf(this->RefCount >= 0, TEXT("RefCount of FLuaArrayMemory %p is %d"), this, this->RefCount);
	return this->RefCount;
}

void FLuaArray::RegisterUsertype(sol::state_view& lua)
{
	sol::usertype<FLuaArray> ut = lua.new_usertype<FLuaArray>(
	"TArray",
	"new", sol::no_constructor,
	sol::call_constructor, [](sol::object innerType, sol::object initial, sol::this_state lua) ->sol::object
	{
		FProperty* innerprop = UnrealLua::PropertyHelper::CreateNewProperty(innerType, UnrealLua::PropertyNames::NAME_UnrealLuaArrayInnerProperty);
		if(!innerprop)
		{
			std::string innerTypeStr = sol::utility::to_string(innerType);
			LUA_LOG("Unable to create TArray with inner %hs. Returning nil.", innerTypeStr.c_str())
			return sol::nil;
		}
		verify(innerprop->GetFName() == UnrealLua::PropertyNames::NAME_UnrealLuaArrayInnerProperty);
		verify(innerprop->Owner == nullptr);
		//the created innerprop will be assigned to the FLuaArray in the constructor
		return sol::object(lua, sol::in_place_type<FLuaArray>, *innerprop, initial, lua);
	},
	"Add", &FLuaArray::Lua_Add,
	"AddAt", &FLuaArray::Lua_AddAt,
	"Get", &FLuaArray::Lua_Get,
	"Num", &FLuaArray::Lua_Num,
	"Set", &FLuaArray::Lua_Set,
	"Clear", &FLuaArray::Lua_Clear,
	"Empty", &FLuaArray::Lua_Clear,
	"IsEmpty", &FLuaArray::Lua_IsEmpty,
	"IsValidIndex", &FLuaArray::Lua_IsValidIndex,
	"KeepAll", &FLuaArray::Lua_KeepAll,
	"Filter", &FLuaArray::Lua_KeepAll,
	"RemoveAll", &FLuaArray::Lua_RemoveAll,
	"ForEach", &FLuaArray::Lua_ForEach,
	"Copy", &FLuaArray::Lua_Copy,
	"Remove", &FLuaArray::Lua_Remove,
	"ToTable", &FLuaArray::Lua_ToTable,
	"Find", &FLuaArray::Lua_Find,
	"FindLast", &FLuaArray::Lua_FindLast,
	"Contains", &FLuaArray::Lua_Contains,
	"Pop", &FLuaArray::Lua_Pop,
	"Top", &FLuaArray::Lua_Top,
	"Last", &FLuaArray::Lua_Last,
	"Shuffle", &FLuaArray::Lua_Shuffle,
	
	sol::meta_function::length, &FLuaArray::Lua_Num,
	sol::meta_function::index, &FLuaArray::__index,
	sol::meta_function::pairs, &FLuaArray::__pairs,
	sol::meta_function::ipairs, &FLuaArray::__ipairs,
	sol::meta_function::new_index, &FLuaArray::Lua_AddAt
	//sol::meta_function::bitwise_left_shift, &FLuaArray::LUa_Append,
	);
}


FLuaArray::FLuaArray()
	: Inner(nullptr)
	, Data()
{
	checkNoEntry();
}

FLuaArray::FLuaArray(FArrayProperty& arrayprop, FScriptArray* referencedArray, bool bAsReference) //create copy
	: Inner(nullptr)
	, Data()
{
	if(bAsReference)
	{
		verify(arrayprop.Inner->Owner != nullptr);
		verify(arrayprop.Inner->GetFName() != UnrealLua::PropertyNames::NAME_UnrealLuaArrayInnerProperty);
		Inner = arrayprop.Inner;
		this->Data.Emplace<FScriptArray*>(referencedArray);
	}
	else
	{
		//copy
		this->Inner = UnrealLua::PropertyHelper::CreateNewProperty(arrayprop.Inner, UnrealLua::PropertyNames::NAME_UnrealLuaArrayInnerProperty);
		this->Data.Emplace<FLuaScriptArray*>(new FLuaScriptArray(this->Inner));
		this->AddRef();
		Copy(this->GetScriptArray(), this->Inner, referencedArray, arrayprop.Inner);
	}
}

//This constructor is used by sol_lua_push to send in either a copy or pointer of a TArray<Inner>
//The property has already been constructed
FLuaArray::FLuaArray(FProperty& inner, FScriptArray* otherScriptArray, bool bAsReference)
	: Inner(&inner)
	, Data()
{
	verify(this->Inner->GetFName() == UnrealLua::PropertyNames::NAME_UnrealLuaArrayInnerProperty);
	verify(this->Inner->Owner == nullptr);
	if(bAsReference)
	{
		verify(otherScriptArray != nullptr);
		this->Data.Emplace<FScriptArray*>(otherScriptArray);
	}
	else
	{
		//copy
		this->Data.Emplace<FLuaScriptArray*>(new FLuaScriptArray(this->Inner));
		this->AddRef();
		Copy(this->GetScriptArray(), this->Inner, otherScriptArray, &inner);
	}
}

//Call constructor, the inner property is a new property and will be taken by this LuaArray
FLuaArray::FLuaArray(FProperty& inner, sol::object initial, sol::this_state lua)
	: Inner(&inner)
	, Data()
{
	verify(this->Inner->GetFName() == UnrealLua::PropertyNames::NAME_UnrealLuaArrayInnerProperty);
	verify(this->Inner->Owner == nullptr);
	this->Data.Emplace<FLuaScriptArray*>(new FLuaScriptArray(this->Inner));
	this->AddRef();
	if(initial.get_type() == sol::type::number)
	{
		int32 num = FMath::Max(0, initial.as<int32>());
		FScriptArrayHelper helper = FScriptArrayHelper::CreateHelperFormInnerProperty(this->Inner, this->GetScriptArray());
		helper.Resize(num);		
	}
	else if(initial.is<sol::table>())
	{
		sol::table tbl = initial;
		this->InitFromTable(tbl);
	}
}

//Copy constructor
FLuaArray::FLuaArray(const FLuaArray& other)
	: Inner(nullptr)
	, Data(other.Data)
{
	if(this->OwnsMemory())
	{
		//Inner is managed by SharedPtr in memory
		Inner = other.Inner;
		this->AddRef();
	}
	else
	{
		if(other.Inner->GetFName() == UnrealLua::PropertyNames::NAME_UnrealLuaArrayInnerProperty)
		{
			//This Inner is a manually created one, so we need to create a new one for the new array
			verify(other.Inner->Owner == nullptr)
			Inner = UnrealLua::PropertyHelper::CreateNewProperty(other.Inner, UnrealLua::PropertyNames::NAME_UnrealLuaArrayInnerProperty);
		}
		else
		{
			//this is a UProperty reference
			verify(other.Inner->Owner != nullptr)
			Inner = other.Inner;
		}
	}
}

FLuaArray::FLuaArray(FLuaArray&& other) noexcept
	: Inner(other.Inner)
	, Data(other.Data)
{
	if(this->OwnsMemory())
	{
		//must add ref BEFORE decreasing ref from other, to keep data alive
		this->AddRef();
	}
	other.Inner = nullptr;
	other.Reset();
}

FLuaArray::~FLuaArray()
{
	this->Reset();
}

void FLuaArray::Reset()
{
	this->RemoveRef();
	if (this->Data.IsType<FScriptArray*>())
	{
		if (this->Inner && this->Inner->GetFName() == UnrealLua::PropertyNames::NAME_UnrealLuaArrayInnerProperty)
		{
			delete this->Inner;
		}
	}
	this->Data.Emplace<std::nullptr_t>();
	this->Inner = nullptr;
}


bool FLuaArray::IsValid() const
{
	//after rework, FArray should always be valid
	return this->Inner != nullptr && this->GetScriptArray() != nullptr;
}

FLuaArray& FLuaArray::Lua_ForEach(sol::protected_function func, sol::variadic_args args)
{
	if(!func.valid())
	{
		return *this;
	}
	FProperty* inner = this->Inner;
	
	FScriptArrayHelper helper = FScriptArrayHelper::CreateHelperFormInnerProperty(inner, this->GetScriptArray());

	for (int32 index = 0; index < this->GetScriptArray()->Num(); index++)
	{
		void* ptr = GetMemPtrForIndex(index);
		FGetPropertyValueParams getValueParams{inner, ptr, 0,func.lua_state()};
		func(UnrealLua::PropertyHelper::GetPropertyValue(getValueParams));
	}
	return *this;
}
FLuaArray& FLuaArray::Lua_KeepAll(sol::protected_function func, sol::variadic_args args, const sol::this_state lua)
{
	return this->Filter_Internal(func, args, true, lua);
}

FLuaArray& FLuaArray::Lua_RemoveAll(sol::protected_function func, sol::variadic_args args, const sol::this_state lua)
{
	return this->Filter_Internal(func, args, false, lua);
}

//Filter for deciding which elements to keep
FLuaArray& FLuaArray::Filter_Internal(sol::protected_function& func, sol::variadic_args& args, bool bShouldKeep, const sol::this_state lua)
{
	FProperty* inner = this->Inner;
	if(func.valid())
	{
		FScriptArrayHelper helper{FScriptArrayHelper::CreateHelperFormInnerProperty(inner ,this->GetScriptArray())};
		for(int32 i = helper.Num() - 1; i >= 0; i--)
		{
			void* v = this->GetMemPtrForIndex(i);
			FGetPropertyValueParams getValueParams{inner, v, 0,func.lua_state()};
			sol::object obj = UnrealLua::PropertyHelper::GetPropertyValue(getValueParams);
			sol::function_result res = func(obj, args);

			//If either
			//1. Result not valid or
			//2. We should keep but got false as result
			//3. We should not keep and get confirmation
			if(!res.valid() || (bShouldKeep && !res.get<bool>()) || (!bShouldKeep && res.get<bool>()))
			{
				helper.RemoveValues(i);
			}
		}				
	}
	else if(!bShouldKeep)
	{
		FScriptArrayHelper helper{FScriptArrayHelper::CreateHelperFormInnerProperty(inner ,this->GetScriptArray())};
		helper.EmptyValues();
	}
	return *this;
}

sol::variadic_results FLuaArray::Any(sol::object num_o, bool bInplace)
{
	sol::variadic_results results{};
	if(!this->IsValid())
	{
		return results;
	}

	FProperty* inner = this->Inner;
	
	const int32 maxIndex = this->GetScriptArray()->Num() -1;
	FScriptArrayHelper helper{FScriptArrayHelper::CreateHelperFormInnerProperty(inner,this->GetScriptArray())};
	helper.EmptyValues();
	
	if(maxIndex == -1)
	{
		return results;
	}
	int32 num = !num_o.valid() ? 1 : num_o.as<int32>();
	num = FMath::Clamp(num, 1, this->GetScriptArray()->Num());
	if(num == 1)
	{
		//get a single random item
		const int32 randIndex = FMath::RandRange(0, maxIndex);
		FGetPropertyValueParams params{inner, this->GetMemPtrForIndex(randIndex), 0, num_o.lua_state()};
		results.emplace_back(UnrealLua::PropertyHelper::GetPropertyValue(params));
		return results;
	}
	if(num == maxIndex)
	{
		//just get all entries and shuffle
		//TArray<sol::object> objs;
	}
	
	TArray<int32> alreadyChosen{};
	
	while(num > 0)
	{
		const int32 randIndex = FMath::RandRange(0, maxIndex);
		if(alreadyChosen.Contains(randIndex))
		{
			continue;
		}
		//must be unique
		alreadyChosen.Add(randIndex);
		FGetPropertyValueParams params{inner, this->GetMemPtrForIndex(randIndex), 0, num_o.lua_state()};
		results.emplace_back(UnrealLua::PropertyHelper::GetPropertyValue(params));
		num--;
	}
	
	return results;
}

void FLuaArray::Push(sol::object obj, sol::this_state lua)
{
	if(!this->IsValid())
	{
		return;
	}
	this->Lua_Add(obj);
}

sol::object FLuaArray::Lua_Pop(sol::this_state lua)
{
	if(!this->IsValid() || this->Num() == 0)
	{
		return sol::nil;
	}
	FProperty* inner = this->Inner;
	sol::object ret = this->Lua_Get(this->Num(), lua); //Note : 1-based indexing used, will be corrected in Lua_Get
	FScriptArrayHelper helper{FScriptArrayHelper::CreateHelperFormInnerProperty(inner,this->GetScriptArray())};
	helper.RemoveValues(this->Num()-1);
	return ret;
}

sol::object FLuaArray::Lua_Top(sol::this_state lua)
{
	if(!this->IsValid() || this->Num() == 0)
	{
		return sol::nil;
	}
	return this->Lua_Get(this->Num(), lua); //Note : 1-based indexing used, will be corrected in Lua_Get
}

sol::object FLuaArray::Lua_Last(int32 indexFromEnd, sol::this_state lua)
{
	if(!this->IsValid() || this->Num() == 0)
	{
		return sol::nil;
	}
	return this->Lua_Get(this->Num() - indexFromEnd, lua); //Note : 1-based indexing used, will be corrected in Lua_Get
}

void FLuaArray::Copy(FScriptArray* destArray, FProperty* destInnerProp, const FScriptArray* srcArray, FProperty* srcInnerProp)
{
	if(!destInnerProp->SameType(srcInnerProp))
	{
		LUA_LOG_ERROR("Can't copy Lua arrays of different inner type %s and %s!", *destInnerProp->GetCPPType(), *srcInnerProp->GetCPPType())
		return;
	}
	FScriptArrayHelper SrcArrayHelper = FScriptArrayHelper::CreateHelperFormInnerProperty(srcInnerProp, srcArray);
	FScriptArrayHelper DestArrayHelper = FScriptArrayHelper::CreateHelperFormInnerProperty(destInnerProp, destArray);

	int32 Num = SrcArrayHelper.Num();
	if (!(destInnerProp->PropertyFlags & CPF_IsPlainOldData))
	{
		DestArrayHelper.EmptyAndAddValues(Num);
	}
	else
	{
		DestArrayHelper.EmptyAndAddUninitializedValues(Num);
	}
	
	if (Num > 0)
	{
		size_t Size = destInnerProp->GetElementSize();
		
		if (!(destInnerProp->PropertyFlags & CPF_IsPlainOldData))
		{
			for (int32 i = 0; i < Num; i++)
			{
				uint8* SrcData = SrcArrayHelper.GetElementPtr(i);
				uint8* DestData = DestArrayHelper.GetElementPtr(i);
				destInnerProp->CopyCompleteValue(DestData, SrcData);
			}
		}
		else
		{
			uint8* SrcData = (uint8*)SrcArrayHelper.GetRawPtr();
			uint8* DestData = (uint8*)DestArrayHelper.GetRawPtr();
			FMemory::Memcpy(DestData, SrcData, Num * Size);
		}
		
	}
}

FLuaArray FLuaArray::Lua_Copy() const
{
	//create a copy
	FProperty* newProp = UnrealLua::PropertyHelper::CreateNewProperty(this->Inner, UnrealLua::PropertyNames::NAME_UnrealLuaArrayInnerProperty);
	return FLuaArray{*newProp, this->GetScriptArray(), false};;
}

/*
void FLuaArray::CloneArray(FScriptArray * destArray, FProperty* innerProperty, const FScriptArray * srcArray) {
	// blueprint stack will destroy the TArray
	// so deep-copy construct FScriptArray
	// it's very expensive
	if (!srcArray || !destArray || srcArray->Num() == 0)
	{
		return;
	}
	FScriptArrayHelper destHelper = FScriptArrayHelper::CreateHelperFormInnerProperty(innerProperty, destArray);
	destHelper.EmptyAndAddUninitializedValues(srcArray->Num());
	uint8* memWriteLocation = destHelper.GetRawPtr();
	uint8* memReadLocation = (uint8*)srcArray->GetData();
	for (int index = 0; index < srcArray->Num(); index++)
	{
		innerProperty->CopySingleValue(memWriteLocation, memReadLocation);
		memWriteLocation += innerProperty->GetElementSize();
		memReadLocation += innerProperty->GetElementSize();
	}
}
*/

void FLuaArray::MoveArray(FScriptArray * destArray, FProperty& innerProperty, FScriptArray * srcArray) {
	checkNoEntry();
	// blueprint stack will destroy the TArray
	// so deep-copy construct FScriptArray
	// it's very expensive
	if (!srcArray || srcArray->Num() == 0)
	{
		return;
	}
	FScriptArrayHelper helper = FScriptArrayHelper::CreateHelperFormInnerProperty(&innerProperty, destArray);
	helper.MoveAssign(srcArray);
}

sol::table FLuaArray::Lua_ToTable(sol::this_state lua_s)
{
	sol::state_view lua = lua_s;
	sol::table results = lua.create_table();
	if(!this->IsValid())
	{
		return results;
	}
	int32 num = this->GetScriptArray()->Num();
	FProperty* inner = this->Inner;
	FScriptArrayHelper helper{FScriptArrayHelper::CreateHelperFormInnerProperty(inner,this->GetScriptArray())};
	for(int32 i = 0; i < num; i++)
	{
		FGetPropertyValueParams params{inner, this->GetMemPtrForIndex(i), 0, lua_s};
		results[i+1] = UnrealLua::PropertyHelper::GetPropertyValue(params);
	}
	return results;
}


void FLuaArray::InitFromTable(const sol::table& tbl)
{
	this->Clear();
	for(int i = 1; i <= tbl.size(); ++i)
	{
		this->Lua_Add(tbl[i]);
	}
}

FProperty* FLuaArray::GetInner() const
{
	return this->Inner;
}

FScriptArray* FLuaArray::GetScriptArray() const
{
	return this->Data.IsType<FScriptArray*>() ? this->Data.Get<FScriptArray*>() : static_cast<FScriptArray*>(this->Data.Get<FLuaScriptArray*>());
}

void FLuaArray::LuaToTArrayIndexCorrection(int32& index)
{
	//Lua 1-index correction
	if constexpr(!UnrealLua::Compilation::ZERO_INDEXING_CORRECTION_FOR_UE_CONTAINERS)
	{
		index--;
	}
}

sol::object FLuaArray::__index(const FLuaArray* self, sol::object key, sol::this_state lua)
{
	if (!self)
	{
		return sol::nil;
	}
	FPlatformMisc::Prefetch(self->GetData());
	if(!self->IsValid())
	{ 
		return sol::nil;
	}
	return (*self)[key];
}

sol::object FLuaArray::operator[](sol::object key) const
{
	if(!key.is<int>())
	{
		return sol::nil;
	}
	int32 index = key.as<int32>();
	LuaToTArrayIndexCorrection(index);
	if (!this->IsValidIndex(index))
	{
		//no error message here, this gets called by ipairs() and the first invalid index indicates end of sequence,
		//so an invalid index is expected at some point
		//If someone wants an error message, they can use arr:Get(<index>) instead.
		return sol::nil;
	}
	FProperty* inner = this->GetInner();
	void* v = this->GetMemPtrForIndex(index);
	FGetPropertyValueParams params{inner, v, 0, key.lua_state()};
	return UnrealLua::PropertyHelper::GetPropertyValue(params);
}

uint8 * FLuaArray::GetMemPtrForIndex(int32 index) const 
{
	return (uint8*)this->GetScriptArray()->GetData() + index * this->Inner->GetElementSize();
}

uint8* FLuaArray::AddDefaultGetMemPtr()
{
	FProperty* inner = this->GetInner();
	FScriptArrayHelper helper = FScriptArrayHelper::CreateHelperFormInnerProperty(inner,this->GetScriptArray());
	int32 newIndex = helper.AddValue();
	return GetMemPtrForIndex(newIndex);
}

void FLuaArray::ConstructDefaultItems(int32 index, int32 count)
{
	uint8 *Dest = GetMemPtrForIndex(index);
	if (this->Inner->PropertyFlags & CPF_ZeroConstructor)
	{
		FMemory::Memzero(Dest, count * this->Inner->GetElementSize());
	}
	else
	{
		for (int32 i = 0; i < count; i++, Dest += this->Inner->GetElementSize())
		{
			this->Inner->InitializeValue(Dest);
		}
	}
}

bool FLuaArray::OwnsMemory() const
{
	return this->Data.IsType<FLuaScriptArray*>();
}

void FLuaArray::AddRef()
{
	verify(this->Data.IsType<FLuaScriptArray*>());
	this->Data.Get<FLuaScriptArray*>()->AddRef();
}

int32 FLuaArray::RemoveRef()
{
	if(this->OwnsMemory())
	{
		if(this->Data.Get<FLuaScriptArray*>()->RemoveRef() == 0)
		{
			delete this->Data.Get<FLuaScriptArray*>();
			this->Data.Emplace<std::nullptr_t>();
		}
	}
	return -1;
}

bool FLuaArray::IsUPropertyReference() const
{
	return !this->OwnsMemory();
}

int32 FLuaArray::Lua_Add(sol::object obj)
{
	if(!this->IsValid())
	{
		return -1;
	}
	if(obj.valid())
	{
		FProperty* inner = this->GetInner();
		if (!UnrealLua::PropertyHelper::IsCompatibleType(inner, obj))
		{
			return -1;
		}
		uint8* indexptr = this->AddDefaultGetMemPtr(); //create space for new item
		
		TSetPropertyValueParams params{inner, indexptr, 0, obj};
		UnrealLua::PropertyHelper::SetPropertyValue_Direct(params);
	}
	else //Add default value
	{
		this->AddDefaultGetMemPtr(); //create space for new item
	}
	return this->GetScriptArray()->Num() -1; //return index to added item
}


sol::object FLuaArray::Lua_AddAt(int32 index, sol::object obj)
{
	if(!this->IsValid())
	{
		return sol::nil;
	}
	FPlatformMisc::Prefetch(this->GetData());
	index--; //Lua index correction
	if(index < 0)
	{
		return sol::nil;
	}
	FProperty* inner = this->GetInner();
	if(obj.valid())
	{
		if (!UnrealLua::PropertyHelper::IsCompatibleType(inner, obj))
		{
			return sol::nil;
		}
	}
	if(this->Num() <= index)
	{
		this->Resize(index + 1);
	}
	void* memloc = this->GetMemPtrForIndex(index);
	TSetPropertyValueParams params{inner, memloc, 0, obj};
	UnrealLua::PropertyHelper::SetPropertyValue_Direct(params);

	FGetPropertyValueParams getParams{inner, memloc, 0, obj.lua_state()};
	return UnrealLua::PropertyHelper::GetPropertyValue(getParams);
}

sol::object FLuaArray::Lua_Get(int32 index, sol::this_state lua)
{
	if(!this->IsValid())
	{
		return sol::nil;
	}
	LuaToTArrayIndexCorrection(index);
	if (!this->IsValidIndex(index))
	{
		LUA_LOG_ERROR("Array Get : Lua index %d (C index: %d) out of range", index+1, index)
		return sol::nil;
	}
	FProperty* inner = this->GetInner();
	void* v = this->GetMemPtrForIndex(index);
	FGetPropertyValueParams getParams{inner, v, 0, lua};
	return UnrealLua::PropertyHelper::GetPropertyValue(getParams);
}

sol::object FLuaArray::Lua_Set(int32 index, sol::object value_o, sol::this_state lua)
{
	if(!this->IsValid())
	{
		return sol::nil;
	}
	LuaToTArrayIndexCorrection(index);
	if (!this->IsValidIndex(index))
	{
		LUA_LOG_ERROR("Array Set : Lua index %d (C index: %d) out of range", index+1, index)
		return sol::nil;
	}
	FProperty* inner = this->GetInner();
	void* v = this->GetMemPtrForIndex(index);
	TSetPropertyValueParams params{inner, v, 0, value_o};
	UnrealLua::PropertyHelper::SetPropertyValue_Direct(params);
	return value_o;
}

void FLuaArray::Lua__Remove(int32 index, int32 count)
{
	if(!this->IsValid())
	{
		return;
	}
	LuaToTArrayIndexCorrection(index);
	if(index < 0 || index >= this->GetScriptArray()->Num() || count <= 0)
	{
		return;
	}
	FProperty* inner = this->GetInner();
	FScriptArrayHelper helper{FScriptArrayHelper::CreateHelperFormInnerProperty(inner,this->GetScriptArray())};
	helper.RemoveValues(index, count);
}


void FLuaArray::Lua__RemoveAt(int32 index) const
{
	if(!this->IsValid())
	{
		return;
	}
	LuaToTArrayIndexCorrection(index);
	if (!this->IsValidIndex(index))
	{
		LUA_LOG_ERROR("Array get index %d out of range", index)
		return;
	}
	FScriptArrayHelper helper{nullptr, this->GetScriptArray()};
	helper.RemoveValues(index, 1);
}

bool FLuaArray::Lua_IsValidIndex(int32 index) const
{
	LuaToTArrayIndexCorrection(index);
	return this->IsValid() && this->IsValidIndex(index);
}

void FLuaArray::Lua_Clear()
{
	this->Clear();
}

int32 FLuaArray::Lua_Num() const
{
	return this->IsValid() ? this->GetScriptArray()->Num() : -1;
}

int32 FLuaArray::Lua_Find(sol::object toSearch, sol::this_state lua)
{
	if(!this->IsValid())
	{
		return INDEX_NONE;
	}
	if (this->Num() == 0)
	{
		return INDEX_NONE;
	}
	FProperty* inner = this->GetInner();
	if (!UnrealLua::PropertyHelper::IsCompatibleType(inner, toSearch))
	{
		return INDEX_NONE;
	}
	
	FScriptArrayHelper helper = FScriptArrayHelper::CreateHelperFormInnerProperty(inner, this->GetScriptArray());
	
	const int32 ElementCount = helper.Num();

	int32 foundIndex = INDEX_NONE;


	FDefaultConstructedPropertyElement temp(inner);
	
	TSetPropertyValueParams params{inner, temp.GetObjAddress(), 0, toSearch};
	UnrealLua::PropertyHelper::SetPropertyValue_Direct(params);

	for (int32 ElementIndex = 0; ElementIndex < ElementCount; ++ElementIndex)
	{
		uint8* elementPtr = helper.GetRawPtr(ElementIndex);
		if (inner->Identical(elementPtr, temp.GetObjAddress()))
		{
			foundIndex = ElementIndex;
			break;
		}
	}
	
	if(foundIndex != INDEX_NONE)
	{
		foundIndex++; //Lua index correction
	}
	return foundIndex;
}

int32 FLuaArray::Lua_FindLast(sol::object toSearch, sol::this_state lua)
{
	if(!this->IsValid())
	{
		return INDEX_NONE;
	}
	if (this->Num() == 0)
	{
		return INDEX_NONE;
	}
	FProperty* inner = this->GetInner();
	if (!UnrealLua::PropertyHelper::IsCompatibleType(inner, toSearch))
	{
		return INDEX_NONE;
	}
	
	FScriptArrayHelper helper = FScriptArrayHelper::CreateHelperFormInnerProperty(inner, this->GetScriptArray());
	
	const int32 ElementCount = helper.Num();

	int32 foundIndex = INDEX_NONE;

	void* eleMemory = FMemory_Alloca_Aligned(this->Inner->GetElementSize(), inner->GetMinAlignment());

	TSetPropertyValueParams params{inner, eleMemory, 0, toSearch};
	UnrealLua::PropertyHelper::SetPropertyValue_Direct(params);

	for (int32 ElementIndex = ElementCount - 1; ElementIndex >= 0; --ElementIndex)
	{
		if (this->Inner->Identical(helper.GetRawPtr(ElementIndex), eleMemory))
		{
			foundIndex = ElementIndex;
		}
	}
	this->Inner->DestroyValue(eleMemory);
	if(foundIndex != INDEX_NONE)
	{
		foundIndex++; //Lua index correction
	}
	return foundIndex;
}

bool FLuaArray::Lua_Contains(sol::object toSearch, sol::this_state lua)
{
	return this->Lua_Find(toSearch, lua) != INDEX_NONE;
}

int FLuaArray::__next(lua_State* L)
{
	// this gets called
	// to start the first iteration, and every
	// iteration there after
	// the state you passed in pairs is argument 1
	// the key value is argument 2
	// we do not care about the key value here
	lua_iterator_state& it_state = sol::stack::get<sol::user<lua_iterator_state>>(L, 1);
	auto& it = it_state.it;
	if (it.GetLogicalIndex() == it_state.last.GetLogicalIndex()) {
		return sol::stack::push(L, sol::lua_nil);
	}
	int32 iteratorindex = it.GetLogicalIndex();
	// 2 values are returned (pushed onto the stack):
	// the key and the value
	// the state is left alone

	sol::object obj = it_state.owner.Lua_Get(iteratorindex, L);
	
	int pushed = sol::stack::push(L, iteratorindex);
	pushed += sol::stack::push(L, obj);
	++it;
	return pushed;
}

int FLuaArray::__pairs(lua_State* L)
{
	FLuaArray& mt = sol::stack::get<FLuaArray&>(L, 1);
	lua_iterator_state it_state(mt);
	// pairs expects 3 returns:
	// the "next" function on how to advance,
	// the "table" itself or some state,
	// and an initial key value (can be nil)

	// next function controls iteration
	int pushed = sol::stack::push(L, &FLuaArray::__next);
	pushed += sol::stack::push<sol::user<lua_iterator_state>>(L, std::move(it_state));
	pushed += sol::stack::push(L, sol::lua_nil);
	return pushed;
}

int FLuaArray::__ipairs(lua_State* L)
{
	FLuaArray& mt = sol::stack::get<FLuaArray&>(L, 1);
	lua_iterator_state it_state(mt);
	// pairs expects 3 returns:
	// the "next" function on how to advance,
	// the "table" itself or some state,
	// and an initial key value (can be nil)

	// next function controls iteration
	int pushed = sol::stack::push(L, &FLuaArray::__next);
	pushed += sol::stack::push<sol::user<lua_iterator_state>>(L, std::move(it_state));
	pushed += sol::stack::push(L, sol::lua_nil);
	return pushed;
}

int32 FLuaArray::Add(const void* Item)
{
	const int32 Index = AddDefaulted();
	uint8 *Dest = GetData(Index);
	Inner->CopySingleValue(Dest, Item);
	return Index;
}

int32 FLuaArray::AddDefaulted(int32 Count)
{
	int32 Index = GetScriptArray()->Add(Count, Inner->GetElementSize(), Inner->GetMinAlignment());
	Construct(Index, Count);
	return Index;
}

int32 FLuaArray::AddUninitialized(int32 Count)
{
	return GetScriptArray()->Add(Count, Inner->GetElementSize(), Inner->GetMinAlignment());
}

int32 FLuaArray::Find(const void* Item) const
{
	int32 Index = INDEX_NONE;
	for (int32 i = 0; i < this->GetScriptArray()->Num(); ++i)
	{
		const uint8 *CurrentItem = GetData(i);
		if (Inner->Identical(Item, CurrentItem))
		{
			Index = i;
			break;
		}
	}
	return Index;
}

void FLuaArray::Insert(const void* Item, int32 Index)
{
	if (Index >= 0 && Index <= this->GetScriptArray()->Num())
	{
		GetScriptArray()->Insert(Index, 1, Inner->GetElementSize(), Inner->GetMinAlignment());
		Construct(Index, 1);
		uint8 *Dest = GetData(Index);
		Inner->CopySingleValue(Dest, Item);
	}
}

void FLuaArray::Lua_Remove(int32 Index)
{
	if (IsValidIndex(Index))
	{
		Destruct(Index);
		GetScriptArray()->Remove(Index, 1, Inner->GetElementSize(), Inner->GetMinAlignment());
	}
}

void FLuaArray::Clear()
{
	if (this->GetScriptArray()->Num() > 0)
	{
		Destruct(0, this->GetScriptArray()->Num());
		GetScriptArray()->Empty(0, Inner->GetElementSize(), Inner->GetMinAlignment());
	}
}

bool FLuaArray::Reserve(int32 Size)
{
	if (this->GetScriptArray()->Num() > 0)
	{
		return false;
	}
	GetScriptArray()->Empty(Size, Inner->GetElementSize(), Inner->GetMinAlignment());
	return true;
}

void FLuaArray::Resize(int32 NewSize)
{
	if (NewSize >= 0)
	{
		int32 Count = NewSize - this->GetScriptArray()->Num();
		if (Count > 0)
		{
			AddDefaulted(Count);
		}
		else if (Count < 0)
		{
			Destruct(NewSize, -Count);
			GetScriptArray()->Remove(NewSize, -Count, Inner->GetElementSize(), Inner->GetMinAlignment());
		}
	}
}

void FLuaArray::Get(int32 Index, void* OutItem) const
{
	if (IsValidIndex(Index))
	{
		Inner->CopySingleValue(OutItem, GetData(Index));
	}
}

void FLuaArray::Set(int32 Index, const void* Item)
{
	if (IsValidIndex(Index))
	{
		Inner->CopySingleValue(GetData(Index), Item);
	}
}

void FLuaArray::Swap(int32 A, int32 B)
{
	if (A != B)
	{
		if (IsValidIndex(A) && IsValidIndex(B))
		{
			GetScriptArray()->SwapMemory(A, B, Inner->GetElementSize());
		}
	}
}

void FLuaArray::Lua_Shuffle()
{
	int32 LastIndex = this->GetScriptArray()->Num() - 1;
	for (int32 i = 0; i <= LastIndex; ++i)
	{
		int32 Index = FMath::RandRange(i, LastIndex);
		if (i != Index)
		{
			GetScriptArray()->SwapMemory(i, Index, Inner->GetElementSize());
		}
	}
}

void FLuaArray::Append(const FLuaArray& SourceArray)
{
	int32 numOther = SourceArray.GetScriptArray()->Num();
	if (numOther > 0)
	{
		int32 Index = AddDefaulted(numOther);
		for (int32 i = 0; i < numOther; ++i)
		{
			uint8 *Dest = GetData(Index++);
			const uint8 *Src = SourceArray.GetData(i);
			Inner->CopySingleValue(Dest, Src);
		}
	}
}

uint8* FLuaArray::GetData(int32 Index)
{
	return (uint8*)GetScriptArray()->GetData() + Index * GetInner()->GetElementSize();
}

const uint8* FLuaArray::GetData(int32 Index) const
{
	return (uint8*)GetScriptArray()->GetData() + Index * GetInner()->GetElementSize();
}

void FLuaArray::Construct(int32 Index, int32 Count)
{
	uint8 *Dest = GetData(Index);
	for (int32 i = 0; i < Count; ++i)
	{
		Inner->InitializeValue(Dest);
		Dest += Inner->GetElementSize();
	}
}

void FLuaArray::Destruct(int32 Index, int32 Count)
{
	uint8 *Dest = GetData(Index);
	for (int32 i = 0; i < Count; ++i)
	{
		Inner->DestroyValue(Dest);
		Dest += Inner->GetElementSize();
	}
}

