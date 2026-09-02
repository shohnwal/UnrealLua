// Fill out your copyright notice in the Description page of Project Settings.


#include "IntelliSense/LuaTypesRepository.h"

#include "LuaContext/ScopedLuaContext.h"


void FLuaTypesRepository::PopulateLuaTypesRepository(const FScopedLuaContext& ctx)
{
	const sol::table& registryTable = ctx.GetRegistryTable();	
	
	registryTable.for_each_stack([this](sol::stack_object& key, sol::stack_object& value)
	{
		if (key.get_type() != sol::type::string)
		{
			return;
		}
		std::string_view strv = key.as<std::string_view>();
		if (strv.empty())
		{
			return;
		}
		if (value.is<FLuaUClass>())
		{
			this->TypeNameToTypeInfoMap.Emplace(strv.data(), value);
		}
		else if (value.is<FLuaUStruct>())
		{
			this->TypeNameToTypeInfoMap.Emplace(strv.data(), value);
		}
		else if (value.is<FLuaPrimitiveCPPType>())
		{
			this->TypeNameToTypeInfoMap.Emplace(strv.data(), value);
		}
		else if (UnrealLua::LightUserdata::IsEnum(value))
		{
			
		}
		else if (UnrealLua::LightUserdata::IsUObjectType<UBlueprintFunctionLibrary>(value))
		{
			//validType = true;
		}
	});
}
