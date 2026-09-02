// Fill out your copyright notice in the Description page of Project Settings.


#include "LuaTypes/LuaLightUserdata.h"

#include "Config/UnrealLuaConfig.h"
#include "UObjectRegistry/LuaUObjectRegistry.h"

extern "C" {
#include "lua.h"
#include "lobject.h"
#include "lstate.h"
}
#include "LuaCoreDelegates.h"
#include "Utility/LuaLogMacros.h"
#include "Reflection/FunctionDescr.h"
#include "LuaStackHandler/LuaStackHandler.h"
#include <bit>
#include "BlueprintSupport/UnrealLuaUtility.h"
#include "BlueprintSupport/UnrealLuaGameplayStatics.h"
#include "LuaCallHelpers/LuaCallContext.h"
#include "LuaCallHelpers/LuaScriptOverrideCalls.h"
#include "UObjectRegistry/LuaUObjectItem.h"
#include "SubSystem/UnrealLuaGameWorldSubsystem.h"
#include "UObjectRegistry/UnrealLuaUObjectRegistry.h"
#include "Utility/LuaUtility.h"
#include "Utility/UnrealVersion.h"
/*
** Convert an acceptable index to a pointer to its respective value.
** Non-valid indices return the special nil value 'G(L)->nilvalue'.
*/


static const FDelegateHandle fLuaLightUserdataLuaTypeHandle = FLuaCoreDelegates::OnRegisterLuaUsertypes.AddStatic(&ILuaLightUserdata::RegisterUsertype);


static_assert(alignof(FUnrealLuaLightUserdataWrapper) == 8);
static_assert(sizeof(FUnrealLuaLightUserdataWrapper) == 8);

ILuaLightUserdata* UnrealLua::LightUserdata::GetLuaLightUserDataFromLuaObj(const sol::stack_object& value)
{
	if(!value.valid() || value.get_type() != sol::type::lightuserdata)
	{
		return nullptr;
	}
	lua_State* L = value.lua_state();
	TValue* val = Utility::index2value(L, value.stack_index());
	FUnrealLuaLightUserdataWrapper wrapper{val->value_.p};

	if (!wrapper.IsCustom())
	{
		return nullptr;
	}
	return wrapper.GetCustom();
}

sol::object UnrealLua::LightUserdata::MakeFFunctionDescrReferenceObject(lua_State* L, const FFunctionDescr* func)
{
	//verify that the func ptr isn't tagged
	static_assert(alignof(decltype(func)) == 8);
	
	FUnrealLuaLightUserdataWrapper wrapper{func};
	verify(wrapper.IsFuncDescr());
	uint8* memPtr = static_cast<uint8*>(wrapper.GetTaggedLightUserdataPseudoPtr());
	return sol::make_object(L, sol::light(memPtr));
}

int UnrealLua::LightUserdata::PushFFunctionDescrReferenceObject(sol::this_state L, const FFunctionDescr* func)
{
	//verify that the func ptr isn't tagged
	static_assert(alignof(decltype(func)) == 8);
	std::uintptr_t funcPtr = std::uintptr_t(func);
	verify((funcPtr & EUnrealLuaLightUserdataType::All) == 0)
	
	FUnrealLuaLightUserdataWrapper wrapper{func};
	verify(wrapper.IsFuncDescr());
	uint8* memPtr = static_cast<uint8*>(wrapper.GetTaggedLightUserdataPseudoPtr());
	return sol::stack::push(L, sol::light(memPtr));
}

void UnrealLua::LightUserdata::AddReferencedUObject(const lu_byte& tag, const Value& value, FReferenceCollector& collector)
{
	if(tag != LUA_VLIGHTUSERDATA)
	{
		return;
	}
	FUnrealLuaLightUserdataWrapper uobjectdata{value.p};
	if (FLuaUObjectItemHandle* handle = uobjectdata.GetUObjectItemHandle())
	{
		handle->AddReferencedUObject(collector);
	}
}


int UnrealLua::LightUserdata::PushUObject(lua_State* L, ::UObject* obj)
{
	if (!IsValid(obj))
	{
		return sol::stack::push(L, sol::nil);
	}
	else
	{
		FUnrealLuaLightUserdataWrapper lightUserData{obj};
		verify(lightUserData.IsUObject())
		uint8* taggedLightUserdataPseudoPtr = static_cast<uint8*>(lightUserData.GetTaggedLightUserdataPseudoPtr());
		return sol::stack::push(L, sol::light(taggedLightUserdataPseudoPtr));
	}
}

int UnrealLua::LightUserdata::PushUEnum(const ::UEnum* uenum, lua_State* L)
{
	if (!IsValid(uenum))
	{
		return sol::stack::push(L, sol::nil);
	}
	else
	{
		FUnrealLuaLightUserdataWrapper lightUserData{uenum};
		verify(lightUserData.IsUEnum())
		uint8* taggedLightUserdataPseudoPtr = static_cast<uint8*>(lightUserData.GetTaggedLightUserdataPseudoPtr());
		return sol::stack::push(L, sol::light(taggedLightUserdataPseudoPtr));
	}
}

bool UnrealLua::LightUserdata::IsEnum(const sol::stack_object& self)
{
	const FUnrealLuaLightUserdataWrapper wrapper{self};
	return wrapper.IsUEnum();
}

bool UnrealLua::LightUserdata::IsEnum(const sol::object& self)
{
	const FUnrealLuaLightUserdataWrapper wrapper{self};
	return wrapper.IsUEnum();
}

UEnum* UnrealLua::LightUserdata::GetUEnum(const sol::object& object)
{
	const FUnrealLuaLightUserdataWrapper wrapper{object};
	return wrapper.GetUEnum();
}

UEnum* UnrealLua::LightUserdata::GetUEnum(const sol::stack_object& object)
{
	const FUnrealLuaLightUserdataWrapper wrapper{object};
	return wrapper.GetUEnum();
}

bool UnrealLua::LightUserdata::IsEnumEntry(const sol::stack_object& self)
{
	const FUnrealLuaLightUserdataWrapper wrapper{self};
	return wrapper.IsUEnumEntry();
}

bool UnrealLua::LightUserdata::IsEnumEntry(const sol::object& self)
{
	const FUnrealLuaLightUserdataWrapper wrapper{self};
	return wrapper.IsUEnumEntry();
}

FLuaUEnumEntry* UnrealLua::LightUserdata::GetEnumEntry(const sol::stack_object& self)
{
	const FUnrealLuaLightUserdataWrapper wrapper{self};
	return wrapper.GetUEnumEntry();
}

FLuaUEnumEntry* UnrealLua::LightUserdata::GetEnumEntry(const sol::object& self)
{
	const FUnrealLuaLightUserdataWrapper wrapper{self};
	return wrapper.GetUEnumEntry();
}

sol::object UnrealLua::LightUserdata::GetUObjectAsTaggedLightUserdata(TObjectPtr<::UObject> object, lua_State* L)
{
	//return sol::make_object(L, object.Get());
	if (!IsValid(object))
	{
		return sol::nil;
	}
	const FUnrealLuaLightUserdataWrapper wrapper{object};
	uint8* taggedLightUserdataPseudoPtr = static_cast<uint8*>(wrapper.GetTaggedLightUserdataPseudoPtr());
	return sol::make_object(L, sol::light(taggedLightUserdataPseudoPtr));
}

sol::object UnrealLua::LightUserdata::GetUObjectAsTaggedLightUserdata(FLuaUObjectItemHandle* handle, lua_State* L)
{
	if (!handle || !IsValid(handle->GetUObject()))
	{
		return sol::nil;
	}
	const FUnrealLuaLightUserdataWrapper wrapper{handle};
	uint8* taggedLightUserdataPseudoPtr = static_cast<uint8*>(wrapper.GetTaggedLightUserdataPseudoPtr());
	return sol::make_object(L, sol::light(taggedLightUserdataPseudoPtr));
}

sol::object UnrealLua::LightUserdata::GetUEnumAsTaggedLightUserdata(TObjectPtr<::UEnum> uenum, lua_State* L)
{
	if (!uenum)
	{
		return sol::nil;
	}
	const FUnrealLuaLightUserdataWrapper wrapper{uenum};
	uint8* taggedLightUserdataPseudoPtr = static_cast<uint8*>(wrapper.GetTaggedLightUserdataPseudoPtr());
	return sol::make_object(L, sol::light(taggedLightUserdataPseudoPtr));	
}

sol::object UnrealLua::LightUserdata::GetUEnumValueAsTaggedLightUserdata(FLuaUEnumEntry* enumEntry, lua_State* L)
{
	if (!enumEntry || !enumEntry->IsValid())
	{
		return sol::nil;
	}
	const FUnrealLuaLightUserdataWrapper wrapper{enumEntry};
	uint8* taggedLightUserdataPseudoPtr = static_cast<uint8*>(wrapper.GetTaggedLightUserdataPseudoPtr());
	return sol::make_object(L, sol::light(taggedLightUserdataPseudoPtr));
}

int UnrealLua::LightUserdata::PushUEnumValueAsTaggedLightUserdata(FLuaUEnumEntry* enumEntry, lua_State* L)
{
	if (!enumEntry || !enumEntry->IsValid())
	{
		return 0;
	}
	const FUnrealLuaLightUserdataWrapper wrapper{enumEntry};
	uint8* taggedLightUserdataPseudoPtr = static_cast<uint8*>(wrapper.GetTaggedLightUserdataPseudoPtr());
	return sol::stack::push(L, sol::light(taggedLightUserdataPseudoPtr));
}

void ILuaLightUserdata::RegisterUsertype(sol::state_view& lua)
{
	{
		LUA_LOG("Setting up lightuserdata metamethods")
		//Set up lightuserdata call metamethod for FFunctionDescr*
		int32* ptr = nullptr;

		lua["ptr"] = sol::light(ptr);
		verify(lua["ptr"].get_type() == sol::type::lightuserdata);
		verify(lua["ptr"].get_type() != sol::type::userdata);
	
		sol::table lightUserDataMeta = lua.create_table();
		lightUserDataMeta[to_string(sol::meta_function::call)] = UnrealLua::LightUserdata::Call;
		lightUserDataMeta[to_string(sol::meta_function::index)] = sol::c_call<decltype(&UnrealLua::LightUserdata::Index), &UnrealLua::LightUserdata::Index>;
		lightUserDataMeta[to_string(sol::meta_function::new_index)] = sol::c_call<decltype(&UnrealLua::LightUserdata::NewIndex), &UnrealLua::LightUserdata::NewIndex>;
		lightUserDataMeta[to_string(sol::meta_function::to_string)] = sol::c_call<decltype(&UnrealLua::LightUserdata::ToString), &UnrealLua::LightUserdata::ToString>;
		lightUserDataMeta[to_string(sol::meta_function::length)] = sol::c_call<decltype(&UnrealLua::LightUserdata::Length), &UnrealLua::LightUserdata::Length>;
	
		lua["ptrMeta"] = lightUserDataMeta;
		lua.safe_script("debug.setmetatable(ptr, ptrMeta)");
	
		lua["ptr"] = sol::nil;
		lua["ptrMeta"] = sol::nil;
	}
	{
		LUA_LOG("Setting up lua function metamethods")
		
		lua.safe_script("__meta_func__ = function() end");
		sol::table funcMeta = lua.create_table();
		lua.safe_script("debug.setmetatable(__meta_func__, funcMeta)");
	}
}

sol::variadic_results UnrealLua::LightUserdata::Call(sol::stack_object ud, sol::stack_object self, sol::variadic_args args)
{
	verify(ud.get_type() == sol::type::lightuserdata);
	verify(ud.get_type() != sol::type::userdata);

	lua_State* lua = ud.lua_state();
	TValue* val = Utility::index2value(lua, ud.stack_index());
	
	const FUnrealLuaLightUserdataWrapper wrapper{val->value_.p};
	return wrapper.Call(ud, self, args);
}

int UnrealLua::LightUserdata::Index(lua_State* L)
{
	sol::stack_object ud{L,1};
	const FUnrealLuaLightUserdataWrapper wrapper{ud};
	return wrapper.Index(L);
}

int UnrealLua::LightUserdata::NewIndex(lua_State* L)
{
	verify(sol::stack::top(L) == 3);
	sol::stack_object ud{L,1};
	const FUnrealLuaLightUserdataWrapper wrapper{ud};
	return wrapper.NewIndex(L);
}

int UnrealLua::LightUserdata::ToString(lua_State* L)
{
	sol::stack_object ud{L,1};
	const FUnrealLuaLightUserdataWrapper wrapper{ud};
	return wrapper.ToString(L);
}

int UnrealLua::LightUserdata::Length(lua_State* L)
{
	sol::stack_object ud{L,1};
	const FUnrealLuaLightUserdataWrapper wrapper{ud};
	return wrapper.Length(L);
}

int UnrealLua::LightUserdata::Delay(lua_State* L)
{
	sol::stack_object ud{L,1};
	const FUnrealLuaLightUserdataWrapper wrapper{ud};
	return wrapper.Delay(L);
}

int UnrealLua::LightUserdata::SetTimer(lua_State* L)
{
	sol::stack_object ud{L,1};
	const FUnrealLuaLightUserdataWrapper wrapper{ud};
	return wrapper.SetTimer(L);
}

int UnrealLua::LightUserdata::__AddOnValueChanged(lua_State* L)
{
	sol::stack_object ud{L,1};
	const FUnrealLuaLightUserdataWrapper wrapper{ud};
	return wrapper.__AddOnValueChanged(L);
}

int UnrealLua::LightUserdata::__RemoveOnValueChanged(lua_State* L)
{
	sol::stack_object ud{L,1};
	const FUnrealLuaLightUserdataWrapper wrapper{ud};
	return wrapper.__RemoveOnValueChanged(L);
}

int UnrealLua::LightUserdata::__AddReplicatedSubobject(lua_State* L)
{
	sol::stack_object ud{L,1};
	const FUnrealLuaLightUserdataWrapper wrapper{ud};
	return wrapper.__AddReplicatedSubobject(L);
}

int UnrealLua::LightUserdata::__RemoveReplicatedSubobject(lua_State* L)
{
	sol::stack_object ud{L,1};
	const FUnrealLuaLightUserdataWrapper wrapper{ud};
	return wrapper.__RemoveReplicatedSubobject(L);
}

int UnrealLua::LightUserdata::__LoadLuaScript(lua_State* L)
{
	sol::stack_object ud{L,1};
	const FUnrealLuaLightUserdataWrapper wrapper{ud};
	return wrapper.__LoadLuaScript(L);
}

int UnrealLua::LightUserdata::__Destroy(lua_State* L)
{
	sol::stack_object ud{L,1};
	const FUnrealLuaLightUserdataWrapper wrapper{ud};
	return wrapper.__Destroy(L);
}


int UnrealLua::LightUserdata::__Super(lua_State* L)
{
	sol::stack_object ud{L,1};
	const FUnrealLuaLightUserdataWrapper wrapper{ud};
	return wrapper.__Super(L);
}

int UnrealLua::LightUserdata::__SetLuaTickEnabled(lua_State* L)
{
	sol::stack_object ud{L,1};
	const FUnrealLuaLightUserdataWrapper wrapper{ud};
	return wrapper.__SetLuaTickEnabled(L);
}

int UnrealLua::LightUserdata::__SetBlueprintTickEnabled(lua_State* L)
{
	sol::stack_object ud{L,1};
	const FUnrealLuaLightUserdataWrapper wrapper{ud};
	return wrapper.__SetBlueprintTickEnabled(L);
}

int UnrealLua::LightUserdata::__SpawnActor(lua_State* L)
{
	sol::stack_object ud{L,1};
	const FUnrealLuaLightUserdataWrapper wrapper{ud};
	return wrapper.__SpawnActor(L);
}

struct UNREALLUA_API FUnrealLuaLightUserdataMetamethodProxy
{
	constexpr explicit FUnrealLuaLightUserdataMetamethodProxy(const char* funcName, lua_CFunction methodPtr)
		: Hash(UnrealLua::HashUtility::StrCrc32(funcName)), FuncName(funcName), MetaFunc(methodPtr)  
	{}
	const uint32 Hash;
	const std::string_view FuncName;
	const lua_CFunction MetaFunc;
};

const static TArray<FUnrealLuaLightUserdataMetamethodProxy, TFixedAllocator<14>> metamethods
{
	FUnrealLuaLightUserdataMetamethodProxy{"Delay", &UnrealLua::LightUserdata::Delay},
	FUnrealLuaLightUserdataMetamethodProxy{"SetTimer", &UnrealLua::LightUserdata::SetTimer},
	FUnrealLuaLightUserdataMetamethodProxy{"_AddOnValueChanged", &UnrealLua::LightUserdata::__AddOnValueChanged},
	FUnrealLuaLightUserdataMetamethodProxy{"_RemoveOnValueChanged", &UnrealLua::LightUserdata::__RemoveOnValueChanged},
	FUnrealLuaLightUserdataMetamethodProxy{"_LoadLuaScript", &UnrealLua::LightUserdata::__LoadLuaScript},
	FUnrealLuaLightUserdataMetamethodProxy{"_AddReplicatedSubobject", &UnrealLua::LightUserdata::__AddReplicatedSubobject},
	FUnrealLuaLightUserdataMetamethodProxy{"_RemoveReplicatedSubobject", &UnrealLua::LightUserdata::__RemoveReplicatedSubobject},
	FUnrealLuaLightUserdataMetamethodProxy{"_Destroy", &UnrealLua::LightUserdata::__Destroy},
	FUnrealLuaLightUserdataMetamethodProxy{"Super", &UnrealLua::LightUserdata::__Super},
	FUnrealLuaLightUserdataMetamethodProxy{"_SetLuaTickEnabled", &UnrealLua::LightUserdata::__SetLuaTickEnabled},
	FUnrealLuaLightUserdataMetamethodProxy{"_SetBlueprintTickEnabled", &UnrealLua::LightUserdata::__SetBlueprintTickEnabled},
	FUnrealLuaLightUserdataMetamethodProxy{"_SpawnActor", &UnrealLua::LightUserdata::__SpawnActor}
};

int UnrealLua::LightUserdata::TryPushMetaMethod(std::string_view key, lua_State* L)
{
	uint32 hash = UnrealLua::HashUtility::StrCrc32(key.data());
	const FUnrealLuaLightUserdataMetamethodProxy* mm = metamethods.FindByPredicate([hash, &key](const FUnrealLuaLightUserdataMetamethodProxy& item)
	{
		return item.Hash == hash && item.FuncName == key;
	});
	if (mm == nullptr)
	{
		return sol::stack::push(L, sol::nil);
	}
	else
	{
		return sol::stack::push(L, mm->MetaFunc);
	}
}

UObject* UnrealLua::LightUserdata::GetUObject(const sol::stack_object self)
{
	const FUnrealLuaLightUserdataWrapper wrapper{self};
	return wrapper.GetUObject();
}

UObject* UnrealLua::LightUserdata::GetUObject(const sol::object& self)
{
	const FUnrealLuaLightUserdataWrapper wrapper{self};
	return wrapper.GetUObject();
}

FFunctionDescr* UnrealLua::LightUserdata::GetFunctionDescr(const sol::stack_object self)
{
	const FUnrealLuaLightUserdataWrapper wrapper{self};
	return wrapper.GetFunctionDescr();
}

FFunctionDescr* UnrealLua::LightUserdata::GetFunctionDescr(const sol::object& self)
{
	const FUnrealLuaLightUserdataWrapper wrapper{self};
	return wrapper.GetFunctionDescr();
}

bool UnrealLua::LightUserdata::IsUObject(const sol::stack_object& obj)
{
	return GetUObject(obj) != nullptr;
}

bool UnrealLua::LightUserdata::IsUObject(const sol::object& obj)
{
	return GetUObject(obj) != nullptr;
}

sol::variadic_results ILuaLightUserdata::operator()(sol::stack_object self, sol::variadic_args args)
{
	return {};
}

UnrealLua::LightUserdata::EUnrealLuaLightUserdataType ILuaLightUserdata::GetLightUserdataType() const
{
	checkNoEntry();
	return UnrealLua::LightUserdata::EUnrealLuaLightUserdataType::None;
}

FUnrealLuaLightUserdataWrapper::FUnrealLuaLightUserdataWrapper(const sol::stack_object obj)
{
	if (obj.get_type() != sol::type::lightuserdata)
	{
		return;
	}
	TValue* val = UnrealLua::Utility::index2value(obj.lua_state(), obj.stack_index());
	this->LightUserdataPseudoPtr = std::bit_cast<std::uintptr_t>(val->value_.p);
}

FUnrealLuaLightUserdataWrapper::FUnrealLuaLightUserdataWrapper(const sol::object& obj)
{
	if (obj.get_type() != sol::type::lightuserdata)
	{
		return;
	}
	sol::state_view lua{obj.lua_state()};
	sol::stack::push(lua, obj);
	TValue* val = UnrealLua::Utility::index2value(obj.lua_state(), sol::stack::top(lua));
	this->LightUserdataPseudoPtr = std::bit_cast<std::uintptr_t>(val->value_.p);
	sol::stack::pop<sol::object>(lua);;
}


bool FUnrealLuaLightUserdataWrapper::IsUObject() const
{
	return this->HasTag(UnrealLua::LightUserdata::UObject);
}

bool FUnrealLuaLightUserdataWrapper::IsUObjectCallContext() const
{
	return this->HasTag(UnrealLua::LightUserdata::FunctionCallContext);	
}

bool FUnrealLuaLightUserdataWrapper::IsFuncDescr() const
{
	return this->HasTag(UnrealLua::LightUserdata::UFunctionDescr);
}

bool FUnrealLuaLightUserdataWrapper::IsUEnum() const
{
	return this->HasTag(UnrealLua::LightUserdata::UEnum);
}

bool FUnrealLuaLightUserdataWrapper::IsUEnumEntry() const
{
	return this->HasTag(UnrealLua::LightUserdata::UEnumEntry);
}

bool FUnrealLuaLightUserdataWrapper::IsCustom() const
{
	return this->HasTag(UnrealLua::LightUserdata::Custom);
}

FFunctionDescr* FUnrealLuaLightUserdataWrapper::GetFunctionDescr() const
{
	FFunctionDescr* descr = nullptr;
	if (this->IsFuncDescr())
	{
		descr = static_cast<FFunctionDescr*>(this->GetUntaggedPtr());
	}
	return descr;
}

FLuaCallContext* FUnrealLuaLightUserdataWrapper::GetUObjectCallContext() const
{
	FLuaCallContext* context = nullptr;
	if (this->IsUObjectCallContext())
	{
		context = static_cast<FLuaCallContext*>(this->GetUntaggedPtr());
	}
	return context;
}

UObject* FUnrealLuaLightUserdataWrapper::GetUObject() const
{
	UObject* obj = nullptr;
	if (this->IsUObject())
	{
		FLuaUObjectItemHandle* handle = this->GetUObjectItemHandle();
		obj = handle->GetUObject();
	}
	else if (this->IsUObjectCallContext())
	{
		FLuaCallContext* context = this->GetUObjectCallContext();
		obj = context->GetUObject();
	}
	return obj;
}

FLuaUObjectItemHandle* FUnrealLuaLightUserdataWrapper::GetUObjectItemHandle() const
{
	FLuaUObjectItemHandle* handle = nullptr;
	if (this->IsUObject())
	{
		handle = static_cast<FLuaUObjectItemHandle*>(this->GetUntaggedPtr());
	}
	return handle;
}

FLuaUObjectItem* FUnrealLuaLightUserdataWrapper::GetUObjectItem() const
{
	if (!this->IsUObject())
	{
		return nullptr;
	}
	FLuaUObjectItemHandle* handle = static_cast<FLuaUObjectItemHandle*>(this->GetUntaggedPtr());
	if (!handle)
	{
		return nullptr;
	}
	return handle->Item;
}

FLuaUObjectItemHandle* FUnrealLuaLightUserdataWrapper::GetUEnumItemHandle() const
{
	if (!this->IsUEnum())
	{
		return nullptr;
	}
	FLuaUObjectItemHandle* handle = static_cast<FLuaUObjectItemHandle*>(this->GetUntaggedPtr());
	return handle;
}

FLuaUEnumEntry* FUnrealLuaLightUserdataWrapper::GetUEnumEntry() const
{
	if (!this->IsUEnumEntry())
	{
		return nullptr;
	}
	FLuaUEnumEntry* handle = static_cast<FLuaUEnumEntry*>(this->GetUntaggedPtr());
	return handle;
}

UEnum* FUnrealLuaLightUserdataWrapper::GetUEnum() const
{
	if (!this->IsUEnum())
	{
		return nullptr;
	}
	FLuaUObjectItemHandle* handle = this->GetUEnumItemHandle();
	if (!handle)
	{
		return nullptr;
	}
	return Cast<UEnum>(handle->GetUObject());
}

ILuaLightUserdata* FUnrealLuaLightUserdataWrapper::GetCustom() const
{
	if (!this->IsCustom())
	{
		return nullptr;
	}
	ILuaLightUserdata* custom = static_cast<ILuaLightUserdata*>(this->GetUntaggedPtr());
	return custom;
}

void* FUnrealLuaLightUserdataWrapper::GetTaggedLightUserdataPseudoPtr() const
{
	return std::bit_cast<void*>(this->LightUserdataPseudoPtr);
}

void* FUnrealLuaLightUserdataWrapper::GetUntaggedPtr() const
{
	return std::bit_cast<void*>(this->LightUserdataPseudoPtr & ~UnrealLua::LightUserdata::EUnrealLuaLightUserdataType::All);
}

void FUnrealLuaLightUserdataWrapper::SetPtrAndTag(const void* ptr, UnrealLua::LightUserdata::EUnrealLuaLightUserdataType tag)
{
	verify(ptr != nullptr)
	//make sure we use clean pointers!
	verify((std::bit_cast<std::uintptr_t>(ptr) & UnrealLua::LightUserdata::EUnrealLuaLightUserdataType::All) == 0)
	//make sure we don't overwrite any data, only use clean wrappers!
	verify(this->LightUserdataPseudoPtr == 0);
	
	//Set tagged ptr
	this->LightUserdataPseudoPtr = std::bit_cast<std::uintptr_t>(ptr) | tag;

	//verify
	verify(this->HasTagAndPtrValue(ptr, tag));
}

bool FUnrealLuaLightUserdataWrapper::HasTag(UnrealLua::LightUserdata::EUnrealLuaLightUserdataType tag) const
{
	UnrealLua::LightUserdata::EUnrealLuaLightUserdataType type = static_cast<UnrealLua::LightUserdata::EUnrealLuaLightUserdataType>(this->LightUserdataPseudoPtr & UnrealLua::LightUserdata::EUnrealLuaLightUserdataType::All); 
	return type == tag;
}

bool FUnrealLuaLightUserdataWrapper::HasTagAndPtrValue(const void* ptr, UnrealLua::LightUserdata::EUnrealLuaLightUserdataType tag) const
{
	void* untagged = this->GetUntaggedPtr();
	bool pointerCorrect = untagged == ptr;
	bool tagCorrect = this->HasTag(tag);
	return pointerCorrect && tagCorrect;
}

UnrealLua::LightUserdata::EUnrealLuaLightUserdataType FUnrealLuaLightUserdataWrapper::GetType() const
{
	return static_cast<UnrealLua::LightUserdata::EUnrealLuaLightUserdataType>(this->LightUserdataPseudoPtr & std::uintptr_t(UnrealLua::LightUserdata::EUnrealLuaLightUserdataType::All));
}

sol::variadic_results FUnrealLuaLightUserdataWrapper::Call(sol::stack_object ud, sol::stack_object self, sol::variadic_args args) const
{
	verify(ud.get_type() == sol::type::lightuserdata);
	verify(ud.get_type() != sol::type::userdata);

	if (this->IsFuncDescr())
	{
		FFunctionDescr* descr = GetFunctionDescr();
		return (*descr)(self, args);
	}
	else if (this->IsCustom())
	{
		ILuaLightUserdata* ptr = UnrealLua::LightUserdata::GetLuaLightUserDataFromLuaObj(ud);
		if(!ptr)
		{
			return {};
		}
		return (*ptr)(self, args);
	}
	return {};
}


FUnrealLuaLightUserdataWrapper::FUnrealLuaLightUserdataWrapper(void* maybeTaggedLightUserdataPtr)
{
	static_assert(sizeof(void*) == sizeof(FUnrealLuaLightUserdataWrapper));
	static_assert(alignof(void*) == alignof(FUnrealLuaLightUserdataWrapper));
	this->LightUserdataPseudoPtr = std::uintptr_t(maybeTaggedLightUserdataPtr);	
}

FUnrealLuaLightUserdataWrapper::FUnrealLuaLightUserdataWrapper(const FFunctionDescr* functionDescr)
{
	this->SetPtrAndTag(functionDescr, UnrealLua::LightUserdata::EUnrealLuaLightUserdataType::UFunctionDescr);
}

FUnrealLuaLightUserdataWrapper::FUnrealLuaLightUserdataWrapper(const UObject* obj)
{
	FLuaUObjectItemHandle* handle = UnrealLua::UObjectRegistry::GetUObjectItemHandle(obj);
	verify(handle != nullptr);
	this->SetPtrAndTag(handle, UnrealLua::LightUserdata::EUnrealLuaLightUserdataType::UObject);
}

FUnrealLuaLightUserdataWrapper::FUnrealLuaLightUserdataWrapper(FLuaUObjectItemHandle* uobjectHandle)
{
	this->SetPtrAndTag(uobjectHandle, UnrealLua::LightUserdata::EUnrealLuaLightUserdataType::UObject);
}

FUnrealLuaLightUserdataWrapper::FUnrealLuaLightUserdataWrapper(const UEnum* uenum)
{
	FLuaUObjectItemHandle* handle = UnrealLua::UObjectRegistry::GetMetaObjectItemHandle(uenum);
	verify(handle != nullptr);
	this->SetPtrAndTag(handle, UnrealLua::LightUserdata::EUnrealLuaLightUserdataType::UEnum);
}

FUnrealLuaLightUserdataWrapper::FUnrealLuaLightUserdataWrapper(const FLuaUEnumEntry* uenumEntry)
{
	verify(uenumEntry && uenumEntry->IsValid());
	this->SetPtrAndTag(uenumEntry, UnrealLua::LightUserdata::EUnrealLuaLightUserdataType::UEnumEntry);
}

int FUnrealLuaLightUserdataWrapper::Index(lua_State* L) const
{
	verify(sol::stack::top(L) == 2);
	sol::stack_object ud{L,1};
	sol::stack_object key{L,2};
	verify(ud.get_type() == sol::type::lightuserdata);
	verify(ud.get_type() != sol::type::userdata);
	
	/*
	if (!this->IsTaggedLightUserdata())
	{
		return 0;
	}
	*/
	
	TValue* val = UnrealLua::Utility::index2value(L, ud.stack_index());

	if (this->IsUObject())
	{
		FLuaUObjectItemHandle* handle = this->GetUObjectItemHandle();
		return handle->__index(key);
	}
	else if (this->IsFuncDescr())
	{	
		return 0;
	}
	else if (this->IsUEnum())
	{
		FLuaUObjectItemHandle* handle = this->GetUEnumItemHandle();
		return handle->__indexAsUEnum(key);
	}
	else if (this->IsUEnumEntry())
	{
		return 0;
	}
	else if (ILuaLightUserdata* ptr = UnrealLua::LightUserdata::GetLuaLightUserDataFromLuaObj(ud))
	{
		return ptr->__index(key);
	}
	return 0;
}

int FUnrealLuaLightUserdataWrapper::NewIndex(lua_State* lua) const
{
	verify(sol::stack::top(lua) == 3);
	sol::stack_object ud{lua,1};
	sol::stack_object key{lua,2};
	sol::stack_object value{lua,3};
	
	if (this->IsUObject())
	{
		FLuaUObjectItemHandle* handle = this->GetUObjectItemHandle();
		handle->__newindex(key, value, lua);
	}
	else if (this->IsFuncDescr())
	{
		return 0;
	}
	else if (this->IsUEnum())
	{
		return 0;
	}
	else if (ILuaLightUserdata* ptr = UnrealLua::LightUserdata::GetLuaLightUserDataFromLuaObj(ud))
	{
		ptr->__newindex(key, value);
	}
	return 0;
}

int FUnrealLuaLightUserdataWrapper::ToString(lua_State* lua) const
{
	verify(sol::stack::top(lua) >= 1);
	sol::stack_object ud{lua,1};
	
	if (this->IsUObject())
	{
		FLuaUObjectItemHandle* handle = this->GetUObjectItemHandle();
		return handle->__tostring(lua);
	}
	else if (this->IsFuncDescr())
	{
		return sol::stack::push(lua, "UFunction");
	}
	else if (this->IsUEnum())
	{
		FLuaUObjectItemHandle* handle = this->GetUEnumItemHandle();
		return handle->__tostring(lua);
	}
	else if (this->IsUEnumEntry())
	{
		FLuaUEnumEntry* entry = this->GetUEnumEntry();
		return entry->__toString(lua);
	}
	else if (ILuaLightUserdata* ptr = UnrealLua::LightUserdata::GetLuaLightUserDataFromLuaObj(ud))
	{
		return sol::stack::push(lua, "Custom");
		//ptr->__tostring(lua);
	}
	return sol::stack::push(lua, "lightuserdata");
}

int FUnrealLuaLightUserdataWrapper::Length(lua_State* lua) const
{
	verify(sol::stack::top(lua) >= 1);
	sol::stack_object ud{lua,1};

	if (this->IsUObject())
	{
		FLuaUObjectItemHandle* handle = this->GetUObjectItemHandle();
		return sol::stack::push(lua, 0);
		//return handle->__length(lua);
	}
	else if (this->IsFuncDescr())
	{
		FFunctionDescr* descr = this->GetFunctionDescr();
		int32 numParams = descr->InputParms.Num() + descr->OutParms.Num() + (descr->ReturnParm ? 1 : 0);
		return sol::stack::push(lua, numParams);
	}
	else if (this->IsUEnum())
	{
		UEnum* uenum = this->GetUEnum();
		if (!uenum)
		{
			return sol::stack::push(lua, 0);
		}
		return sol::stack::push(lua, uenum->GetMaxEnumValue());
	}
	else if (this->IsUEnumEntry())
	{
		FLuaUEnumEntry* entry = this->GetUEnumEntry();
		if (!entry)
		{
			return 0;
		}
		return sol::stack::push(lua, entry->__toNumber(lua));
	}
	else if (ILuaLightUserdata* ptr = UnrealLua::LightUserdata::GetLuaLightUserDataFromLuaObj(ud))
	{
		return sol::stack::push(lua, "Custom");
		//ptr->__tostring(lua);
	}
	return sol::stack::push(lua, "lightuserdata");
}

int FUnrealLuaLightUserdataWrapper::Delay(lua_State* L) const
{
	sol::state_view lua{L};
	verify(sol::stack::top(lua) >= 1);
	sol::stack_object ud{lua,1};

	if (this->IsUObject())
	{
		UObject* obj = this->GetUObject();
		if (UUnrealLuaGameWorldSubsystem* ss = lua["World"])
		{
			sol::variadic_args args{lua, 1, sol::stack::top(L)};
			return sol::stack::push(L, ss->Delay(args, sol::this_state{L}));
		}
	}
	return 0;
}

int FUnrealLuaLightUserdataWrapper::SetTimer(lua_State* L) const
{
	sol::state_view lua{L};
	verify(sol::stack::top(lua) >= 1);
	sol::stack_object ud{lua,1};

	if (this->IsUObject())
	{
		UObject* obj = this->GetUObject();
		if (UUnrealLuaGameWorldSubsystem* ss = lua["World"])
		{
			sol::variadic_args args{lua, 1, sol::stack::top(L)};
			return sol::stack::push(L, ss->SetTimer(args, sol::this_state{L}));
		}
	}
	return 0;
}

int FUnrealLuaLightUserdataWrapper::__AddOnValueChanged(lua_State* L) const
{
	if (!this->IsUObject())
	{
		return 0;
	}
	sol::state_view lua{L};
	int numArgs = sol::stack::top(lua); 
	verify(numArgs >= 1);
	
	if (numArgs < 4)
	{
		return 0;
	}
	
	sol::stack_object self{lua,1};
	sol::stack_object propStr{lua,2};
	sol::stack_object target{lua,3};
	sol::stack_object targetCallback{lua,4};

	UObject* targetObj = UnrealLua::LightUserdata::GetUObject(target);
	
	if(propStr.get_type() != sol::type::string || !targetObj || targetCallback.get_type() != sol::type::string)
	{
		return 0;
	}
	
	if (FLuaUObjectItem* item = this->GetUObjectItem())
	{
		if (UObject* obj = this->GetUObject())
		{
			sol::variadic_args additionalCallbackArgs_{lua, 5, numArgs};
			sol::string_view propStrv = propStr.as<sol::string_view>();
			sol::string_view callbackStrv = targetCallback.as<sol::string_view>();
			uint64 handle = item->AddOnValueChangedListener(propStrv, targetObj, callbackStrv, additionalCallbackArgs_); 
			return sol::stack::push(L, handle);
		}
	}
	return 0;
}

int FUnrealLuaLightUserdataWrapper::__RemoveOnValueChanged(lua_State* L) const
{
	//__RemoveOnValueChanged(handle) --loop over all values and try to remove by handle
	//__RemoveOnValueChanged(obj) --loop over all values and try to remove by obj
	//__RemoveOnValueChanged("MyBool", handle) --remove handle from "MyBool" value
	//__RemoveOnValueChanged("MyBool", obj) --loop over all values and try to remove handles

	if (!this->IsUObject())
	{
		return 0;
	}
	int top = sol::stack::top(L);
	if (top < 2)
	{
		return 0;
	}
	sol::stack_object arg1{L,2};
	sol::stack_object arg2{L,3};
	
	FLuaUObjectItem* thisItem = this->GetUObjectItem();
	if(!thisItem)
	{
		return 0;
	}
	if(arg1.is<int>())
	{
		uint64 handle = arg1.as<uint64>();
		thisItem->RemoveOnValueChangedListenerViaHandle(handle);
	}
	else if(UObject* target = UnrealLua::LightUserdata::GetUObject(arg1))
	{
		thisItem->RemoveOnValueChangedListenerViaObject(target);
	}
	else if(arg1.get_type() == sol::type::string)
	{
		sol::string_view propStr = arg1.as<sol::string_view>();
		if(arg2.is<int>())
		{
			uint64 handle = arg2.as<uint64>();
			thisItem->RemoveOnValueChangedListenerFromScriptValueViaHandle(propStr, handle);
		}
		else if(UObject* arg2Target = UnrealLua::LightUserdata::GetUObject(arg2))
		{
			thisItem->RemoveOnValueChangedListenerFromScriptValueViaObject(propStr, arg2Target);
		}
	}
	return 0;
}

int FUnrealLuaLightUserdataWrapper::__AddReplicatedSubobject(lua_State* L) const
{
	if (sol::stack::top(L) < 3)
	{
		return sol::stack::push(L, false);
	}
	if (!this->IsUObject())
	{
		return sol::stack::push(L, false);
	}
	UObject* me = this->GetUObject();
	if(!me)
	{
		return sol::stack::push(L, false);
	}
	sol::stack_object subobject_o{L, 2};
	UObject* subobject = UnrealLua::LightUserdata::GetUObject(subobject_o);
	if(!subobject)
	{
		return sol::stack::push(L, false);
	}	
#if UNREALLUA_UE_VERSION_OLDER_THAN(5,8,0)
	UEnum* enumPtr = StaticEnum<ELifetimeCondition>();
#else
	UEnum* enumPtr = FindObject<UEnum>(nullptr, TEXT("/Script/CoreUObject.ElifetimeCondition"));
#endif
	int64 value = 0;
	sol::stack_object condition_o{L, 3};
	if(condition_o.is<FLuaUEnumEntry>())
	{
		const FLuaUEnumEntry& entry = condition_o.as<FLuaUEnumEntry>();
		if(entry.uenum == enumPtr)
		{
			value = entry.Value;
		}
	}
	if(!enumPtr->IsValidEnumValue(value))
	{
		return sol::stack::push(L, false);
	}
	ELifetimeCondition condition = static_cast<ELifetimeCondition>(value);
	bool success = UUnrealLuaUtility::AddReplicatedSubobject(me, subobject, condition);
	return sol::stack::push(L, success);
}

int FUnrealLuaLightUserdataWrapper::__RemoveReplicatedSubobject(lua_State* L) const
{
	UObject* me = this->GetUObject();
	if(!me)
	{
		return sol::stack::push(L, false);
	}
	sol::stack_object subobject_o{L, 2};
	UObject* subobj = UnrealLua::LightUserdata::GetUObject(subobject_o);
	if(!subobj)
	{
		return sol::stack::push(L, false);
	}
	bool success = UUnrealLuaUtility::RemoveReplicatedSubobject(me, subobj);
	return sol::stack::push(L, success);
}

int FUnrealLuaLightUserdataWrapper::__LoadLuaScript(lua_State* L) const
{
	UObject* obj = this->GetUObject();
	if(!obj)
	{
		return sol::stack::push(L, false);
	}
	sol::stack_object forceReload{L, 2};
	bool bForceReload = forceReload.valid() ? forceReload.as<bool>() : false;
	bool success = UUnrealLuaUtility::LoadLuaScriptForObject(obj, bForceReload);
	return sol::stack::push(L, success);
}

int FUnrealLuaLightUserdataWrapper::__Destroy(lua_State* L) const
{
	UObject* obj = this->GetUObject();
	if (!obj)
	{
		sol::stack::push(L, false);
	}
	//LUA_LOG("Lua UObject : Destroying object %s", *GetNameSafe(obj))
	if(AActor* actor = Cast<AActor>(obj))
	{
		actor->Destroy();
	}
	else
	{
		obj->MarkAsGarbage();
	}
	return sol::stack::push(L, true);
}

int FUnrealLuaLightUserdataWrapper::__Super(lua_State* L) const
{
	//sol::stack_object obj{L, 1};
	//sol::stack_object funcName{L, 2};
	//sol::variadic_args args {L, 3, sol::stack::top(L)};
	return UnrealLua::LuaScriptCall::SuperCall(L);
	//sol::variadic_results results = UnrealLua::LuaScriptCall::SuperCall(obj, funcName, args); 
	//return sol::stack::push(L, results);
}

int FUnrealLuaLightUserdataWrapper::__SetLuaTickEnabled(lua_State* L) const
{
	if (!this->IsUObject())
	{
		return 0;
	}
	sol::stack_object enabled{L, 2};
	if (!enabled.valid() || enabled.get_type() != sol::type::boolean)
	{
		return 0;
	}
	bool bEnabled = enabled.as<bool>();
	FLuaUObjectItem* item = this->GetUObjectItem();
	if (!item)
	{
		return 0;
	}
	item->SetLuaTickEnabled(bEnabled);
	return 0;
}

int FUnrealLuaLightUserdataWrapper::__SetBlueprintTickEnabled(lua_State* L) const
{
	if (!this->IsUObject())
	{
		return 0;
	}
	sol::stack_object enabled{L, 2};
	if (!enabled.valid() || enabled.get_type() != sol::type::boolean)
	{
		return 0;
	}
	bool bEnabled = enabled.as<bool>();
	FLuaUObjectItem* item = this->GetUObjectItem();
	if (!item)
	{
		return 0;
	}
	item->SetBlueprintTickEnabled(bEnabled);
	return 0;
}

int FUnrealLuaLightUserdataWrapper::__SpawnActor(lua_State* L) const
{
	sol::variadic_args args{L, 1, sol::stack::top(L)};
	if(args.size() == 0)
	{
		return 0;
	}
	if(!UnrealLua::LightUserdata::IsUObject(args[0].as<sol::stack_object>()))
	{
		return 0;
	}
	
	UUnrealLuaGameplayStatics* statics = UUnrealLuaGameplayStatics::StaticClass()->GetDefaultObject<UUnrealLuaGameplayStatics>();
	
	UFunction* spawnActorFunc = statics->FindFunctionChecked("SpawnActor");
	
	FFunctionDescr f{spawnActorFunc};
	sol::variadic_results results = f.PerformCall(statics, args);

	return sol::stack::push(L, results);
}

bool FUnrealLuaLightUserdataWrapper::IsInvalidUObjectReference() const
{
	if (this->IsUObject())
	{
		return !this->GetUObjectItemHandle()->IsValid();
	}
	return false;
}

bool FUnrealLuaLightUserdataWrapper::IsValidUObjectReference() const
{
	if (this->IsUObject())
	{
		return this->GetUObjectItemHandle()->IsValid();
	}
	return false;
}
