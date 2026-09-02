// Fill out your copyright notice in the Description page of Project Settings.


#include "ToolWidgets/SLuaNewDefaultScriptEditorWindow.h"

#include "SlateOptMacros.h"
#include "UnrealEngine.h"
#include "Framework/Notifications/NotificationManager.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/MessageDialog.h"
#include "Reflection/PropertyHelper_Utility.h"
#include "UIWidgets/GamescreenDockableWindowWidget.h"
#include "UIWidgets/SPropertySelectionList.h"
#include "UIWidgets/SUFunctionSelectionList.h"
#include "UnrealOverrides/LuaClassOverrideRegistry.h"
#include "UObjectRegistry/LuaUObjectRegistry.h"
#include "Utility/LuaLogMacros.h"
#include "Utility/ParseUObjectToLua.h"
#include "Widgets/SWindow.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Widgets/Text/STextBlock.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SLuaNewDefaultScriptEditorWindow::Construct(const FArguments& InArgs)
{
	this->ScriptClass = TStrongObjectPtr<UClass>(InArgs._UClass);
	this->OnDefaultScriptCreated = InArgs._OnDefaultScriptCreated;	
	verify(this->ScriptClass.IsValid());
	
	struct FAvailableFunctionInfo
	{
		UFunction* func = nullptr;
		bool overridable = false;
	};
	const TArray<FName>& forbiddenFunctions = InArgs._ForbiddenFunctions;
	const TArray<FName>& preselectedFunctionnames = InArgs._PreselectedFunctionNames;
	const TArray<UFunction*> preselectedFunctions = InArgs._PreselectedFunctions;
	
	FString shortFilePath = "Lua/" + FLuaClassOverrideRegistry::GetDefaultLuaScriptPathForUClass_LuaFolderRelative(this->ScriptClass.Get(), true);
	FString relativeFilePath = 	FPaths::ProjectContentDir() + shortFilePath;
	FString fullFilePath = FPaths::ConvertRelativePathToFull(relativeFilePath);
	
	auto normalStyle = FAppStyle::GetWidgetStyle<FTextBlockStyle>("NormalText");
	

	bool fileAlreadyExists = IFileManager::Get().FileExists(*fullFilePath);
	
	FString fileSavingMsg = fileAlreadyExists
		? FString::Printf(TEXT("Warning: File already exists: %s"), *shortFilePath)
		: FString::Printf(TEXT("File location: %s"), *shortFilePath);
	FSlateColor fileSavingMsgColor = fileAlreadyExists ? FSlateColor(FLinearColor{1, 1, 0, 1}) : normalStyle.ColorAndOpacity;
	
	SGamescreenDockableWindowWidget::Construct(SGamescreenDockableWindowWidget::FArguments()
	.ExternalWindowAnchors(InArgs._ExternalWindowAnchors)
	.ExternalWindowSize(InArgs._ExternalWindowSize)
	.ExternalWindowPosition(InArgs._ExternalWindowPosition)
	.GameScreenAnchors(InArgs._GameScreenAnchors)
	.BackgroundColor(InArgs._BackgroundColor)
	.Title(InArgs._Title)
	.Session(InArgs._Session)
	.GameScreenAlignment(InArgs._GameScreenAlignment)
	.InitiallyHidden(InArgs._InitiallyHidden)
	.StartAsWindow(InArgs._StartAsWindow)
	.DraggableInGameScreen(true)
	);
	
	this->WindowBodyVBox->AddSlot()
	[
		SNew(STextBlock)
		.Text(FText::FromString(FString::Printf(TEXT("Creating Lua file for UClass %s"),*this->ScriptClass.Get()->GetName())))
		.Justification(ETextJustify::Center)
	]
	.VAlign(VAlign_Top)
	.AutoHeight()
	.Padding(10,10,10,10);
	
	this->WindowBodyVBox->AddSlot()
	[
		SNew(STextBlock)
		.Text(FText::FromString(fileSavingMsg))
		.Justification(ETextJustify::Center)
		.ColorAndOpacity(fileSavingMsgColor)
	]
	.Padding(10,10,10,10)
	.AutoHeight();
	
	TArray<UFunction*> availableFunctions = SUFunctionSelectionList::CreateDefaultFunctionSelectionList(this->ScriptClass.Get(), forbiddenFunctions);
	
	this->WindowBodyVBox->AddSlot()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		[
			ConstructEditableTextBox(InArgs)
		]
		.HAlign(HAlign_Fill)
		.Padding(10,0,10,0)
		+ SHorizontalBox::Slot()
		[
			ConstructWidgetSwitcher(this->ScriptClass.Get(), availableFunctions, preselectedFunctions, preselectedFunctionnames)
		]
		.HAlign(HAlign_Right)
		.AutoWidth()
		.Padding(0,0,10,0)
	]
	.VAlign(VAlign_Fill);
	
	this->WindowBodyVBox->AddSlot()
	[
		SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			[
				SNew(SButton)
				[
					SNew(STextBlock)
					.Text(FText::AsCultureInvariant("Ok"))
					.Font(FAppStyle::GetWidgetStyle<FTextBlockStyle>("NormalText").Font)
					.Visibility(EVisibility::SelfHitTestInvisible)
					.Margin(FMargin(2,2,2,2))
				]
				.ButtonStyle(&FButtonStyle::GetDefault())
				.OnClicked_Lambda([this, fullFilePath, shortFilePath]()
				{
					if (!this->ScriptClass.IsValid())
					{
						return FReply::Handled();
					}
					if (IFileManager::Get().FileExists(*fullFilePath))
					{
						FText displayText = FText::AsCultureInvariant("File already exists. Do you want to overwrite it?\n" + shortFilePath);
						const EAppReturnType::Type clicked = FMessageDialog::Open(EAppMsgCategory::Warning, EAppMsgType::YesNo, displayText);
						if (clicked == EAppReturnType::No)
						{
							return FReply::Handled();
						}
					}
					FString fileContent = this->EditTextBox->GetText().ToString();
								
					if (FFileHelper::SaveStringToFile(fileContent, *fullFilePath))
					{
						FString popupMessage = "Created default UObject Lua script file:\n" + fullFilePath;
						LUA_LOG("%s", *popupMessage)
	
						FNotificationInfo Info(FText::AsCultureInvariant(popupMessage));
						Info.ExpireDuration = 10.0f;
						Info.FadeOutDuration = 0.2f;
						Info.bFireAndForget = true;
						FSlateNotificationManager::Get().AddNotification(Info);

						FLuaClassOverrideRegistry& registry = UnrealLua::UObjectRegistry::GetLuaClassOverrideRegistry();
						registry.RequestMakeUClassOverridable(this->ScriptClass.Get());
						
						this->OnDefaultScriptCreated.ExecuteIfBound();
						
						this->Shutdown();
					}
					return FReply::Handled();;
				})
			]
			.AutoWidth()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.FillWidth(1)
			+ SHorizontalBox::Slot()
			[
				SNew(SButton)
				[
					SNew(STextBlock).Text(FText::AsCultureInvariant("Cancel"))
					.Font(FAppStyle::GetWidgetStyle<FTextBlockStyle>("NormalText").Font)
					.Visibility(EVisibility::SelfHitTestInvisible)
					.Margin(FMargin(2,2,2,2))
				]
				.ButtonStyle(&FButtonStyle::GetDefault())
				.OnClicked_Lambda([this]()
				{
					this->Shutdown();
					return FReply::Handled();
				})
			]
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.AutoWidth()
			.FillWidth(1)
			.Padding(10,10,10,10)
	]
	.VAlign(VAlign_Bottom)
	.AutoHeight();
	
	this->FillTextBox();
}

void SLuaNewDefaultScriptEditorWindow::FillTextBox()
{
	this->EditTextBox->SetText(FText::GetEmpty());
	
	TCHAR prefix = this->ScriptClass->IsChildOf<AActor>() ? 'A' : 'U';
	FString tableName = "Script";
	FStringBuilderBase fileBuilder;
	fileBuilder << "---@Type " << prefix << this->ScriptClass->GetName() << "\n";
	fileBuilder << "local " << tableName << " = ... or {}\n";
	fileBuilder << "\n";
	
	for (FProperty* prop : this->FunctionsPropertySwitcher->PropertyListWidget->GetSelectedProperties())
	{
		fileBuilder << UnrealLua::ParseUtility::ParsePropertyToLuaFunctionTemplateString(tableName, prop);
		fileBuilder << "\n";		
	}

	for (UFunction* func : this->FunctionsPropertySwitcher->FunctionListWidget->GetSelectedFunctions())
	{
		fileBuilder << UnrealLua::ParseUtility::ParseUFunctionToLuaFunctionTemplateString(tableName, func);
		fileBuilder << "\n";
	}
	fileBuilder << "return Script\n";

	this->EditTextBox->SetText(FText::FromString(fileBuilder.ToString()));
}

TSharedRef<SMultiLineEditableTextBox> SLuaNewDefaultScriptEditorWindow::ConstructEditableTextBox(const FArguments& InArgs)
{
	this->EditTextBox = SNew(SMultiLineEditableTextBox)
	//.Text(FText::FromString(InArgs._FileContent))
	.Text(FText::GetEmpty())
	.IsReadOnly(true);
	return this->EditTextBox.ToSharedRef();
}

TSharedRef<SFunctionPropertyListSwitcher> SLuaNewDefaultScriptEditorWindow::ConstructWidgetSwitcher(UStruct* ustruct, const TArray<UFunction*>& functions, const TArray<UFunction*>& preSelectedFunctions, const TArray<FName>& preSelectedFunctionNames)
{
	this->FunctionsPropertySwitcher = SNew(SFunctionPropertyListSwitcher)
		.TargetStruct(ustruct)
		.Functions(functions)
		.SingleChoiceMode(false)
		.PreselectedFunctions(preSelectedFunctions)
		.PreselectedFunctionNames(preSelectedFunctionNames)
		;
	this->FunctionsPropertySwitcher->OnFunctionCheckboxChanged.BindSP(this, &SLuaNewDefaultScriptEditorWindow::NotifyCheckedFunctionChanged);
	this->FunctionsPropertySwitcher->OnPropertyCheckboxChanged.BindSP(this, &SLuaNewDefaultScriptEditorWindow::NotifyCheckedPropertyChanged);
	return this->FunctionsPropertySwitcher.ToSharedRef();
}


void SLuaNewDefaultScriptEditorWindow::NotifyCheckedFunctionChanged(UFunction* function, bool bIsChecked)
{
	this->FillTextBox();
}

void SLuaNewDefaultScriptEditorWindow::NotifyCheckedPropertyChanged(FProperty* property, bool bIsChecked)
{
	this->FillTextBox();
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
