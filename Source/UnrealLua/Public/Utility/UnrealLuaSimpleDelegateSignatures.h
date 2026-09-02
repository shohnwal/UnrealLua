#pragma once
#include "CoreMinimal.h"
#include "Input/Reply.h"
#include "UnrealLuaSimpleDelegateSignatures.generated.h"

class AActor;

UDELEGATE()
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FActorMulticastDelegate, AActor*, actor);
DECLARE_DYNAMIC_DELEGATE_OneParam(FActorDelegate, AActor*, actor);
DECLARE_MULTICAST_DELEGATE_OneParam(FObjectMulticastDelegate, UObject*);
DECLARE_DELEGATE_OneParam( FSimpleActorNativeDelegate, AActor*);
DECLARE_DELEGATE_OneParam( FSimpleUObjectNativeDelegate, UObject*);
DECLARE_DELEGATE_OneParam( FSimpleClassArrayNativeDelegate, TArray<UClass*>& );
DECLARE_DELEGATE_OneParam( FSimpleStringDelegate, FString );
DECLARE_DELEGATE_TwoParams( FDoubleStringDelegate, FString, FString );
DECLARE_DELEGATE_OneParam( FSimpleBoolDelegate, bool );
DECLARE_DELEGATE_OneParam(FStructDelegate, UStruct*);
DECLARE_DELEGATE_TwoParams(FObjectStringDelegate, UObject*, FString );
DECLARE_DELEGATE_OneParam(FSimpleWidgetDelegate, TSharedRef<SWidget>);
DECLARE_DELEGATE_RetVal_OneParam( FReply, FActorDelegate_RetFReply,	AActor*);