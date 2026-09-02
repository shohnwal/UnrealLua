#pragma once
#include "CoreMinimal.h"
#include "Reflection/StructTemplateConcepts.h"
#include "sol/sol.hpp"
#include "StructUtils/InstancedStruct.h"
#include "StructUtils/SharedStruct.h"
#include "WeakStructView.generated.h"

///////////////////////////////////////////////////////////////// FWeakStructView /////////////////////////////////////////////////////////////////

/**
 * FWeakStructView is "typed" struct pointer, it contains pointer to struct plus UScriptStruct pointer.
 * FWeakStructView does not own the memory and will not free it when out of scope.
 * It should be only used to pass struct pointer in a limited scope, or when the user controls the lifetime of the struct being stored.
 * E.g. instead of passing ref or pointer to a FInstancedStruct, you should use FConsTWeakStructView or FWeakStructView to pass around a view to the contents.
 * FWeakStructView is passed by value.
 * FWeakStructView is similar to FStructOnScope, but FWeakStructView is a view only (FStructOnScope can either own the memory or be a view)
 * const FWeakStructView prevents the struct from pointing at a different instance of a struct. However the actual struct 
 * data can be mutated. Use FConsTWeakStructView to prevent mutation of the actual struct data.
 * See FConsTWeakStructView for examples.
 */

struct FWeakSharedStruct;

USTRUCT(BlueprintType)
struct UNREALLUA_API FWeakStructView
{
	GENERATED_BODY()
public:
	
	using NoLuaAutoPush = std::true_type;

	FWeakStructView();
	FWeakStructView(const FWeakStructView& other);
	FWeakStructView(FWeakStructView&& other) noexcept;
	~FWeakStructView();

	explicit FWeakStructView(const UScriptStruct* InScriptStruct, void* InStructMemory = nullptr)
		: ScriptStruct(const_cast<UScriptStruct*>(InScriptStruct))
		  , StructMemory(static_cast<uint8*>(InStructMemory))
	{}

	explicit FWeakStructView(FInstancedStruct& InstancedStruct)
		: FWeakStructView(InstancedStruct.GetScriptStruct(), InstancedStruct.GetMutableMemory())
	{}

	explicit FWeakStructView(const FSharedStruct& SharedStruct)
		: FWeakStructView(SharedStruct.GetScriptStruct(), SharedStruct.GetMemory())
	{}

	explicit FWeakStructView(const FWeakSharedStruct& SharedStruct);

	template<IsUStruct S>
	FWeakStructView(const S& s)
	: FWeakStructView(S::StaticStruct(), const_cast<S*>(&s))
	{
		
	}
	
	template<IsUStruct S>
	FWeakStructView(S& s) 
	: FWeakStructView(S::StaticStruct(), &s)
	{
		
	}


	/** Creates a new FWeakStructView from the templated struct. Note its not safe to make InStruct const ref as the original object may have been declared const */
	template<IsUStruct T>
	static FWeakStructView Make(T& InStruct)
	{
		UE::StructUtils::CheckStructType<T>();
		return FWeakStructView(TBaseStructure<T>::Get(), reinterpret_cast<uint8*>(&InStruct));
	}

	/** Returns mutable reference to the struct, this getter assumes that all data is valid. */
	template<typename T>
	T& Get() const
	{
		return UE::StructUtils::GetStructRef<T>(ScriptStruct, StructMemory);
	}

	/** Returns mutable pointer to the struct, or nullptr if cast is not valid. */
	template<typename T>
	T* GetPtr() const
	{
		return UE::StructUtils::GetStructPtr<T>(ScriptStruct, StructMemory);
	}

	/** Returns mutable reference to the struct, this getter assumes that all data is valid. */
	template<typename T>
	UE_DEPRECATED(5.3, "Use Get() instead")
	T& GetMutable() const
	{
		return Get<T>();
	}

	/** Returns mutable pointer to the struct, or nullptr if cast is not valid. */
	template<typename T>
	UE_DEPRECATED(5.3, "Use GetPtr() instead")
	T* GetMutablePtr() const
	{
		return GetPtr<T>();
	}

	/** Returns struct type. */
	UScriptStruct* GetScriptStruct() const
	{
		return ScriptStruct;
	}

	/** Reset to empty. */
	void Reset()
	{
		this->StructMemory = nullptr;
		this->ScriptStruct = nullptr;
	}
	
	FWeakStructView& operator=(const FWeakStructView& other)
	{
		this->ScriptStruct = other.ScriptStruct;
		this->StructMemory = other.StructMemory;
		return *this;
	}

	/** Comparison operators. Note: it does not compare the internal structure itself */
	template <typename OtherType>
	bool operator==(const OtherType& Other) const
	{
		return ((ScriptStruct == Other.GetScriptStruct()) && (StructMemory == Other.GetMemory()));
	}

	template <typename OtherType>
	bool operator!=(const OtherType& Other) const
	{
		return !operator==(Other);
	}

	bool IsValid() const;
	sol::object __index(sol::stack_object key, sol::this_state lua);
	void __newindex(sol::stack_object key, sol::stack_object value, sol::this_state lua);

	sol::object Lua_Copy(sol::this_state lua) const;
	sol::object MakeSharedStruct(sol::this_state lua);
	sol::object MakeInstancedStruct(sol::this_state lua);
	uint8* GetMemory() const;

protected:
	UPROPERTY(VisibleAnywhere, Category = "UnrealLua")
	UScriptStruct* ScriptStruct = nullptr;
	uint8* StructMemory = nullptr;
};

/**
 * TWeakStructView is a type-safe FWeakStructView wrapper against the given BaseStruct type.
 * 
 * Example:
 *
 *	TWeakStructView<FTestStructBase> Test;
 *
 *	TArray<TWeakStructView<FTestStructBase>> TestArray;
 */
template<typename BaseStructT>
struct UNREALLUA_API TWeakStructView : FWeakStructView
{
public:

	//explicit TWeakStructView() = default;

	template<typename T = BaseStructT, typename = std::enable_if_t<std::is_base_of_v<BaseStructT, std::decay_t<T>>>>
	TWeakStructView(BaseStructT& InStruct)
		: FWeakStructView(TBaseStructure<BaseStructT>::Get(), reinterpret_cast<uint8*>(&InStruct))
	{}

	template<typename T = BaseStructT, typename = std::enable_if_t<std::is_base_of_v<BaseStructT, std::decay_t<T>>>>
	explicit TWeakStructView(uint8* InStructMemory = nullptr)
		: FWeakStructView(T::StaticStruct(), InStructMemory)
	{}

	template<typename T = BaseStructT, typename = std::enable_if_t<std::is_base_of_v<BaseStructT, std::decay_t<T>>>>
	TWeakStructView(TInstancedStruct<T>& InstancedStruct)
		: FWeakStructView(InstancedStruct.GetScriptStruct(), InstancedStruct.GetMutableMemory())
	{}

	template<typename T = BaseStructT, typename = std::enable_if_t<std::is_base_of_v<BaseStructT, std::decay_t<T>>>>
	TWeakStructView(const TSharedStruct<T>& SharedStruct)
		: FWeakStructView(SharedStruct.GetScriptStruct(), SharedStruct.GetMemory())
	{}

	/** Returns mutable reference to the struct, this getter assumes that all data is valid. */
	template<typename T = BaseStructT, typename = std::enable_if_t<std::is_base_of_v<BaseStructT, std::decay_t<T>>>>
	T& Get() const
	{
		return UE::StructUtils::GetStructRef<BaseStructT>(ScriptStruct, StructMemory);
	}

	/** Returns mutable pointer to the struct, or nullptr if cast is not valid. */
	template<typename T = BaseStructT, typename = std::enable_if_t<std::is_base_of_v<BaseStructT, std::decay_t<T>>>>
	T* GetPtr() const
	{
		return UE::StructUtils::GetStructPtr<T>(ScriptStruct, StructMemory);
	}

	/** Comparison operators. Note: it does not compare the internal structure itself */
	template<typename T = BaseStructT, typename = std::enable_if_t<std::is_base_of_v<BaseStructT, std::decay_t<T>>>>
	bool operator==(const TWeakStructView<T>& Other) const
	{
		return ((ScriptStruct == Other.GetScriptStruct()) && (StructMemory == Other.GetMemory()));
	}

	template<typename T = BaseStructT, typename = std::enable_if_t<std::is_base_of_v<BaseStructT, std::decay_t<T>>>>
	bool operator!=(const TWeakStructView<T>& Other) const
	{
		return !operator==(Other);
	}
	
	/** Simplified accessor for members of the guaranteed minimum structure type */
	BaseStructT* operator->() const
	{
		check(IsValid());
		return GetPtr();
	}
};