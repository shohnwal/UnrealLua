#include "ScriptableUObject/LuaScriptSettings.h"


/*
TScriptInterface<ILuaContext> FLuaScriptSettings::GetLuaContext(const UObject* scriptOwner)
{
	if(!this->LuaContext)
	{
		if(this->LuaContextClass)
		{
			this->LuaContext = this->LuaContextClass->GetDefaultObject();
		}
		else
		{
			this->LuaContext = UUnrealLuaUtilityBlueprintFunctionLibrary::GetLuaContext(scriptOwner);
		}
	}
	return this->LuaContext;
}
*/
