#pragma once
#include "CoreMinimal.h"
#include "StructUtils/SharedStruct.h"
#include "WeakSharedStruct.generated.h"

USTRUCT()
struct UNREALLUA_API FWeakSharedStruct
{
	GENERATED_BODY()

	FWeakSharedStruct(): StructMemoryPtr(nullptr)
	{
		
	}

	explicit FWeakSharedStruct(FSharedStruct& target)
	{
		if(target.IsValid())
		{
			StructMemoryPtr = *reinterpret_cast<TSharedPtr<FStructSharedMemory>*>(&target);
		}
		else
		{
			StructMemoryPtr = nullptr;
		}
	}

	bool IsValid() const { return StructMemoryPtr.IsValid(); }

	FSharedStruct ToSharedStruct();

	/** Returns struct type. */
	const UScriptStruct* GetScriptStruct() const
	{
		return StructMemoryPtr.IsValid() ? &(StructMemoryPtr.Pin().Get()->GetScriptStruct()) : nullptr;
	}

	TObjectPtr<const UScriptStruct>* const GetScriptStructPtr() const
	{
		return StructMemoryPtr.IsValid() ? &StructMemoryPtr.Pin().Get()->GetScriptStructPtr() : nullptr;
	}

	/** Returns a mutable pointer to struct memory. */
	uint8* GetMemory() const
	{
		return StructMemoryPtr.IsValid() ? StructMemoryPtr.Pin().Get()->GetMutableMemory() : nullptr;
	}

	bool operator==(const FWeakSharedStruct& other) const
	{
		return this->GetScriptStruct() == other.GetScriptStruct() && this->GetMemory() == other.GetMemory(); 
	}
	
	bool operator==(const FSharedStruct& other) const
	{
		return this->GetScriptStruct() == other.GetScriptStruct() && this->GetMemory() == other.GetMemory(); 
	}

	bool operator!=(const FWeakSharedStruct& other) const
	{
		return !operator==(other);
	}

	bool operator!=(const FSharedStruct& other) const
	{	
		return !operator==(other);
	}
	
	/** Returns reference to the struct, this getter assumes that all data is valid. */
	template<typename T>
	T& Get() const
	{
		return UE::StructUtils::GetStructRef<T>(GetScriptStruct(), GetMemory());
	}

	/** Returns pointer to the struct, or nullptr if cast is not valid. */
	template<typename T>
	T* GetPtr() const
	{
		return UE::StructUtils::GetStructPtr<T>(GetScriptStruct(), GetMemory());
	}
	
	TWeakPtr<FStructSharedMemory> StructMemoryPtr;
};

inline FSharedStruct FWeakSharedStruct::ToSharedStruct()
{
	FSharedStruct target{};
	if(this->IsValid())
	{
		TSharedPtr<FStructSharedMemory>* targetStructMemoryPtr = reinterpret_cast<TSharedPtr<FStructSharedMemory>*>(&target);
		*targetStructMemoryPtr = this->StructMemoryPtr.Pin();		
	}
	return target;
}
