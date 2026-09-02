#pragma once
#include "Delegates/Delegate.h"
#include "Delegates/DelegateCombinations.h"

class IFileManager;
DECLARE_MULTICAST_DELEGATE_OneParam(FBooleanMulticastDelegate, bool);

namespace UnrealLua::MetaCache
{
	void UNREALLUA_API CreateClassMetaDatabase(bool bForce = false);
	void UNREALLUA_API CreateUClassMetaData(UClass* uclass, const FString& metaLuaDirectory, IFileManager& files, bool bClearCache);
	void UNREALLUA_API CreateUScriptStructMetaData(UScriptStruct* strct, const FString& metaLuaDirectory, IFileManager& files, bool bClearCache);
	void UNREALLUA_API CreateUEnumMetaData(UEnum* uenum, const FString& metaLuaDirectory, IFileManager& files, bool bClearCache);

	extern UNREALLUA_API FBooleanMulticastDelegate OnProcessUpdate;

	FString UNREALLUA_API GetLuaMetaFolderDir();

	extern const char* LuaMetaFolderSuffix;

	extern const char* FUObjectPropertyWrapperAdditions;
	extern const char* PrimitivesDef;
	extern const char* FDelegatesDef;
	extern const char* TArrayDef;
	extern const char* TSetDef;
	extern const char* TMapDef;
	extern const char* StructUtilsDef;
	extern const char* TSubclassOfDef;
	extern const char* TWeakObjectPtrDef;
	extern const char* TScriptInterfaceDef;
	extern const char* GlobalFuncs;
}