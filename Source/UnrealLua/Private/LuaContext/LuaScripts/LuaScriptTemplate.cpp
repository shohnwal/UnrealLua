#include "LuaContext/LuaScripts/LuaScriptTemplate.h"

#include "EnhancedInputComponent.h"
#include "Utility/LuaLogMacros.h"
#include "Config/UnrealLuaConstants.h"
#include "LuaTypes/LuaEnum.h"
#include "Reflection/PropertyHelper.h"
#include "LuaContext/LuaScripts/LoadedLuaScriptCollection.h"

FLuaScriptTemplate::FLuaScriptTemplate()
	: LuaTable(), SubobjectOverrides(), ScriptAttributes(), bHasTickFunction()
{
}

FLuaScriptTemplate::FLuaScriptTemplate(sol::table& mainScriptTable)
	: LuaTable(), SubobjectOverrides(), ScriptAttributes(), bHasTickFunction()
{
	if (mainScriptTable.valid())
	{
		//no changes to the template allowed
		sol::table overridesTable = mainScriptTable[UnrealLua::LuaScriptKeys::ScriptAttributes::SubObjectOverrides];
		if (overridesTable.valid())
		{
			this->SubobjectOverrides = overridesTable;
			mainScriptTable[UnrealLua::LuaScriptKeys::ScriptAttributes::SubObjectOverrides] = sol::nil;
		}

		sol::table attributesTbl = mainScriptTable[UnrealLua::LuaScriptKeys::ScriptAttributes::ScriptAttributesKey];
		if (attributesTbl.valid())
		{
			UnrealLua::PropertyHelper::InitializeStructFromTable(this->ScriptAttributes, attributesTbl, false);
			mainScriptTable[UnrealLua::LuaScriptKeys::ScriptAttributes::ScriptAttributesKey] = sol::nil;
		}
		this->ScriptAttributes.ReplicationFrequency = FMath::Max(0.0f, this->ScriptAttributes.ReplicationFrequency);
	
		bool bHasTick = mainScriptTable[UnrealLua::PropertyNames::NAME_ReceiveTick].is<sol::function>();
		bool bHasUMGTick = mainScriptTable[UnrealLua::PropertyNames::NAME_Tick].is<sol::function>();

		if(bHasTick || bHasUMGTick)
		{
			this->bHasTickFunction = true;
		}
		
		sol::table repLayoutTbl = mainScriptTable[UnrealLua::LuaScriptKeys::ScriptAttributes::ReplicatedPropertiesKey];
		mainScriptTable[UnrealLua::LuaScriptKeys::ScriptAttributes::ReplicatedPropertiesKey] = sol::nil;
		if (repLayoutTbl.valid())
		{
			this->RepLayoutTable = repLayoutTbl;
		}

		this->LuaTable = mainScriptTable;

		this->LockTemplate();
	}
}

bool FLuaScriptTemplate::IsLuaScriptValid() const
{
	return this->LuaTable.valid();
}

void FLuaScriptTemplate::LockTemplate()
{
	if(!this->LuaTable.valid())
	{
		return;
	}
	sol::state_view lua{this->LuaTable.lua_state()};
	sol::table metatable = lua.create_table();
	metatable["__newindex"] = []()
	{
		LUA_LOG_ERROR("No changes to the template script allowed")
	};
	this->LuaTable[sol::metatable_key] = metatable;
	if(this->SubobjectOverrides.valid())
	{
		this->SubobjectOverrides[sol::metatable_key] = metatable;	
	}
}

bool FLuaScriptTemplate::IsValid() const
{
	return this->LuaTable.valid();
}

bool FLuaScriptTemplate::ShouldOverrideInput() const
{
	return this->ScriptAttributes.OverrideInput;
}

bool FLuaScriptTemplate::AutoRegisterReplicatedSubobject() const
{
	return this->ScriptAttributes.AutoRegisterForReplicationInOuter;
}

bool FLuaScriptTemplate::HasTickFunction() const
{
	return this->bHasTickFunction;
}

bool FLuaScriptTemplate::StartWithTickEnabled() const
{
	return this->ScriptAttributes.StartWithTickEnabled;
}

ELifetimeCondition FLuaScriptTemplate::GetObjectReplicationCondition() const
{
	return this->ScriptAttributes.ObjectReplicationCondition;
}

float FLuaScriptTemplate::GetReplicationFrequency() const
{
	return this->ScriptAttributes.ReplicationFrequency;
}

sol::table FLuaScriptTemplate::GetSubobjectOverrides() const
{
	return this->SubobjectOverrides;
}

sol::table FLuaScriptTemplate::GetRepLayoutTable()
{
	return this->RepLayoutTable;
}

sol::table FLuaScriptTemplate::GetScriptTable()
{
	return this->LuaTable;
}

sol::object FLuaScriptTemplate::GetScriptValueInternal(const sol::object& key)
{
	if(!this->LuaTable.valid())
	{
		return sol::nil;
	}
	return this->LuaTable[key];
}

sol::object FLuaScriptTemplate::GetSubobjectScriptValue(FName subobjectName, const sol::object& key)
{
	if(!this->IsLuaScriptValid())
	{
		return sol::nil;
	}
	auto casted = StringCast<char>(*subobjectName.ToString());
	const char* subObjectNameStr = casted.Get();
	sol::table subObjectTbl = this->LuaTable.raw_get_or<sol::table>(subObjectNameStr, sol::nil);
	if(subObjectTbl.valid())
	{
		return subObjectTbl.raw_get_or<sol::object>(key, sol::nil);				
	}	
	return sol::nil;
}

TMap<FString, FString> FLuaScriptTemplate::LuaScriptToString()
{
	    if(!this->LuaTable.valid())
    {
        static TMap<FString, FString> InvalidScript = {{"ScriptTable","not valid"}};
        return InvalidScript;
    }

    TMap<FString, FString> result;
    sol::state_view lua = this->LuaTable.lua_state();
    sol::function toString = lua["tostring"];
    this->LuaTable.for_each([toString, &result](sol::object key, sol::object value_o)
    {
        std::string keystr = toString(key);
        FString value = TEXT("<undefined>");
        switch(value_o.get_type())
        {
            case sol::type::string:
                value = FString::Printf(TEXT("<string> : %hs"), value_o.as<sol::string_view>().data());
                break;
            case sol::type::number:
                if(value_o.is<int>())
                {
                    int64 number = value_o.as<int64>();
                    value = FString::Printf(TEXT("<int> : %I64d"), number);
                }
                else
                {
                    double number = value_o.as<double>();
                    value = FString::Printf(TEXT("<float> : %f"), number);
                }
                break;
            case sol::type::thread:
                value = TEXT("<Thread>");
                break;
        case sol::type::boolean:
                value = value_o.as<bool>() ? TEXT("<bool> : true") : TEXT("<bool> : false");
                break;
            case sol::type::function:
                value = TEXT("<function>");
                break;
            case sol::type::userdata:
                value = TEXT("<userdata>");
                break;
            case sol::type::lightuserdata:
                value = TEXT("<lightuserdata>");
                break;
            case sol::type::table:
                value = TEXT("<table>");
                break;
            case sol::type::poly:
                break;
        default: ;
        }
        
        result.Emplace(
            keystr.c_str(), value
        );
    });
    return result;
}
