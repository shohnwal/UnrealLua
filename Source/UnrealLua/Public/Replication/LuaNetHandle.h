#pragma once
#include "CoreMinimal.h"
#include "Misc/Crc.h"
#include "UObject/StructOpsTypeTraits.h"
#include "LuaNetHandle.generated.h"

USTRUCT(BlueprintType)
struct UNREALLUA_API FLuaNetHandle
{
	GENERATED_BODY()

	FLuaNetHandle()
		: HandleValue(0)
	{
		
	}
	
	explicit FLuaNetHandle(UObject* obj);

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	int32 HandleValue;
	bool IsValid() const
	{
		return HandleValue != 0;
	}

	bool operator==(const FLuaNetHandle& other) const
	{
		return this->HandleValue == other.HandleValue;
	}
	
	static FORCEINLINE uint32 GetKeyHash(const FLuaNetHandle& This)
	{
		return FCrc::TypeCrc32(This.HandleValue);
	}
	
	static FORCEINLINE uint32 GetTypeHash(const FLuaNetHandle& This)
	{
		return FCrc::TypeCrc32(This.HandleValue);
	}

	bool NetSerialize(FArchive& ar, UPackageMap* map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits<FLuaNetHandle> : public TStructOpsTypeTraitsBase2<FLuaNetHandle>
{
	enum
	{
		WithNetSerializer = true,
	};
};

FORCEINLINE uint32 GetTypeHash(const FLuaNetHandle& This)
{
	return FCrc::TypeCrc32(This.HandleValue);
}

USTRUCT()
struct UNREALLUA_API FRegisteredLuaNetObjectInfo
{
	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere)
	TWeakObjectPtr<UObject> RegisteredObject = nullptr;
	UPROPERTY(VisibleAnywhere, meta=(ShowOnlyInnerProperties))
	FLuaNetHandle LuaNetHandle = {};
	bool operator==(const FRegisteredLuaNetObjectInfo& other) const
	{
		return RegisteredObject == other.RegisteredObject;
	}

	bool NetSerialize(FArchive& ar, UPackageMap* map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits<FRegisteredLuaNetObjectInfo> : public TStructOpsTypeTraitsBase2<FRegisteredLuaNetObjectInfo>
{
	enum
	{
		WithNetSerializer = true,
	};
};