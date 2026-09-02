#include "LuaContext/LoadedLuaGameModeSettings.h"

FLoadedLuaGameModeSettings::FLoadedLuaGameModeSettings(const TArray<FString>& usedMods, const FName& gameType)
	: UsedMods(usedMods), CurrentGameMode(gameType), GameModeLoadErrorMessages(), bIsLuaLoaded(false)
{
	
}

void FLoadedLuaGameModeSettings::Reset(const FName& gameMode, const TArray<FString>& mods)
{
	this->CurrentGameMode = gameMode;
	this->UsedMods = mods;
	this->GameModeLoadErrorMessages.Empty();
}

void FLoadedLuaGameModeSettings::Append(const FLoadedLuaGameModeSettings& other)
{
	this->GameModeLoadErrorMessages.Append(other.GameModeLoadErrorMessages);
}
