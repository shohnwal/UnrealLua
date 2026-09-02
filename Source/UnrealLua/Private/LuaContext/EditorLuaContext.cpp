#include "LuaContext/EditorLuaContext.h"
//#if WITH_EDITOR
//#include "LuaContext/EditorLuaContext.h"
/*
#include "Editor.h"
#include "HotReload/Public/IHotReload.h"
#include "PropertyEditorDelegates.h"
#include "Kismet2/KismetEditorUtilities.h"

#include <bitset>
*/
//#include "Editor/UnrealEd/Public/Kismet2/BlueprintEditorUtils.h"
//#include "Editor/UnrealEd/Public/Kismet2/KismetEditorUtilities.h"
//#include "Editor/UnrealEd/Public/Kismet2/KismetReinstanceUtilities.h"

/*
void UEditorLuaContext::ResetGameMode(const TArray<FString>& modFolders, const FName& gameMode)
{
	this->LoadedGameModeSettings.Reset(gameMode, modFolders);
	this->ResetGameModeInternal(modFolders, gameMode);
}
*/
/*
bool UEditorLuaContext::ShouldCreateSubsystem(UObject* Outer) const
{
	return false;
}

void UEditorLuaContext::Initialize(FSubsystemCollectionBase& Collection)
{
	LUA_LOG("Initializing EditorLuaContext %s", *GetFullNameSafe(this))
	Super::Initialize(Collection);
	verifyf(false, TEXT("This won't work, it would require a game session to be active in the editor..."));
	checkNoEntry();
	this->GetScopedLuaContext().InitializeLuaState(ELuaContextType::Editor);
	this->LoadGameMode("Editor");
}

void UEditorLuaContext::Deinitialize()
{
	LUA_LOG("Destroying ULuaContext %s", *GetFullNameSafe(this))
	checkSlow(IsInGameThread());
	this->LuaContext = nullptr;
	Super::Deinitialize();
}

FScopedLuaContext& UEditorLuaContext::GetScopedLuaContext()
{
	if(!this->LuaContext.IsValid())
	{
		this->LuaContext = MakeShared<FScopedLuaContext>();
	}
	verify(this->LuaContext.IsValid())
	return *this->LuaContext.Get();
}

void UEditorLuaContext::LoadGameMode(const FName& name)
{
	if(!this->LuaContext.IsValid())
	{
		this->LuaContext = MakeShared<FScopedLuaContext>();
	}
	this->SetupLuaGameMode({}, name);
}

void UEditorLuaContext::BroadcastLoadEvent(TScriptInterface<ILuaContext> ictx, FName gameModeName, ELuaLoadEventType loadState)
{
	this->OnLuaGameModeReloadEventNative.Broadcast(this, gameModeName, loadState);
}
#endif


void ULuaContext::OnbjectReplaced(const TMap<UObject*, UObject*>& ReplacedObjects)
{

	for(auto& pair : ReplacedObjects)
	{
		if(ULuaContext* oldCtx = Cast<ULuaContext>(pair.Key))
		{
			LUA_LOG("Replacing ULuaContext")
			ULuaContext* newCtx = CastChecked<ULuaContext>(pair.Value);
			newCtx->LuaContext = oldCtx->LuaContext;
		}
	}

}
*/
