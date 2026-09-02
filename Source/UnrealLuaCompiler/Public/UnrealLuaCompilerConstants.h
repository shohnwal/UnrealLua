#pragma once
#include "UObject/NameTypes.h"

namespace UnrealLua::CompilerConstants
{
	static const FName NAME_ExposeOnSpawn(TEXT("ExposeOnSpawn"));
	static const FName NAME_EditFixedSize(TEXT("EditFixedSize"));
	static const FName NAME_DisplayName(TEXT("DisplayName"));
	static const FName NAME_NoBlueprintsOfChildren(TEXT("NoBlueprintsOfChildren"));
	static const FName NAME_Evt_ScriptName(TEXT("ScriptName"));
	static const FName NAME_AllowPrivateAccess(TEXT("AllowPrivateAccess"));
	static const FName NAME_Meta_EditorOnly(TEXT("EditorOnly"));

	static const FName NAME_Class_DefaultConfig(TEXT("DefaultConfig"));
	static const FName NAME_Actor_DefaultComponent(TEXT("DefaultComponent"));
	static const FName NAME_Actor_OverrideComponent(TEXT("OverrideComponent"));
	static const FName NAME_Actor_BindComponent(TEXT("BindComponent"));
	static const FName NAME_Actor_RootComponent(TEXT("RootComponent"));
	static const FName NAME_Actor_Attach(TEXT("Attach"));
	static const FName NAME_Actor_AttachSocket(TEXT("AttachSocket"));
	static const FName NAME_AnyStructRef(TEXT("__ANY_STRUCT_REF"));
	static const FName NAME_Function_MixinArgument(TEXT("MixinArgument"));
	static const FName NAME_Function_DefaultToSelf(TEXT("DefaultToSelf"));

	const static FName FUNCMETA_BlueprintThreadSafe("BlueprintThreadSafe");
	const static FName FUNCMETA_NotBlueprintThreadSafe("NotBlueprintThreadSafe");
	const static FName FUNCMETA_BlueprintProtected("BlueprintProtected");
	const static FName FUNCMETA_CrumbFunction("CrumbFunction");
	const static FName FUNCMETA_ScriptNoOp("ScriptNoOp");
	
	const static TCHAR* PROPNAME_StaticSharedStorage(TEXT("StaticShared"));
	
	const static TCHAR* PROPFLAG_Static(TEXT("Static"));
	const static TCHAR* PROPFLAG_BlueprintAssignable(TEXT("BlueprintAssignable"));
	
	const static TCHAR* PROPFLAG_BlueprintReadOnly(TEXT("BlueprintReadOnly"));
	const static TCHAR* PROPFLAG_BlueprintReadWrite(TEXT("BlueprintReadWrite"));
	const static TCHAR* PROPFLAG_VisibleAnywhere(TEXT("VisibleAnywhere"));
	const static TCHAR* PROPFLAG_EditAnywhere(TEXT("EditAnywhere"));
	const static TCHAR* PROPFLAG_VisibleDefaultsOnly(TEXT("BlueprintReadWrite"));
	const static TCHAR* PROPFLAG_EditDefaultsOnly(TEXT("EditDefaultsOnly"));
	const static TCHAR* PROPFLAG_VisibleInstanceOnly(TEXT("VisibleInstanceOnly"));
	const static TCHAR* PROPFLAG_EditInstanceOnly(TEXT("EditInstanceOnly"));
	const static TCHAR* PROPFLAG_Replicated(TEXT("Replicated"));
	const static TCHAR* PROPFLAG_ReplicatedUsing(TEXT("ReplicatedUsing"));
	
	const static TCHAR* FUNCPROPFLAG_ByRef(TEXT("Ref"));
	
	const static TCHAR* FUNCFLAG_Static(TEXT("Static"));
	const static TCHAR* FUNCFLAG_BlueprintCallable(TEXT("BlueprintCallable"));
	const static TCHAR* FUNCFLAG_NotBlueprintCallable(TEXT("NotBlueprintCallable"));
	const static TCHAR* FUNCFLAG_ClientRPC(TEXT("Client"));
	const static TCHAR* FUNCFLAG_ServerRPC(TEXT("Server"));
	const static TCHAR* FUNCFLAG_MulticastRPC(TEXT("Multicast"));
	const static TCHAR* FUNCFLAG_ReliableRPC(TEXT("Reliable"));
}