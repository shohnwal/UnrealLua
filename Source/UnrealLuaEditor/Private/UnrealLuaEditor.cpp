#include "UnrealLuaEditor.h"

#include "ContentBrowserMenuContexts.h"
#include "PropertyEditorModule.h"
#include "ToolMenuDelegates.h"
#include "ToolMenus.h"
#include "ToolMenuSection.h"
#include "DetailsViews/LuaScriptValueDetails.h"
#include "DetailsViews/LuaUObjectItemViewDetails.h"
#include "DetailsViews/WatchedActorViewDetails.h"
#include "DetailsViews/WeakStructViewDetails.h"
#include "ScriptFileTools/BlueprintEditorLuaScriptToolbar.h"
#include "Subsystem/UnrealLuaTools.h"

#define LOCTEXT_NAMESPACE "FUnrealLuaEditorModule"

class UContentBrowserAssetContextMenuContext;


void FUnrealLuaEditorModule::StartupModule()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomPropertyTypeLayout("LuaUObjectItemView", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FLuaUObjectItemViewDetails::MakeInstance));    
	PropertyModule.RegisterCustomPropertyTypeLayout("LuaScriptValue", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FLuaScriptValueDetails::MakeInstance));    
	PropertyModule.RegisterCustomPropertyTypeLayout("WeakStructView", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FWeakStructViewDetails::MakeInstance));    
	PropertyModule.RegisterCustomPropertyTypeLayout("UnrealLuaDebugActorWatcher", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FWatchedActorViewDetails::MakeInstance));    
	PropertyModule.NotifyCustomizationModuleChanged();
	
	FCoreDelegates::OnAllModuleLoadingPhasesComplete.AddRaw(this, &FUnrealLuaEditorModule::NotifyAllModuleLoadingPhasesComplete);
}

void FUnrealLuaEditorModule::ShutdownModule()
{
	// Unregister the details customization
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomPropertyTypeLayout("LuaUObjectItemView");
		PropertyModule.UnregisterCustomPropertyTypeLayout("LuaScriptValue");
		PropertyModule.UnregisterCustomPropertyTypeLayout("WeakStructView");
		PropertyModule.UnregisterCustomPropertyTypeLayout("FUnrealLuaDebugActorWatcher");
		PropertyModule.NotifyCustomizationModuleChanged();
	}
}

void FUnrealLuaEditorModule::NotifyAllModuleLoadingPhasesComplete() const
{
	FBlueprintEditorLuaScriptToolbar::Initialize();
	
	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("ContentBrowser.AssetContextMenu.AssetActionsSubMenu");
	
	{
		FToolMenuSection& Section = Menu->AddSection("UnrealLuaEditorAssetActions", LOCTEXT("UnrealLuaAssetActions", "Unreal Lua Asset Actions"));
		
		FToolMenuEntry& Entry = Section.AddDynamicEntry("AssetManagerEditorViewCommands", FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& InSection)
		{
			UContentBrowserAssetContextMenuContext* Context = InSection.FindContext<UContentBrowserAssetContextMenuContext>();
			if (Context)
			{
				//create action
				FToolUIActionChoice createLuaScriptFileAction(FExecuteAction::CreateLambda([Context]()
				{
					for(FAssetData selectedAsset : Context->SelectedAssets)
					{
						FSoftObjectPath softObjectPath = selectedAsset.GetSoftObjectPath();
						FString softObjectPathString = softObjectPath.ToString();
						
						bool isNative = false;
						if (softObjectPathString.StartsWith(TEXT("/Script/")))
						{
							isNative = true;
						}
						else if (softObjectPathString.StartsWith(TEXT("/Game/")))
						{
							if (!softObjectPathString.EndsWith("_C"))
							{
								softObjectPathString.Append("_C");
							}
						}
						
						LUA_LOG("Selected %s for creating default Lua script", *softObjectPathString);
						FSoftClassPath softClassPath = FSoftClassPath(softObjectPathString);
						UClass* uclass = softClassPath.TryLoadClass<UObject>();
						
						UUnrealLuaTools::Get()->ShowLuaScriptEditorCreateDefaultScript(uclass);
					}
				}));

				//Add action to section
				InSection.AddEntry(FToolMenuEntry::InitMenuEntry(FName("CreateDefaultLuaScript"), FText::FromString("Create Lua Script File For Asset"), FText::FromString("Creates a default Lua script file in the Content/Lua/AssetDefault/<path> folder."), FSlateIcon(FAppStyle::Get().GetStyleSetName(), "MyIconName"), createLuaScriptFileAction));
			}
		}));
	}
	
	UToolMenu* topEditorMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools"); // Adjust path as needed
	if (topEditorMenu)
	{
		FToolMenuSection* Section = topEditorMenu->FindSection("Tools"); // Or your target section
		if (!Section)
		{
			Section = &topEditorMenu->AddSection("Tools", LOCTEXT("ToolsSection", "Tools"));
		}
    
		Section->AddEntry(FToolMenuEntry::InitMenuEntry(
			"UnrealLuaScriptEditor",
			LOCTEXT("OpenUnrealLuaScriptEditorLabel", "Lua Script Editor"),
			LOCTEXT("OpenUnrealLuaScriptEditorLabelTooltip", "Open Lua Script Editor"),
			FSlateIcon(FAppStyle::Get().GetStyleSetName(), "MainFrame.AboutUnrealEd"),
			FUIAction(FExecuteAction::CreateLambda([]() {
				UUnrealLuaTools::Get()->OpenLuaScriptEditorInUnrealEditor();
			}))
		));
	}
	
	UToolMenu* widgetEditorMenu = UToolMenus::Get()->ExtendMenu("AssetEditor.WidgetBlueprintEditor.ToolBar"); // Adjust path as needed
	if (widgetEditorMenu)
	{
		FToolMenuSection* Section = widgetEditorMenu->FindSection("Tools"); // Or your target section
		if (!Section)
		{
			Section = &widgetEditorMenu->AddSection("Tools", LOCTEXT("ToolsSection", "Tools"));
		}
    
		Section->AddEntry(FToolMenuEntry::InitMenuEntry(
			"UnrealLuaScriptEditor",
			LOCTEXT("OpenUnrealLuaScriptEditorLabel", "Lua Script Editor"),
			LOCTEXT("OpenUnrealLuaScriptEditorLabelTooltip", "Open Lua Script Editor"),
			FSlateIcon(FAppStyle::Get().GetStyleSetName(), "MainFrame.AboutUnrealEd"),
			FUIAction(FExecuteAction::CreateLambda([]() {
				UUnrealLuaTools::Get()->OpenLuaScriptEditorInUnrealEditor();
			}))
		));
	}
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FUnrealLuaEditorModule, UnrealLuaEditor)