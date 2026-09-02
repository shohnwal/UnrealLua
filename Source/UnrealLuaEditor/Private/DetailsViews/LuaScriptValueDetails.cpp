// Fill out your copyright notice in the Description page of Project Settings.


#include "DetailsViews/LuaScriptValueDetails.h"

#include "LuaValue/LuaValueType.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"
#include "Utility/UnrealVersion.h"
#if UNREALLUA_UE_VERSION_NEWER_THAN_OR_EQUAL(5,8,0)
#include "Concepts/UEnum.h"
#endif
#include "Debug/UnrealLuaDebug.h"
#include "Debug/DebugTools/LuaScriptValueEditorTool.h"
#include "LuaValue/LuaScriptValue.h"
#include "UObjectRegistry/LuaUObjectItem.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"


#define LOCTEXT_NAMESPACE "UnrealLuaEditor"

TSharedRef<IPropertyTypeCustomization> FLuaScriptValueDetails::MakeInstance()
{
	return MakeShared<FLuaScriptValueDetails>();
}
/*
FLuaScriptValueDetails::~FLuaScriptValueDetails()
{
	if (this->LuaScriptValueChangedHandle.IsValid() && this->ChildStruct.IsValid())
	{
		FLuaScriptValue* value = std::bit_cast<FLuaScriptValue*>(this->ChildStruct.Get()->GetStructMemory());
		value->RemoveOnValueChangedByHandle(this->LuaScriptValueChangedHandle);
	}
}
*/

void FLuaScriptValueDetails::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle,
                                             class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	StructProperty = StructPropertyHandle;
	StructProperty->SetInstanceMetaData(TEXT("ShowOnlyInnerProperties"), "true");
	TArray<void*> rawData;
	StructProperty->AccessRawData(rawData);
	FLuaScriptValue* value = static_cast<FLuaScriptValue*>(rawData[0]);
	
	//FOnLuaScriptValueChangedDelegate del;
	//value->AddOnValueChangedDelegate(del);
	
	ChildStruct = MakeShared<FStructOnScope>(FLuaScriptValue::StaticStruct(), reinterpret_cast<uint8*>(value));
	
	PropUtils = StructCustomizationUtils.GetPropertyUtilities();

	OnObjectsReinstancedHandle = FCoreUObjectDelegates::OnObjectsReinstanced.AddSP(this, &FLuaScriptValueDetails::OnObjectsReinstanced);


	FString keyString = value->GetKeyNameString();
	
	HeaderRow
	.NameContent()
	[
		StructPropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	[
		SNew(STextBlock)
		.Font(StructCustomizationUtils.GetRegularFont())
		.Text(FText::AsCultureInvariant(keyString))
		.Visibility(EVisibility::SelfHitTestInvisible)
	];
}

void FLuaScriptValueDetails::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	FLuaScriptValue* value = std::bit_cast<FLuaScriptValue*>(this->ChildStruct.Get()->GetStructMemory());
	
	if (value->IsDead())
	{
		auto& row = StructBuilder.AddCustomRow(FText::AsCultureInvariant("LuaScriptValueRow"));
		this->TypeTextWidget = SNew(STextBlock)
			.Font(StructCustomizationUtils.GetRegularFont())
			.Text(FText::AsCultureInvariant("[Dead]"))
			.Visibility(EVisibility::SelfHitTestInvisible);
		
		row.NameContent()
		[
			this->TypeTextWidget.ToSharedRef()
		];
		this->ChildStruct = nullptr;
		this->LuaScriptValueChangedHandle.Reset();
	}
	else
	{
#if UNREALLUA_UE_VERSION_OLDER_THAN(5,8,0)
		UEnum* luaValueTypeEnum = StaticEnum<ELuaValueType>();
#else
		UEnum* luaValueTypeEnum = FindObject<UEnum>(nullptr, TEXT("/Script/UnrealLua.ELuaValueType"));
#endif
		UObject* detailsOuter = nullptr;
		FOnLuaScriptValueChangedNativeDelegate del;
		del.BindSP(this, &FLuaScriptValueDetails::NotifyLuaScriptValueChanged);
		this->LuaScriptValueChangedHandle = value->AddOnValueChangedDelegate(del);

		this->TypeTextWidget = SNew(STextBlock)
			.Font(StructCustomizationUtils.GetRegularFont())
			.Text(FText::AsCultureInvariant(luaValueTypeEnum->GetNameStringByValue(static_cast<int64>(value->GetType()))))
			.Visibility(EVisibility::SelfHitTestInvisible);
		
		this->ContentTextWidget = SNew(STextBlock)
			.Font(StructCustomizationUtils.GetRegularFont())
			.Text(FText::AsCultureInvariant(value->GetLuaValue().ToValueString()))
			.Visibility(EVisibility::SelfHitTestInvisible);
		
		auto& row = StructBuilder.AddCustomRow(FText::AsCultureInvariant("LuaScriptValueRow"));
		row.NameContent()
		.HAlign(HAlign_Fill)
		[
			SNew(STextBlock)
			.Font(StructCustomizationUtils.GetRegularFont())
			.Text(FText::AsCultureInvariant(value->GetKeyNameString()))
			.Visibility(EVisibility::SelfHitTestInvisible)
		];
		row.ValueContent()
		.HAlign(HAlign_Fill)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Fill)
				[
					this->TypeTextWidget.ToSharedRef()
				]
				
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Right)
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.ContentPadding(FMargin(1.f,1.f))
					.Visibility(EVisibility::Visible)
					.OnClicked(this, &FLuaScriptValueDetails::OnEditScriptValueButtonClicked)
					[
						SNew(STextBlock)
						.Font(StructCustomizationUtils.GetRegularFont())
						.Text(FText::AsCultureInvariant("Edit"))
						.ColorAndOpacity(FLinearColor::White)
					]
				]
			]
			+ SVerticalBox::Slot()
			[
				this->ContentTextWidget.ToSharedRef()
			]
			
		];
	}
	
}

void FLuaScriptValueDetails::NotifyLuaScriptValueChanged(FLuaValue val)
{
	if (!this->ChildStruct.IsValid())
	{
		return;
	}
	FLuaScriptValue* value = std::bit_cast<FLuaScriptValue*>(this->ChildStruct.Get()->GetStructMemory());
	if (val.IsDead())
	{
		value->RemoveOnValueChangedByHandle(this->LuaScriptValueChangedHandle);
		this->LuaScriptValueChangedHandle.Reset();
		this->ChildStruct.Reset();
		if (PropUtils.IsValid())
		{
			PropUtils->RequestForceRefresh();
		}
	}
	else
	{
#if UNREALLUA_UE_VERSION_OLDER_THAN(5,8,0)
		UEnum* luaValueTypeEnum = StaticEnum<ELuaValueType>();
#else
		UEnum* luaValueTypeEnum =  FindObject<UEnum>(nullptr, TEXT("/Script/UnrealLua.ELuaValueType"));
#endif
		this->ContentTextWidget.Get()->SetText(FText::AsCultureInvariant(value->GetLuaValue().ToValueString()));
		this->TypeTextWidget.Get()->SetText(FText::AsCultureInvariant(luaValueTypeEnum->GetNameStringByValue(static_cast<int64>(value->GetType()))));	
	}
}

FReply FLuaScriptValueDetails::OnEditScriptValueButtonClicked() const
{
	//Try to find an outer UObject so we could perhaps use the World's Lua state to compile the edit string
	
	UObject* foundOuterUObject = nullptr;
	TSharedPtr<IPropertyHandle> parentHandle = this->StructProperty->GetParentHandle();
	if (parentHandle && parentHandle->GetProperty()->IsA<FArrayProperty>())
	{
		TSharedPtr<IPropertyHandle> luaUObjectItemhandle = parentHandle->GetParentHandle();
		if (luaUObjectItemhandle && luaUObjectItemhandle->GetProperty()->IsA<FStructProperty>())
		{
			FStructProperty* sprop = CastField<FStructProperty>(luaUObjectItemhandle->GetProperty());
			if (sprop->Struct == FLuaUObjectItem::StaticStruct())
			{
				TArray<void*> data{};
				luaUObjectItemhandle->AccessRawData(data);
				if (!data.IsEmpty())
				{
					FLuaUObjectItem* item = static_cast<FLuaUObjectItem*>(data[0]);
					LUA_LOG("Start editing lua script value in Object %s", *GetNameSafe(item->GetUObject()))
					foundOuterUObject = item->GetUObject();
				}
			}
		}
	}
	if (!foundOuterUObject)
	{
		TArray<UObject*> objects;
		this->StructProperty->GetOuterObjects(objects);
		if (!objects.IsEmpty())
		{
			foundOuterUObject = objects[0];
		}
	}
	
	if (!foundOuterUObject)
	{
		LUA_LOG_WARNING("Can't find outer UObject to edit LuaScriptValue")
		return FReply::Handled();
	}

	UUnrealLuaDebug* debug = UUnrealLuaDebug::Get();
	if (foundOuterUObject && debug)
	{
		void* scriptValueMem = this->ChildStruct->GetStructMemory();
		FLuaScriptValue* scriptValue = static_cast<FLuaScriptValue*>(scriptValueMem);
		
		FInstancedStruct params{};
		FUnrealLuaDebugEditScriptValueToolData& data = params.InitializeAs<FUnrealLuaDebugEditScriptValueToolData>();
		data.Context = foundOuterUObject;
		data.LuaScriptValuePtr = scriptValue;
		
		//debug->SetActiveTool(ULuaScriptValueEditorTool::StaticClass(), params);
	}
	return FReply::Handled();
}

void FLuaScriptValueDetails::OnObjectsReinstanced(const TMap<UObject*, UObject*>& objectMap)
{
	// Force update the details when BP is compiled, since we may cached hold references to the old object or class.
	if (!objectMap.IsEmpty() && PropUtils.IsValid())
	{
		PropUtils->RequestRefresh();
	}
}
#undef LOCTEXT_NAMESPACE