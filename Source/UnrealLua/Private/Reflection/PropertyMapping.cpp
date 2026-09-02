// Fill out your copyright notice in the Description page of Project Settings.


#include "Reflection/PropertyMapping.h"
#include "UObject/UnrealType.h"
#include "Reflection/FunctionDescr.h"
#include "Utility/UnrealLuaHash.h"


FHashedFieldMapping::FHashedFieldMapping(uint32 hash, FProperty& prop): Field(), LuaStringNameHash(hash)
{
	this->Field.Emplace<FProperty*>(&prop);
	this->FieldName = prop.GetFName();
}

FHashedFieldMapping::FHashedFieldMapping(uint32 hash, UFunction& func)
	: Field(), LuaStringNameHash(hash)
{
	this->FieldName = func.GetFName();
	this->Field.Emplace<FFunctionDescr*>(new FFunctionDescr(&func));
}

FHashedFieldMapping::FHashedFieldMapping(const FHashedFieldMapping& other)
	: Field(), FieldName(other.FieldName), LuaStringNameHash(other.LuaStringNameHash)
{
	if(other.IsProperty())
	{
		this->Field.Emplace<FProperty*>(other.GetProperty());
	}
	else if(other.IsFunction())
	{
		this->Field.Emplace<FFunctionDescr*>(new FFunctionDescr(*other.GetFunction()));
	}
	else
	{
		this->Field.Emplace<std::nullptr_t>();
	}
}

FHashedFieldMapping::FHashedFieldMapping(FHashedFieldMapping&& other) noexcept
	: Field(MoveTemp(other.Field)), FieldName(other.FieldName), LuaStringNameHash(other.LuaStringNameHash)
{
	other.Field = {};
	other.LuaStringNameHash = 0;
}

FHashedFieldMapping::~FHashedFieldMapping()
{
	if(this->IsFunction())
	{
		delete this->GetFunction();
		this->Field.Emplace<std::nullptr_t>();
	}
}

FHashedFieldMapping& FHashedFieldMapping::operator=(const FHashedFieldMapping& other)
{
	this->LuaStringNameHash = other.LuaStringNameHash;
	this->FieldName = other.FieldName;
	if(other.IsProperty())
	{
		this->Field.Emplace<FProperty*>(other.GetProperty());
	}
	else if(other.IsFunction())
	{
		this->Field.Emplace<FFunctionDescr*>(new FFunctionDescr(*other.GetFunction()));
	}
	else
	{
		this->Field.Emplace<std::nullptr_t>();
	}
	return *this;
}

FName FHashedFieldMapping::GetMappingFName() const
{
	return this->IsProperty() ? GetFNameSafe(this->GetProperty()) : this->IsFunction() ? GetFNameSafe(this->GetFunction()->Func) : NAME_None;
}

//Called by UObjectRegistry -> FLuaUObjectItem::AddReferencedObjects
//Called only from "non-meta" UObjects (aka not UClass/UScriptStruct/UEnum entries)
////to indicate which meta items are still needed
void FUStructPropertyMapping::AddReferencedObjects(FReferenceCollector& collector)
{
	collector.AddReferencedObject(this->OwningField);
	if (this->HostBlueprintLibrary != nullptr)
	{
		collector.AddReferencedObject(this->HostBlueprintLibrary);
	}
}

const FHashedFieldMapping* FUStructPropertyMapping::FindMapping(const sol::string_view& strv) const
{
	uint32 hash = UnrealLua::HashUtility::StrCrc32(strv.data());
	return this->FindMapping(hash);
}

const FHashedFieldMapping* FUStructPropertyMapping::FindMapping(uint32 hash) const
{
	return this->PropertyMappings.Find(FHashedFieldMapping{hash});
}

bool FUStructPropertyMapping::AddFunction(UFunction& func)
{
	UStruct* owningStruct = CastChecked<UStruct>(this->OwningField);
	FName funcName = func.GetFName();
	auto cast = StringCast<char>(*funcName.ToString());
	const char* str = cast.Get();
	uint32 hash = UnrealLua::HashUtility::StrCrc32(str);
				
	if(auto* existing = this->FindMapping(hash))
	{
		verifyf(funcName == existing->GetMappingFName(), TEXT("Overlapping UFunction hash : %ul %s vs previous %ul %s"), hash, *funcName.ToString(), existing->LuaStringNameHash, *existing->GetMappingFName().ToString())
		verify(existing->IsFunction())
		return false;
	}
	this->PropertyMappings.Emplace(FHashedFieldMapping{hash, func});
	return true;
}

bool FUStructPropertyMapping::AddProperty(FProperty& prop)
{
	UStruct* owningStruct = CastChecked<UStruct>(this->OwningField);
	FString usedString = owningStruct->GetAuthoredNameForField(&prop);//propName.ToString();
			
	auto cast = StringCast<char>(*usedString);
	const char* str = cast.Get();
	uint32 hash = UnrealLua::HashUtility::StrCrc32(str);
	this->PropertyMappings.Emplace(FHashedFieldMapping{hash, prop});
	return true;
}

bool FUStructPropertyMapping::AddExternalMapping(const FHashedFieldMapping& externalMapping)
{
	bool wasAlreadyInSet = false;
	
	[[maybe_unused]]
	FHashedFieldMapping& added = this->PropertyMappings.FindOrAdd(externalMapping, &wasAlreadyInSet);
	
	return !wasAlreadyInSet;
}
