#pragma once

#include "CoreMinimal.h"
#include "sol/forward.hpp"
#include "LuaScriptInstanceHandle.generated.h"

enum class ELuaScriptReloadStage;
class ILuaContext;
struct FSetLuaScriptUObjectMemberPropertyWrapperParams;
struct FLuaRepLayout;
struct FLuaOverrideCallParams;
struct FLuaScriptInstance;
class ULoadedLuaScriptCollection;
class ULuaComponent;
struct FLuaScriptReloadCache;
struct FLuaUObjectItem;

USTRUCT(BlueprintType)
struct UNREALLUA_API FLuaScriptInstanceHandle
{
	GENERATED_BODY()

	FLuaScriptInstanceHandle();
	~FLuaScriptInstanceHandle();
	
	//bool ProcessEvent(FLuaOverrideCallParams& params) const;

	void Reset();
	bool IsValid() const;
	UObject* GetScriptOwner() const;
	void SetOwner(UObject* Object);
	void NotifyLuaScriptReload(ELuaScriptReloadStage luaScriptReloadStage, FLuaScriptReloadCache* luaScriptReloadCache);

	FLuaScriptInstanceHandle(const FLuaScriptInstanceHandle& other);
	FLuaScriptInstanceHandle(FLuaScriptInstanceHandle&& instance) noexcept;
	explicit FLuaScriptInstanceHandle(ULoadedLuaScriptCollection* coll) noexcept;
	
	FLuaRepLayout* GetRepLayout() const;

	FLuaScriptInstanceHandle& operator=(const FLuaScriptInstanceHandle& other);
	FLuaScriptInstanceHandle& operator=(FLuaScriptInstanceHandle&& other) noexcept;

	bool CanReplicate() const;

	sol::state_view GetLuaStateView() const;
	sol::this_state GetLuaThisState() const;
	ULoadedLuaScriptCollection* GetLuaScriptCollection() const;
	
	TMap<FString, FString> LuaScriptToString() const;

	void InitializeLuaReplication() const;
	const FDelegateHandle& GetLuaScriptReloadDelegateHandle() const;
	void SetLuaScriptReloadDelegateHandle(const FDelegateHandle newHandle);

private:
	void InitRepLayout(FLuaUObjectItem& Item, const FLuaRepLayout& Replayout) const;

	TWeakObjectPtr<ULoadedLuaScriptCollection> LuaScriptCollection = nullptr;
	TWeakObjectPtr<UObject> Owner = nullptr;
	FDelegateHandle LuaScriptReloadHandle = {};

	static FLuaScriptInstanceHandle& Invalid();
};

template<>
struct TStructOpsTypeTraits<FLuaScriptInstanceHandle> : public TStructOpsTypeTraitsBase2<FLuaScriptInstanceHandle>
{
	enum
	{
		WithZeroConstructor            = true,                         // struct can be constructed as a valid object by filling its memory footprint with zeroes.
	};
};
