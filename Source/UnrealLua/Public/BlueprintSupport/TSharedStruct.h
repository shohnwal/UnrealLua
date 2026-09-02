#pragma once
#include "CoreMinimal.h"
#include "SharedStruct.h"
#if 0
#include "TSharedStruct.generated.h"



struct UNREALLUA_API FBlueprintSharedStruct : public FSharedStruct
{
	GENERATED_BODY()
};

/**
 * TInstancedStruct is a type-safe FInstancedStruct wrapper against the given BaseStruct type.
 * @note When used as a property, this automatically defines the BaseStruct property meta-data.
 * 
 * Example:
 *
 *	UPROPERTY(EditAnywhere, Category = Foo)
 *	TInstancedStruct<FTestStructBase> Test;
 *
 *	UPROPERTY(EditAnywhere, Category = Foo)
 *	TArray<TInstancedStruct<FTestStructBase>> TestArray;
 */
template<typename BaseStructT>
struct TSharedStruct
{
public:
	TSharedStruct() = default;

	template<typename T = BaseStructT, typename = std::enable_if_t<std::is_base_of_v<BaseStructT, std::decay_t<T>>>>
	TSharedStruct(const TSharedStruct<T>& InOther)
		: SharedStruct(InOther.InstancedStruct)
	{
	}

	template<typename T = BaseStructT, typename = std::enable_if_t<std::is_base_of_v<BaseStructT, std::decay_t<T>>>>
	TSharedStruct(TSharedStruct<T>&& InOther)
		: SharedStruct(MoveTemp(InOther.InstancedStruct))
	{
	}

	template<typename T = BaseStructT, typename = std::enable_if_t<std::is_base_of_v<BaseStructT, std::decay_t<T>>>>
	TSharedStruct& operator=(const TSharedStruct<T>& InOther)
	{
		if (this != &InOther)
		{
			SharedStruct = InOther.InstancedStruct;
		}
		return *this;
	}

	template<typename T = BaseStructT, typename = std::enable_if_t<std::is_base_of_v<BaseStructT, std::decay_t<T>>>>
	TSharedStruct& operator=(TSharedStruct<T>&& InOther)
	{
		if (this != &InOther)
		{
			SharedStruct = MoveTemp(InOther.InstancedStruct);
		}
		return *this;
	}

	/** Initializes from a raw struct type and optional data. */
	void InitializeAsScriptStruct(const UScriptStruct* InScriptStruct, const uint8* InStructMemory = nullptr)
	{
		checkf(InScriptStruct->IsChildOf(TBaseStructure<BaseStructT>::Get()), TEXT("ScriptStruct must be a child of BaseStruct!"));
		SharedStruct.InitializeAs(InScriptStruct, InStructMemory);
	}

	/** Initializes from struct type and emplace construct. */
	template<typename T = BaseStructT, typename... TArgs, typename = std::enable_if_t<std::is_base_of_v<BaseStructT, std::decay_t<T>>>>
	void InitializeAs(TArgs&&... InArgs)
	{
		SharedStruct.InitializeAs<T>(Forward<TArgs>(InArgs)...);
	}

	/** Creates a new TSharedStruct from templated struct type. */
	template<typename T = BaseStructT, typename = std::enable_if_t<std::is_base_of_v<BaseStructT, std::decay_t<T>>>>
	static TSharedStruct Make()
	{
		TSharedStruct This;
		This.SharedStruct.InitializeAs(TBaseStructure<T>::Get(), nullptr);
		return This;
	}

	/** Creates a new TSharedStruct from templated struct. */
	template<typename T = BaseStructT, typename = std::enable_if_t<std::is_base_of_v<BaseStructT, std::decay_t<T>>>>
	static TSharedStruct Make(const T& Struct)
	{
		TSharedStruct This;
		This.SharedStruct.InitializeAs(TBaseStructure<T>::Get(), reinterpret_cast<const uint8*>(&Struct));
		return This;
	}

	/** Creates a new TSharedStruct from the templated type and forward all arguments to constructor. */
	template<typename T = BaseStructT, typename... TArgs, typename = std::enable_if_t<std::is_base_of_v<BaseStructT, std::decay_t<T>>>>
	static TSharedStruct Make(TArgs&&... InArgs)
	{
		TSharedStruct This;
		This.SharedStruct.template InitializeAs<T>(Forward<TArgs>(InArgs)...);
		return This;
	}

	/** Returns struct type. */
	const UScriptStruct* GetScriptStruct() const
	{
		return SharedStruct.GetScriptStruct();
	}

	/** Returns const pointer to raw struct memory. */
	const uint8* GetMemory() const
	{
		return SharedStruct.GetMemory();
	}

	/** Reset to empty. */
	void Reset()
	{
		SharedStruct.Reset();
	}

	/** Returns const reference to the struct, this getter assumes that all data is valid. */
	template<typename T = BaseStructT, typename = std::enable_if_t<std::is_base_of_v<BaseStructT, std::decay_t<T>>>>
	const T& Get() const
	{
		return SharedStruct.Get<T>();
	}

	/** Returns const pointer to the struct, or nullptr if cast is not valid. */
	template<typename T = BaseStructT, typename = std::enable_if_t<std::is_base_of_v<BaseStructT, std::decay_t<T>>>>
	const T* GetPtr() const
	{
		return SharedStruct.GetPtr<T>();
	}

	/** Returns a mutable pointer to raw struct memory. */
	uint8* GetMutableMemory()
	{
		return SharedStruct.GetMemory();
	}

	/** Returns mutable reference to the struct, this getter assumes that all data is valid. */
	template<typename T = BaseStructT, typename = std::enable_if_t<std::is_base_of_v<BaseStructT, std::decay_t<T>>>>
	T& GetMutable()
	{
		return SharedStruct.Get<T>();
	}

	/** Returns mutable pointer to the struct, or nullptr if cast is not valid. */
	template<typename T = BaseStructT, typename = std::enable_if_t<std::is_base_of_v<BaseStructT, std::decay_t<T>>>>
	T* GetMutablePtr()
	{
		return SharedStruct.GetPtr<T>();
	}

	/** Returns True if the struct is valid.*/
	bool IsValid() const
	{
		return SharedStruct.IsValid();
	}

	/** Comparison operators. Deep compares the struct instance when identical. */
	template<typename T = BaseStructT, typename = std::enable_if_t<std::is_base_of_v<BaseStructT, std::decay_t<T>>>>
	bool operator==(const TSharedStruct<T>& Other) const
	{
		return SharedStruct == Other.InstancedStruct;
	}

	template<typename T = BaseStructT, typename = std::enable_if_t<std::is_base_of_v<BaseStructT, std::decay_t<T>>>>
	bool operator!=(const TSharedStruct<T>& Other) const
	{
		return SharedStruct != Other.InstancedStruct;
	}

	void AddReferencedObjects(class FReferenceCollector& Collector)
	{
		SharedStruct.AddStructReferencedObjects(Collector);
	}

private:
	/**
	 * Note:
	 *   TSharedStruct is a wrapper for a FInstancedStruct (rather than inheriting) so that it can provide a locked-down type-safe 
	 *   API for use in C++, without being able to accidentally take a reference to the untyped API to workaround the restrictions.
	 * 
	 *   TSharedStruct MUST be the same size as FInstancedStruct, as the reflection layer treats a TSharedStruct as a FInstancedStruct.
	 *   This means that any reflected APIs (like ExportText) that accept an FInstancedStruct pointer can also accept a TSharedStruct pointer.
	 */
	FSharedStruct SharedStruct;
};
#endif