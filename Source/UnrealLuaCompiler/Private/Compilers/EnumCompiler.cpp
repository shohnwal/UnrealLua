#include "Compilers/EnumCompiler.h"

#include "Engine/UserDefinedEnum.h"
#include "UnrealLuaCompiler.h"
#include "UObject/Class.h"
#include "UObject/Package.h"
#include "Prototypes/EnumPrototype.h"
#if WITH_EDITOR
#include "Kismet2/EnumEditorUtils.h"
#endif

#define LOCTEXT_NAMESPACE "UnrealLua"
UField* UnrealLua::Compiler::CreateAndFillEnum(FUnrealLuaCompilerUEnumPrototype& prototype, UPackage* destinationPackage)
{
	FString newEnumName = prototype.TypeName.ToString();
	
	UObject* existing = FindObjectSafe<UObject>(destinationPackage, *newEnumName);
	if (existing)
	{
		prototype.Compiler->SetError(FString::Printf(TEXT("Can not compile Lua-Compiled UEnum %s from file %s: A type with such a name already exists"), *newEnumName, *prototype.FileName));
		return nullptr;
	}
	TArray<TPair<FName, int64>> values = prototype.GetEnumValues();
	
	values.Sort([](const TPair<FName, int64>& a, const TPair<FName, int64>& b)
	{
		return a.Value < b.Value;
	});
	
	UUserDefinedEnum* newEnum = NewObject<UUserDefinedEnum>(destinationPackage, UUserDefinedEnum::StaticClass(), *newEnumName, RF_Public | RF_MarkAsNative);
#if UNREALLUA_UE_VERSION_NEWER_THAN_OR_EQUAL(5,8,0)
	newEnum->SetEnums(values, UEnum::ECppForm::Namespaced, newEnum->GetUnderlyingType(), EEnumFlags::None, UEnum::EAddMaxKeyIfMissing::No);
#else
	newEnum->SetEnums(values, UEnum::ECppForm::Namespaced);
#endif
#if WITH_METADATA
	newEnum->SetMetaData(TEXT("BlueprintType"), TEXT("True"));
	newEnum->SetMetaData(TEXT("DisplayName"), *newEnumName);
	for (int32 index = 0; index < newEnum->NumEnums(); index++)
	{
		FString displayName = newEnum->GetNameStringByIndex(index);
		newEnum->SetMetaData(TEXT("DisplayName"), *displayName, index);
		newEnum->RemoveMetaData(TEXT("Hidden"), index);
	}
#endif
#if WITH_EDITOR
	FEnumEditorUtils::UpgradeDisplayNamesFromMetaData(newEnum);
	FEnumEditorUtils::EnsureAllDisplayNamesExist(newEnum);
#endif
	prototype.SetCompiledEnum(newEnum);
	prototype.SetFinishedCompilation();
	return newEnum;
}
#undef LOCTEXT_NAMESPACE
