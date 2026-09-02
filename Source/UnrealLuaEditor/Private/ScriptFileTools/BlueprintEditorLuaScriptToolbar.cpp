// Fill out your copyright notice in the Description page of Project Settings.


#include "ScriptFileTools/BlueprintEditorLuaScriptToolbar.h"

#include "BlueprintEditorModule.h"
#include "DetailsViews/LuaScriptValueDetails.h"
#include "Framework/Application/SlateApplication.h"
#include "Interfaces/IMainFrameModule.h"
#include "Misc/FileHelper.h"
#include "Subsystem/UnrealLuaTools.h"
#define LOCTEXT_NAMESPACE "UnrealLuaEditor"

class SLuaNewDefaultScriptEditorWindow;

void FBlueprintEditorLuaScriptToolbar::Initialize()
{
	FBlueprintEditorLuaScriptToolbarCommands::Register();
	auto& BlueprintModule = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>("Kismet");
	BlueprintModule.OnGatherBlueprintMenuExtensions().AddStatic(&FBlueprintEditorLuaScriptToolbar::OnGatherExtensions);
}

void FBlueprintEditorLuaScriptToolbar::OnGatherExtensions(TSharedPtr<FExtender> Extender, UBlueprint* Blueprint)
{
	// This is called for all blueprint editors.
	// So first make sure we want to modify the toolbar for this one.
	//if (Blueprint == nullptr)
	//	return;
	//if (Blueprint->ParentClass == nullptr)
	//	return;
	//if (Blueprint->ParentClass->IsChildOf(UMyClass::StaticClass()) == false)
	//	return;

	const FBlueprintEditorLuaScriptToolbarCommands& Commands = FBlueprintEditorLuaScriptToolbarCommands::Get();

	// This specific editor needs its own Command List with delegates that include the blueprint pointer
	TSharedPtr<FUICommandList> CommandList = MakeShareable(new FUICommandList);
	CommandList->MapAction(
		Commands.ShowLuaScriptWindowUICommand,
		FExecuteAction::CreateStatic(
			&FBlueprintEditorLuaScriptToolbar::ShowLuaScriptWindow,
			Blueprint));

	Extender->AddToolBarExtension(
		"Asset", 
		EExtensionHook::After, 
		CommandList,
		FToolBarExtensionDelegate::CreateStatic(&FBlueprintEditorLuaScriptToolbar::ExtendToolBar));
	
	Extender->AddMenuExtension(
	"Tools", 
	EExtensionHook::After, 
	CommandList,
	FMenuExtensionDelegate::CreateStatic(&FBlueprintEditorLuaScriptToolbar::ExtendToolsMenu));
}

void FBlueprintEditorLuaScriptToolbar::ExtendToolBar(class FToolBarBuilder& Builder)
{
	//FSlateIcon IconBrush = FSlateIcon(
	//FAppStyle::GetAppStyleSetName(),
	//"MyClassEditor.DoMyThing", 
	//"MyClassEditor.DoMyThing.Small");

	const FBlueprintEditorLuaScriptToolbarCommands& Commands = FBlueprintEditorLuaScriptToolbarCommands::Get();

	Builder.AddToolBarButton(
		Commands.ShowLuaScriptWindowUICommand,
		NAME_None, 
		FText::AsCultureInvariant("Lua"),
		TAttribute<FText>(),
		FSlateIcon()
	);
}

void FBlueprintEditorLuaScriptToolbar::ExtendToolsMenu(FMenuBuilder& MenuBuilder)
{
	const FBlueprintEditorLuaScriptToolbarCommands& Commands = FBlueprintEditorLuaScriptToolbarCommands::Get();

	MenuBuilder.AddMenuEntry(
		Commands.ShowLuaScriptWindowUICommand,
		NAME_None, 
		FText::AsCultureInvariant("Lua Script Editor"),
		TAttribute<FText>(),
		FSlateIcon()
	);
}

void FBlueprintEditorLuaScriptToolbar::ShowLuaScriptWindow(UBlueprint* Blueprint)
{
	if (Blueprint == nullptr)
	{
		return;
	}
	UClass* generatedUClass = Blueprint->GeneratedClass;
	if (generatedUClass == nullptr)
	{
		return;
	}
	UUnrealLuaTools::Get()->ShowLuaScriptEditorCreateDefaultScript(generatedUClass);
}

FBlueprintEditorLuaScriptToolbarCommands::FBlueprintEditorLuaScriptToolbarCommands()
	: TCommands<FBlueprintEditorLuaScriptToolbarCommands>(
	TEXT("UnrealLuaToolsEditorCommands"), // Context name for fast lookup
	LOCTEXT("UnrealLuaToolsEditorCommandsContexts", "UnrealLua Tools Editor Commands"), // Localized context name for displaying
	NAME_None, // Parent
	FAppStyle::GetAppStyleSetName()
)
{
	
}

void FBlueprintEditorLuaScriptToolbarCommands::RegisterCommands()
{
	// Show toggles
	UI_COMMAND(ShowLuaScriptWindowUICommand, "Open Lua Script Window for Asset", "Create or edit Lua script for asset", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
