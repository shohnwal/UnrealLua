// Fill out your copyright notice in the Description page of Project Settings.


#include "LuaContext/LuaImportRegistry.h"

#include "LuaCoreDelegates.h"
#include "Utility/LuaLogMacros.h"
#include "sol/sol.hpp"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LuaTypes/LuaUClass.h"
#include "LuaTypes/LuaUStruct.h"
#include "LuaContext/ScopedLuaContext.h"
#include "LuaTypes/LuaPrimitives.h"
#include "StringHandling/UnrealLuaStringCache.h"
#include "UObject/UObjectIterator.h"
#include "GameFramework/Actor.h" 
#include "SubSystem/UnrealLuaEngineSubsystem.h"
#include "UObjectRegistry/UnrealLuaUObjectRegistry.h"
#include "UnrealLua.h"
#include "UObjectRegistry/LuaUObjectRegistry.h"

static const FDelegateHandle fLuaImportResultLuaTypeHandle = FLuaCoreDelegates::OnRegisterLuaUsertypes.AddStatic(&FLuaImportResult::RegisterUsertype);

void FLuaImportResult::RegisterUsertype(sol::state_view& lua)
{
	lua.new_usertype<FLuaImportRegistry>(
		"UnrealTypeRegistry",
		"new", sol::no_constructor,
		sol::meta_function::call, []() { LUA_LOG("bla!") return 123;},
		sol::meta_function::index, &FLuaImportRegistry::__index,
		sol::meta_function::new_index, &FLuaImportRegistry::__newindex
	);

}

sol::object FLuaImportResult::As(sol::object key, sol::this_state lua_view)
{
	if(this->ImportedObject.valid())
	{
		sol::state_view lua = lua_view;
		lua["UE"][key] = this->ImportedObject;
	}
	return this->ImportedObject;
}

sol::object FLuaImportResult::Get()
{
	return this->ImportedObject;
}

FLuaImportRegistry& FLuaImportRegistry::Get()
{
	return UUnrealLuaEngineSubsystem::Get()->GetLuaImportRegistry();
}

void FLuaImportRegistry::InitializeLuaContext(FScopedLuaContext& ctx)
{
	LUA_LOG("LuaImportRegistry registering native Unreal types in Lua context %s", *ctx.GetLuaContextName())
	
	sol::state_view lua = ctx.GetLuaState();
	
	sol::table& registryTable= ctx.GetRegistryTable(); 
	registryTable["import"] = [this](sol::object arg, sol::this_state lua)
	{
		return this->UImport(arg, lua);
	};
	
	registryTable["UE"] = this;
	
	/*
	Create a second layer behind _G which contains all the imported datatypes and libs
	That allows users to just write the type name without polluting the general _G space
	
	local hit = FHitResult()	-- calls __index on _G, looking up the import registry, which will return 
								-- the UE-native FHitResult UScriptStruct type, which then gets called 
								-- and a new struct gets instantiated
	
	Assigning a __newindex to _G will behave the same as before. The user may have to pay
	Atteinton as to not shadow imported types, but that can be easily rectified by nilling the _G entry:
	
	FHitResult = "nope"			-- imported FHitResult type is now hidden by the string entry in _G
	local var = FHitResult()	-- will throw error, due to an attempt to call a string
	FHitResult = nil			-- clear the key in _G
	local hit = FHitResult()	-- works again as expected
	
	*/
	sol::table _G = lua.globals();
	sol::table _Gmeta = lua.create_table();
	_Gmeta["__index"] = [this](sol::object _G_self, sol::object key, sol::this_state thisState)
	{
		return this->__index(key, thisState);
	};
	_G[sol::metatable_key] = _Gmeta;
	
	//make sure UE is in hidden layer
	verify(lua["UE"].get_or<FLuaImportRegistry*>(nullptr) == this);
	verify(_G.raw_get_or<FLuaImportRegistry*>("UE", nullptr) == nullptr)
	
	lua["UE"]["boolean"] = sol::object(lua, sol::in_place_type<FLuaPrimitiveCPPType>,FLuaPrimitiveCPPType{ELuaSupportedClassCastFlags::LUA_CASTCLASS_FBoolProperty});
	lua["UE"]["bool"] = sol::object(lua, sol::in_place_type<FLuaPrimitiveCPPType>,FLuaPrimitiveCPPType{ELuaSupportedClassCastFlags::LUA_CASTCLASS_FBoolProperty});
	
	lua["UE"]["byte"] = sol::object(lua, sol::in_place_type<FLuaPrimitiveCPPType>,FLuaPrimitiveCPPType{ELuaSupportedClassCastFlags::LUA_CASTCLASS_FByteProperty});
	lua["UE"]["uint8"] = sol::object(lua, sol::in_place_type<FLuaPrimitiveCPPType>,FLuaPrimitiveCPPType{ELuaSupportedClassCastFlags::LUA_CASTCLASS_FByteProperty});

	lua["UE"]["int8"] = sol::object(lua, sol::in_place_type<FLuaPrimitiveCPPType>,FLuaPrimitiveCPPType{ELuaSupportedClassCastFlags::LUA_CASTCLASS_FInt8Property});
	
	lua["UE"]["uint16"] = sol::object(lua, sol::in_place_type<FLuaPrimitiveCPPType>,FLuaPrimitiveCPPType{ELuaSupportedClassCastFlags::LUA_CASTCLASS_FUInt16Property});
	
	lua["UE"]["int16"]= sol::object(lua, sol::in_place_type<FLuaPrimitiveCPPType>,FLuaPrimitiveCPPType{ELuaSupportedClassCastFlags::LUA_CASTCLASS_FInt16Property});
	
	lua["UE"]["uint32"] = sol::object(lua, sol::in_place_type<FLuaPrimitiveCPPType>,FLuaPrimitiveCPPType{ELuaSupportedClassCastFlags::LUA_CASTCLASS_FUInt32Property});

	lua["UE"]["int32"] = sol::object(lua, sol::in_place_type<FLuaPrimitiveCPPType>,FLuaPrimitiveCPPType{ELuaSupportedClassCastFlags::LUA_CASTCLASS_FIntProperty});

	lua["UE"]["uint64"] = sol::object(lua, sol::in_place_type<FLuaPrimitiveCPPType>,FLuaPrimitiveCPPType{ELuaSupportedClassCastFlags::LUA_CASTCLASS_FUInt64Property});
	
	lua["UE"]["int"] = sol::object(lua, sol::in_place_type<FLuaPrimitiveCPPType>,FLuaPrimitiveCPPType{ELuaSupportedClassCastFlags::LUA_CASTCLASS_FInt64Property});	
	lua["UE"]["int64"] = sol::object(lua, sol::in_place_type<FLuaPrimitiveCPPType>,FLuaPrimitiveCPPType{ELuaSupportedClassCastFlags::LUA_CASTCLASS_FInt64Property});

	lua["UE"]["float"] = sol::object(lua, sol::in_place_type<FLuaPrimitiveCPPType>,FLuaPrimitiveCPPType{ELuaSupportedClassCastFlags::LUA_CASTCLASS_FFloatProperty});
	
	lua["UE"]["double"] = sol::object(lua, sol::in_place_type<FLuaPrimitiveCPPType>,FLuaPrimitiveCPPType{ELuaSupportedClassCastFlags::LUA_CASTCLASS_FDoubleProperty});
	
	lua["UE"]["number"] = sol::object(lua, sol::in_place_type<FLuaPrimitiveCPPType>,FLuaPrimitiveCPPType{ELuaSupportedClassCastFlags::LUA_CASTCLASS_FDoubleProperty});
		
	lua["UE"]["str"] = sol::object(lua, sol::in_place_type<FLuaPrimitiveCPPType>,FLuaPrimitiveCPPType{ELuaSupportedClassCastFlags::LUA_CASTCLASS_FStrProperty});

	//Hack primitives because these types don't have any ::StaticStruct()
	lua["UE"]["FString"] = sol::object(lua, sol::in_place_type<FLuaPrimitiveCPPType>,FLuaPrimitiveCPPType{ELuaSupportedClassCastFlags::LUA_CASTCLASS_FStrProperty});

	//string is already taken by string library...
	//lua["UE"]["string"] = sol::object(lua, sol::in_place_type<FLuaPrimitiveCPPType>,{ELuaSupportedClassCastFlags::LUA_CASTCLASS_FStrProperty});
	lua["UE"]["FText"] = sol::object(lua, sol::in_place_type<FLuaPrimitiveCPPType>,FLuaPrimitiveCPPType{ELuaSupportedClassCastFlags::LUA_CASTCLASS_FTextProperty});
	lua["UE"]["FName"] = sol::object(lua, sol::in_place_type<FLuaPrimitiveCPPType>,FLuaPrimitiveCPPType{ELuaSupportedClassCastFlags::LUA_CASTCLASS_FNameProperty});

	this->LoadEnums(ctx);
	this->LoadUClasses(ctx);
	this->LoadUScriptStructs(ctx);
	if (UUnrealLuaEngineSubsystem::IsGameSessionActive())
	{
		this->LoadBlueprintLibraries(ctx);
	}
	
	LUA_LOG("Registered all Unreal types.")

	//LUA_LOG("Globals after importing everything:")
	//lua.safe_script("for k,v in pairs(_G) do print(tostring(k) .. ':' .. tostring(v)) end");
}

FLuaImportRegistry::~FLuaImportRegistry()
{
	LUA_LOG("~FLuaImportRegistry")
}

sol::object FLuaImportRegistry::UImport(sol::object& name_o, sol::this_state thisState)
{
	FScopedLuaContext* ctx = FScopedLuaContext::GetLuaContextFromLuaState(thisState);
	sol::table& registryTable = ctx->GetRegistryTable(); 
	sol::object existing = registryTable[name_o];
	
	if(existing.valid())
	{
		return existing;
	}
	
	if (name_o.get_type() != sol::type::string)
	{
		return sol::nil;
	}

	sol::string_view key = name_o.as<sol::string_view>();


	return this->UImportPath(key, thisState);
}

void FLuaImportRegistry::ClearImportCache()
{
	verify(!UUnrealLuaEngineSubsystem::IsGameSessionActive())
	this->ImportsCache.Empty();
}

sol::object FLuaImportRegistry::UImportPath(const std::string_view& key, sol::this_state thisState)
{
	if (key.empty() || key.length() < 3)
	{
		return sol::nil;
	}
		
	//Native classes always start with upper letter, 
	//Blueprint paths start with /Game/... and native types start with A, U, E or F and always have an uppercase letter at second position
	//so if neither is valid, it can't possibly be a UType
	if(!key.starts_with("/")) 
		//&& !(key.starts_with("E") || key.starts_with("A") || key.starts_with("U") || key.starts_with("F") && isupper(key[1])))
	{
		return sol::nil;
	}
	
	UObject* obj = nullptr;
	
	FName name = UnrealLua::StringCache::GetFNameForStringView(key);
	FString str = name.ToString();
	//TSoftObjectPtr<UField>* found = this->ImportsCache.Find(str);
	//if (found != nullptr)
	//{
	//	obj = found->LoadSynchronous();
	//}
	
	if (!obj)
	{
		obj = StaticLoadObject(UObject::StaticClass(), nullptr, *name.ToString());
		if(!obj)
		{
			LUA_LOG_ERROR("Failed to import %hs : Path did not result in a valid object", key.data())
			return sol::nil;
		}
	}
	
	sol::state_view lua{thisState};
	sol::object result{sol::nil};
	if(UClass* uclass = Cast<UClass>(obj))
	{
		
		FString className = uclass->GetFName().ToString();
		
		LUA_LOG("Found UClass to import %hs", key.data());
		if (UUnrealLuaEngineSubsystem::IsGameSessionActive())
		{
			this->ImportsCache.Add(str, uclass);
		}
		
		if(uclass->HasAnyClassFlags(EClassFlags::CLASS_CompiledFromBlueprint))
		{
			className.RemoveFromEnd("_C");
		}
		
		result = sol::make_object<FLuaUClass>(lua, uclass);
		
		if (uclass->IsChildOf<UBlueprintFunctionLibrary>())
		{
			this->RegisterBlueprintLibrary(uclass->GetDefaultObject<UBlueprintFunctionLibrary>(), *FScopedLuaContext::GetLuaContextFromLuaState(thisState));
		}
	}
	else if(UScriptStruct* ss = Cast<UScriptStruct>(obj))
	{
		LUA_LOG("Found UScriptStruct to import %hs", key.data());
		if (UUnrealLuaEngineSubsystem::IsGameSessionActive())
		{
			this->ImportsCache.Add(str, ss);
		}
		
		result = sol::make_object<FLuaUStruct>(lua, ss);
	}
	else if(UEnum* uenum = Cast<UEnum>(obj))
	{
		LUA_LOG("Found UEnum to import %hs", key.data());
		if (UUnrealLuaEngineSubsystem::IsGameSessionActive())
		{
			this->ImportsCache.Add(str, uenum);
		}
		result = UnrealLua::UObjectRegistry::GetEnumWrapperLuaObject(uenum, lua.lua_state());
	}
	if(!result.valid())
	{
		LUA_LOG_ERROR("Failed to import %hs : Path did not result in a valid UClass / UEnum / UScriptStruct object. Only objects of these types can be imported!", key.data())
		return sol::nil;
	}
	else
	{
		sol::object inner = sol::make_object(lua, true);

		std::string objType = UnrealLua::LuaTypes::TypeInfo::UType(result, inner, thisState);
		
		//LUA_LOG("Lua import registry __index returning type %hs for path %hs", objType.c_str(), key.data())
	}
	lua["UE"][key.data()] = result;
	return result;
}

void FLuaImportRegistry::LateRegisterNewModuleAssets(FScopedLuaContext& ctx, const TArray<UClass*>& newUClasses, const TArray<UScriptStruct*>& newScriptStructs, const TArray<UEnum*>& newEnums, const TArray<UBlueprintFunctionLibrary*>& newBlueprintLibraries)
{
	sol::state_view lua = ctx.GetLuaState();
	
	for (UEnum* uenum : newEnums)
	{
		this->RegisterUEnum(uenum, ctx);
	}
	for (UScriptStruct* ss : newScriptStructs)
	{
		this->RegisterUScriptStruct(ss, ctx);
	}
	for (UClass* uclass : newUClasses)
	{
		this->RegisterUClass(uclass, ctx);
	}
	for (UBlueprintFunctionLibrary* lib : newBlueprintLibraries)
	{
		this->RegisterBlueprintLibrary(lib, ctx);
	}
}

sol::object FLuaImportRegistry::__index(sol::object key, sol::this_state thisState)
{
	return this->UImport(key, thisState);
}

void FLuaImportRegistry::__newindex(sol::object key, sol::object value, sol::this_state thisState)
{
	if(key.valid() && key.get_type() == sol::type::string && value.valid())
	{
		//@TODO : Make checks about value type, should only be meta items
		sol::string_view strv = key.as<sol::string_view>();
		FScopedLuaContext* ctx = FScopedLuaContext::GetLuaContextFromLuaState(thisState);
		sol::table registryTable = ctx->GetRegistryTable();
		if(registryTable[key].valid())
		{
			LUA_LOG_ERROR("LuaImportRegistry name conflict: Can not __new_index-import %hs, alias already taken", strv.data());
			return;
		}
		
		bool validType = false;
		if (value.is<FLuaUClass>())
		{
			validType = true;
		}
		else if (value.is<FLuaUStruct>())
		{
			validType = true;
		}
		else if (value.is<FLuaPrimitiveCPPType>())
		{
			validType = true;
		}
		else if (UnrealLua::LightUserdata::IsEnum(value))
		{
			validType = true;
		}
		else if (UnrealLua::LightUserdata::IsUObjectType<UBlueprintFunctionLibrary>(value))
		{
			validType = true;
		}
		if (!validType)
		{
			std::string typeStr = UnrealLua::LuaTypes::TypeInfo::UType(value, true, -1);
			LUA_LOG_ERROR("Invalid type to import to Lua Import registry: %hs", typeStr.data());
			return;			
		}
		registryTable[key] = value;
	}
	else
	{
		LUA_LOG_ERROR("DERP_newindex!")
		checkNoEntry()
	}
}

void FLuaImportRegistry::LoadUClasses(FScopedLuaContext& ctx)
{
	LUA_LOG("Registering UClasses")

	for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
	{
		UClass* uclass = *ClassIt;
		this->RegisterUClass(uclass, ctx);
	}
}

void FLuaImportRegistry::RegisterUClass(UClass* uclass, FScopedLuaContext& ctx)
{
	if (!uclass->IsNative()/* || uclass->IsChildOf<UBlueprintFunctionLibrary>()*/)
	{
		return;
	}

	//hidedropdown, and deprecated.
	if (uclass->HasAnyClassFlags(CLASS_Deprecated | CLASS_NewerVersionExists))
	{
		///NOTE : Did not ignore abstract classes, so we can have things like TArray("T<UActorComponent>")
		return;
	}
			
	FString className = uclass->GetFName().ToString();

	UPackage* package = uclass->GetPackage();
	const bool bIsEditorOnlyPackage = package->HasAllPackagesFlags(PKG_EditorOnly) || uclass->IsEditorOnly();
	const bool bIsUncookedOrDev = package->HasAnyPackageFlags(PKG_UncookedOnly | PKG_Developer);
	if(bIsEditorOnlyPackage)
	{
		return;
	}
	if(bIsUncookedOrDev)
	{
		return;
	}

	UObject* cdo = uclass->GetDefaultObject();
	std::string key = "U";
	if(cdo->IsA(AActor::StaticClass()))
	{
		key = "A";
	}
	else if(uclass->IsChildOf(UInterface::StaticClass()))
	{
		//LUA_LOG("Registering Interface %s", *uclass->GetName())
		key = "I";			
	}
	key += StringCast<char>(*className).Get();
		
	sol::table registryTable = ctx.GetRegistryTable();
	verify(registryTable.valid())
	sol::state_view lua = ctx.GetLuaState();
	
	if(registryTable[key].valid())
	{
		LUA_LOG_ERROR("LuaImportRegistry name conflict: UClass entry %s already taken",*className)
		return;
	}
	//LUA_LOG_ERROR("Registering UClass %hs",key.c_str())
	lua["UE"][key] = uclass;
}


void FLuaImportRegistry::LoadUScriptStructs(FScopedLuaContext& ctx)
{
	LUA_LOG("Registering UScriptStructs")
	
	sol::state_view lua = ctx.GetLuaState();

	//Note : FString, FText and FName are not registered here, because they
	//don't have any UScriptStructs, instead they are registered as primitives
	//in FLuaPrimitives::RegisterPrimitiveWrappers
	
	for (TObjectIterator<UScriptStruct> scriptStructIt; scriptStructIt; ++scriptStructIt)
	{
		UScriptStruct* scriptStruct = *scriptStructIt;
		this->RegisterUScriptStruct(scriptStruct, ctx);
	}

	verify(lua["UE"]["FVector"].valid());
	verify(lua["UE"]["FRotator"].valid());
	verify(lua["UE"]["FTransform"].valid());
}

void FLuaImportRegistry::RegisterUScriptStruct(UScriptStruct* scriptStruct, FScopedLuaContext& ctx)
{
	if (!scriptStruct->IsNative())
	{
		return;
	}

	if(scriptStruct == UnrealLua::StaticPackages::InstancedStruct || scriptStruct == UnrealLua::StaticPackages::SharedStruct)
	{
		return;
	}
			
	FName className = scriptStruct->GetFName();
	verify(className != NAME_None)
	FString classNameStr = className.ToString();
	verify(!classNameStr.IsEmpty())
			
	UPackage* package = scriptStruct->GetPackage();
	const bool bIsEditorOnlyPackage = package->HasAllPackagesFlags(PKG_EditorOnly) || scriptStruct->IsEditorOnly();
	const bool bIsUncookedOrDev = package->HasAnyPackageFlags(PKG_UncookedOnly | PKG_Developer);
	if(bIsEditorOnlyPackage)
	{
		return;
	}
	if(bIsUncookedOrDev)
	{
		return;
	}
	
	sol::table registryTable = ctx.GetRegistryTable();
	verify(registryTable.valid())
	sol::state_view lua = ctx.GetLuaState();
		
	std::string key = "F";
	key += StringCast<char>(*classNameStr).Get();
	if(registryTable[key].valid())
	{
		LUA_LOG_ERROR("LuaImportRegistry name conflict: UScriptStruct entry %s already taken",*classNameStr)
		return;
	}
	lua["UE"][key] = scriptStruct;
}

void FLuaImportRegistry::LoadEnums(FScopedLuaContext& ctx)
{
	LUA_LOG("Registering native enums")

	for (TObjectIterator<UEnum> ClassIt; ClassIt; ++ClassIt)
	{
		UEnum* uenum = *ClassIt;
		this->RegisterUEnum(uenum, ctx);
	}
}

void FLuaImportRegistry::RegisterUEnum(UEnum* uenum, FScopedLuaContext& ctx)
{
	if (!uenum->IsNative())
	{
		return;
	}

	// Ignore abstract, hidedropdown, and deprecated.
	if (uenum->HasAnyEnumFlags(EEnumFlags::NewerVersionExists))
	{
		return;
	}
		
	FString enumName = uenum->GetFName().ToString();

	UPackage* package = uenum->GetPackage();
	const bool bIsEditorOnlyPackage = package->HasAllPackagesFlags(PKG_EditorOnly) || uenum->IsEditorOnly();
	const bool bIsUncookedOrDev = package->HasAnyPackageFlags(PKG_UncookedOnly | PKG_Developer);
	if(bIsEditorOnlyPackage)
	{
		return;
	}
	if(bIsUncookedOrDev)
	{
		return;
	}
	
	sol::table registryTable = ctx.GetRegistryTable();
	verify(registryTable.valid())
	sol::state_view lua = ctx.GetLuaState();
	
	std::string key = StringCast<char>(*enumName).Get();
		
	if(registryTable[key].valid())
	{
		LUA_LOG_ERROR("LuaImportRegistry name conflict: UEnum entry %s already taken",*enumName)
		return;
	}
		

	
	//LUA_LOG("Registering UEnum %s", *enumName);

	sol::object created = UnrealLua::UObjectRegistry::GetEnumWrapperLuaObject(uenum, registryTable.lua_state());
	verify(UnrealLua::IsEnum(created));
		
	{
		UEnum* createduenum = UnrealLua::LightUserdata::GetUEnum(created);
		verify(createduenum == uenum);
	}
	
	verify(lua["UE"].get_or<FLuaImportRegistry*>(nullptr) == this);
	verify(lua.globals().raw_get_or<FLuaImportRegistry*>("UE", nullptr) == nullptr)
	
	lua["UE"][key] = created;

	{
		sol::object added = lua["UE"][key];
		verify(UnrealLua::IsEnum(added));
		UEnum* addeduenum = UnrealLua::LightUserdata::GetUEnum(created);
		verify(addeduenum == uenum);
	}
}

void FLuaImportRegistry::LoadBlueprintLibraries(FScopedLuaContext& ctx)
{
	LUA_LOG("Registering Blueprint Libraries in Lua state")

	TArray<UClass*> Libraries;
	GetDerivedClasses(UBlueprintFunctionLibrary::StaticClass(), Libraries);

	for(const UClass* libraryClass : Libraries)
	{
		// Ignore abstract libraries/classes
		if (!libraryClass || libraryClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
		{
			continue;;
		}
		if (!libraryClass->IsNative())
		{
			continue;
		}
		if(libraryClass->IsEditorOnly())
		{
			continue;
		}
		UBlueprintFunctionLibrary* libObj = libraryClass->GetDefaultObject<UBlueprintFunctionLibrary>();
		this->RegisterBlueprintLibrary(libObj, ctx);
	}
}

void FLuaImportRegistry::RegisterBlueprintLibrary(UBlueprintFunctionLibrary* lib, FScopedLuaContext& ctx)
{
	UClass* libraryClass = lib->GetClass();
	FString className = libraryClass->GetName();// + libraryClass->GetPrefixCPP();
	/*
	if(FLuaConfic.bTrimBlueprintLibraryNames)
	{
			className.RemoveFromStart("Blueprint");
			className.RemoveFromEnd("Library");
			className.RemoveFromEnd("Function");
			className.RemoveFromEnd("Blueprint");
			if(className.IsEmpty())
			{
				LUA_LOG_WARNING("Can not register blueprint library %s because its name is not very distinct", *libraryClass->GetName())
				return;
			}
		}
	*/
	sol::table registryTable = ctx.GetRegistryTable();
	verify(registryTable.valid())
	sol::state_view lua = ctx.GetLuaState();

	std::string key = StringCast<char>(*className).Get();

	sol::object existing = registryTable[key]; 
	if(existing.valid())
	{
		//std::string other = lua["tostring"](existing).get<std::string>();
		LUA_LOG_ERROR("LuaImportRegistry name conflict: Blueprint Function Library entry %s already taken",*className)
		return;
	}
	registryTable[key] = lib;
}
