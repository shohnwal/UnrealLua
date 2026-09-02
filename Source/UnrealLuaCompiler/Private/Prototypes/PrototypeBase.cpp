// Fill out your copyright notice in the Description page of Project Settings.


#include "Prototypes/PrototypeBase.h"

#include "Utility/UnrealVersion.h"

FString UnrealLua::Compiler::IPrototypeBase::GetTypeNameString() const
{
	return this->TypeName.ToString();
}

FName UnrealLua::Compiler::IPrototypeBase::GetTypeName() const
{
	return this->TypeName;
}

FString UnrealLua::Compiler::IPrototypeBase::GetFullPathString()
{
#if UNREALLUA_UE_VERSION_OLDER_THAN(5,8,0)
	FStringBuilderBase builder;
#else
	TStringBuilder<256> builder;
#endif
	if (this->OwnerPrototype)
	{
		builder << this->OwnerPrototype->GetFullPathString();
	}
	builder << "/";
	builder << this->GetTypeNameString();
	return builder.ToString();
}

void UnrealLua::Compiler::IPrototypeBase::SetIsError()
{
	//this should only be done once!
	verify(!this->bIsError);
	this->bIsError = true;
}

bool UnrealLua::Compiler::IPrototypeBase::GetIsError() const
{
	return this->bIsError;
}