#include "EngineUtils.h"
#include "HairStrandsInterface.h"
#include "Config/UnrealLuaConstants.h"
#include "Interface/LuaContext.h"
#include "LuaContext/ScopedLuaContext.h"
#include "ScriptableUObject/LuaScriptSettings.h"
#include "UObjectRegistry/UnrealLuaUObjectRegistry.h"
#include "UObjectRegistry/LuaUObjectRegistry.h"
#include "Engine/World.h"
#include "BlueprintSupport/UnrealLuaUtility.h"
#include "SubSystem/UnrealLuaEngineSubsystem.h"
#include "UObjectRegistry/LuaUObjectItem.h"

bool UnrealLua::UObjectRegistry::LoadLuaScript(UObject* obj, bool bForceReload)
{
	verify(IsInGameThread())
	if(!IsValid(obj))
	{
		return false;
	}
	
	if (UField* ufield = Cast<UField>(obj))
	{
		//Meta classes can not have scripts
		return false;
	}
	
	if (!UUnrealLuaEngineSubsystem::IsGameSessionActive())
	{
		return false;
	}
	
	TScriptInterface<ILuaContext> ctx = UUnrealLuaUtility::GetLuaContext(obj);
	if(!ctx)
	{
		LUA_LOG("%s : No lua context running ", *GetPathNameSafe(obj))
		return false;
	}
	
	return LoadLuaScriptInternal(obj, bForceReload, ctx);
}

bool UnrealLua::UObjectRegistry::LoadLuaScriptInternal(UObject* obj, bool bIsReloading, TScriptInterface<ILuaContext>& ictx)
{
	//LUA_LOG("trying to load lua script for %s", *GetFullNameSafe(obj))
	
	UUnrealLuaUObjectRegistry* registry = UUnrealLuaUObjectRegistry::Get();
	
	FLuaOverriddenClassInfo* overrideInfo = registry->GetOverrideRegistry().GetOverridenClassInfo(obj->GetClass()); 
	if(!overrideInfo)
	{
		//No info about that class being overridable
		LUA_LOG_WARNING("Can not load LuaScript for %s of class %s: No override class info found.", *GetFullNameSafe(obj), *GetFullNameSafe(obj->GetClass()))
		return false;
	}

	FLuaUObjectItem& item = registry->GetUObjectItem(obj);
	
#if !UE_BUILD_SHIPPING
	//a few debug safety checks
	verify(item.Object == obj);
	verify(IsValid(item.Object));
	verify(IsValid(obj));
	//this shouldn't happen, because we checked for UField-types above
	verify(!item.bIsMetaItem)
	//Metadata should already be properly set up and linked by getting the FLuaUObjectItem above
	FLuaUObjectItem& baseItem = registry->GetMetaObjectItem(obj->GetClass());
	
	verify(baseItem.Object != nullptr);
	verify(baseItem.PropertyMapping.IsValid())
	verify(item.PropertyMapping.IsValid())
	verify(item.PropertyMapping == baseItem.PropertyMapping)
#endif
	
	if(!bIsReloading && item.IsLuaScripted())
	{
		//no reload requested and already has a valid script
		return false;
	}
	
	/*
	  Script path priority:
	  1. By default, script path is empty -> look up default asset path
	  2. Allow LuaConfig to override that path
	  3. If GetLuaScriptSettings returns non-empty string, use that, allowing each object individually to override it
	*/
	
	FLuaScriptSettings scriptSettings{};
	
	// 1. Check whether object wants individual script path
	if (overrideInfo->bImplementsLuaScriptable)
	{
		scriptSettings = ILuaScriptable::Execute_GetLuaScriptSettings(obj);
	}
	else if (overrideInfo->bHasGetLuaScriptSettingsFunction)
	{
		UFunction* func = obj->GetClass()->FindFunctionByName(UnrealLua::PropertyNames::NAME_GetLuaScriptSettings);
		verify(IsValid(func))
		FStructProperty* returnProperty = CastFieldChecked<FStructProperty>(func->GetReturnProperty());
		verify(returnProperty->Struct == FLuaScriptSettings::StaticStruct());
		
		void* memory = FMemory_Alloca_Aligned(func->ParmsSize, func->GetMinAlignment());
		
		FStructOnScope params{func, static_cast<uint8*>(memory)};
		obj->ProcessEvent(func, params.GetStructMemory());
		scriptSettings = *returnProperty->ContainerPtrToValuePtr<FLuaScriptSettings>(memory);
	}
	
	//2. If individual UObject did return empty path, check if default script path got overridden via LuaConfig
	if (scriptSettings.ScriptPathOverride.IsEmpty() && overrideInfo != nullptr && !overrideInfo->ScriptPath.IsEmpty())
	{
		scriptSettings.ScriptPathOverride = overrideInfo->ScriptPath;
	}
	
	FScopedLuaContext& sctx = ictx->GetScopedLuaContext();

	FLuaScriptInstanceHandle handle{};

	//3. If a scriptPath override is set, try loading Lua script
	if(!scriptSettings.ScriptPathOverride.IsEmpty())
	{
		handle = sctx.GetLuaScriptHandle(scriptSettings);
	}
	
	//4. if no script could be loaded from the given path, try looking up a Lua script based on class name path
	if(!handle.IsValid())
	{
		scriptSettings.ScriptPathOverride = overrideInfo->DefaultAssetScriptFilePath;
		handle = sctx.GetLuaScriptHandle(scriptSettings);
	}

	if(!handle.IsValid())
	{
		//no valid lua script found, use a dummy script so it's functional either way
		handle = sctx.GetLuaScriptHandle({});
	}

	verify(handle.IsValid());

	item.SetLuaScriptHandle(handle, bIsReloading);
	
	verify(item.GetLuaScriptHandle().IsValid());

	return true;
}

//Speed up script loading for netload actors by looking up LuaContext only once
bool UnrealLua::UObjectRegistry::LoadLuaScriptsForNetLoadActors(UWorld* world, bool bForceReload)
{
	TScriptInterface<ILuaContext> ctx = UUnrealLuaUtility::GetLuaContext(world);
	if (!ctx)
	{
		return false;
	}
	for (TActorIterator<AActor> It(world, AActor::StaticClass()); It; ++It)
	{
		AActor* actor = *It;
		if(actor->bNetStartup)
		{
			//@TODO : What about actors that are not ULuaScriptable but are designated Lua-compatible via LuaConfig?
			//if(actor->Implements<ULuaScriptable>())
			{
				UnrealLua::UObjectRegistry::LoadLuaScriptInternal(actor, false, ctx);
			}	
		}
	}
	return true;
}

FLuaUObjectItemHandle* UnrealLua::UObjectRegistry::GetUObjectItemHandle(const UObject* object)
{
	const FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetUObjectItem(object);
	return item.CurrentHandle;
}

FLuaUObjectItemHandle* UnrealLua::UObjectRegistry::GetMetaObjectItemHandle(const UField* obj)
{
	const FLuaUObjectItem& item = UnrealLua::UObjectRegistry::GetMetaObjectItem(obj);
	return item.CurrentHandle;
}
