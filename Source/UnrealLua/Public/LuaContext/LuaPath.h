#pragma once
#include "CoreMinimal.h"
#include <string>
#include "LuaPath.generated.h"

class UUnrealLuaMod;


enum class ELuaPathFlags : uint8
{
	None			= 0,      // No location
	BaseGame		= 1 << 0, // Located in base game Content folder
	Mod				= 1 << 1, // Located in a mod folder
	Root			= 1 << 2, // /Lua/ in either Content folder or Mod folder
	Lib				= 1 << 3, // /Lua/Lib/ in either Content folder or Mod folder
	DefaultGameMode	= 1 << 4, // /Lua/GameMode/Default/ in either Content folder or Mod folder
	NamedGameMode	= 1 << 5, // /Lua/GameMode/<GameModeName>/ in either Content folder or Mod folder
	UnrealTypes		= 1 << 6, // /Lua/UnrealTypes/ in either Content folder or Mod folder
	AnyExceptUTypes	= UINT8_MAX - (1 << 6), // /Lua/UnrealTypes/ in either Content folder or Mod folder
	BaseGameRoot	= BaseGame | Root,
	ModRoot			= Mod | Root,
	Any				= UINT8_MAX // All locations
};
ENUM_CLASS_FLAGS(ELuaPathFlags)

USTRUCT()
struct UNREALLUA_API FLuaPathElement
{
	GENERATED_BODY()
	
	FLuaPathElement();
	FLuaPathElement(const FString& pathStr, ELuaPathFlags flags);
	void Set(const FString& str);
	
	const FString& GetFString() const
	{
		return Path;
	}
	const std::string& GetStdString() const
	{
		return path;
	}

	bool HasAnyFlags(ELuaPathFlags flags) const;

private:
	UPROPERTY(VisibleAnywhere, meta=(AllowPrivateAccess=true))
	FString Path = "";
	std::string path = "";
	ELuaPathFlags Locationflags = ELuaPathFlags::None;
};


USTRUCT()
struct UNREALLUA_API FLuaPath
{
	GENERATED_BODY()
	FLuaPath();
	FLuaPath(const TArray<UUnrealLuaMod*>& mods, const FName& gamemode, const ELuaPathFlags allowedPathLocation);
	void SetPackagePath(const FString& path);
	void SetPackagePath(const TArray<FString>& Path);

	void SetupPackagePaths(const TArray<UUnrealLuaMod*>& mods, const FName& gamemode, const ELuaPathFlags allowedPathLocation = ELuaPathFlags::Any);
	
	void AddPath(const FString& path, const ELuaPathFlags pathFlags = ELuaPathFlags::None);
	void AddPath(const FLuaPathElement& path);

	TArray<FLuaPathElement>::RangedForIteratorType begin() { return Paths.begin(); }
	TArray<FLuaPathElement>::RangedForConstIteratorType begin() const { return Paths.begin(); }
	TArray<FLuaPathElement>::RangedForIteratorType end() { return Paths.end(); }
	TArray<FLuaPathElement>::RangedForConstIteratorType end() const { return Paths.end(); }

private:
	//Paths are full system directory paths (i.e. C:/MyGame/Content/Lua/ or /media/user/MyDrive/MyGame/Content/Lua)
	UPROPERTY(VisibleAnywhere, meta=(AllowPrivateAccess=true))
	TArray<FLuaPathElement> Paths;
	
	ELuaPathFlags UsedFlags = ELuaPathFlags::None;
};