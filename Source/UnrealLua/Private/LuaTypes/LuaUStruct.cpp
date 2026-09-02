// Fill out your copyright notice in the Description page of Project Settings.


#include "LuaTypes/LuaUStruct.h"

#include "LuaCoreDelegates.h"
#include "Config/UnrealLuaConstants.h"
#include "LuaTypes/LuaScriptStruct.h"
#include "Reflection/PropertyHelper.h"

static const FDelegateHandle fLuaUStructLuaTypeHandle = FLuaCoreDelegates::OnRegisterLuaUsertypes.AddStatic(&FLuaUStruct::RegisterUsertype);

static int ustructcount = 0;

void FLuaUStruct::RegisterUsertype(sol::state_view& lua)
{
	lua.new_usertype<FLuaUStruct>(
		"UStruct",
		"new", sol::no_constructor,
		sol::meta_function::to_string, &FLuaUStruct::ToString
	);
}

FLuaUStruct::FLuaUStruct()
	: ScriptStruct()
{
}

FLuaUStruct::FLuaUStruct(UScriptStruct* metaData)
	:ScriptStruct(MakeShared<FSoftObjectPath>(metaData))
{}

FLuaUStruct::FLuaUStruct(const UScriptStruct* metaData)
	: ScriptStruct(MakeShared<FSoftObjectPath>(metaData))
{
}

FLuaUStruct::FLuaUStruct(FLuaUStruct* metaData)
	: ScriptStruct(metaData->ScriptStruct)
{
}

FLuaUStruct::FLuaUStruct(const FLuaUStruct* metaData)
	:ScriptStruct(metaData->ScriptStruct)
{
}

FLuaUStruct::FLuaUStruct(const FLuaUStruct& metaData)
	: ScriptStruct(metaData.ScriptStruct)
{
}

FLuaUStruct::FLuaUStruct(FLuaUStruct&& other) noexcept
	: ScriptStruct(other.ScriptStruct)
{
	other.ScriptStruct.Reset();
}

FLuaUStruct::FLuaUStruct(const FSoftObjectPath& path)
	: ScriptStruct(MakeShared<FSoftObjectPath>(path))
{
}

FLuaUStruct::~FLuaUStruct()
{
	this->ScriptStruct.Reset();
}

sol::object FLuaUStruct::operator()(sol::variadic_args args, sol::this_state lua)
{
	const UScriptStruct* strct = this->TryLoad();
	if(!strct)
	{
		return sol::nil;
	}
	if (strct == UnrealLua::StaticPackages::VectorStruct)
	{
		auto constructor = [](sol::variadic_args va) -> FVector
		{
			if (va.size() == 1) { return FVector(va[0].get<float>(), 0, 0); }
			else if (va.size() == 2) { return FVector(va[0].get<float>(), va[1].get<float>(), 0); }
			else { return FVector(va[0].get<float>(), va[1].get<float>(), va[2].get<float>()); }
		};
		return sol::make_object<FVector>(lua.lua_state(), constructor(args));
	}
	else if (strct == UnrealLua::StaticPackages::Vector2DStruct)
	{
		auto constructor = [](sol::variadic_args va) -> FVector2D
		{
			if (va.size() == 1) { return FVector2D(va[0].get<float>(), 0); }
			else if (va.size() >= 2) { return FVector2D(va[0].get<float>(), va[1].get<float>()); }
			else { return FVector2D(0, 0); }
		};
		return sol::make_object<FVector2D>(lua.lua_state(), constructor(args));
	}
	else if (strct == UnrealLua::StaticPackages::RotatorStruct)
	{
		auto constructor = [](sol::variadic_args va) -> FRotator
		{
			if (va.size() == 1)
			{
				return FRotator(va[0].get<double>(), 0, 0);
			}
			else if (va.size() == 2)
			{
				return FRotator(va[0].get<double>(), va[1].get<double>(), 0);
			}
			else
			{
				return FRotator(va[0].get<double>(), va[1].get<double>(), va[2].get<double>());
			}
		};
		return sol::make_object<FRotator>(lua.lua_state(), constructor(args));
	}
	else if (strct == UnrealLua::StaticPackages::TransformStruct)
	{
		/*
		auto constructor = [](sol::variadic_args va) -> FTransform
		{
			if (va.size() == 1)
			{
				return FVector2D(va[0].get<float>(), 0);
			}
			else if (va.size() >= 2)
			{
				return FVector2D(va[0].get<float>(), va[1].get<float>());
			}
			else
			{
				return FVector2D(0, 0);
			}
		};*/
		unimplemented();
		return sol::make_object<FTransform>(lua.lua_state(), FTransform::Identity);
	}
	return sol::make_object<FLuaScriptStruct>(lua, this, args);
}

UScriptStruct* FLuaUStruct::TryLoad() const
{
	if(this->ScriptStruct.IsValid())
	{
		return Cast<UScriptStruct>(this->ScriptStruct->TryLoad());
	}
	return nullptr;
}

FSoftObjectPath FLuaUStruct::GetPath() const
{
	if(this->ScriptStruct.IsValid())
	{
		return *this->ScriptStruct.Get();
	}
	return nullptr;
}

std::string FLuaUStruct::ToString()
{
	FSoftObjectPath path = this->GetPath();
	FString pathStr = path.ToString();
	std::string str{StringCast<char>(*pathStr).Get()};
	return str;
}
