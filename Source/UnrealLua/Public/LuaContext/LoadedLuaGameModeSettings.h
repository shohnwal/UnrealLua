#pragma once
#include "CoreMinimal.h"
#include "LoadedLuaGameModeSettings.generated.h"

struct FModInfo;

UENUM(BlueprintType)
enum class ELoadResultType : uint8
{
	//Lua game mode loading successful
	SUCCESS 	UMETA(DisplayName="Success"),
	//Lua game mode loading failed
	FAILED		UMETA(DisplayName="Failed"),
};

USTRUCT(BlueprintType) 
struct UNREALLUA_API FLoadedLuaGameModeSettings
{
	GENERATED_BODY()
public:	
	FLoadedLuaGameModeSettings() : UsedMods(), CurrentGameMode(NAME_None), GameModeLoadErrorMessages(), bIsLuaLoaded(false)
	{}
	
	FLoadedLuaGameModeSettings(const TArray<FString>& usedMods, const FName& gameType);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FString> UsedMods;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName CurrentGameMode;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FString> GameModeLoadErrorMessages;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bIsLuaLoaded = false;

	void Reset(const FName& gameMode, const TArray<FString>& modFolders);

	void Append(const FLoadedLuaGameModeSettings& other);
};