#pragma once

#include "CoreMinimal.h"
//#include "EditorSubsystem.h"
#include "LoadedLuaGameModeSettings.h"
#include "Interface/LuaContext.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LuaContextHelper.h"
#include "sol/sol.hpp"
#include "Engine/EngineBaseTypes.h"

#include "GameLuaContext.generated.h"
/**
 * 
 */

class USeamlessTravelLuaDataStorage;

UENUM()
enum class ELuaStateType : uint8
{
	DefaultState
};

class UGameLuaContext;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnGameLuaContextInitializedDelegate, UGameLuaContext*);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnLevelTravelUpdate, UGameLuaContext*, gameLuaContext, bool, bIsLeavingMap, bool, isSeamlessTravel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorldBeginPlayUpdateDelegate, bool, bWorldHasBegunPlay);

class UNetConnection;
class AGameModeBase;
UCLASS(BlueprintType)
class UNREALLUA_API UGameLuaContext : public UGameInstanceSubsystem, public ILuaContext
{
public:
	GENERATED_BODY()
	UGameLuaContext();

	UPROPERTY(BlueprintAssignable)
	FOnWorldBeginPlayUpdateDelegate OnWorldBeginPlayUpdate;
	
	UPROPERTY(BlueprintAssignable)
	FOnLevelTravelUpdate OnLevelTravelUpdate;

	FOnLuaGameModeLoadEventNative OnLuaGameModeReloadEventNative;
	
	UPROPERTY(BlueprintAssignable)
	FOnLuaGameModeLoadEventDelegate OnLuaGameModeReloadEvent;

	static UGameLuaContext* Get(UObject* worldContext);

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	sol::state_view GetLuaState();

	virtual bool IsReadyForFinishDestroy() override;
	
	virtual void LoadGameMode(const FName& name) override;
	void UnloadLuaGameMode();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
	virtual bool AllowMods() override { return true; }
	
	bool CanRunLua() const;
	
	void NotifyPreClientTravel(const FString& /*PendingURL*/, ETravelType /*TravelType*/, bool /*bIsSeamlessTravel*/);
	void Tick(float deltaTime);
	
	virtual void Deinitialize() override;
	virtual FScopedLuaContext& GetScopedLuaContext() override;
	TSharedPtr<FScopedLuaContext> GetScopedLuaContextSharedPtr();
	//bool HandleNetReceivedLuaGameModeString(UNetConnection* connection, uint8 MessageByte, const FString& MessageStr);
public:
	virtual FLoadedLuaGameModeSettings& GetLoadedLuaModeSettings() override;
	virtual void BroadcastLoadEvent(TScriptInterface<ILuaContext> ictx, FName gameModeName, ELuaLoadEventType loadState) override;
	void UnloadLua();
	bool IsLuaLoaded();
	TScriptInterface<ILuaContext> GetBlueprintLuaContext(ELuaStateType luaStateType);

	void NotifyPreWorldChange(bool bSeamlessTravel);
	
	void NotifyWorldBeginPlay();
	void NotifyWorldEndPlay();

	UPROPERTY()
	TObjectPtr<USeamlessTravelLuaDataStorage> SeamlessTravelLuaDataStorage = nullptr;
private:
	TSharedPtr<FScopedLuaContext> LuaContext;

	UPROPERTY()
	TMap<ELuaStateType, TScriptInterface<ILuaContext>> ChildLuaStates;
};