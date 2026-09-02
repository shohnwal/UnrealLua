// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "CoreMinimal.h"
#include "Misc/TVariant.h"
#include <string_view>
#include "UObject/ObjectPtr.h"
#include "PropertyMapping.generated.h"
struct FLuaScriptValueKey;
struct FFunctionDescr;
class UBlueprintFunctionLibrary;
class ULuaStructUFunctionLibrary;

USTRUCT()
struct UNREALLUA_API FHashedFieldMapping
{
	GENERATED_BODY()
	
	FHashedFieldMapping()
		: Field(), LuaStringNameHash(0)
	{
	}

	explicit FHashedFieldMapping(uint32 hash)
		: Field(), LuaStringNameHash(hash)
	{
	}

	FHashedFieldMapping(uint32 hash, FProperty& prop);

	FHashedFieldMapping(uint32 hash, UFunction& func);

	FHashedFieldMapping(const FHashedFieldMapping& other);

	FHashedFieldMapping(FHashedFieldMapping&& other) noexcept;

	~FHashedFieldMapping();
	
	FName GetMappingFName() const;

	bool IsFunction() const
	{
		return this->Field.IsType<FFunctionDescr*>();
	}

	bool IsProperty() const
	{
		return this->Field.IsType<FProperty*>();
	}
	
	FProperty* TryGetProperty() const
	{
		if(this->IsProperty())
		{
			return this->Field.Get<FProperty*>();
		}
		return nullptr;
	}

	FProperty* GetProperty() const
	{
		return this->Field.Get<FProperty*>();
	}

	const FFunctionDescr* TryGetFunction() const
	{
		if(this->IsFunction())
		{
			return this->Field.Get<FFunctionDescr*>();
		}
		return nullptr;
	}


	const FFunctionDescr* GetFunction() const
	{
		return this->Field.Get<FFunctionDescr*>();
	}

	FHashedFieldMapping& operator=(const FHashedFieldMapping& other);

	bool operator==(const FHashedFieldMapping& other) const
	{
		return this->LuaStringNameHash == other.LuaStringNameHash;
	}

	bool operator<(const FHashedFieldMapping& other) const
	{
		return this->LuaStringNameHash < other.LuaStringNameHash;
	}
	
	static uint32 GetKeyHash(const FHashedFieldMapping& This)
	{
		return This.LuaStringNameHash;
	}

	TVariant<std::nullptr_t, FProperty*, FFunctionDescr*> Field = {};
	//@TODO : Use HashedStringKey from UnrealLuaStringLibrary instead
	UPROPERTY(VisibleAnywhere)
	FName FieldName = NAME_None;
	UPROPERTY(VisibleAnywhere)
	uint32 LuaStringNameHash = 0;
};

inline uint32 GetTypeHash(const FHashedFieldMapping& This)
{
	return This.LuaStringNameHash;
}

USTRUCT()
struct UNREALLUA_API FUStructPropertyMapping
{
	GENERATED_BODY()
	
	void AddReferencedObjects(FReferenceCollector& collector);
	
	const FHashedFieldMapping* FindMapping(const std::string_view& strv) const;
	const FHashedFieldMapping* FindMapping(uint32 hash) const;
	
	bool AddFunction(UFunction& func);
	bool AddProperty(FProperty& prop);
	bool AddExternalMapping(const FHashedFieldMapping& externalMapping);
	
	bool operator==(const FUStructPropertyMapping& other) const
	{
		return other.OwningField == this->OwningField;
	}

	bool operator<(const FUStructPropertyMapping& other) const
	{
		return OwningField < other.OwningField;
	}
	
	//The source UClass, UScriptStruct or UEnum of this mapping
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UField> OwningField = nullptr;
	
	//All the properties and UFunctions of that field (including inherited)
	UPROPERTY(VisibleAnywhere)
	TSet<FHashedFieldMapping> PropertyMappings = {};
	
	//To emulate function calls for structs,
	//If this is a property mapping of a UScriptStruct,
	//this may point to a BlueprintFunctionLibrary in the same
	//folder with the same name + _Library
	UPROPERTY()
	TObjectPtr<UBlueprintFunctionLibrary> HostBlueprintLibrary = nullptr;
};