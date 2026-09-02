#include "LuaCompilerSourceFileInfo.h"

#include "Algo/ForEach.h"
#include "Prototypes/StructPrototypeBase.h"

bool FLuaCompilerSourceFileLine::HasDefinedType() const
{
	return this->DefinedType != nullptr;
}

void FLuaCompilerSourceFileInfo::ProcessFile()
{
	TArray<FString> lines{};
	this->FullSource.ParseIntoArrayLines(lines, false);
	
	/*
	Algo::ForEach(lines, [](FString& line)
	{
		line.TrimStartInline();
	});
	*/
	
	this->Lines.Reset(lines.Num());
	
	for (int32 lineIndex = 0; lineIndex < lines.Num(); ++lineIndex)
	{
		this->Lines.Emplace(lines[lineIndex], "");	
	}
}

const FString& FLuaCompilerSourceFileInfo::GetFullSource()
{
	return this->FullSource;
}

const TArray<FLuaCompilerSourceFileLine>& FLuaCompilerSourceFileInfo::GetLines()
{
	return this->Lines;
}

TArray<FLuaCompilerSourceFileLine>& FLuaCompilerSourceFileInfo::GetLinesMutable()
{
	return this->Lines;
}

TArray<FLuaCompilerSourceFileLine> FLuaCompilerSourceFileInfo::GetTrimmedLines() const
{
	TArray<FLuaCompilerSourceFileLine> outLines;
	for (int32 lineIndex = 0; lineIndex < this->Lines.Num(); ++lineIndex)
	{
		outLines.Add(this->Lines[lineIndex]);	
	}
	Algo::ForEach(outLines, [](FLuaCompilerSourceFileLine& line)
	{
		line.Line.TrimStartInline();
	});
	
	return outLines;
}

const FLuaCompilerSourceFileLine& FLuaCompilerSourceFileInfo::GetLine(int32 line)
{
	verify(this->Lines.IsValidIndex(line));
	return this->Lines[line];
}

void FLuaCompilerSourceFileInfo::SetDefinedTypeAtLine(int32 line, UnrealLua::Compiler::IStructPrototypeBase* proto)
{
	verify(this->Lines.IsValidIndex(line));
	verify(proto != nullptr);
	
	FLuaCompilerSourceFileLine& lineInfo = this->Lines[line];
	verify(lineInfo.DefinedType == nullptr);
	
	switch (proto->GetPrototypeCategory())
	{
	case UnrealLua::Compiler::EUnrealLuaCompilerPrototypeType::Enum:
		verify(lineInfo.Line.StartsWith("UENUM"));
		break;
	case UnrealLua::Compiler::EUnrealLuaCompilerPrototypeType::Class:
		verify(lineInfo.Line.StartsWith("UCLASS"));
		break;
	case UnrealLua::Compiler::EUnrealLuaCompilerPrototypeType::ScriptStruct:
		verify(lineInfo.Line.StartsWith("USTRUCT"));
		break;
	case UnrealLua::Compiler::EUnrealLuaCompilerPrototypeType::Interface:
		verify(lineInfo.Line.StartsWith("UINTERFACE"));
		break;
	default:
		checkNoEntry();
		break;
	}
	
	lineInfo.DefinedType = proto;
}