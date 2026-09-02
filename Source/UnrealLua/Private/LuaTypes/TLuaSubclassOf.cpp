// Fill out your copyright notice in the Description page of Project Settings.


#include "LuaTypes/TLuaSubclassOf.h"

#include "LuaCoreDelegates.h"

static const FDelegateHandle fLuaTSubclassOfLuaTypeHandle = FLuaCoreDelegates::OnRegisterLuaUsertypes.AddStatic(&TLuaSubclassOf::RegisterUsertype);

void TLuaSubclassOf::RegisterUsertype(sol::state_view& lua)
{
	lua.new_usertype<TLuaSubclassOf>(
		"TSubclassOf",
		sol::call_constructor, [](sol::object arg) -> sol::object
		{
			FLuaUClass uclass{arg};
			if (!uclass.Valid())
			{
				return sol::nil;
			}
			return sol::make_object(arg.lua_state(),TLuaSubclassOf{uclass});
		},
		"IsValid", &TLuaSubclassOf::IsValid,
		sol::meta_function::to_string, [](TLuaSubclassOf* self)
		{
			std::string str = StringCast<char>(*("TSubclassOf " + self->ClassPath.GetSoftClassPath().ToString())).Get(); 
			return str;
		}
	);	
}

UClass* TLuaSubclassOf::GetUClass() const
{
	if (!this->IsValid())
	{
		return nullptr;
	}
	return this->ClassPath.TryLoadClass();
}

bool TLuaSubclassOf::IsValid() const
{
	return this->ClassPath.Valid();
}
