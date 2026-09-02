// Fill out your copyright notice in the Description page of Project Settings.


#include "UnrealLuaCompiler.h"

#include "Utility/LuaLogMacros.h"
#include "Config/UnrealLuaConfig.h"
//#include "Engine/UserDefinedEnum.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "LuaContext/StandaloneLuaContext.h"
#include "Utility/LuaFileLister.h"

#include "Kismet/KismetSystemLibrary.h"
#include "Misc/FileHelper.h"
#include "Misc/MessageDialog.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Reflection/PropertyHelper.h"
#include "Serialization/AsyncLoadingEvents.h"
#include "SubSystem/UnrealLuaEngineSubsystem.h"
#include "UnrealOverrides/LuaClassOverrideRegistry.h"
#include "LuaCompilerSourceFileInfo.h"
#include "UnrealLuaCompilerConstants.h"
#include "UnrealLua/Public/UnrealOverrides/UnrealLuaCompiledUFunction.h"
#include "Prototypes/UClassPrototype.h"
#include "Prototypes/UScriptStructPrototype.h"
#include "Compilers/CompilerHelper.h"
#include "FileSystem/UnrealLuaFileSystemEntry.h"
#include "Prototypes/EnumPrototype.h"
#include "Prototypes/InterfacePrototype.h"
#include "Prototypes/UFunctionPrototype.h"
#include "Subsystem/UnrealLuaFileSystem.h"
#include "Utility/UnrealVersion.h"

const char* UUnrealLuaCompiler::CompilerThrowErrorMsg = "Compilation failed, see compiler logs for details";

const FName UnreaLLuaCompiledPackageName = FName(TEXT("/UnrealLua/Compiled"));

namespace UnrealLua::Usertypes::Registration
{
	void RegisterUsertypes(sol::state_view& lua)
	{
		lua.new_usertype<UnrealLua::Compiler::FUnrealLuaCompilerUClassPrototype>(
			"_FUnrealLuaCompilerUClassPrototype_",
			"new", sol::no_constructor,
			"Implements", &UnrealLua::Compiler::FUnrealLuaCompilerUClassPrototype::AddInterfaces,
			sol::meta_function::call, &UnrealLua::Compiler::FUnrealLuaCompilerUClassPrototype::__call
		);

		lua.new_usertype<UnrealLua::Compiler::FUnrealLuaCompilerUScriptStructPrototype>(
			"_FUnrealLuaCompilerUScriptStructPrototype_",
			"new", sol::no_constructor,
			sol::meta_function::call, &UnrealLua::Compiler::FUnrealLuaCompilerUScriptStructPrototype::__call
		);
		
		lua.new_usertype<UnrealLua::Compiler::FUnrealLuaCompilerUInterfacePrototype>(
			"_FUnrealLuaCompilerUInterfacePrototype_",
			"new", sol::no_constructor,
			sol::meta_function::call, &UnrealLua::Compiler::FUnrealLuaCompilerUInterfacePrototype::__call
		);
		
		lua.new_usertype<UnrealLua::Compiler::FUnrealLuaCompilerUEnumPrototype>(
			"_FUnrealLuaCompilerUEnumPrototype_",
			"new", sol::no_constructor,
			//sol::base_classes, sol::bases<UnrealLua::Compiler::IPrototypeBase, UnrealLua::Compiler::IStructPrototypeBase>(),
			sol::meta_function::call, &UnrealLua::Compiler::FUnrealLuaCompilerUEnumPrototype::__call
		);
		lua["UENUM"] = sol::c_call<decltype(&UUnrealLuaCompiler::__UENUM), &UUnrealLuaCompiler::__UENUM>;
		lua["UCLASS"] = sol::c_call<decltype(&UUnrealLuaCompiler::__UCLASS), &UUnrealLuaCompiler::__UCLASS>;
		lua["USTRUCT"] = sol::c_call<decltype(&UUnrealLuaCompiler::__USTRUCT), &UUnrealLuaCompiler::__USTRUCT>;
		lua["UINTERFACE"] = sol::c_call<decltype(&UUnrealLuaCompiler::__UINTERFACE), &UUnrealLuaCompiler::__UINTERFACE>;
	}
	
	//static FDelegateHandle compilerUsertypeRegistrationHandle = FLuaCoreDelegates::OnRegisterLuaUsertypes.AddStatic(&UnrealLua::Usertypes::Registration::RegisterUsertypes);
}

namespace UnrealLua::UTypeCompiler
{
	UUnrealLuaCompiler* Compiler = nullptr;
}

void UUnrealLuaCompiler::Initialize(FSubsystemCollectionBase& Collection)
{
	LUA_LOG("UUnrealLuaCompiler::Initialize()")
	Super::Initialize(Collection);
	this->LuaSystem = UUnrealLuaEngineSubsystem::Get();
	verify(IsValid(this->LuaSystem))
	UnrealLua::UTypeCompiler::Compiler = this;
	this->LuaSystem->OnEndFrame.BindUObject(this, &UUnrealLuaCompiler::NotifyEndFrame);
	this->LuaSystem->OnTriggerCompiler.BindUObject(this, &UUnrealLuaCompiler::Start);
	FModuleManager::Get().OnModulesChanged().AddUObject(this, &UUnrealLuaCompiler::NotifyModuleChanged);
	
}

void UUnrealLuaCompiler::Deinitialize()
{
	LUA_LOG("UUnrealLuaCompiler::Deinitialize()")
	Super::Deinitialize();
}

UUnrealLuaCompiler* UUnrealLuaCompiler::Get()
{
	return UnrealLua::UTypeCompiler::Compiler;
}

void UUnrealLuaCompiler::NotifyAllModulesLoaded()
{
	//this->Start();
}

void UUnrealLuaCompiler::Start()
{
	LUA_LOG("UUnrealLuaCompiler::Start()")
	UnrealLua::UTypeCompiler::Compiler = this;

	this->UnrealLuaCompilerUTypePackage = NewObject<UPackage>(nullptr, UnreaLLuaCompiledPackageName, RF_Public | RF_Standalone);
	this->UnrealLuaCompilerUTypePackage->AddToRoot();
	this->UnrealLuaCompilerUTypePackage->SetPackageFlags(PKG_ContainsScript | PKG_CompiledIn);
	
	
	
	this->UnrealLuaInputActionPackage = NewObject<UPackage>(nullptr, FName(TEXT("/UnrealLua/Compiled/InputAction")), RF_Public | RF_Standalone);
	this->UnrealLuaInputActionPackage->AddToRoot();
	this->UnrealLuaInputActionPackage->SetPackageFlags(PKG_CompiledIn);
	
    this->Compile();
	
	if (!this->HasErrors())
	{
		//@TODO : Save compiled files as backup
	}
	
	//IAssetRegistry::Get()->AddPath()
	
	UnrealLua::UTypeCompiler::Compiler = nullptr;
	LUA_LOG("UUnrealLuaCompiler: done")
}

bool UUnrealLuaCompiler::HandleErrorBox(const TArray<FString>& errors)
{
	EAppReturnType::Type clicked = this->DisplayErrorBox(errors);
	if (clicked == EAppReturnType::Cancel)
	{
		LUA_LOG_WARNING("UnrealLua: User requested closing app to due compilation error");
		RequestEngineExit(TEXT("UnrealLua: Compilation Error. User requested closing."));
		verify(IsEngineExitRequested())
		return false;
	}	
	else if(clicked == EAppReturnType::Retry)
	{
		LUA_LOG_WARNING("UnrealLua compiler: User requested a compilation retry")
		return false;
	}
	else if (clicked == EAppReturnType::Continue)
	{
		LUA_LOG_WARNING("UnrealLua compiler: User requested to throw away UnrealLua UTypes and start anyway")
		return true;
	}
	
	LUA_LOG_WARNING("UnrealLua compiler: Automatic request for recompilation due to wrong message box feedback")	
	return false;
}

EAppReturnType::Type UUnrealLuaCompiler::DisplayErrorBox(const TArray<FString>& errors)
{
	const char* title = "Compilation error";
#if UNREALLUA_UE_VERSION_OLDER_THAN(5,8,0)
	FStringBuilderBase errorStr;
#else
	TStringBuilder<4096> errorStr;
#endif
	for (const FString& str: errors)
	{
		errorStr << *str << "\n";
	}
    const char* instructions = R"###(Please fix all Lua compilation errors.
    Press 'retry' to continue after fixing the issues.
    Press 'continue' to disable Lua compiled Unreal types and run anyway. 
    Press 'cancel' to close the program.)###";
    
	FString errorDetails = errorStr.ToString();
    FText txt = FText::FromString(FString::Printf(TEXT("%hs\n--------------\n%s\n--------------\n%hs"), title, *errorDetails, instructions));
    const EAppReturnType::Type clicked = FMessageDialog::Open(EAppMsgCategory::Error, EAppMsgType::CancelRetryContinue, txt);

	return clicked;    
}

EAppReturnType::Type UUnrealLuaCompiler::DisplayRunawayErrorBox(const TArray<FString>& errors)
{
	const char* title = "Compilation error:";
    
#if UNREALLUA_UE_VERSION_OLDER_THAN(5,8,0)
	FStringBuilderBase errorStr;
#else
	TStringBuilder<4096> errorStr;
#endif
	
	for (const FString& str: errors)
	{
		errorStr << str << "\n";
	}
	const char* instructions = R"###(Please fix all Lua compilation errors.
    Press 'Ok' to cancel incomplete compilation and continue running. This
	may cause issues or crashes. 
    Press 'Cancel' to close the program.)###";
    
	FText txt = FText::FromString(FString::Printf(TEXT("%hs\n----------------------------------\n%s\n----------------------------------\n%hs"), title, errorStr.GetData(), instructions));
	const EAppReturnType::Type clicked = FMessageDialog::Open(EAppMsgCategory::Error, EAppMsgType::OkCancel, txt);

	return clicked;
}

void UUnrealLuaCompiler::BuildInputActions(FScopedLuaContext& ctx)
{
	if(!UUnrealLuaConfig::IsLuaEnabled())
	{
		return;
	}
	if(UUnrealLuaEngineSubsystem::IsGameSessionActive())
	{
		LUA_LOG_ERROR("Can't build action inputs while a Lua Game Session is active")
		return;
	}
	FLuaPath path;
	path.SetupPackagePaths({}, NAME_None, ELuaPathFlags::UnrealTypes | ELuaPathFlags::BaseGame);
	FLuaFileLister inputActionFilesLister{path, "ActionInputs/", false, {"_", "."}, {UUnrealLuaConfig::GetLuaScriptModFileExtension()}};
	
	
	for(FString& luaFilePath : inputActionFilesLister.FoundFullFilePaths)
	{
		sol::protected_function_result result = ctx.RunSingleScriptFile(StringCast<char>(*luaFilePath).Get());
		if (!result.valid() || result.return_count() != 1)
		{
			continue;
		}
		sol::object returnobject = result[0];
		if (!returnobject.valid())
		{
			continue;
		}
		if (returnobject.is<sol::table>())
		{
			sol::table inputActionInfoTbl = returnobject.as<sol::table>();
			if(inputActionInfoTbl.valid())
			{
				FString fileName = FPaths::GetBaseFilename(luaFilePath, true);
				verify(!fileName.EndsWith(".lua"));
				
				UInputAction* newInputAction = NewObject<UInputAction>(this->UnrealLuaInputActionPackage, UInputAction::StaticClass(), *fileName);
				UnrealLua::PropertyHelper::InitializeUObjectFromTable(newInputAction, inputActionInfoTbl);
				
				this->InputActions.Add(newInputAction);
			}					
		}
	}
}

void UUnrealLuaCompiler::RunPrototypeLuaFiles(FScopedLuaContext& ctx)
{
	LUA_LOG("Running prototype lua files")
	
	lua_State* L = ctx.GetLuaThisState().lua_state();
	//FLuaPath path;
	//path.SetupPackagePaths({}, NAME_None, ELuaPathFlags::UnrealTypes | ELuaPathFlags::BaseGame);
	//FLuaFileLister uclassFileLister{path, "", true, {"_", "."}, {UUnrealLuaConfig::GetLuaScriptModFileExtension()}};

	TSharedPtr<FUnrealLuaFileSystemEntry> comoilerTypesDirectory = UUnrealLuaFileSystem::Get()->GetUnrealTypesCompilerDirectory();
	TArray<TSharedPtr<FUnrealLuaFileSystemEntry>> files = comoilerTypesDirectory->GetAllFilesRecursive({"_", "."}, {UUnrealLuaConfig::GetLuaScriptModFileExtension(), ".mod.lua"});
	
	//Run lua files to produce prototypes
	for(auto& file : files)
	{
		verify(file->IsFile())
		this->CurrentlyRunFilePath = file->GetFullPath();
		
		LUA_LOG("Running prototype lua file %s", *this->CurrentlyRunFilePath)
		
		verify(!this->FileContents.Contains(this->CurrentlyRunFilePath))
		
		
		FLuaCompilerSourceFileInfo& fileContent = this->FileContents.Emplace(this->CurrentlyRunFilePath, "");
		fileContent.FullSource = file->LoadFileToString();
		
		//FFileHelper::LoadFileToString(fileContent.FullSource, *luaFilePath);
		fileContent.ProcessFile();
	
		this->bIsExecutingLuaFile = true;
		sol::protected_function_result result = ctx.RunString(*fileContent.GetFullSource(), {});
		this->bIsExecutingLuaFile = false;
		//sol::protected_function_result result = ctx.RunSingleScriptFile(StringCast<char>(*luaFilePath).Get(), sol::variadic_args{ctx.GetLuaThisState().lua_state()});
		this->CurrentlyRunFilePath = "";
		if (!result.valid())
		{
			sol::error err = result;
			std::string msg = err.what();
			LUA_LOG_ERROR("%hs", msg.c_str())
			this->ErrorLog.Add("Compilation aborted during Prototype creation");
			this->ErrorLog.Add(msg.c_str());
			LUA_LOG("Aborting compilation due to UnrealLua UType error")
			return;
		}
	}
	LUA_LOG("Running prototype lua files finished")
}

bool UUnrealLuaCompiler::Compile()
{
	if (!UUnrealLuaConfig::IsLuaEnabled())
	{
		return false;
	}
	
	LUA_LOG("UnrealLua Compiling Custom Types");
	
    bool contiueDespiteErrorsAndDiscardUnrealLuaUTypes = false;
	do {
		verify(this->CompilerLuaContext == nullptr);
		this->ErrorLog.Empty();
		this->UnrealLuaUTypePrototypes.Empty();
		this->CurrentlyCompiledType.Reset();
		this->CompiledEnums.Empty();
		this->CompiledClasses.Empty();
		this->CompiledStructs.Empty();
		this->FileContents.Empty();
		
		contiueDespiteErrorsAndDiscardUnrealLuaUTypes = false;
		
		UUnrealLuaEngineSubsystem::Get()->UObjectRegistry->UClassOverrideRegistry.DisableUFunctionOverriding();
		
		this->CompilerLuaContext = NewObject<UStandaloneLuaContext>(this, UStandaloneLuaContext::StaticClass(), "UnrealLuaCompilerLuaContext");
		this->CompilerLuaContext->InitializeLuaStateAndLoadGameMode(ELuaContextType::UnrealLuaTypeCompiler, "UnrealLuaTypeContext", ".Compiler");
		FScopedLuaContext& ctx = this->CompilerLuaContext->GetScopedLuaContext();
		
		//Hack in compiler usertypes
		{
			sol::state_view lua = ctx.GetLuaState();
			UnrealLua::Usertypes::Registration::RegisterUsertypes(lua);
		}
		
		//this->BuildInputActions(ctx);
		
		this->RunPrototypeLuaFiles(ctx);
		
		if (!this->HasErrors())
        {
			this->CompileUnrealTypes(ctx);
        }
		
		if (this->HasErrors())
		{
			//display text
			contiueDespiteErrorsAndDiscardUnrealLuaUTypes = this->HandleErrorBox(this->ErrorLog);
			
			//@TODO : clear out already compiled UTypes
			TArray<UObject*> objs;
			GetObjectsWithPackage(this->UnrealLuaCompilerUTypePackage, objs, false);
			for (UObject* obj : objs)
			{
				obj->Rename(*("TRASH__" + obj->GetName() + "__UNREALLUA"), GetTransientPackage(), REN_DoNotDirty | REN_DontCreateRedirectors | REN_NonTransactional);
				obj->ConditionalBeginDestroy();
			}
			objs.Empty();
			GetObjectsWithPackage(this->UnrealLuaCompilerUTypePackage, objs, true);
			verify(objs.IsEmpty())
	
			this->CompiledEnums.Empty();
			this->CompiledStructs.Empty();
			this->CompiledClasses.Empty();
		}
		else
		{
			this->BackupCompiledTypes();
		}
				
		//Discard remaining prototypes:
		//Either they got compiled into real types or we have an error,
		//in either case we don't need them anymore
		this->UnrealLuaUTypePrototypes.Empty();
		this->CurrentlyCompiledType.Reset();
		this->FileContents.Empty();
		
		this->CompilerLuaContext->ConditionalBeginDestroy();
		this->CompilerLuaContext = nullptr;
		
		UUnrealLuaEngineSubsystem::Get()->UObjectRegistry->UClassOverrideRegistry.EnableUFunctionOverriding();
		
		if (IsEngineExitRequested())
		{
			return false;
		}
		
		if (this->HasErrors() && !contiueDespiteErrorsAndDiscardUnrealLuaUTypes)
		{
			LUA_LOG_WARNING("UnrealLua: Retrying compilation");
		}
	} while (this->HasErrors() && !contiueDespiteErrorsAndDiscardUnrealLuaUTypes);
	
	verify((!this->HasErrors() && !contiueDespiteErrorsAndDiscardUnrealLuaUTypes) || (this->HasErrors() && contiueDespiteErrorsAndDiscardUnrealLuaUTypes));
	
	if (this->HasErrors())
	{
		this->TryRecoverWithBackupTypes();
	}
	
	for (TTuple<FName, TObjectPtr<UScriptStruct>> pair : this->CompiledStructs)
	{
		UScriptStruct* uss = pair.Value.Get();
		NotifyRegistrationEvent(UnreaLLuaCompiledPackageName, *uss->GetName(), ENotifyRegistrationType::NRT_Struct, ENotifyRegistrationPhase::NRP_Finished, nullptr, false, uss);
	}
	
	for (TTuple<FName, TObjectPtr<UClass>> pair : this->CompiledClasses)
	{
		UClass* uclass = pair.Value.Get();
		NotifyRegistrationEvent(UnreaLLuaCompiledPackageName, *uclass->GetName(), ENotifyRegistrationType::NRT_Class, ENotifyRegistrationPhase::NRP_Finished, nullptr, false, uclass);
	}
	
	//IAssetRegistry::Get()->Package
	return true;
}


bool UUnrealLuaCompiler::CompileUnrealTypes(FScopedLuaContext& ctx)
{
	LUA_LOG("UnrealLua UType compilation begin")
	verify(this->CompiledEnums.IsEmpty())
	verify(this->CompiledClasses.IsEmpty())
	verify(this->CompiledStructs.IsEmpty())
	
	//Compile prototypes:
	
	if (!UnrealLua::Compiler::CompilePrototypes(this))
	{
		return false;
	}
	
	verify(!this->HasErrors())
	
	for(TTuple<FString, TUniquePtr<UnrealLua::Compiler::IStructPrototypeBase>>& it : this->UnrealLuaUTypePrototypes)
	{
		UnrealLua::Compiler::IStructPrototypeBase* proto = it.Value.Get();
		UField* compiledField = proto->GetCompiledField();
		verify(IsValid(compiledField))
		verify(compiledField->GetOuter() == this->UnrealLuaCompilerUTypePackage)
		
		//Last verification check
		if (!this->IsCompiledTypeValid(compiledField))
		{
			return false;
		}
		const FName name = compiledField->GetFName();
		if (UClass* newClass = Cast<UClass>(compiledField))
		{
			this->CompiledClasses.Emplace(name, newClass);
		}
		else if (UScriptStruct* newStruct = Cast<UScriptStruct>(compiledField))
		{
			this->CompiledStructs.Emplace(name, newStruct);
		}
		else if (UEnum* newEnum = Cast<UEnum>(compiledField))
		{
			this->CompiledEnums.Emplace(name, newEnum);
		}
	}
	LUA_LOG("UnrealLua UType compilation finished")
	return true;
}

int UUnrealLuaCompiler::__UENUM(lua_State* L)
{
	UUnrealLuaCompiler* compiler = UUnrealLuaCompiler::Get();
	verify(compiler != nullptr);
	verify(compiler->bIsExecutingLuaFile);
	
	lua_Debug ar{};
	lua_getstack(L, 1, &ar);
	lua_getinfo(L, "Sunl", &ar);
	//LUA_LOG("Begin compiling UEnum from file %hs at line %d", ar.source, ar.currentline)
	
	sol::variadic_args args{L, 1, sol::stack::top(L)};
	
	if (compiler->CompilationInProgress())
	{
		SetError("UClass prototype compilation interrupted by an UENUM compiler prototype attempt. Please only compile one type at time!");
		return 0;
	}
	
	compiler->CurrentlyCompiledType = MakeUnique<UnrealLua::Compiler::FUnrealLuaCompilerUEnumPrototype>();
	UnrealLua::Compiler::IStructPrototypeBase* proto = compiler->CurrentlyCompiledType.Get();
	proto->Run(compiler, args);
	verify(!proto->GetIsError())
	return sol::stack::push<UnrealLua::Compiler::FUnrealLuaCompilerUEnumPrototype*>(L, static_cast<UnrealLua::Compiler::FUnrealLuaCompilerUEnumPrototype*>(compiler->CurrentlyCompiledType.Get()));
}


int UUnrealLuaCompiler::__UCLASS(lua_State* L)
{
	UUnrealLuaCompiler* compiler = UUnrealLuaCompiler::Get();
	verify(compiler != nullptr);
	verify(compiler->bIsExecutingLuaFile);
	
	sol::variadic_args args{L, 1, sol::stack::top(L)};
	if (compiler->CompilationInProgress())
	{
		SetError("UClass prototype compilation interrupted by another compiler prototype attempt. Please only compile one type at time!");
		return 0;
	}
	
	compiler->CurrentlyCompiledType = MakeUnique<UnrealLua::Compiler::FUnrealLuaCompilerUClassPrototype>();
	UnrealLua::Compiler::FUnrealLuaCompilerUClassPrototype* proto = static_cast<UnrealLua::Compiler::FUnrealLuaCompilerUClassPrototype*>(compiler->CurrentlyCompiledType.Get());
	proto->Run(compiler, args);
	verify(!proto->GetIsError())
	return sol::stack::push<UnrealLua::Compiler::FUnrealLuaCompilerUClassPrototype*>(L, static_cast<UnrealLua::Compiler::FUnrealLuaCompilerUClassPrototype*>(compiler->CurrentlyCompiledType.Get()));
}

int UUnrealLuaCompiler::__USTRUCT(lua_State* L)
{
	UUnrealLuaCompiler* compiler = UUnrealLuaCompiler::Get();
	verify(compiler != nullptr);
	verify(compiler->bIsExecutingLuaFile);
	
	lua_Debug ar{};
	lua_getstack(L, 1, &ar);
	lua_getinfo(L, "Sunl", &ar);
	//LUA_LOG("Begin compiling UScriptStruct from file %hs at line %d", ar.source, ar.currentline)
	
	sol::variadic_args args{L, 1, sol::stack::top(L)};
	if (compiler->CompilationInProgress())
	{
		SetError("UScriptStruct prototype compilation interrupted by another compiler attempt. Please only compile one type at time!");
		return 0;
	}
	compiler->CurrentlyCompiledType = MakeUnique<UnrealLua::Compiler::FUnrealLuaCompilerUScriptStructPrototype>();
	UnrealLua::Compiler::IStructPrototypeBase* proto = compiler->CurrentlyCompiledType.Get();
	proto->Run(compiler, args);
	
	//if there are any errors, it should have thrown a sol::error
	verify(!proto->GetIsError())
	
	return sol::stack::push<UnrealLua::Compiler::FUnrealLuaCompilerUScriptStructPrototype*>(L, static_cast<UnrealLua::Compiler::FUnrealLuaCompilerUScriptStructPrototype*>(compiler->CurrentlyCompiledType.Get()));
}

int UUnrealLuaCompiler::__UINTERFACE(lua_State* L)
{
	UUnrealLuaCompiler* compiler = UUnrealLuaCompiler::Get();
	verify(compiler != nullptr);
	verify(compiler->bIsExecutingLuaFile);
	
	sol::variadic_args args{L, 1, sol::stack::top(L)};
	if (compiler->CompilationInProgress())
	{
		SetError("Prototype compilation interrupted by UInterface compiler prototype attempt. Please only compile one type at time!");
		return 0;
	}
	
	compiler->CurrentlyCompiledType = MakeUnique<UnrealLua::Compiler::FUnrealLuaCompilerUInterfacePrototype>();
	UnrealLua::Compiler::FUnrealLuaCompilerUInterfacePrototype* proto = static_cast<UnrealLua::Compiler::FUnrealLuaCompilerUInterfacePrototype*>(compiler->CurrentlyCompiledType.Get());
	proto->Run(compiler, args);
	verify(!proto->GetIsError())
	return sol::stack::push<UnrealLua::Compiler::FUnrealLuaCompilerUInterfacePrototype*>(L, static_cast<UnrealLua::Compiler::FUnrealLuaCompilerUInterfacePrototype*>(compiler->CurrentlyCompiledType.Get()));
}

int UUnrealLuaCompiler::__MULTICAST_DELEGATE(lua_State* L)
{
	UUnrealLuaCompiler* compiler = UUnrealLuaCompiler::Get();
	verify(compiler != nullptr);
	verify(compiler->bIsExecutingLuaFile);
	
	lua_Debug ar{};
	lua_getstack(L, 1, &ar);
	lua_getinfo(L, "Sunl", &ar);
	//LUA_LOG("Begin compiling Multicast delegate from file %hs at line %d", ar.source, ar.currentline)
	
	sol::variadic_args args{L, 1, sol::stack::top(L)};
	if (compiler->CompilationInProgress())
	{
		SetError("Multicast delegate prototype compilation interrupted by another compiler attempt. Please only compile one type at time!");
		return 0;
	}
	/*
	compiler->CurrentlyCompiledType = MakeUnique<UnrealLua::Compiler::FUnrealLuaCompilerMulticastDelegatePrototype>();
    UnrealLua::Compiler::IStructPrototypeBase* proto = compiler->CurrentlyCompiledType.Get();
    proto->Run(compiler, args);
*/
	return 0;
}


void UUnrealLuaCompiler::SetError(const FString& optionalErrorMsg)
{
	UUnrealLuaCompiler* compiler = UUnrealLuaCompiler::Get();
	if (!compiler)
	{
		LUA_LOG_ERROR("No UnrealLua compiler running!")
		checkNoEntry();
		return;
	}
	compiler->ErrorLog.Empty();
	if (!optionalErrorMsg.IsEmpty())
	{
		compiler->ErrorLog.EmplaceAt(0, optionalErrorMsg);
	}
	if (!compiler->CurrentlyRunFilePath.IsEmpty())
	{
		FString genericError = FString::Printf(TEXT("Current file:\n%s\n"), *compiler->CurrentlyRunFilePath);
		compiler->ErrorLog.EmplaceAt(0, genericError);
		LUA_LOG_ERROR("%s %s", *genericError, *optionalErrorMsg)
	}
	
	if (compiler->ShouldThrowLuaOnError())
	{
		std::string errorMsg = std::format("Compilation failed in file {}. See log for details.", StringCast<char>(*compiler->CurrentlyRunFilePath).Get());
		throw sol::error(errorMsg);		
	}
}

bool UUnrealLuaCompiler::IsCompiledTypeValid(UField* newType) const
{
	if (!IsValid(newType))
	{
		return false;
	}
	FName name = newType->GetFName();
	if (name == NAME_None)
	{
		return false;
	}
	if (!newType->IsIn(this->UnrealLuaCompilerUTypePackage))
	{
		return false;
	}
	if (this->CompiledClasses.Contains(name))
	{
		return false;
	}
	if (this->CompiledStructs.Contains(name))
	{
		return false;
	}
	if (this->CompiledEnums.Contains(name))
	{
		return false;
	}
	return true;
}


bool UUnrealLuaCompiler::HasErrors() const
{
	return !this->ErrorLog.IsEmpty();
}

void UUnrealLuaCompiler::NotifyEndFrame()
{
	if (this->CurrentlyCompiledType.IsValid())
	{
		FString name = this->CurrentlyCompiledType->GetTypeNameString();
		LUA_LOG_ERROR("UnrealLua compiler: Runaway compilation of type: %s", *name)
		this->ErrorLog.Emplace(FString::Printf(TEXT("UnrealLua compiler: Runaway compilation of type: %s"), *name));
		EAppReturnType::Type answer = this->DisplayRunawayErrorBox(this->ErrorLog);
		if (answer == EAppReturnType::Cancel)
		{
			LUA_LOG_WARNING("UnrealLua: User requested closing to due compilation error");
			RequestEngineExit(TEXT("UnrealLua: Compilation Error.\nUser requested closing."));
			return;	
		}
		else if (answer == EAppReturnType::Ok)
		{
			LUA_LOG_WARNING("UnrealLua: User requested ignoring runaway compilation. Canceling currently compiled type");
		}
		else
		{
			LUA_LOG_ERROR("Unknown response %d", (int32)answer)
			checkNoEntry();
		}
		UField* field = this->CurrentlyCompiledType->GetCompiledField();
		if(field->IsIn(this->UnrealLuaCompilerUTypePackage))
		{
			field->ConditionalBeginDestroy();
		}
		this->CurrentlyCompiledType.Reset();
	}
}

TArray<UClass*> GetClassesFromModule(const FName& ModuleName)
{
	TArray<UClass*> Classes;
	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* Class = *It;
		if (Class->GetPackage() && Class->GetPackage()->GetFName() == ModuleName)
		{
			Classes.Add(Class);
		}
	}
	return Classes;
}

void UUnrealLuaCompiler::NotifyModuleChanged(FName moduleName, EModuleChangeReason moduleChangeReason)
{
	if (moduleChangeReason == EModuleChangeReason::ModuleLoaded)
	{
	}
}

void UUnrealLuaCompiler::CommitPrototype(UnrealLua::Compiler::IStructPrototypeBase* commitedBy)
{
	verify(this->bIsExecutingLuaFile);
	verify(this->CurrentlyCompiledType.IsValid());
	verify(this->CurrentlyCompiledType.Get() == commitedBy);
	verify(!this->CurrentlyCompiledType->GetIsError());
	verify(this->CurrentlyCompiledType->IsCommitReady());
	const FString& className = this->CurrentlyCompiledType->GetTypeName().ToString();
	verify(!className.IsEmpty())

	UObject* existing = FindObjectSafe<UObject>(this->UnrealLuaCompilerUTypePackage, *className);
	if (existing)
	{
		this->SetIsErrorWithArgs("Can not create Lua-Compiled UClass %s from file %s: A type with such a name already exists in the package", *className, *this->CurrentlyRunFilePath);
		return;
	}

	if (this->UnrealLuaUTypePrototypes.Contains(className))
	{
		this->SetError(FString::Printf(TEXT("Can not commit Lua-Compiled UClass %s from file %s: A prototype with such a name already exists"), *className, *this->CurrentlyRunFilePath));
		return;
	}
	UnrealLua::Compiler::IStructPrototypeBase* prototype = this->CurrentlyCompiledType.Release();
	this->UnrealLuaUTypePrototypes.Emplace(className,TUniquePtr<UnrealLua::Compiler::IStructPrototypeBase>(prototype));
}

lua_State* UUnrealLuaCompiler::GetCompilerLuaState() const
{
	if (this->CompilerLuaContext == nullptr)
	{
		return nullptr;
	}
	verify(this->CompilerLuaContext->GetScopedLuaContext().IsInitialized())
	//verify(this->CompilerLuaContext->GetScopedLuaContext().IsLuaLoaded())
	return this->CompilerLuaContext->GetScopedLuaContext().GetLuaThisState().lua_state();
}

sol::state_view UUnrealLuaCompiler::GetCompilerLuaStateView() const
{
	return { this->GetCompilerLuaState()};
}

FScopedLuaContext& UUnrealLuaCompiler::GetCompilerScopedLuaStateContext() const
{
	return this->CompilerLuaContext->GetScopedLuaContext();
}

void UUnrealLuaCompiler::NotifyLuaStateActiveChange(UUnrealLuaEngineSubsystem* ss, const TScriptInterface<ILuaContext> ictx, bool isActive)
{
	if (!isActive)
	{
		for (const TTuple<FName, TObjectPtr<UClass>>& pair : this->CompiledClasses)
		{
			UClass* uclass = pair.Value;
			for (TFieldIterator<UFunction> it(uclass, EFieldIterationFlags::None); it; ++it)
			{
				if (UUnrealLuaCompiledUFunction* func = Cast<UUnrealLuaCompiledUFunction>(*it))
				{
					func->RemoveLuaContext(ictx);
				}
			}
		}
	}
}

bool UUnrealLuaCompiler::CompilationInProgress() const
{
	const bool compilationInProgress = this->CurrentlyCompiledType.IsValid();
	if (compilationInProgress)
	{
		UnrealLua::Compiler::EUnrealLuaCompilerPrototypeType type = this->CurrentlyCompiledType.Get()->GetPrototypeCategory();
		verify(type == UnrealLua::Compiler::EUnrealLuaCompilerPrototypeType::Class || type == UnrealLua::Compiler::EUnrealLuaCompilerPrototypeType::ScriptStruct || type == UnrealLua::Compiler::EUnrealLuaCompilerPrototypeType::Interface);
	}
	return compilationInProgress;
}

FString UUnrealLuaCompiler::GetUnrealName(bool bIsStruct, const FString& ClassName)
{
	FString UnrealName = ClassName;
	if (UnrealName.Len() >= 2)
	{
		if (bIsStruct ? (UnrealName[0] == 'F') : (UnrealName[0] == 'U' || UnrealName[0] == 'A'))
		{
			if (FChar::IsUpper(UnrealName[1]))
			{
				UnrealName = UnrealName.Mid(1);
			}
		}
	}
	return UnrealName;
}

void UUnrealLuaCompiler::BackupCompiledTypes()
{
	UnrealLua::Compiler::BackupCompiledPrototypes(this);
}

void UUnrealLuaCompiler::TryRecoverWithBackupTypes()
{
}
