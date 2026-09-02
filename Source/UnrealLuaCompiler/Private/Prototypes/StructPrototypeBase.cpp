// Fill out your copyright notice in the Description page of Project Settings.


#include "Prototypes/StructPrototypeBase.h"

#include "Utility/LuaLogMacros.h"
#include "sol/sol.hpp"
#include "UnrealLuaCompiler.h"
#include "Prototypes/UFunctionPrototype.h"
#include "Prototypes/UPropertyPrototype.h"
#include <string>
#include "Internationalization/Regex.h"
#include "LuaTypes/LuaPrimitives.h"
#include "UnrealLuaCompilerConstants.h"

namespace UnrealLua::Compiler
{
	void IStructPrototypeBase::Run(UUnrealLuaCompiler* compiler, const sol::variadic_args& args)
	{
		this->Compiler = compiler;
		this->FileName = compiler->CurrentlyRunFilePath;
		
		lua_State* L = args.lua_state();
		lua_Debug ar{};
		lua_getstack(L, 1, &ar);
		lua_getinfo(L, "Sunl", &ar);
		//LUA_LOG("Begin compiling type from file %hs at line %d", ar.source, ar.currentline)
	
		this->DefinedLine = ar.currentline - 1;
		
		FLuaCompilerSourceFileInfo& fileInfo = compiler->FileContents.FindChecked(this->FileName);
		fileInfo.SetDefinedTypeAtLine(this->DefinedLine, this);
	}

	void IStructPrototypeBase::HandleBodyStep(sol::stack_object arg)
	{
		if (!arg.valid())
		{
			this->SetIsError("No struct body provided");
			return;
		}
		
		if (arg.get_type() != sol::type::table)
		{
			this->SetIsError("Struct body is not a table");
			return;
		}
		
		sol::table body = arg.as<sol::table>();
		int32 bodySize = body.size();
		
			
		bool bAllowUFunctions = this->AreUFunctionsAllowed();
		
		//create properties for types with missing UProperties/UFunction declarations
		body.for_each_stack([bAllowUFunctions, this](sol::stack_object key, sol::stack_object value)
		{
			if (this->GetIsError())
			{
				return;
			}
			verify(value.valid()) // having a table entry with nil would be strange, but check just in case
			verify(key.valid()) // having a table entry with nil would be strange, but check just in case
			
			if (this->GetIsError())
			{
				return;
			}
			//The key determines the property name, so it must be a string
			if (key.get_type() != sol::type::string)
			{
				return;
			}
			sol::string_view keyStrv = key.as<sol::string_view>();
			
			if (keyStrv.empty())
			{
				this->SetIsError("Found empty key for property");
				return;
			}		
			
			if (value.get_type() == sol::type::function)
			{
				if (!bAllowUFunctions)
				{
					this->SetIsError(FString::Printf(TEXT("UFunction %hs not allowed in current UStruct type!"), keyStrv.data()));
					return;
				}
				sol::function func = value.as<sol::function>();
				
				FUnrealLuaCompilerUFunctionPrototype funcProto = FUnrealLuaCompilerUFunctionPrototype(*this);
				funcProto.TypeName = keyStrv.data();
				funcProto.Func = func;
				//funcProto.FuncFlags = TArray<FString>({"BlueprintCallable"});
				funcProto.CheckValidity();
				verify(!funcProto.GetIsError())
				this->AddUFunction(funcProto);
			}
			else
			{
				//value might be a type or a default value
				//we will figure this out during FProperty compilation
				FUnrealLuaCompilerUPropertyPrototype propProto = FUnrealLuaCompilerUPropertyPrototype(*this);
				propProto.TypeName = keyStrv.data();
				propProto.Assign(sol::make_object(value.lua_state(), value));
				//by default, properties can bbe edited and are read/write available
				propProto.PropertyFlags |= CPF_Edit | CPF_BlueprintVisible;
				propProto.CheckValidity();
				verify(!propProto.GetIsError())
				this->AddProperty(propProto);
			}
		});
		
		if (this->GetIsError())
		{
			this->SetIsError("UClass prototype has errors, returning");
			return;
		}

		//check for duplicate fields will be done during compilation of FProperties
		
		UUnrealLuaCompiler* compiler = UUnrealLuaCompiler::Get();
		this->SetCommitReady();
		compiler->CommitPrototype(this);
	}

	bool IStructPrototypeBase::ParseBody()
	{
		TArray<FLuaCompilerSourceFileLine> lines = this->Compiler->FileContents.FindChecked(this->FileName).GetTrimmedLines();
		
		if (this->AreUPropertiesAllowed())
		{
			if (!this->ParseProperties(lines))
			{
				return false;
			}
		}
		if (this->AreUFunctionsAllowed())
		{
			if (!this->ParseFunctions(lines))
			{
				return false;
			}
		}
		return true;
	}

	bool IStructPrototypeBase::ParseProperties(const TArrayView<FLuaCompilerSourceFileLine> lines)
	{
		if (!this->AreUPropertiesAllowed())
		{
			return true;
		}
		const FLuaCompilerSourceFileLine& definedLineInfo = lines[this->DefinedLine];
		verify(definedLineInfo.DefinedType == this);
		
		//Start from the line below the UCLASS/USTRUCT declaration (definedline + 1)
		for (int32 lineIndex = this->DefinedLine + 1; lineIndex < lines.Num() - 1; ++lineIndex)
		{
			const FString* potentialUPropertyLine = &lines[lineIndex].Line;
			if (lines[lineIndex].HasDefinedType())
			{
				//Another type started, we're done
				break;
			}
			if (!potentialUPropertyLine->StartsWith("---@UPROPERTY("))
			{				
				continue;
			}
			
			const FString* potentialAssignmentLine = &lines[lineIndex + 1].Line;
			
			int32 foundIndex = -1;
			
			if (!potentialAssignmentLine->FindChar(TCHAR('='), foundIndex))
			{
				//no variable assignment line below ---@UProperty / ---@Field
				
				this->SetIsErrorWithArgs("Error while parsing Prototypes for type %s: Expected UProperty field assigment at line %d, but found line %s instead", *this->GetTypeNameString(), lineIndex, **potentialUPropertyLine);
				return false;
			}
			FString varName = potentialAssignmentLine->Left(foundIndex);
			varName.TrimEndInline();
			if(varName.IsEmpty())
			{
				//no variable name next to "="... technically we should never land here, as the table would be invalid, it should have given an error way earlier
				this->SetIsErrorWithArgs("Error while parsing Prototypes for type %s: Parsed field/UProperty at line %d : %s has an empty name", *this->GetTypeNameString(), lineIndex, **potentialUPropertyLine);
				return false;
			}
			FUnrealLuaCompilerUPropertyPrototype* foundProto = this->GetProperties().FindByPredicate([&varName](const FUnrealLuaCompilerUPropertyPrototype& item)
			{
				return item.GetTypeNameString() == varName;
			});
			if (!foundProto)
			{
				this->SetIsErrorWithArgs("Error while parsing Prototypes for type %s: Parsed field/UProperty %s at line has no matching uproperty prototype", *this->GetTypeNameString(), *varName);
				return false;
			}
			if (!potentialUPropertyLine->EndsWith(")", ESearchCase::IgnoreCase))
			{
				foundProto->SetIsErrorWithArgs("UFUNCTION line for var %s does not end with a closing paranthesis ')'", *varName);
				return false;
			}
			foundProto->LineDefined = lineIndex;
			
			EPropertyFlags propFlags = CPF_None;
			
			foundProto->PropFlagsString = *potentialUPropertyLine;
			foundProto->PropFlagsString.TrimStartAndEndInline();
				
			foundProto->PropFlagsString.RemoveFromStart("---@UPROPERTY(", ESearchCase::IgnoreCase);
			foundProto->PropFlagsString.RemoveFromEnd(")", ESearchCase::IgnoreCase);
			
			TArray<FString>& propFlagsStrArray = foundProto->PropFlags;;
			foundProto->PropFlagsString.ParseIntoArray(propFlagsStrArray, TEXT(","));
			for (auto& flag : propFlagsStrArray)
			{
				flag.TrimStartAndEndInline();
			}
			//Flags validity will be evaluated in Property compiler			
		}
		return true;
	}

	TArray<UnrealLua::Compiler::FUnrealLuaCompilerUFunctionPrototype>& IStructPrototypeBase::GetFunctionPrototypes()
	{
		checkNoEntry();
		static TArray<UnrealLua::Compiler::FUnrealLuaCompilerUFunctionPrototype> dummy{};
		return dummy;
	}

	bool IStructPrototypeBase::ParseFunctions(const TArrayView<FLuaCompilerSourceFileLine> lines)
	{
		if (!this->AreUFunctionsAllowed())
		{
			return true;
		}
			/* Regex for finding lua func args:
			- any whitespace
			- a variable name of letters and numbers (capture group 1) followed by whitespace
			- equal sign followed by whitespace
			- function keyword, followed by whitespace
			- opening brackets
			- arg list, separated by commas (capture group 2)
			- closing brackets
			NOTE: This does not test for syntax errors. The regex would accept variable names with beginning digits, but
			since this gets only executed after the function has been compiled, there shouldn't be any syntax errors
		 */
		
		const FLuaCompilerSourceFileLine& definedLineInfo = lines[this->DefinedLine];
		verify(definedLineInfo.DefinedType == this);

		FString luaFuncSignatureRegexPatternStr = "\\s*([a-zA-Z0-9]*)\\s*=\\s*function\\s*\\(([a-zA-Z0-9,\\s]*)\\)";
		FRegexPattern luaFuncSignatureRegexPattern{luaFuncSignatureRegexPatternStr};
		
		
		//@TODO : Support template types! ---@param TArray<FString>
		//@TODO : Support const ---@param const int32
		FString paramRegex = "\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s+([a-zA-Z0-9_\"\\/]+)(&)?\\s*(.*)$";
			
		//Regex for input parm, support optional ref
		//---@Param arg1 FMyStruct& "/Script/Blueprint" Comment
		FString luaInputParamRegexPatternStr = "---@Param" + paramRegex;
		FRegexPattern luaInputParamRegexPattern{luaInputParamRegexPatternStr};
		
		//@TODO : Support template types! TArray<FString>
		//Regex for output parm, support optional ref
		//---@Return arg1 FMyType90ABC& "/Script/Blueprint" Comment
		FString luaOutputParamRegexPatternStr = "---@Return" + paramRegex;
		FRegexPattern luaOutputParamRegexPattern{luaOutputParamRegexPatternStr};
		
		UUnrealLuaCompiler* compiler = UUnrealLuaCompiler::Get();
		sol::state_view lua{compiler->GetCompilerLuaState()};
		
		auto evalParam = [this, lua](bool bIsInputParam, int32 paramIndex, FUnrealLuaCompilerUFunctionPrototype& funcProto, FRegexPattern& pattern, const FString* metaLine) -> bool
		{
			FRegexMatcher paramMatcher{pattern, *metaLine};
			if (paramMatcher.FindNext())
			{
				FString paramName = paramMatcher.GetCaptureGroup(1);
				FString paramType = paramMatcher.GetCaptureGroup(2);
				FString optionalRef = paramMatcher.GetCaptureGroup(3);
				FString optionalComment = paramMatcher.GetCaptureGroup(4);
				
				//no need to create a property for self, it's already built into Unreal that it requires a self UObject during obj->ProcessEvent
				if(bIsInputParam && paramIndex == 0 && paramName.Equals("self"))
				{
					return true;
				}

				if (paramName.IsEmpty() || paramType.IsEmpty())
				{
					return false;
				}
						
				bool asRef = optionalRef.Equals("&");
				
				//LUA_LOG("Found ---@Param %s %s%s %s %s", *paramName, *paramType, *optionalRef, *paramType, *optionalComment)
						
				paramType.RemoveFromStart("\"");
				paramType.RemoveFromEnd("\"");
						
				sol::object type = lua["UE"][StringCast<char>(*paramType).Get()];
						
				if (type.valid())
				{
					//Note: For intrinsic types such as FVector or FRotator, the import registry will have returned the
					//UScriptStruct*, which will we use here to set the FStructProperty.UStruct
					std::string typeStr = UnrealLua::LuaTypes::TypeInfo::UTypeInternal(type, true, -1);
					//LUA_LOG("using already imported type %hs", typeStr.c_str())
					//use type directly
					
					bool merged = true;
					FUnrealLuaCompilerUPropertyPrototype* parmProto = funcProto.Params.FindByPredicate([&paramName](const FUnrealLuaCompilerUPropertyPrototype& item)
					{
						return item.TypeName == *paramName;	
					});
					if (!parmProto)
					{
						merged = false;
						parmProto = &funcProto.Params.Emplace_GetRef(funcProto);
						parmProto->TypeName = *paramName;
						parmProto->Assign(type);
					}
					parmProto->PropertyFlags |= CPF_Parm;
					parmProto->PropertyFlags |= CPF_BlueprintVisible;
					if (asRef)
					{	
						//From FKismetCompilerContext::CreatePropertiesFromList
						parmProto->PropertyFlags |= CPF_ReferenceParm;
						parmProto->PropertyFlags |= CPF_OutParm;
					}
					if (!bIsInputParam)
					{
						parmProto->PropertyFlags |= CPF_OutParm;
					}
					//@TODO : const
					//@TODO : As per FKismetCompilerContext::CreatePropertiesFromList, Arrays must always be passed as reference and outparm 
					//"ALWAYS pass array parameters as out params, so they're set up as passed by ref"
					return true;
				}
				else
				{
					//no need to check further, import registry should have already loaded it via path and if it's not valid now we won't find anythin else
					return false;
				}
			}
			//no matching pattern
			return false;
		};
		
		for (FUnrealLuaCompilerUFunctionPrototype& funcProto : this->GetFunctionPrototypes())
		{
			lua_State* L = funcProto.Func.lua_state();
			sol::stack::push(L, funcProto.Func);
			lua_Debug ar;
			lua_getinfo(L, ">Sun", &ar);
			
			//Lua lists lines with 1-index, while the array is 0-indexed
			int32 firstLineOfFunc = ar.linedefined - 1;
			
			//LUA_LOG("Parsing function %hs at line %d - %d", ar.name, ar.linedefined, ar.lastlinedefined)
			const FString& funcDefinitionLine = lines[firstLineOfFunc].Line;
			
			verify(funcDefinitionLine.Contains("function"));
			FRegexMatcher luaFuncSignatureMatcher{luaFuncSignatureRegexPattern, funcDefinitionLine};

			TArray<FString> funcInputParamNames{};
			
			funcProto.bIsSelfCall = false;
			if (luaFuncSignatureMatcher.FindNext())
			{
				FString funcNameStr = luaFuncSignatureMatcher.GetCaptureGroup(1);
				FString argStr = luaFuncSignatureMatcher.GetCaptureGroup(2);
				TArray<FString> args;
				argStr.ParseIntoArray(args, TEXT(","));
				
				bool foundSelf = false;
				if(!args.IsEmpty())
				{
					FString& selfMaybe = args[0];
					selfMaybe.TrimStartAndEndInline();
					
					if (selfMaybe.Equals("self"))
					{
						funcProto.bIsSelfCall = true;
					}
					
					for (FString& arg : args)
					{
						arg.TrimStartAndEndInline();
						//LUA_LOG("Found %s arg : %s", *funcNameStr, *arg)
						funcInputParamNames.Emplace(*arg);
					}
				}
			}
			
			//Search for ---@UFUNCTION definition
			//Search from func definition upwards,
			//as long as we find ---@ lines we can go further up
			int32 metaLineIndex = firstLineOfFunc - 1;
			bool foundMetaData = false;
			int32 foundUFuncLineIndex = -1;
			
			
			//search upward from lua func for ---@UFUNCTION
			while (metaLineIndex > 0)
			{
				const FString* metaLine = &lines[metaLineIndex].Line;
				if (metaLine->StartsWith("---@"))
				{
					foundMetaData = true;
					if (metaLine->StartsWith("---@UFUNCTION(", ESearchCase::IgnoreCase))
					{
						if (!metaLine->EndsWith(")", ESearchCase::IgnoreCase))
						{
							funcProto.SetIsError("UFUNCTION line does not end with a closing paranthesis ')'");
							return false;
						}
						foundUFuncLineIndex = metaLineIndex;
						break;
					}
					metaLineIndex--;
				}
				else
				{
					//no further lines that could provide relevant metadata
					break;
				}
			}
			
			//start going down again, parsing for params and return values
			
			int32 paramIndex = 0;
			metaLineIndex++;
			
			if(foundMetaData)
			{
				while (metaLineIndex < firstLineOfFunc)
				{
					const FString* metaLine = &lines[metaLineIndex].Line;
					verify(metaLine->StartsWith("---@"))
					if (metaLine->StartsWith("---@Param", ESearchCase::IgnoreCase))
					{
						if (evalParam(true, paramIndex, funcProto, luaInputParamRegexPattern, metaLine))
						{
							//@TODO : Check parm names?
						}
						paramIndex++;
					}
					else if (metaLine->StartsWith("---@Return", ESearchCase::IgnoreCase))
					{
						if (evalParam(false, paramIndex, funcProto, luaOutputParamRegexPattern, metaLine))
						{
							//@TODO : check parm names?
						}
						paramIndex++;
					}
					metaLineIndex++;
				}
			}
			
			verify(metaLineIndex == firstLineOfFunc);
			if (foundUFuncLineIndex != -1)
			{
				const FString& ufuncLine = lines[foundUFuncLineIndex].Line;
				
				FString ufuncLineCopy = ufuncLine;
				ufuncLineCopy.TrimStartAndEndInline();
				
				ufuncLineCopy.RemoveFromStart("---@UFUNCTION(", ESearchCase::IgnoreCase);
				ufuncLineCopy.RemoveFromEnd(")", ESearchCase::IgnoreCase);
				
				ufuncLineCopy.ParseIntoArray(funcProto.FuncFlags, TEXT(","));
				for (auto& flag : funcProto.FuncFlags)
				{
					flag.TrimStartAndEndInline();
				}
			}
		}
		return true;
	}

	void IStructPrototypeBase::SetFinishedCompilation()
	{
		//this should only be called once!
		verify(!this->CompilationFinished)
		this->CompilationFinished = true;
	}

	bool IStructPrototypeBase::HasFinishedCompilation() const
	{
		return this->CompilationFinished;
	}
}
