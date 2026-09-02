#pragma once
#include "CoreMinimal.h"
#include "LuaToolsSession.h"
#include "Engine/HitResult.h"
#include "Subsystem/UnrealLuaTools.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tools/UnrealLuaToolDelegates.h"
#include "UnrealLuaToolsSession.generated.h"
class UGameLuaContext;
class SUnrealLuaDraggableBoxOverlay;
class SUnrealLuaMainMenu;
class SConstraintCanvas;
class SUnrealLuaObjectInspector;
class UUnrealLuaTool;
class UUnrealLuaToolsMainMenu;
class UGameViewportClient;
class UGameInstance;
struct FInstancedStruct;

UCLASS()
class UNREALLUATOOLS_API UUnrealLuaToolsSession : public UGameInstanceSubsystem, public ILuaToolsSession
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
	void NotifyGameInstanceStart(UGameInstance* gameInstance);
	
	virtual void Deinitialize() override;
	
	void SetActiveTool(UUnrealLuaTool* tool, const FUnrealLuaTooleActivateCallback& preActivateCallback = {});
	void SetActiveTool(TSubclassOf<UUnrealLuaTool> toolClass, const FUnrealLuaTooleActivateCallback& preActivateCallback = {});
	void DeactivateTool(UUnrealLuaTool* toolToDeactivate);
	void DeactivateTool(TSubclassOf<UUnrealLuaTool> debugToolClass);
	bool IsCurrentTool(TSubclassOf<UUnrealLuaTool> debugToolClass) const;
	bool IsCurrentTool(const UUnrealLuaTool* querier) const;
	bool ToggleTool(TSubclassOf<UUnrealLuaTool> debugToolClass);
	void DeactivateCurrentToolInternal();
	
	UGameLuaContext* GetLuaContext() const;
	virtual TSharedPtr<FScopedLuaContext> GetScopedLuaContext() const override;
	virtual ELuaToolsSessionType GetSessionType() const override { return ELuaToolsSessionType::Game; }

	void NotifyInputKeyEvent(const FInputKeyEventArgs& InputKeyEventArgs);
	void Tick(float dt);
	
	
	UUnrealLuaTool* GetTool(TSubclassOf<UUnrealLuaTool> toolClass) const;
	
	template<typename T>
	requires std::derived_from<T, UUnrealLuaTool>
	T* GetTool() const
	{
		return Cast<T>(this->GetTool(T::StaticClass()));
	}
	
	void UpdateInputMode();
	virtual void AddInputModeOverride(TSharedRef<SGamescreenDockableWindowWidget> widget) override;
	virtual void RemoveInputModeOverride(TSharedRef<SGamescreenDockableWindowWidget> widget) override;
	
	virtual UGameViewportClient* GetViewportClient() const override;
	virtual UGameInstance* GetGameInstance() const override;
	virtual FOnInputKeySignature& GetOninputKeyEvent() override;
	virtual TSharedPtr<SConstraintCanvas> GetCanvas() const override;

	UPROPERTY(VisibleAnywhere, Category = "UnrealLua")
	TWeakObjectPtr<UGameViewportClient> GameViewportClient = nullptr;
	UPROPERTY(VisibleAnywhere, Category = "UnrealLua")
	TMap<TObjectPtr<UClass>, TObjectPtr<UUnrealLuaTool>> Tools = {};
	UPROPERTY(VisibleAnywhere, Category = "UnrealLua")
	TObjectPtr<UUnrealLuaTool> CurrentTool = {};
	UPROPERTY(VisibleAnywhere, Category = "UnrealLua")
	TObjectPtr<UGameLuaContext> GameLuaContext = nullptr; 

	TSharedPtr<SUnrealLuaMainMenu> MainMenu = nullptr;
	TSharedPtr<SConstraintCanvas> MainCanvas = nullptr;
	
	FKey DebugKey = EKeys::F6;
	
	TArray<TWeakPtr<SGamescreenDockableWindowWidget>> InputModeOverriders = {};
	
	FOnInputKeySignature OnInputKeyEvent = {};
};