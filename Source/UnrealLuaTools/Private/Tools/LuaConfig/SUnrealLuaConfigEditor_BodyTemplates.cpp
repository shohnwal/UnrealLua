
#include "Tools/LuaConfig/SUnrealLuaConfigEditor.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBox.h"

FLuaConfigEditorBodyTemplate SUnrealLuaConfigEditor::MakeBodyTemplate(EConfigBodyType bodyType)
{
	if (bodyType == EConfigBodyType::General)
	{
		FLuaConfigEditorBodyTemplate generalTemplate{"General Settinggs", {
			{
			"Is Lua Enabled", 
			SNew(SCheckBox)
				.IsChecked_Lambda([this](){ return this->TempConfigData.bLuaEnabled ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;})
				.OnCheckStateChanged_Lambda([this](ECheckBoxState checkState) { this->TempConfigData.bLuaEnabled = checkState == ECheckBoxState::Checked; }),
				UUnrealLuaConfig::GetConfigPropertyDescriptionForPropertyName(GET_MEMBER_NAME_CHECKED(FUnrealLuaConfigData, bLuaEnabled))
			},
			{
				"Lua Tools Menu Key", 
				SNew(SBox).MinDesiredWidth(100).Content()[SAssignNew(DebugKeyTextBox, SEditableTextBox).Justification(ETextJustify::Center)
				.Text(FText::AsCultureInvariant(this->TempConfigData.UnrealLuaToolsMenuKey.ToString()))
				.OnKeyDownHandler_Lambda([this]( const FGeometry&, const FKeyEvent& key){  this->NotifyDebugKeyTextBoxInput(key); return FReply::Handled(); })],
				UUnrealLuaConfig::GetConfigPropertyDescriptionForPropertyName(GET_MEMBER_NAME_CHECKED(FUnrealLuaConfigData, UnrealLuaToolsMenuKey))
			},
			{
				"Allow Lua Tick", 
				SNew(SCheckBox)
				.IsChecked_Lambda([this](){ return this->TempConfigData.bOverrideTick ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;})
				.OnCheckStateChanged_Lambda([this](ECheckBoxState checkState) { this->TempConfigData.bOverrideTick = checkState == ECheckBoxState::Checked; }),
				UUnrealLuaConfig::GetConfigPropertyDescriptionForPropertyName(GET_MEMBER_NAME_CHECKED(FUnrealLuaConfigData, bOverrideTick))
			},
			{
				"Override Input Actions", 
				SNew(SCheckBox)
				.IsChecked_Lambda([this](){ return this->TempConfigData.bOverrideInput ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;})
				.OnCheckStateChanged_Lambda([this](ECheckBoxState checkState) { this->TempConfigData.bOverrideInput = checkState == ECheckBoxState::Checked; }),
				UUnrealLuaConfig::GetConfigPropertyDescriptionForPropertyName(GET_MEMBER_NAME_CHECKED(FUnrealLuaConfigData, bOverrideInput))
			},
			//,
			//{
			//	"Lua Disabled Maps", 
			//	SNew(SCheckBox)
			//	.IsChecked_Lambda([this](){ return this->TempConfigData.LuaDisabledMaps ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;})
			//	.OnCheckStateChanged_Lambda([this](ECheckBoxState checkState) { this->TempConfigData.bOverrideInput = checkState == ECheckBoxState::Checked; }),
			//	UUnrealLuaConfig::GetConfigPropertyDescriptionForPropertyName(GET_MEMBER_NAME_CHECKED(FUnrealLuaConfigData, LuaDisabledMaps))
			//},
			//,
			//{
			//	"Lua Disabled Game Modes", 
			//	SNew(SCheckBox)
			//	.IsChecked_Lambda([this](){ return this->TempConfigData.LuaDisabledMaps ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;})
			//	.OnCheckStateChanged_Lambda([this](ECheckBoxState checkState) { this->TempConfigData.bOverrideInput = checkState == ECheckBoxState::Checked; }),
			//	UUnrealLuaConfig::GetConfigPropertyDescriptionForPropertyName(GET_MEMBER_NAME_CHECKED(FUnrealLuaConfigData, LuaDisabledMaps))
			//},
			{
				"Use package path for native default scripts", 
				SNew(SCheckBox)
				.IsChecked_Lambda([this](){ return this->TempConfigData.bUsePackagePathForNativeDefaultScripts ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;})
				.OnCheckStateChanged_Lambda([this](ECheckBoxState checkState) { this->TempConfigData.bUsePackagePathForNativeDefaultScripts = checkState == ECheckBoxState::Checked; }),
				UUnrealLuaConfig::GetConfigPropertyDescriptionForPropertyName(GET_MEMBER_NAME_CHECKED(FUnrealLuaConfigData, bUsePackagePathForNativeDefaultScripts))
			},
			{
				"Use package path for Blueprint default scripts", 
				SNew(SCheckBox)
				.IsChecked_Lambda([this](){ return this->TempConfigData.bUsePackagePathForBlueprintDefaultScripts ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;})
				.OnCheckStateChanged_Lambda([this](ECheckBoxState checkState) { this->TempConfigData.bUsePackagePathForBlueprintDefaultScripts = checkState == ECheckBoxState::Checked; }),
				UUnrealLuaConfig::GetConfigPropertyDescriptionForPropertyName(GET_MEMBER_NAME_CHECKED(FUnrealLuaConfigData, bUsePackagePathForBlueprintDefaultScripts))
			}
		}};
		return generalTemplate;
	}
	else if (bodyType == EConfigBodyType::Advanced)
	{
			
		FLuaConfigEditorBodyTemplate advancedemplate{"Advanced Settings", 
			{
			{
				"Enable Lua Replication", 
				SNew(SCheckBox)
				.IsChecked_Lambda([this](){ return this->TempConfigData.bEnableLuaReplication ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;})
				.OnCheckStateChanged_Lambda([this](ECheckBoxState checkState) { this->TempConfigData.bEnableLuaReplication = checkState == ECheckBoxState::Checked; }),
				UUnrealLuaConfig::GetConfigPropertyDescriptionForPropertyName(GET_MEMBER_NAME_CHECKED(FUnrealLuaConfigData, bEnableLuaReplication))
				},
			{
				"Multithreaded Lua Replication", 
				SNew(SCheckBox)
				.IsChecked_Lambda([this](){ return this->TempConfigData.bMultithreadedReplication ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;})
				.OnCheckStateChanged_Lambda([this](ECheckBoxState checkState) { this->TempConfigData.bMultithreadedReplication = checkState == ECheckBoxState::Checked; }),
				UUnrealLuaConfig::GetConfigPropertyDescriptionForPropertyName(GET_MEMBER_NAME_CHECKED(FUnrealLuaConfigData, bMultithreadedReplication))
			},
			{
				"Allow client-to-server Lua RPCs", 
				SNew(SCheckBox)
				.IsChecked_Lambda([this](){ return this->TempConfigData.bAllowClientToServerRPCs ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;})
				.OnCheckStateChanged_Lambda([this](ECheckBoxState checkState) { this->TempConfigData.bAllowClientToServerRPCs = checkState == ECheckBoxState::Checked; }),
				UUnrealLuaConfig::GetConfigPropertyDescriptionForPropertyName(GET_MEMBER_NAME_CHECKED(FUnrealLuaConfigData, bAllowClientToServerRPCs))
				},
			{
				"Server ignore invalid Lua RPCs", 
				SNew(SCheckBox)
				.IsChecked_Lambda([this](){ return this->TempConfigData.bIgnoreInvalidServerRPC ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;})
				.OnCheckStateChanged_Lambda([this](ECheckBoxState checkState) { this->TempConfigData.bIgnoreInvalidServerRPC = checkState == ECheckBoxState::Checked; }),
				UUnrealLuaConfig::GetConfigPropertyDescriptionForPropertyName(GET_MEMBER_NAME_CHECKED(FUnrealLuaConfigData, bIgnoreInvalidServerRPC))
				},
			{
				"Self-Test on Startup", 
				SNew(SCheckBox)
				.IsChecked_Lambda([this](){ return this->TempConfigData.bSelfTestOnStartup ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;})
				.OnCheckStateChanged_Lambda([this](ECheckBoxState checkState) { this->TempConfigData.bSelfTestOnStartup = checkState == ECheckBoxState::Checked; }),
				UUnrealLuaConfig::GetConfigPropertyDescriptionForPropertyName(GET_MEMBER_NAME_CHECKED(FUnrealLuaConfigData, bSelfTestOnStartup))
				},
			{
				"Enable UnrealLua Compiler", 
				SNew(SCheckBox)
				.IsChecked_Lambda([this](){ return this->TempConfigData.bCompilerEnabled ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;})
				.OnCheckStateChanged_Lambda([this](ECheckBoxState checkState) { this->TempConfigData.bCompilerEnabled = checkState == ECheckBoxState::Checked; }),
				UUnrealLuaConfig::GetConfigPropertyDescriptionForPropertyName(GET_MEMBER_NAME_CHECKED(FUnrealLuaConfigData, bCompilerEnabled))
				},
				{
					"Allow Lua write on ReadOnly Properties", 
					SNew(SCheckBox)
					.IsChecked_Lambda([this](){ return this->TempConfigData.bAllowWriteOnReadOnlyProperties ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;})
					.OnCheckStateChanged_Lambda([this](ECheckBoxState checkState) { this->TempConfigData.bAllowWriteOnReadOnlyProperties = checkState == ECheckBoxState::Checked; }),
					UUnrealLuaConfig::GetConfigPropertyDescriptionForPropertyName(GET_MEMBER_NAME_CHECKED(FUnrealLuaConfigData, bAllowWriteOnReadOnlyProperties))
				},
				{
					"Allow Lua write on CDOs", 
					SNew(SCheckBox)
					.IsChecked_Lambda([this](){ return this->TempConfigData.bAllowWriteOnCDO ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;})
					.OnCheckStateChanged_Lambda([this](ECheckBoxState checkState) { this->TempConfigData.bAllowWriteOnCDO = checkState == ECheckBoxState::Checked; }),
					UUnrealLuaConfig::GetConfigPropertyDescriptionForPropertyName(GET_MEMBER_NAME_CHECKED(FUnrealLuaConfigData, bAllowWriteOnCDO))
				}
			}
		};
		return advancedemplate;
	}
	else if (bodyType == EConfigBodyType::Mods)
	{
		FLuaConfigEditorBodyTemplate modsTemplate {
			"Lua Mods Settings", 
			{
				{
					"Mods Directory Location", 
					SNew(SBox).MinDesiredWidth(200).Content()[SNew(SEditableTextBox).Justification(ETextJustify::Center)
					.Text(FText::AsCultureInvariant(this->TempConfigData.ModsDirectoryLocation))
					.OnTextChanged_Lambda([this]( const FText& text)
					{
						this->TempConfigData.ModsDirectoryLocation = text.ToString();
					})],
					UUnrealLuaConfig::GetConfigPropertyDescriptionForPropertyName(GET_MEMBER_NAME_CHECKED(FUnrealLuaConfigData, ModsDirectoryLocation))
				},
				//{
				//	"Lua Script Mod File Extension", 
				//	SNew(SBox).MinDesiredWidth(200).Content()[SNew(SEditableTextBox).Justification(ETextJustify::Center)
				//	.Text(FText::AsCultureInvariant(this->TempConfigData.ScriptLoadModFileExtension))
				//	.OnTextChanged_Lambda([this]( const FText& text)
				//	{
				//		this->TempConfigData.ScriptLoadModFileExtension = text.ToString();
				//	})],
				//	UUnrealLuaConfig::GetConfigPropertyDescriptionForPropertyName(GET_MEMBER_NAME_CHECKED(FUnrealLuaConfigData, ScriptLoadModFileExtension))
				//},
			},
		};
		return modsTemplate;
	}
	else if (bodyType == EConfigBodyType::GC)
	{
		FLuaConfigEditorBodyTemplate gcTemplate {
			"Lua Garbage Collection Settings", 
			{
				{
					"Use Incremental UnrealLua GC Mode", 
					SNew(SCheckBox)
					.IsChecked_Lambda([this](){ return this->TempConfigData.GCMode == EUnrealLuaGCMode::Incremental ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;})
					.OnCheckStateChanged_Lambda([this](ECheckBoxState checkState) { this->TempConfigData.GCMode = checkState == ECheckBoxState::Checked ? EUnrealLuaGCMode::Incremental : EUnrealLuaGCMode::PostDestroy; }),
					UUnrealLuaConfig::GetConfigPropertyDescriptionForPropertyName(GET_MEMBER_NAME_CHECKED(FUnrealLuaConfigData, GCMode))
				},
				{
					"Incremental UnrealLua GC Limit", 
					SNew(SBox).MinDesiredWidth(100).Content()[SNew(SEditableTextBox).Justification(ETextJustify::Center)
					.Text(FText::AsCultureInvariant(FString::FromInt(this->TempConfigData.LuaIncrementalGCLimit)))
					.OnTextChanged_Lambda([this]( const FText& text)
					{
						int32 newValue = FMath::Clamp(FCString::Atoi(*text.ToString()), 10, 1000);
						this->TempConfigData.LuaIncrementalGCLimit = newValue;
					})],
					UUnrealLuaConfig::GetConfigPropertyDescriptionForPropertyName(GET_MEMBER_NAME_CHECKED(FUnrealLuaConfigData, LuaIncrementalGCLimit))
				},
				{
					"Multithreaded Garbage Collection", 
					SNew(SCheckBox)
					.IsChecked_Lambda([this](){ return this->TempConfigData.bMultithreadGC ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;})
					.OnCheckStateChanged_Lambda([this](ECheckBoxState checkState) { this->TempConfigData.bMultithreadGC = checkState == ECheckBoxState::Checked ? true : false; }),
					UUnrealLuaConfig::GetConfigPropertyDescriptionForPropertyName(GET_MEMBER_NAME_CHECKED(FUnrealLuaConfigData, bMultithreadGC))
				},
				{
					"Lua GC Step Pause", 
					SNew(SBox).MinDesiredWidth(100).Content()[SNew(SEditableTextBox).Justification(ETextJustify::Center)
					.Text(FText::AsCultureInvariant(FString::FromInt(this->TempConfigData.LuaGCStepPause)))
					.OnTextChanged_Lambda([this]( const FText& text)
					{
						int32 newValue = FMath::Clamp(FCString::Atoi(*text.ToString()), 100, 1000);
						this->TempConfigData.LuaGCStepPause = newValue;
					})],
					UUnrealLuaConfig::GetConfigPropertyDescriptionForPropertyName(GET_MEMBER_NAME_CHECKED(FUnrealLuaConfigData, LuaGCStepPause))
				},
				{
					"Lua GC Step Multiplier", 
					SNew(SBox).MinDesiredWidth(100).Content()[SNew(SEditableTextBox).Justification(ETextJustify::Center)
					.Text(FText::AsCultureInvariant(FString::FromInt(this->TempConfigData.LuaGCStepMultiplier)))
					.OnTextChanged_Lambda([this]( const FText& text)
					{
						int32 newValue = FMath::Clamp(FCString::Atoi(*text.ToString()), 50, 1000);
						this->TempConfigData.LuaGCStepMultiplier = newValue;
					})],
					UUnrealLuaConfig::GetConfigPropertyDescriptionForPropertyName(GET_MEMBER_NAME_CHECKED(FUnrealLuaConfigData, LuaGCStepMultiplier))
				},
			{
				"Lua GC Step Size", 
				SNew(SBox).MinDesiredWidth(100).Content()[SNew(SEditableTextBox).Justification(ETextJustify::Center)
				.Text(FText::AsCultureInvariant(FString::FromInt(this->TempConfigData.LuaGCStepSize)))
				.OnTextChanged_Lambda([this]( const FText& text)
				{
					int32 newValue = FMath::Clamp(FCString::Atoi(*text.ToString()), 10, 100);
					this->TempConfigData.LuaGCStepSize = newValue;
				})],
				UUnrealLuaConfig::GetConfigPropertyDescriptionForPropertyName(GET_MEMBER_NAME_CHECKED(FUnrealLuaConfigData, LuaGCStepSize))
				},
			},
		};
		return gcTemplate;
	}
	return {};
}
