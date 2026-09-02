// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "UObject/Object.h"

class UBlueprint;
class FExtender;
class FMyClassEditorToolbarCommands;
/**
 * 
 */
class FBlueprintEditorLuaScriptToolbar
{
public:
	static void Initialize();

protected:
	static void OnGatherExtensions(TSharedPtr<FExtender> Extender, UBlueprint* Blueprint);

	static void ExtendToolBar(class FToolBarBuilder& Builder);
	static void ExtendToolsMenu(FMenuBuilder& MenuBuilder);

	static void ShowLuaScriptWindow(class UBlueprint* Blueprint);
};

class FBlueprintEditorLuaScriptToolbarCommands : public TCommands<FBlueprintEditorLuaScriptToolbarCommands>
{
public:
	FBlueprintEditorLuaScriptToolbarCommands();
	// TCommand<> interface
	virtual void RegisterCommands() override;
	// End of TCommand<> interface
public:
	TSharedPtr<FUICommandInfo> ShowLuaScriptWindowUICommand;
};

namespace UnrealLua
{
	void CreateDefaultScriptFileForClass(UClass* uclass);
}