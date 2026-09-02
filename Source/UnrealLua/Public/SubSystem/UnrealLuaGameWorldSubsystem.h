#pragma once
#include "LuaContext/GameLuaContext.h"
#include "Interface/LuaScriptable.h"
#include "ScriptableUObject/LuaScriptSettings.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/World.h"
#include "Misc/TVariant.h"
#include "UnrealLuaGameWorldSubsystem.generated.h"

struct FLuaValue;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameSessionEventDelegate);

namespace UnrealLua
{
	typedef TVariant<sol::function, FString> FTimerCallback;
}

struct UNREALLUA_API FLuaTimerData
{
	FLuaTimerData();

	static FLuaTimerData GenerateNewData();

	static int64 GenerateNewHandle();
	int64 LuaTimerHandle;
	FTimerHandle TimerManagerHandle = {};
	sol::object Target = {};
	sol::object Callback = {};
	std::vector<sol::object> Args = {};
	bool loop = false;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FFloatDelegate, float);
UCLASS(BlueprintType, Transient)
class UNREALLUA_API UUnrealLuaGameWorldSubsystem : public UTickableWorldSubsystem, public ILuaScriptable
{
	GENERATED_BODY()
public:
	FFloatDelegate OnTick;
	
	virtual bool IsTickableInEditor() const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	FString ParseGameMode();
	virtual void PostInitialize() override;
	void TryLoadGameMode();
	virtual TStatId GetStatId() const override;
	void NotifyWorldBeginPlayUpdate(bool worldHasBegunPlay);
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	UFUNCTION(BlueprintImplementableEvent)
	void ReceiveBeginPlay();
	
	virtual void Tick(float DeltaTime) override;
	UFUNCTION(BlueprintImplementableEvent)
	void ReceiveTick(float DeltaTime);
	void TickLuaContexts();
	
	void NotifyActorDestroyed(AActor* actor);
	
	virtual void Deinitialize() override;
	void NotifyPlaySessionEnded();
	
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	
	virtual ETickableTickType GetTickableTickType() const override;
	
	virtual FLuaScriptSettings GetLuaScriptSettings_Implementation() override;

	virtual void SetLuaScriptSettings_Implementation(FLuaScriptSettings newSettings) override;
	
	void NotifyLuaContextInitialized(UGameLuaContext* gameLuaContext);

	void RegisterManualTick(UObject* obj, bool bSetTickEnabled);
private:
	void ClearTimers();
	void SetLuaContext(UGameLuaContext* ctx);

	FTimerHandle tickContextsHandle = {};
public:

	FSimpleMulticastDelegate OnLuaReady = {};

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UWorld> World;

	UPROPERTY()
	FString UsedLuaGameModeName;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UGameLuaContext> LuaContext;
	
	UPROPERTY(VisibleAnywhere)
	FLuaScriptSettings LuaScriptSettings;
	UPROPERTY(VisibleAnywhere)
	FName PersistentLevelName;

	bool bHasBegunPlay = false;

	UFUNCTION()
	int64 SetTimer(UObject* target, FLuaValue callback, float interval, bool loop, float initialDelay, TArray<FLuaValue> additionalArgs);

	//UFUNCTION()
	//int64 SetTimerWithStringCallback(UObject* target, FString callback, float interval, bool loop, float initialDelay, TArray<FLuaValue> additionalArgs);

	UFUNCTION()
	int64 Delay(UObject* target, FLuaValue callback, float delay, TArray<FLuaValue> additionalArgs);
	
	//UFUNCTION()
	//int64 DelayWithStringCallback(UObject* target, FString callback, float delay, TArray<FLuaValue> additionalArgs);
	
	sol::object Delay(sol::variadic_args args, sol::this_state lua);
	sol::object SetTimer(sol::variadic_args args, sol::this_state lua);
	sol::object SetTimer(sol::object target, sol::object callback, float interval_o, bool loop_o, float initialDelay, std::vector<sol::object>& callbackArgs, sol::this_state lua);
	void ClearTimersForObject(const UObject* objectToClear);
	
	void NotifyTimerTriggered(int64 luaTimerHandle);

	TArray<FLuaTimerData> QueuedTimers = {};

	UPROPERTY(BlueprintAssignable)
	FOnGameSessionEventDelegate OnGameSessionBegin;
	
	UPROPERTY(BlueprintAssignable)
	FOnGameSessionEventDelegate OnGameSessionEnd;

	UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	bool IsPaused() const;
	
	UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	AActor* SpawnActor( UClass* InClass, FVector const Location, FRotator const Rotation);
	
	UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	AActor* SpawnActorAbsolute( UClass* Class, FTransform const& AbsoluteTransform);

	UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	AActor* SpawnActorDeferred(UClass* Class, FTransform const& Transform, AActor* Owner = nullptr, APawn* Instigator = nullptr, ESpawnActorCollisionHandlingMethod CollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::Undefined, ESpawnActorScaleMethod TransformScaleMethod = ESpawnActorScaleMethod::MultiplyWithRoot);
	
	UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	AGameModeBase* GetAuthGameMode() const;
	
	UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	AGameStateBase* GetGameState() const;

	//UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	ENetMode GetNetMode() const;
	
	UFUNCTION(Blueprintable)
	bool DestroyActor( AActor* Actor, bool bNetForce=false, bool bShouldModifyLevel=true );
};
