#pragma once
#include "CoreMinimal.h"
#include "ScriptableUObject/LuaScriptSettings.h"
#include "Engine/GameInstance.h"
#include "Interface/LuaContext.h"
#include "Interface/LuaScriptable.h"

#include "LuaGameInstance.generated.h"

enum class ELuaLoadEventType : uint8;
class UGameLuaContext;
enum class ELuaScriptType : uint8;
class ULuaContext;
class ULuaGameInstance;

enum ENet_NMT_GameSpecific
{
	Invalid,
	LuaGameModeString,
	AckKeyword,
	AckFName
};

UCLASS(Blueprintable, BlueprintType)
class UNREALLUA_API ULuaGameInstance : public UGameInstance, public ILuaScriptable
{
public:
	GENERATED_BODY()

	virtual void Init() override;

	void NotifyLuaLoadEvent(TScriptInterface<ILuaContext> luaContext, FName name, ELuaLoadEventType loadEvent);
	virtual void PostLuaLoadFinish() {};
	void LoadLuaScriptInternal();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLuaScriptSettings LuaScriptSettings;

	virtual void ModifyClientTravelLevelURL(FString& LevelName) override;
	
	virtual FLuaScriptSettings GetLuaScriptSettings_Implementation() override;

	virtual void SetLuaScriptSettings_Implementation(FLuaScriptSettings newSettings) override;

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UGameLuaContext> LuaContext;
};