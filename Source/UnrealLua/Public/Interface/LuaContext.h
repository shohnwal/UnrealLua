// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LuaContext/LuaPath.h"
#include "UObject/Interface.h"
#include "LuaContext.generated.h"

enum class ELuaPathFlags : uint8;
class ILuaContext;
enum class ELuaLoadEventType : uint8;
struct FLoadedLuaGameModeSettings;
struct FScopedLuaContext;


// This class does not need to be modified.
UINTERFACE()
class UNREALLUA_API ULuaContext : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
namespace UnrealLua
{
	//static TScriptInterface<ILuaContext> GLuaContext = nullptr;
}


class UNREALLUA_API ILuaContext
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	virtual FScopedLuaContext& GetScopedLuaContext() = 0;
	virtual void LoadGameMode(const FName& name) = 0;
	virtual FLoadedLuaGameModeSettings& GetLoadedLuaModeSettings() = 0;
	virtual bool AllowMods() = 0;
	virtual void BroadcastLoadEvent(TScriptInterface<ILuaContext> ictx, FName gameModeName, ELuaLoadEventType loadState) = 0;
	UWorld* GetWorldFromUObject() const;
protected:
	void SetupLuaGameModeInternal(const FName& gameMode, const ELuaPathFlags luaPathFlags = ELuaPathFlags::AnyExceptUTypes);
};
