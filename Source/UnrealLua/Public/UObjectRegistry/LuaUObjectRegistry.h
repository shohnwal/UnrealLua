#pragma once 
#include "CoreMinimal.h"
#include "sol.hpp"
#include "Replication/LuaNetHandle.h"
#include "UObject/ScriptInterface.h"
class ILuaContext;
class AActor;
class ULuaScriptReplicationComponent;
struct FLuaScriptInstanceHandle;
struct FLuaUObjectItem;
struct FLuaClassOverrideRegistry;
struct FLuaUObjectItemHandle;

typedef TMulticastDelegate<void(UObject*)> FUObjectExistenceEventDelegate;

namespace UnrealLua::UObjectRegistry
{	
	UNREALLUA_API bool LoadLuaScript(UObject* obj, bool bForceReload);
	UNREALLUA_API bool LoadLuaScriptInternal(UObject* obj, bool bIsReloading, TScriptInterface<ILuaContext>& ictx);
	UNREALLUA_API bool LoadLuaScriptsForNetLoadActors(UWorld* world, bool bForceReload);
	
	
	UNREALLUA_API FLuaUObjectItemHandle* GetUObjectItemHandle(const UObject* object);
	UNREALLUA_API FLuaUObjectItemHandle* GetMetaObjectItemHandle(const UField* obj);
	
	UNREALLUA_API FLuaUObjectItem& GetUObjectItem(const UObject* Object);
	UNREALLUA_API FLuaUObjectItem* TryGetUObjectItem(int32 index);
	UNREALLUA_API FLuaUObjectItem* TryGetUObjectItem(const UObject* obj);
	UNREALLUA_API FLuaUObjectItem& GetMetaObjectItem(const UField* obj);
	
	//UNREALLUA_API inline FLuaUObjectItemHandle* GetUObjectItemHandle(const UObject* Object);

	UNREALLUA_API UObject* GetObject(int32 index);
	
	UNREALLUA_API FLuaNetHandle GetLuaNetHandleForObject(UObject* object);
	UNREALLUA_API int PushUObjectAsLightUserdata(lua_State* luaState, UObject* things);
	UNREALLUA_API sol::object GetUObjectAsLightUserdata(lua_State* luaState, UObject* object);
	UNREALLUA_API void LinkUpRegisteredUObject(UObject* obj);
	
	UNREALLUA_API FLuaScriptInstanceHandle& GetLuaScriptHandle(UObject* object);
	UNREALLUA_API FLuaScriptInstanceHandle& GetLuaScriptHandle(ULuaScriptReplicationComponent* replicator);
	
	UNREALLUA_API FUObjectExistenceEventDelegate& OnNewObjectEvent();
	UNREALLUA_API FUObjectExistenceEventDelegate& OnRemovedObjectEvent();

	UNREALLUA_API void NotifyUObjectCreated(UObject* Object, int32 Index);
	UNREALLUA_API void NotifyUObjectDeleted(const UObjectBase* Object, int32 Index);
	UNREALLUA_API void NotifyActorDestroyed(AActor* actor);
	
	UNREALLUA_API void CleanUpObjectsForLuaContext(const TScriptInterface<ILuaContext>& ictx);
	
	UNREALLUA_API sol::object GetEnumWrapperLuaObject(UEnum* uenum, sol::this_state lua);
	UNREALLUA_API sol::object GetEnumValueWrapper(UEnum* uenum, int64 value, sol::this_state lua);
	UNREALLUA_API int PushEnumValueWrapper(TObjectPtr<UEnum> Enum, int64 Val, sol::this_state Lua);

	void UNREALLUA_API RemoveUsedItem(FLuaUObjectItem* luaUObjectItem);
	
	void UNREALLUA_API RequestMakeUClassOverridable(UClass* uclass);
	
	UNREALLUA_API FLuaClassOverrideRegistry& GetLuaClassOverrideRegistry();
}
