#pragma once
#include "CoreMinimal.h"
#include "LuaCompilerSourceFileInfo.generated.h"

namespace UnrealLua::Compiler
{
	struct IStructPrototypeBase;
}

USTRUCT(BlueprintType)
struct UNREALLUA_API FLuaCompilerSourceFileLine
{
	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString Line = "";
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString DefinedTypeName = "";
	
	UnrealLua::Compiler::IStructPrototypeBase* DefinedType = nullptr;
	
	bool HasDefinedType() const;
};

USTRUCT(BlueprintType)
struct UNREALLUA_API FLuaCompilerSourceFileInfo
{
	GENERATED_BODY()
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString FileName = "";
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString FullFileName = "";
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString FullSource = "";
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FLuaCompilerSourceFileLine> Lines = {};
	
	void ProcessFile();
	const FString& GetFullSource();
	const TArray<FLuaCompilerSourceFileLine>& GetLines();
	TArray<FLuaCompilerSourceFileLine>& GetLinesMutable();
	TArray<FLuaCompilerSourceFileLine> GetTrimmedLines() const;
	const FLuaCompilerSourceFileLine& GetLine(int32 line);
	void SetDefinedTypeAtLine(int32 line, UnrealLua::Compiler::IStructPrototypeBase* proto);
};