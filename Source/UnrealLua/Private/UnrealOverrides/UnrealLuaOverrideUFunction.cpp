// Fill out your copyright notice in the Description page of Project Settings.


#include "UnrealOverrides/UnrealLuaOverrideUFunction.h"

#include "Blueprint/UserWidget.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "LuaCallHelpers/LuaScriptOverrideCalls.h"
#include "Misc/StringBuilder.h"
//#include "UObject/LinkerLoad.h"
#include "Utility/UnrealVersion.h"

void UUnrealLuaOverrideUFunction::Initialize()
{
}


FString GetUFunctionFlagsAsString(const UFunction* const func)
{
	EFunctionFlags flags = func->FunctionFlags;
#if UNREALLUA_UE_VERSION_OLDER_THAN(5,8,0)
	FStringBuilderBase str;
#else
	TStringBuilder<256> str;
#endif
	if (flags & FUNC_Native)
	{
		str << " FUNC_Native";
	}
	if (flags & FUNC_Event)
	{
		str << " FUNC_Event";
	}
	if (flags & FUNC_NetValidate)
	{
		str << " FUNC_NetValidate";
	}
	if (flags & FUNC_Final)
	{
		str << " FUNC_Final";
	}
	if (flags & FUNC_UbergraphFunction)
	{
		str << " FUNC_UbergraphFunction";
	}
	if (flags & FUNC_BlueprintEvent)
	{
		str << " FUNC_BlueprintEvent";
	}
	if (flags & FUNC_Exec)
	{
		str << " FUNC_Exec";
	}
	if (flags & FUNC_Net)
	{
		str << " FUNC_Net";
	}
	if (flags & FUNC_NetValidate)
	{
		str << " FUNC_NetValidate";
	}
	if (flags & FUNC_BlueprintCallable)
	{
		str << " FUNC_BlueprintCallable";
	}
	if (flags & FUNC_BlueprintPure)
	{
		str << " FUNC_BlueprintPure";
	}
	if (flags & FUNC_EditorOnly)
	{
		str << " FUNC_EditorOnly";
	}
	if (flags & FUNC_Static)
	{
		str << " FUNC_Static";
	}
	return str.ToString();
}

bool operator==(const FNativeFunctionLookup& lhs, const FNativeFunctionLookup& rhs)
{
	return lhs.Pointer == rhs.Pointer && lhs.Name == rhs.Name;
}



void UUnrealLuaOverrideUFunction::BeginDestroy()
{
	//LUA_LOG("Deleting ULuaFunction %s %p", *GetNameSafe(this), this)
	this->SetPropertiesSize(0);
	this->NumParms = 0;
	this->ChildProperties = nullptr;
	this->Children = nullptr;
	this->ParmsSize = 0;
	this->PropertiesSize = 0;
	this->FirstPropertyToInit = nullptr;
	this->DestructorLink = nullptr;
	this->PropertyLink = nullptr;
#if WITH_EDITOR
	this->PropertyWrappers = {};
#endif
	this->ReturnValueOffset = 0;
	this->EventGraphCallOffset = 0;
	this->EventGraphFunction = 0;
	UFunction::BeginDestroy();
}

void UUnrealLuaOverrideUFunction::FinishDestroy()
{
	UFunction::FinishDestroy();
}

void UUnrealLuaOverrideUFunction::Bind()
{
	const auto Outer = GetOuter();
	if (Outer && Outer->GetName().StartsWith("REINST_"))
	{
		FunctionFlags &= ~FUNC_Native;
		return;
	}
	Super::Bind();
}
