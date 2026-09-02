// Fill out your copyright notice in the Description page of Project Settings.
#include "LuaContext/ScopedLuaContext.h"

#include "LuaContext/LuaImportRegistry.h"

extern "C" {
	#include "lstate.h"
}
#include "lua.hpp"
#include "Utility/LuaLogMacros.h"
#include "Config/UnrealLuaConstants.h"
#include "Engine/Engine.h"
#include "Tests/LuaSelfTests.h"
#include "LuaTypes/LuaUsertypes.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/CoreDelegates.h"
#include "LuaContext/LuaScripts/LoadedLuaScriptCollection.h"
#include "Utility/LuaFileLister.h"
#include "utility/is_integer.hpp"
#include "utility/to_string.hpp"
#include "UnrealLua.h"
#include "sol/sol.hpp"
#include "Config/UnrealLuaConfig.h"
#include "Engine/GameInstance.h"
#include "Input/LuaStateInputHandler.h"
#include "Interface/LuaContext.h"
#include "LuaTypes/LuaLightUserdata.h"
#include "Mods/UnrealLuaMod.h"
#include "SubSystem/UnrealLuaModSubsystem.h"

class ILuaContext;
class FLuaUStruct;

namespace UnrealLua::Memory
{
	/*
	** About the realloc function:
	** void *frealloc (void *ud, void *ptr, size_t osize, size_t nsize);
	** ('osize' is the old size, 'nsize' is the new size)
	**
	** - frealloc(ud, p, x, 0) frees the block 'p' and returns NULL.
	** Particularly, frealloc(ud, NULL, 0, 0) does nothing,
	** which is equivalent to free(NULL) in ISO C.
	**
	** - frealloc(ud, NULL, x, s) creates a new block of size 's'
	** (no matter 'x'). Returns NULL if it cannot create the new block.
	**
	** - otherwise, frealloc(ud, b, x, y) reallocates the block 'b' from
	** size 'x' to size 'y'. Returns NULL if it cannot reallocate the
	** block to the new size.
	*/
	static void *Alloc (void *ud, void *ptr, size_t osize, size_t nsize) {
		//(void)ud;
		FScopedLuaContext* owningContext  = static_cast<FScopedLuaContext*>(ud);

		if(ptr == nullptr)
		{
			//Allocate new object
			
			//When ptr is NULL, osize encodes the kind of object that Lua is allocating.
			//osize is any of LUA_TSTRING, LUA_TTABLE, LUA_TFUNCTION, LUA_TUSERDATA, or LUA_TTHREAD
			//when (and only when) Lua is creating a new object of that type. When osize is some other
			//value, Lua is allocating memory for something else
			if(osize == LUA_TUSERDATA)
			{
				//LUA_LOG("Lua allocating %llu bytes for userdata", nsize)
				return FMemory::Realloc(ptr, nsize);
			}
			else if (osize == LUA_TSTRING)
			{
				return FMemory::Realloc(ptr, nsize);
			}
			else
			{
				return FMemory::Realloc(ptr, nsize);
			}
		}
		else
		{
			//Reallocate or delete, depends on nsize
			//if nsize == 0 : free memory
			//if nsize > 0: reallocate object to new memory block
			
			//An allocated item got deleted or will be reallocated.
			//If it's the same as currently pointed at for the GC collection, reset GC
			if(ptr == owningContext->NextGCObjectToCheck)
			{
				owningContext->ResetIncrementalGC();
			}
			
			//When ptr is not NULL, osize is the size of the block pointed by ptr, that is,
			//the size given when it was allocated or reallocated
			//@TODO : Investigate whether this can be somehow used for better mem-pooling
			
			if(nsize == 0)
			{
				//When nsize is zero, the allocator must behave like free and return NULL.
				//LUA_LOG("Lua freeing %llu bytes", osize)
				FMemory::Free(ptr);
				return nullptr;
			}
			else
			{
				//If nsize is > 0, we should reallocate the ptr memory
				//to a new memory block of nsize size
				return FMemory::Realloc(ptr, nsize);	
			}
		}
	}	
}

FScopedLuaContext::FScopedLuaContext()
	: FGCObject(), ContextType(ELuaContextType::None), LuaPath(FLuaPath()), LuaState(),OwningLuaContext(nullptr), LoadedScripts({}), LuaStateLock(), LoadedGameModeSettings(), LuaContextName()
{
	
}

FScopedLuaContext::FScopedLuaContext(TScriptInterface<ILuaContext> owningLuaContext, ELuaContextType type, const FString& name)
 : FGCObject(), ContextType(type), LuaPath(FLuaPath()), LuaState(), OwningLuaContext(owningLuaContext), LoadedScripts({}), LuaStateLock(), LoadedGameModeSettings(), LuaContextName(name)
{
	this->IncrementalNumObjectsToCheck = UUnrealLuaConfig::GetLuaIncrementalGCLimit();

	if (this->OwningLuaContext != nullptr)
	{
		UWorld* world = owningLuaContext.GetObject()->GetWorld();
		if (world && world->IsGameWorld())
		{
			this->PlayerInputHandler = NewObject<ULuaStateInputHandler>(this->OwningLuaContext.GetObject());
		}
	}
	
	this->InitializeLuaState();
	
	UObject* owningLuaContextObject = owningLuaContext.GetObject();
	if (owningLuaContextObject)
	{	
		//LUA_EXTRASPACE ptr will get owning ILuaContext ptr
		void* extraSpace = lua_getextraspace(this->LuaState.lua_state());
		UObject** extraSpaceRef = std::bit_cast<UObject**>(extraSpace);
		*extraSpaceRef = owningLuaContextObject;
	}
}

FScopedLuaContext::~FScopedLuaContext()
{
	this->Shutdown();
}

const FString& FScopedLuaContext::GetLuaContextName() const
{
	return this->LuaContextName;
}

FScopedLuaContext* FScopedLuaContext::GetLuaContextFromLuaState(lua_State* L)
{
	if (!L)
	{
		return nullptr;
	}
	return static_cast<FScopedLuaContext*>(L->l_G->ud);
}

sol::table_proxy<sol::global_table&, sol::detail::proxy_key_t<const char* const>> FScopedLuaContext::operator[](const char* const key)
{
	return this->LuaState[key];
}

void FScopedLuaContext::Shutdown()
{
	FScopeLock locked{&this->LuaStateLock};
	lua_State* L = this->GetLuaState().lua_state();
	verify(L != nullptr);
	this->LuaState.globals()[sol::metatable_key] = sol::nil;
	this->NextIndexToHotReloadCheck = 0;
	this->LastCheckedScriptCollection = nullptr;
	
	for (FWeakLuaTableHandle& externalTable : this->ExternalLuaTables)
	{
		externalTable.Invalidate();
	}
	this->ExternalLuaTables.Empty();
	
	for (FWeakLuaCoroutineHandle& externalCoroutine : this->ExternalLuaCoroutines)
	{
		externalCoroutine.Invalidate();
	}
	this->ExternalLuaCoroutines.Empty();
	
	this->LoadedScriptsArray.Empty();
	for(TTuple<FName, TObjectPtr<ULoadedLuaScriptCollection>> scriptColl : this->LoadedScripts)
	{
		ULoadedLuaScriptCollection* coll = scriptColl.Value; 
		if(IsValid(coll))
		{
			coll->Reset();
			coll->MarkAsGarbage();
		}
	}
	this->LoadedScripts.Empty();
	this->LoadedGameModeSettings = {};
	if (this->PlayerInputHandler != nullptr)
	{
		this->PlayerInputHandler->ConditionalBeginDestroy();
		this->PlayerInputHandler = nullptr;
	}
	//this->DelegateRegistry.Clear();
	//this->LuaState = {};
	this->bIsInitialized = false;
}

void FScopedLuaContext::UnloadGameMode()
{
	this->UnloadGameModeInternal();
}

void FScopedLuaContext::UnloadGameModeInternal()
{
	//@TODO : needs to clean up FLuauObjectItems in registry as well
	this->LoadedScripts.Empty();
	TArray<TObjectPtr<ULoadedLuaScriptCollection>> temp = MoveTemp(this->LoadedScriptsArray);
	this->LoadedScriptsArray.Empty();
	for(TObjectPtr<ULoadedLuaScriptCollection> item : temp)
	{
		item->Reset();
	}
	this->LoadedScriptsArray.Empty();
	this->LuaPath = {};
	this->LoadedGameModeSettings = {};	
}

void FScopedLuaContext::InitializeLuaState()
{
	if(bIsInitialized)
	{
		LUA_LOG("LuaContext %s already initialized, can't reinit", *this->LuaContextName)
		return;
	}
	LUA_LOG("LuaContext %s initializing", *this->LuaContextName)
	this->bIsInitialized = true;
	
	this->LoadedGameModeSettings = {};
	this->StartLuaState();
	//LUA_LOG("Lua State base initialized, uses %llu memory before initialization", this->LuaState.memory_used())
	this->SetupLogFunctions();
	if(this->ContextType != ELuaContextType::Minimal)
	{
		FLuaUsertypes::RegisterLuaUserTypes(this->LuaState);		
		this->SetupUnrealTypes();
	}
	//LUA_LOG("Lua State with usertypes initialized, uses %llu memory before initialization", this->LuaState.memory_used())
	LUA_LOG("LuaContext %s initialized!", *this->LuaContextName)
}

void FScopedLuaContext::LateRegisterNewModuleAssets(const TArray<UClass*>& newUClasses, const TArray<UScriptStruct*>& newScriptStructs, const TArray<UEnum*>& newEnums, const TArray<UBlueprintFunctionLibrary*>& newBlueprintLibraries)
{
	FLuaImportRegistry::Get().LateRegisterNewModuleAssets(*this, newUClasses, newScriptStructs, newEnums, newBlueprintLibraries);
}

void FScopedLuaContext::SetupLuaStateForGameMode(const FName& gameMode, const ELuaPathFlags pathFlags)
{
	verify(this->bIsInitialized)
	
	LUA_LOG("Lua Context : Begin loading game mode %s", *gameMode.ToString())

	TArray<FString> modNames;
	TArray<UUnrealLuaMod*> mods;
	
	if(this->ContextType == ELuaContextType::Game && this->OwningLuaContext)
	{
		if(UWorld* world = this->OwningLuaContext->GetWorldFromUObject())
		{
			if(UGameInstance* gi = world->GetGameInstance())
			{
				if(UUnrealLuaModSubsystem* mss = gi->GetSubsystem<UUnrealLuaModSubsystem>())
				{
					mods = mss->GetEnabledMods();
				}		
			}	
		}
	}

	for (UUnrealLuaMod* mod : mods)
	{
		modNames.Add(mod->GetModName());
	}
	
	this->LoadedGameModeSettings = FLoadedLuaGameModeSettings{modNames, gameMode};

	this->LuaPath.SetupPackagePaths(mods, gameMode, pathFlags);

	this->LoadedGameModeSettings.bIsLuaLoaded = true;
	
	if (this->ContextType != ELuaContextType::Minimal)
	{
		std::string uprojectDir = TCHAR_TO_UTF8(*(FPaths::ProjectDir()));
		LUA_LOG("Lua Context loading main.lua from folder %s Lua/", *FPaths::ProjectDir())
	
		sol::object filesToRun = this->ImportLuaScript(std::string{"main.lua"});

		if(filesToRun.valid() && filesToRun.is<sol::table>())
		{
			LUA_LOG("Lua Context running additional init files")
			sol::table filesTbl = filesToRun.as<sol::table>();
			filesTbl.for_each([uprojectDir,this, lua = &this->LuaState](sol::object key, sol::object val)
			{
				if(val.get_type() != sol::type::string)
				{
					return;
				}
				sol::string_view strv = val.as<sol::string_view>();
				LUA_LOG("Lua Context running additional init file %hs", strv.data())
				this->ImportLuaScript(/*uprojectDir + "Lua/" +*/strv);
			});
		}	
	}
	LUA_LOG("Lua Context fully loaded and ready. Now reloading existing scripts")
	for(const TTuple<FName, TObjectPtr<ULoadedLuaScriptCollection>>& pair : this->LoadedScripts)
	{
		pair.Value->Reload();
	}

	//this->LuaState[UnrealLua::constKey::luaContext] = this;
	LUA_LOG("Lua Context fully set up")

}

bool FScopedLuaContext::IsReadyForFinishDestroy()
{
	return !this->IsLuaLoaded();
}

bool FScopedLuaContext::IsLuaLoaded()
{
	return this->LoadedGameModeSettings.bIsLuaLoaded;
}

bool FScopedLuaContext::PerformSelfTest(TScriptInterface<ILuaContext> ctx)
{
	//Should only be called by LuaConfig during startup
	verify(UUnrealLuaConfig::ShouldPerformSelfTest())
	return FLuaConversionTests::Test(ctx);	
}

void FScopedLuaContext::Tick(float deltaTime)
{
	this->TickFileScanner(deltaTime);
}

bool FScopedLuaContext::IsUObjectValid(sol::stack_object obj_o)
{
	return UnrealLua::LightUserdata::IsUObject(obj_o);
}

sol::state_view FScopedLuaContext::GetLuaState() const
{
	return this->LuaState.lua_state();
}

sol::this_state FScopedLuaContext::GetLuaThisState() const
{
	return sol::this_state{this->LuaState.lua_state()};
}


sol::table FScopedLuaContext::CopyTable(sol::table targetTable, sol::table templateTable, bool bShallowCopy)
{
	if(!templateTable.valid() || !targetTable.valid())
	{
		return sol::nil;
	}

	targetTable.clear();

	if(bShallowCopy)
	{
		templateTable.for_each([&targetTable](const sol::object& key, const sol::object& value)
        {
            targetTable.raw_set(key, value);
        });
	}
	
	targetTable.raw_set(sol::metatable_key, templateTable.raw_get<sol::table>(sol::metatable_key));
	
	return targetTable;
}

void FScopedLuaContext::PatchTable(sol::table templateTable, sol::table targetTable)
{
	if(!templateTable.valid() || !targetTable.valid())
	{
		return;
	}
	sol::table oldMetaTable = targetTable.raw_get<sol::table>(sol::metatable_key);
	if(templateTable.valid())
	{
		templateTable.for_each([&targetTable](const sol::object& key, const sol::object& value)
        {
			targetTable.raw_set(key, value);
        });
		
	}
	targetTable.raw_set(sol::metatable_key, oldMetaTable);
}

void FScopedLuaContext::SetupLogFunctions()
{
	LUA_LOG("Lua Context : Setting up log functions")
	sol::state_view lua = this->LuaState;

	lua["print"] = [](std::string msg, bool bToScreen, int32 color) {
		if (bToScreen && GEngine) {
			FColor msg_color;
			switch (color) {
				case 0: msg_color = FColor::White; break;
				case 1: msg_color = FColor::Black; break;
				case 2: msg_color = FColor::Blue; break;
				case 3: msg_color = FColor::Cyan; break;
				case 4: msg_color = FColor::Green; break;
				case 5: msg_color = FColor::Yellow; break;
				case 6: msg_color = FColor::Red; break;
				default: msg_color = FColor::White; break;
			}
			GEngine->AddOnScreenDebugMessage(-1, 10, msg_color, FString((msg).c_str()));
		}
		LUA_LOG("Print : %s", *FString(msg.c_str()));
	};
	
	lua["printError"] = [](std::string msg, bool bToScreen) {
		if (bToScreen && GEngine) {
			GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Red, FString((msg).c_str()));
		}
		LUA_LOG_ERROR("Error : %s", *FString(msg.c_str()));
	};

	lua["printWarning"] = [](std::string msg, bool bToScreen) {
		if (bToScreen && GEngine) {
			GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Yellow, FString((msg).c_str()));
		}
		LUA_LOG_WARNING("Warning : %s", *FString(msg.c_str()));
	};
}

void FScopedLuaContext::StartLuaState()
{
	LUA_LOG("Lua Context : Starting Lua state")
	verify(!this->LoadedGameModeSettings.bIsLuaLoaded);

	this->LuaState = {};
	this->LuaState = sol::state(sol::default_at_panic, UnrealLua::Memory::Alloc, 0, this);
	//this->LuaState.globals().clear();
	sol::state_view lua = this->LuaState;
	lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::table, sol::lib::package, sol::lib::io, sol::lib::string, sol::lib::debug, sol::lib::os, sol::lib::coroutine, sol::lib::table, /*sol::lib::jit, sol::lib::ffi,*/ sol::lib::bit32);
	lua.script(R"(
	function ErrorHandler(error_msg)
		printError(tostring(error_msg), true)
		return error_msg
	end
	)");
	this->LuaState[UnrealLua::LuaScriptKeys::luaContext] = this;
	
	this->RegistryTable = lua.create_table();
	
	sol::protected_function::set_default_handler(lua["ErrorHandler"]);
	
	/*
	 *https://www.lua.org/manual/5.4/manual.html#2.5.1 
	In incremental mode, each GC cycle performs a mark-and-sweep collection in small steps interleaved
	with the program's execution. In this mode, the collector uses three numbers to control its garbage-collection cycles: the
	garbage-collector pause, the garbage-collector step multiplier, and the garbage-collector step size.

	The garbage-collector pause controls how long the collector waits before starting a new cycle.
	The collector starts a new cycle when the use of memory hits n% of the use after the previous collection.
	Larger values make the collector less aggressive. Values equal to or less than 100 mean the collector
	will not wait to start a new cycle. A value of 200 means that the collector waits for the total memory
	in use to double before starting a new cycle. The default value is 200; the maximum value is 1000.

	The garbage-collector step multiplier controls the speed of the collector relative to memory allocation,
	that is, how many elements it marks or sweeps for each kilobyte of memory allocated. Larger values make
	the collector more aggressive but also increase the size of each incremental step. You should not use values
	less than 100, because they make the collector too slow and can result in the collector never finishing a cycle.
	The default value is 100; the maximum value is 1000.

	The garbage-collector step size controls the size of each incremental step, specifically how many bytes the
	interpreter allocates before performing a step. This parameter is logarithmic: A value of n means the interpreter
	will allocate 2n bytes between steps and perform equivalent work during the step. A large value (e.g., 60) makes
	the collector a stop-the-world (non-incremental) collector. The default value is 13, which means steps
	of approximately 8 Kbytes. 
	 */
	lua_gc(this->LuaState, LUA_GCPARAM, UUnrealLuaConfig::GetLuaGCStepPause(), UUnrealLuaConfig::GetLuaGCStepMultiplier(), UUnrealLuaConfig::GetLuaGCStepSize() );

	//UnrealLua::LuaContextRegistry::RegisterLuaState(*this);
}
