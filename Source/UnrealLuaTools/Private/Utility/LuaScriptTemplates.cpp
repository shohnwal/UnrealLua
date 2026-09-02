// Fill out your copyright notice in the Description page of Project Settings.


#include "Utility/LuaScriptTemplates.h"

FString UnrealLuaTools::ScriptTemplates::MakeLuaScriptAttributesTemplate(const FString& scriptName, bool withAnnotations)
{
	FStringBuilderBase builder;
	builder << scriptName;
	builder << 
R"###(.__ScriptAttributes =
{
	--Determines whether Lua tick calls are enabled by default.
	--If false, only tick parent Blueprint.
	--Default: true
	StartWithTickEnabled = true,

	--Used by Lua replicationj system. Determines how often Subobject Properties are 
	--checked for changed values. Does not affect replication frequency of Lua script values
	--Default : 0.0 (check every replication interval)
	PropertyReplicationFrequency = 0,

	--Sets Replication condition for replicating Lua values
	--if ReplicationCondition == COND_Never, Replicated Properties will be ignored
	--Default: "ELifetimeCondition::COND_None", aka replicate to all
	ReplicationCondition = "ELifetimeCondition::COND_None",

	--if true, automatically register in outer Actor/Component for Lua replication
	--Uses ReplicationCondition to determine replication condition
	--Default: false
	AutoRegisterForReplicationInOuter = false
	
	--Whether the applied Lua script should try to generate Input action overrides.
	--Default: false
	--If true, when applying the Lua script, examines all currently bound input actions of the Lua script owner's
	--Inputcomponent (if any) and generates Lua script functions depending on found input action name.	
	--Example: An found input action named IE_Jump gets mapped to
	--  function Character:InputAction_IA_Jump(value, eventType)
	--    return true|false
	--  end
	--  --@param value : bool|float|vector - input action value
	--	--@param eventType : ETriggerEvent-  value of the input event
	--	--@return bool - Whether parent input action should get called (true : Call parent input action)
	--Every time the owner's Inputcomponent input mapping changes, these functions get re-examined
	OverrideInput = true,
}
)###";
	return builder.ToString();
}

FString UnrealLuaTools::ScriptTemplates::MakeLuaScriptReplicationTemplate(const FString& scriptName, bool withAnnotations)
{
	FStringBuilderBase builder;
	builder << scriptName;
	builder << 
R"###(.__ReplicatedProperties =
{
  --List of Lua script values or UProperties of script owner's subobjects to replicate

  --Lua Value replication
  --Property : String  - Lua script value name to replicate (can also be a UProperty name of the script owner)
  --OnRep : String  - Optional, name of Lua script function to call if replication occurs. This is also automatically called by server when the value changes
  --Condition : ELifetimeConditioon  - Replication condition, same as Unreal Engine's built-on replication condition. Default : "ELifetimeCondition::COND_None"
  { Property = "MyBool", OnRep = "OnRep_MyBool", Condition = "ELifetimeCondition::COND_None" },

  --Subobject UProperty replication
  --Subobject : String - UProperty name of subobject to replicate UProperty from
  --Property : String - UProperty name in subobject to replicate
  --OnRep : String  - Optional, name of Lua script function to call if replication occurs.
  { Subobject = "CharacterMovement", Property = "JumpZVelocity",  OnRep = "OnRep_JumpVelocity" }
}
)###";
	return builder.ToString();
}
