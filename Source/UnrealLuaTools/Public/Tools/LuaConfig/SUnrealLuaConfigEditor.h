// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UIWidgets/GamescreenDockableWindowWidget.h"
#include "Config/UnrealLuaConfig.h"


class SEditableTextBox;
class SGridPanel;

struct FLuaConfigEditorBodyTemplateRow
{
	FString RowTitle;
	TSharedPtr<SWidget> Content;
	FString Description;
};

struct FLuaConfigEditorBodyTemplate
{
	FString BodyTitle;
	TArray<FLuaConfigEditorBodyTemplateRow> Rows;
};

class SWidgetSwitcher;

class UNREALLUATOOLS_API SUnrealLuaConfigEditor : public SGamescreenDockableWindowWidget
{
public:
	SLATE_BEGIN_ARGS(SUnrealLuaConfigEditor)
		{
		}
		//GameScreenDockableWindowWidget
	SLATE_ARGUMENT(TScriptInterface<ILuaToolsSession>, Session)
	SLATE_ARGUMENT(FString, Title)
	SLATE_ATTRIBUTE(FAnchors, GameScreenAnchors)
	SLATE_ATTRIBUTE(FAnchors, ExternalWindowAnchors)
	SLATE_ATTRIBUTE(FVector2D, ExternalWindowSize)
	SLATE_ATTRIBUTE(FVector2D, ExternalWindowPosition)
	SLATE_ATTRIBUTE(FLinearColor, BackgroundColor)
	SLATE_ARGUMENT(FVector2D, GameScreenAlignment)
	SLATE_ARGUMENT(bool, InitiallyHidden)
	SLATE_ARGUMENT(bool, StartAsWindow)
	SLATE_END_ARGS()

	enum EConfigBodyType
	{
		General, GC, Advanced, Mods
	};
	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	void NotifyOkButtonClicked();
	void NotifyCancelButtonClicked();

	void NotifyDebugKeyTextBoxInput(const FKeyEvent& key);

protected:
	FLuaConfigEditorBodyTemplate MakeBodyTemplate(EConfigBodyType bodyType);
	void LoadDataFromConfig();
	
	void AddCategory(FString buttonName, FString tooltip, TSharedRef<SWidget> content);
 	
 	TSharedRef<SBox> MakeCategoryBody(const FLuaConfigEditorBodyTemplate& bodyTemplate);
	
	void FillCategoryBody(TSharedRef<SGridPanel> contentGrid, const FLuaConfigEditorBodyTemplate& bodyTemplate, TSharedRef<STextBlock> descriptionText);
	
	FUnrealLuaConfigData TempConfigData = {};
	TSharedPtr<SVerticalBox> CategoryVBox = {};
	TSharedPtr<SWidgetSwitcher> SelectedCategoryContentSwitcher = {};
	
	TSharedPtr<SEditableTextBox> DebugKeyTextBox;
};
