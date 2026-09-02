#include "LuaTypes/LuaUClass.h"

#include "LuaCoreDelegates.h"
#include "Utility/LuaLogMacros.h"
#include "Config/UnrealLuaConfig.h"
#include "GameFramework/Actor.h"
#include "LuaContext/ScopedLuaContext.h"
#include "UObjectRegistry/LuaUObjectItem.h"
#include "UObjectRegistry/UnrealLuaUObjectRegistry.h"
#include "UObject/Class.h"
#include "UObjectRegistry/LuaUObjectRegistry.h"

static const FDelegateHandle fLuaUClassLuaTypeHandle = FLuaCoreDelegates::OnRegisterLuaUsertypes.AddStatic(&FLuaUClass::RegisterUsertype);

void FLuaUClass::RegisterUsertype(sol::state_view& lua)
{
	lua.new_usertype<FLuaUClass>(
		"FLuaUClass",
		sol::call_constructor, sol::factories
		(
			[]() { return sol::nil; },
			[](const FLuaUClass& other) { return FLuaUClass(other); },
			[](const sol::string_view& str) { return FLuaUClass(str); }
		),
		"IsValid", &FLuaUClass::Valid,
		sol::meta_function::index, &FLuaUClass::__index,
		sol::meta_function::new_index, &FLuaUClass::__newindex,
		//sol::meta_function::garbage_collect, sol::default_destructor,
		sol::meta_function::to_string, [](FLuaUClass* self)
		{
			std::string str = StringCast<char>(*("UClass: " + self->Class.Get()->ToString())).Get(); 
			return str;
		},
		"Extend", &FLuaUClass::Extend
	);	
}

FLuaUClass::FLuaUClass()
	: Class()
{
}

FLuaUClass::FLuaUClass(UClass* obj)
	: Class() 
{
	this->Class = MakeShared<FSoftClassPath>(obj);
}

FLuaUClass::FLuaUClass(const UClass* obj)
	: Class()
{
	this->Class = MakeShared<FSoftClassPath>(obj);
}

FLuaUClass::FLuaUClass(sol::object obj)
	: Class()
{
	if (obj.get_type() == sol::type::lightuserdata)
	{
		UObject* uobj = UnrealLua::LightUserdata::GetUObject(obj);
		if(uobj)
		{
			this->Class = MakeShared<FSoftClassPath>(uobj->GetClass());
		}
	}
	else if (obj.is<FLuaUClass>())
	{
		this->Class = obj.as<FLuaUClass&>().Class;
	}
}

FLuaUClass::FLuaUClass(sol::string_view str)
{
	if(str.length() > 0)
	{
		this->Class = MakeShared<FSoftClassPath>(FString(str.data()));
	}
	else
	{
		this->Class .Reset();
	}
}

FLuaUClass::FLuaUClass(const FLuaUClass& other)
	: Class() 
{
	this->Class = other.Class;
}

FLuaUClass::FLuaUClass(const FLuaUClass* other)
	: Class()
{
	this->Class = other->Class;
}

FLuaUClass::FLuaUClass(const FSoftClassPath& path)
{
	this->Class = MakeShared<FSoftClassPath>(path);
}

FLuaUClass::FLuaUClass(FLuaUClass&& other) noexcept
{
	this->Class = MoveTemp(other.Class);
	other.Class.Reset();
}

FLuaUClass& FLuaUClass::operator=(const FLuaUClass& other)
{
	this->Class = other.Class;
	return *this;
}


FLuaUClass::~FLuaUClass()
{
}


sol::object FLuaUClass::operator()(sol::object outerObj, const sol::this_state lua) const
{
	if(this->Class.IsValid())
	{
		return sol::nil;
	}
	UObject* outer = nullptr;
	if (outerObj.get_type() == sol::type::lightuserdata)
	{
		outer = UnrealLua::LightUserdata::GetUObject(outerObj);
	}
	if(outer == nullptr)
	{
		outer = GetTransientPackage();
	}
	UClass* clazz = this->TryLoadClass();
	verify(clazz != nullptr);
	if(clazz->IsChildOf(AActor::StaticClass()))
	{
		LUA_LOG_WARNING("Don't use FLuaUClass() to spawn actors, please use the global function SpawnActor() instead")
		return {};
	}
	UObject* newObj = NewObject<UObject>(outer, clazz);
	return UnrealLua::UObjectRegistry::GetUObjectAsLightUserdata(lua, newObj);
}

bool FLuaUClass::Valid() const
{
	//Check if shared ptr is not null AND that the classpath is valid
	return this->Class.IsValid() && this->Class->IsValid();
}

void FLuaUClass::Set(UClass* uclass)
{
	this->Class = MakeShared<FSoftClassPath>(uclass);
}

int FLuaUClass::__index(lua_State* lua)
{
	sol::stack_object self{lua,1};
	sol::stack_object key{lua,2};
	if(!key.valid())
	{
		return sol::stack::push(lua, sol::nil);
	}

	FLuaUClass& clazz = self.as<FLuaUClass&>();
	UObject* cdo = clazz.TryLoadClass()->GetDefaultObject();
	FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(cdo);
	return item.PushScriptValue(key);
}

void FLuaUClass::__newindex(FLuaUClass* self, sol::object key, sol::object value, sol::this_state lua)
{
	if(!UUnrealLuaConfig::IsLuaWriteOnCDOAllowed())
	{
		LUA_LOG_WARNING("Setting a property is not allowed for UClass / CDOs!")
		return;
	}
	UObject* cdo = self->TryLoadClass()->GetDefaultObject();
	if(cdo)
	{
		FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(cdo);
		item.SetScriptValue(key, value, true);
	}
	
}

UClass* FLuaUClass::TryLoadClass() const
{
	return this->Class.IsValid() ? Cast<UClass>(const_cast<FLuaUClass*>(this)->Class.Get()->TryLoad()) : nullptr;
}

const FSoftClassPath FLuaUClass::GetSoftClassPath() const
{
	return this->Class.IsValid() ? *const_cast<FLuaUClass*>(this)->Class.Get() : FSoftClassPath{};
}

void FLuaUClass::Extend(sol::table extensionTbl)
{
	//UnrealLua::Compiler::ExtendUClass(this->TryLoadClass(), extensionTbl);
}
