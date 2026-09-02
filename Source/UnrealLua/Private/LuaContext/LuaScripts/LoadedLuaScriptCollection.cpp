// Fill out your copyright notice in the Description page of Project Settings.


#include "LuaContext/LuaScripts/LoadedLuaScriptCollection.h"

#include "Utility/LuaLogMacros.h"
#include "LuaContext/ScopedLuaContext.h"
#include "Config/UnrealLuaConstants.h"
#include "UnrealLua.h"
#include "Config/UnrealLuaConfig.h"
#include "sol/sol.hpp"
#include "LuaContext/LuaScripts/LuaScriptTemplate.h"
#include "UObjectRegistry/LuaUObjectItem.h"

FUnrealLuaRepLayoutProperty* FLuaRepLayout::GetRepPropertyForRepIndex(uint8 index) const
{
    return const_cast<FLuaRepLayout*>(this)->RepLayoutProperties.FindByPredicate([index](const FUnrealLuaRepLayoutProperty& item)
    {
        return item.RepLayoutPropertyIndex == index;
    });
}

FUnrealLuaObjectRepLayout* FLuaRepLayout::GetOrCreateObjectReplayout(FName subObjName)
{
    FUnrealLuaObjectRepLayout* replayout = this->ObjectReplayouts.FindByPredicate([subObjName](const FUnrealLuaObjectRepLayout& item)
        {
            return item.SubObjectPropertyName == subObjName;
        });
    if(!replayout)
    {
        replayout = &this->ObjectReplayouts.Emplace_GetRef(FUnrealLuaObjectRepLayout{subObjName, {}});
    }
    return replayout;    
}

FUnrealLuaObjectRepLayout* FLuaRepLayout::GetObjectReplayout(FName subObjName)
{
    FUnrealLuaObjectRepLayout* replayout = this->ObjectReplayouts.FindByPredicate([subObjName](const FUnrealLuaObjectRepLayout& item)
    {
        return item.SubObjectPropertyName == subObjName;
    });
    return replayout;
}

void FLuaRepLayout::BuildSubObjectMappings()
{
    this->ObjectReplayouts.Empty();
    for(FUnrealLuaRepLayoutProperty& repProperty : this->RepLayoutProperties)
    {
        FUnrealLuaObjectRepLayout* objLayout = this->GetOrCreateObjectReplayout(repProperty.SubObject);
        objLayout->ReplicatedProperties.Emplace(&repProperty);
    }
}

ULoadedLuaScriptCollection::ULoadedLuaScriptCollection()
    : OwningContext(nullptr),
      ScriptTemplate(), Replayout(), bCheckedRepLayout(false)
{
}

void ULoadedLuaScriptCollection::Initialize(FScopedLuaContext* owningContext, const FName& fileName, FLoadLuaScriptResult& newScriptTemplate)
{
    verify(newScriptTemplate.IsValid())
    verify(owningContext != nullptr);
    verify(this->OwningContext == nullptr);
    this->OwningContext = owningContext;
    this->ScriptTemplate = FLuaScriptTemplate{newScriptTemplate.FinalResult};
    this->FileInfo = FLoadedLuaScriptCollectionFileInfo{ newScriptTemplate.OriginalFileRequestPath, newScriptTemplate.MainFileInfo, newScriptTemplate.ModFileInfos};
}

void ULoadedLuaScriptCollection::BeginDestroy()
{
    this->Reset();
    UObject::BeginDestroy();
}

void ULoadedLuaScriptCollection::Reset()
{
    this->bCheckedRepLayout = false;
    this->Replayout.Reset();
   // this->FunctionMap = {};
    if(this->OwningContext)
    {
        //this->OwningContext->RemoveScriptCollection(this);
    }
    if(!this->OwningContext || !this->OwningContext->IsLuaLoaded() )
    {
        return;
    }
    this->ScriptTemplate = {};
}

FLuaScriptInstanceHandle ULoadedLuaScriptCollection::GetInstancedLuaScript(FLuaUObjectItem& scriptOwner)
{
    //Create an empty Lua script instance, it still needs to copy values from the SharedScript
    //To do this, call InitializeLuaScriptInstance with the instance's owner
    
    return FLuaScriptInstanceHandle{this};
}

void ULoadedLuaScriptCollection::ApplyLuaScriptTemplateToUObject(FLuaUObjectItem& scriptOwner)
{
    FLuaScriptInstanceHandle& handle = scriptOwner.GetLuaScriptHandle();
    verify(handle.GetLuaScriptCollection() == this);
    verify(handle.GetScriptOwner() == scriptOwner.GetUObject());
    if (!handle.GetLuaScriptReloadDelegateHandle().IsValid())
    {
        //Assign a reload binding:
        //Since FLuaUObjectItems are all dynamically allocated, and thus never move around in memory,we can do a BindRaw
        //When the handle gets overwritten when loading or destroying the lua script, the FLuaScriptInstanceHandle destructor
        //will remove the raw binding
        handle.SetLuaScriptReloadDelegateHandle(this->OnScriptReload.AddRaw(&handle, &FLuaScriptInstanceHandle::NotifyLuaScriptReload));    
    }
    verify(handle.GetLuaScriptReloadDelegateHandle().IsValid())
    
    //Copy over Lua script values and functions from the template
    this->ScriptTemplate.GetScriptTable().for_each([&scriptOwner](const sol::object& key, const sol::object& value)
    {
        if(key.get_type() == sol::type::string)
        {
            //If the string has the same name as a UProperty of the Object,
            //this should automatically create property wrappers as LuaScriptValue type
            sol::string_view strv = key.as<sol::string_view>();
            scriptOwner.SetScriptValue(strv, value, false);
        }
    });
    
    bool enableTick = UUnrealLuaConfig::AllowOverrideTick() && scriptOwner.TickFunc.IsValid() && this->ScriptTemplate.StartWithTickEnabled(); 
    scriptOwner.SetLuaTickEnabled(enableTick);        
}

//only tracked tables can be reloaded
sol::table ULoadedLuaScriptCollection::GetLuaScriptAsTable(bool bTrackScript)
{
    if(this->OwningContext == nullptr)
    {
        return sol::nil;
    }
    verify(this->ScriptTemplate.IsValid());

    return this->ScriptTemplate.GetScriptTable();
}

bool ULoadedLuaScriptCollection::Reload()
{
    if (this->OwningContext == nullptr)
    {
        return false;
    }
    if(this->FileInfo.OriginalFileRequestPath.IsEmpty())
    {
        return true;
    }
    
    LUA_LOG("Reloading Lua file %s", *this->FileInfo.OriginalFileRequestPath);
    std::string path{TCHAR_TO_UTF8(*this->FileInfo.OriginalFileRequestPath)};
    //const sol::table reloadTable = this->OwningContext->ImportLuaScript(path, true, false);

    bool bIsAbsolutePath = this->FileInfo.OriginalFileRequestPath.StartsWith(UnrealLua::Paths::FullProjectDir);
    bool bAllowModding = !bIsAbsolutePath;
    this->OwningContext->LoadLuaScriptFromDisk(this->FileInfo.OriginalFileRequestPath, bIsAbsolutePath, bAllowModding);
    FLoadLuaScriptResult reloadLoadResult = this->OwningContext->LoadLuaScriptFromDisk(this->FileInfo.OriginalFileRequestPath, false, true);

    
    //if new script is bad, abort reload
    if(!reloadLoadResult.IsValid())
    {
        LUA_LOG_ERROR("Reloaded Lua file %s invalid, aborting reload. Reasons:", *this->FileInfo.OriginalFileRequestPath);
        for(auto& str : reloadLoadResult.ErrorMessages)
        {
            LUA_LOG_ERROR("Reason : %s", *str)
        }
        FDateTime currentTime = FDateTime::UtcNow();

        //mark all file infos as current time
        //as soon as one of the files gets updated we will reload it
        for(FLoadedLuaFileInfo& item : this->FileInfo.MainFileInfo)
        {
            item.TimeStamp = currentTime;
        } 
        return false;
    }

    sol::state_view lua = this->OwningContext->GetLuaThisState();
    FLuaScriptReloadCache reloadCache{lua.lua_state()};

    //first, notify all instances
    this->OnScriptReload.Broadcast(ELuaScriptReloadStage::PRERELOAD, &reloadCache);
    
    //Then, clear the script values
    this->OnScriptReload.Broadcast(ELuaScriptReloadStage::CLEARVALUES, nullptr);
    
    //read reloaded script attributes + rep layout

    //Reset RepLayout data
    this->bCheckedRepLayout = false;
    this->Replayout.Reset();

    //update times regardless of validity, so it doesn't spam the file scanner until file gets changed again
    this->FileInfo.MainFileInfo = reloadLoadResult.MainFileInfo;
    this->FileInfo.ModFileInfos = reloadLoadResult.ModFileInfos;

    //create new script template from newly loaded files
    this->ScriptTemplate = FLuaScriptTemplate(reloadLoadResult.FinalResult);
    
    //Let them reload script
    this->OnScriptReload.Broadcast(ELuaScriptReloadStage::RELOADSCRIPT, &reloadCache);
    
    return true;
}

sol::table ULoadedLuaScriptCollection::GetSubobjectOverridesForObjectWithName(const FString& name)
{
    if(!this->ScriptTemplate.IsValid() || name.IsEmpty())
    {
        return sol::nil;
    }
    auto casted = StringCast<char>(*name);

    sol::table overridesTable = this->ScriptTemplate.GetSubobjectOverrides();

    if(!overridesTable.valid())
    {
        return sol::nil;
    }
    
    sol::table overridesForThisSubobject = overridesTable[casted.Get()];
    if(!overridesForThisSubobject.valid())
    {
        return sol::nil;
    }
    return overridesForThisSubobject;
}

bool ULoadedLuaScriptCollection::ShouldOverrideInput() const
{
    return this->ScriptTemplate.ShouldOverrideInput();
}

bool ULoadedLuaScriptCollection::AutoRegisterReplicatedSubobject() const
{
    return this->ScriptTemplate.AutoRegisterReplicatedSubobject();
}

ELifetimeCondition ULoadedLuaScriptCollection::GetReplicationCondition() const
{
    return this->ScriptTemplate.GetObjectReplicationCondition();
}

//Is lazily done, only when an actor is actually about to be replicated
FLuaRepLayout* ULoadedLuaScriptCollection::GetRepLayout()
{
    if(bCheckedRepLayout)
    {
        //May return null if previously checked, but no rep data found
        return this->Replayout.GetMutablePtr<FLuaRepLayout>();
    }
    
    bCheckedRepLayout = true;
    
    if(!this->ScriptTemplate.IsValid())
    {
        return nullptr;
    }

    if(this->ScriptTemplate.GetObjectReplicationCondition() == COND_Never)
    {
        return nullptr;
    }

    sol::table replicatedProps = this->ScriptTemplate.GetRepLayoutTable();

    if(!replicatedProps.valid())
    {
        return nullptr;
    }

    this->Replayout.InitializeAs<FLuaRepLayout>();
    FLuaRepLayout& repLayout  = this->Replayout.GetMutable<FLuaRepLayout>();
    
    repLayout.ReplicationFrequency = this->ScriptTemplate.GetReplicationFrequency();
    repLayout.bAutoRegisterReplicatedSubobject = this->ScriptTemplate.AutoRegisterReplicatedSubobject();

    TSet<uint32> hashSet{};
    //First, examine the int-keyed proprties, 
    for(uint8 index = 1; index <= replicatedProps.size(); index++)
    {
        sol::object value_o = replicatedProps[index].get_or<sol::table>(sol::nil);
        if(value_o.get_type() != sol::type::table)
        {
            continue;
        }
        
        sol::table repPropTbl = value_o.as<sol::table>();
        if(!repPropTbl.valid())
        {
            continue;
        }

        FUnrealLuaRepLayoutProperty repPropData{};
        UnrealLua::PropertyHelper::InitializeStructFromTable(repPropData, repPropTbl);

        if(repPropData.Property == NAME_None)
        {
            continue;
        }
        
        //Make sure no one tried messing with the rep index during table analysis
        repPropData.RepLayoutPropertyIndex = index;

        //Convert FNames to std::string
        repPropData.StringKey = repPropData.Property.ToString();

        repLayout.RepLayoutProperties.Emplace(repPropData);

        repLayout.PropertyReplicationConditionFlags.AddUnique(repPropData.Condition);
    }

    //Note : Even if there are no replicated properties, it's still a valid Replayout,
    //so it counts as a replicated UObject
    
    repLayout.ObjectReplicationCondition = this->ScriptTemplate.GetObjectReplicationCondition();
    repLayout.BuildSubObjectMappings();

    return this->Replayout.GetMutablePtr<FLuaRepLayout>();
}

