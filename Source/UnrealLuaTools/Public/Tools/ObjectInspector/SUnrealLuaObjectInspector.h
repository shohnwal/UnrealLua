// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interface/LuaContext.h"
#include "UIWidgets/GamescreenDockableWindowWidget.h"
#include "UIWidgets/ObjectHierarchy/UObjectHierarchyWidget.h"
#include "UObject/Object.h"
#include "Utility/UnrealLuaSimpleDelegateSignatures.h"
#include "Widgets/SCompoundWidget.h"

class SUObjectHierarchyWidget;
class SLuaScriptValueEditor;
class UUnrealLuaToolsSession;
class UButton;
class SConstraintCanvas;
class SScrollBox;
class SBorder;
class SSubobjectListWidget;
class SHorizontalBox;
class SButton;
class STextBlock;
class SOverlay;
class UGameInstance;
class UUnrealLuaUserWidget;

/**
 * 
 */

class UNREALLUATOOLS_API SUnrealLuaObjectInspector : public SGamescreenDockableWindowWidget
{
	SLATE_BEGIN_ARGS(SUnrealLuaObjectInspector)
		: _Session(nullptr), _StartAsWindow(false)
	{
	}
	SLATE_ARGUMENT(UUnrealLuaToolsSession*, Session)
	SLATE_ARGUMENT(TSharedPtr<SConstraintCanvas>, HostCanvas)
	SLATE_ARGUMENT(FString, Title)
	SLATE_ATTRIBUTE(FAnchors, GameScreenAnchors)
	SLATE_ATTRIBUTE(FAnchors, ExternalWindowAnchors)
	SLATE_ATTRIBUTE(FVector2D, ExternalWindowSize)
	SLATE_ATTRIBUTE(FVector2D, ExternalWindowPosition)
	SLATE_ATTRIBUTE(FLinearColor, BackgroundColor)
	SLATE_EVENT(FSimpleDelegate, OnRequestActorSelection)
	SLATE_ARGUMENT(FVector2D, GameScreenAlignment)
	SLATE_ARGUMENT(bool, InitiallyHidden)
	SLATE_ARGUMENT(bool, StartAsWindow)
	SLATE_EVENT(FObjectStringDelegate, OnRequestEditLuaScriptValue)
	SLATE_END_ARGS()
public:
	void Construct(const FArguments& InArgs);
	virtual void Shutdown() override;
	
	virtual EDockableWindowWidgetOnCloseExternalWindowBehavior GetOnCloseExternalWindowBehavior() const override;
	virtual EDockableWindowWidgetOnCloseGameScreenWidgetBehavior GetOnCloseGameScreenWidgetBehavior() const override;
	
	void NotifyHierarchyItemSelected(TSharedPtr<FUnrealLuaWatchedObjectHierarchyItem> SubobjectHierarchyItem);
	
	virtual FReply NotifyGameScreenCloseButtonClicked() override;
	FReply NotifySelectParentButtonClicked();
	FReply NotifySelectActorButtonClicked();
	FReply NotifySelectWorldButtonClicked();
	FReply NotifyNewScriptValueButtonClicked();
	FReply NotifyLuaContextButtonClicked();
	FReply NotifyCreateDefaultLuaScriptButtonClicked();
	FReply NotifyRunLuaScriptButtonClicked();
	FReply NotifyRequestReloadLuaScriptButtonClicked();
	void NotifyRequestEditLuaScriptValue(FString key);
	void NotifySelectUObjectFromLuaScriptValueKey(FString key);
	void NotifyLuaScriptValueEditorShutdown(TSharedRef<SGamescreenDockableWindowWidget> endingEditor);
	FReply NotifyEditLoadedLuaScript();

	void NotifyUClassOverrideFinished(UClass* uclass);
	void UpdateLuaScriptableObjectFunctionality();
	
	void ClearWatchedData();
	void SelectMainObject(UObject* mainObject);
	void SetWatchedUObject(UObject* newWatchedUObject);
	bool IsValid() const;
	UObject* GetUObject() const;

	
	
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	void NotifyInputKeyEvent(const FInputKeyEventArgs& inputEvent);
private:
	bool HasLuaScriptableObject() const;
	void RefreshLuaScriptValueList();
	void RebuildLuaScriptValueList(const TArray<FString>& keepOpenKeys = {});
	void NotifyWatcherWindowClosed(const TSharedRef<SWindow>& window);
	void UpdateOuterObjectSection(UObject* newOuter);
	TSharedPtr<SLuaScriptValueEditor> FindLuaScriptValueEditor(const FString& key);
	TSharedPtr<SWindow> GetParentWindowIfInWindow();
	
	void NotifyObjectLuaScriptApplied(UObject* Object);
public:
	
	bool bHadValidObjectLastTick = false;
	TWeakObjectPtr<UObject> SelectedMainObject = {};
	TWeakObjectPtr<UObject> WatchedObject = {};
		
	TSharedPtr<STextBlock> WatchedObjectOwnerLabel = {};
	TSharedPtr<STextBlock> WatchedObjectLabel = {};
	TSharedPtr<STextBlock> OuterObjectText = {};
	TSharedPtr<SHorizontalBox> ParentHBox = {};
	
	TSharedPtr<SButton> SelectParentButton = {};
	
	FSimpleDelegate OnRequestActorSelection = {};
	TSharedPtr<SBorder> LuaValuesListBorder = {};
	TSharedPtr<SScrollBox> LuaValuesListScrollBox = {};
	TSharedPtr<SButton> CreateDefaultLuaScriptButton = {};
	TSharedPtr<SButton> ReloadLuaScriptButton;
	TSharedPtr<SButton> RunScriptButton;
	
	FObjectStringDelegate OnRequestEditLuaScriptValue = {};
	
	TArray<TWeakPtr<SLuaScriptValueEditor>> LuaScriptValueEditors = {};
	TSharedPtr<SButton> EditLoadedLuaScriptButton = {};
	TSharedPtr<SVerticalBox> LoadedFilesVBox = {};
	TSharedPtr<SUObjectHierarchyWidget> SubobjectsBrowser = {};
};
