#pragma once
#include "Subsystems/GameInstanceSubsystem.h"
#include "Mods/UnrealLuaGameModInfo.h"
#include "UObject/ScriptInterface.h"
#include "UnrealLuaModSubsystem.generated.h"

class ILuaContext;
enum class ELuaLoadEventType : uint8;
class UUnrealLuaMod;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnModsLockedDelegate, bool, bModsAreLocked);
UCLASS(BlueprintType)
class UNREALLUA_API UUnrealLuaModSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:

	UPROPERTY(BlueprintAssignable)
	FOnModsLockedDelegate OnModsLocked;

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
	UFUNCTION()
	void RefreshMods();

	UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	UUnrealLuaMod* GetMod(const FString& modName);
	
	UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	UUnrealLuaMod* FindMod(const FString& modName);
	
	UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	void SetModEnabled(const FString& modName, bool bIsEnabled);

	UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	void SetModsEnabled(const TArray<FString>& modNames, bool bIsEnabled);

	UFUNCTION(BlueprintCallable, Category= "UnrealLua")
	TArray<UUnrealLuaMod*> GetEnabledMods() const;
	
	UFUNCTION()
	TArray<FString> GetModsNames() const;
	UFUNCTION()
	TArray<FString> GetEnabledModsNames() const;

	UFUNCTION()
	bool AreModsLocked() const;

	UFUNCTION()
	void NotifyWorldBeginPlayUpdate(bool bHasBegunPlay);
	
	void NotifyWorldBeginPlay();
	void NotifyWorldEndPlay();
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	bool bModsLocked;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	FUnrealLuaGameModInfo ModInfo;
	
	UPROPERTY(VisibleAnywhere)
	TArray<TObjectPtr<UUnrealLuaMod>> DiscoveredMods;

	UPROPERTY(VisibleAnywhere)
	TArray<TObjectPtr<UUnrealLuaMod>> ActiveMods;

	UPROPERTY(VisibleAnywhere)
	TArray<TObjectPtr<UUnrealLuaMod>> InactiveMods;

	UFUNCTION()
	void NotifyLuaLoadUpdate(TScriptInterface<ILuaContext> ctx, FName gameMode, ELuaLoadEventType loadEvent);

	void LockMods();
	void UnlockMods();
};
