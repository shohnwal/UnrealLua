// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Templates/Requires.h"
#include "TLuaVariantMeta.h"
#include "Templates/PimplPtr.h"
#include <type_traits>

#include "Serialization/Archive.h"



/**
 * A special tag used to indicate that in-place construction of a variant should take place.
 */
template <typename T>
struct TLuaInPlaceType {};

/**
 * A special tag that can be used as the first type in a TLuaVariant parameter pack if none of the other types can be default-constructed.
 */
struct UNREALLUA_API FEmptyLuaVariantState
{
	/** Allow FEmptyVariantState to be used with FArchive serialization */
	friend inline FArchive& operator<<(FArchive& Ar, FEmptyLuaVariantState&)
	{
		return Ar;
	}
};

template<typename T>
using TLuaVariantPtr = TPimplPtr<T, EPimplPtrMode::DeepCopy>; 
/**
 * A type-safe union based loosely on std::variant. This flavor of variant requires that all the types in the declaring template parameter pack be unique.
 * Attempting to use the value of a Get() when the underlying type is different leads to undefined behavior.
 */
template <typename T, typename... Ts>
class TLuaVariant
#if UE_TVARIANT_TRIVIAL_DESTRUCTOR_USING_CONCEPTS
	: private UnrealLua::Variant::TLuaVariantStorage<T, Ts...>
#else
	: private std::conditional_t<!std::is_trivially_destructible_v<T> || (!std::is_trivially_destructible_v<Ts> || ...), UnrealLua::Variant::TDestructibleVariantStorage<T, Ts...> , UnrealLua::Variant::TLuaVariantStorage<T, Ts...>>
#endif
{
#if UE_TVARIANT_TRIVIAL_DESTRUCTOR_USING_CONCEPTS
	using Super = UnrealLua::Variant::TLuaVariantStorage<T, Ts...>;
#else
	using Super = std::conditional_t<!std::is_trivially_destructible_v<T> || (!std::is_trivially_destructible_v<Ts> || ...), UnrealLua::Variant::TDestructibleVariantStorage<T, Ts...> , UnrealLua::Variant::TLuaVariantStorage<T, Ts...>>;
#endif

	static_assert(!UnrealLua::Variant::TTypePackContainsDuplicates<T, Ts...>::Value, "All the types used in TLuaVariant should be unique");
	static_assert(!UnrealLua::Variant::TContainsReferenceType<T, Ts...>::Value, "TLuaVariant cannot hold reference types");

	// Test for 255 here, because the parameter pack doesn't include the initial T
	static_assert(sizeof...(Ts) <= 255, "TLuaVariant cannot hold more than 256 types");

public:
	/** Default initialize the TLuaVariant to the first type in the parameter pack */
	TLuaVariant()
	{
		static_assert(std::is_constructible_v<T>, "To default-initialize a TLuaVariant, the first type in the parameter pack must be default constructible. Use FEmptyVariantState as the first type if none of the other types can be listed first.");
		::new((void*)&UnrealLua::Variant::CastToStorage(*this).Storage) T();
		TypeIndex = 0;
	}

	/** Perform in-place construction of a type into the variant */
	template <typename U, typename... TArgs>
	explicit TLuaVariant(TLuaInPlaceType<U>&&, TArgs&&... Args)
	{
		constexpr SIZE_T Index = UnrealLua::Variant::TParameterPackTypeIndex<U, T, Ts...>::Value;
		static_assert(Index != (SIZE_T)-1, "The TLuaVariant is not declared to hold the type being constructed");

		::new((void*)&UnrealLua::Variant::CastToStorage(*this).Storage) U(Forward<TArgs>(Args)...);
		TypeIndex = (uint8)Index;
	}

	/** Copy construct the variant from another variant of the same type */
	TLuaVariant(const TLuaVariant& Other)
	{
		TypeIndex = Other.TypeIndex;
		UnrealLua::Variant::TCopyConstructorLookup<T, Ts...>::Construct(TypeIndex, &UnrealLua::Variant::CastToStorage(*this).Storage, &UnrealLua::Variant::CastToStorage(Other).Storage);
	}

	/** Move construct the variant from another variant of the same type */
	TLuaVariant(TLuaVariant&& Other)
	{
		TypeIndex = Other.TypeIndex;
		UnrealLua::Variant::TMoveConstructorLookup<T, Ts...>::Construct(TypeIndex, &UnrealLua::Variant::CastToStorage(*this).Storage, &UnrealLua::Variant::CastToStorage(Other).Storage);
	}

	/** Copy assign a variant from another variant of the same type */
	TLuaVariant& operator=(const TLuaVariant& Other)
	{
		if (&Other != this)
		{
			TLuaVariant Temp = Other;
			Swap(Temp, *this);
		}
		return *this;
	}

	/** Move assign a variant from another variant of the same type */
	TLuaVariant& operator=(TLuaVariant&& Other)
	{
		if (&Other != this)
		{
			TLuaVariant Temp = MoveTemp(Other);
			Swap(Temp, *this);
		}
		return *this;
	}

#if UE_TVARIANT_TRIVIAL_DESTRUCTOR_USING_CONCEPTS
	/** Destruct the underlying type (if appropriate) */
	~TLuaVariant()
		requires(!std::is_trivially_destructible_v<T> || (!std::is_trivially_destructible_v<Ts> || ...))
	{
		UnrealLua::Variant::TDestructorLookup<T, Ts...>::Destruct(TypeIndex, &UnrealLua::Variant::CastToStorage(*this).Storage);
	}
	~TLuaVariant()
		requires(std::is_trivially_destructible_v<T> && (std::is_trivially_destructible_v<Ts> && ...))
	= default;
#else
	// Defer to the storage as to how to destruct the elements
	~TLuaVariant() = default;
#endif

	/** Determine if the variant holds the specific type */
	template <typename U>
	bool IsType() const
	{
		static_assert(UnrealLua::Variant::TParameterPackTypeIndex<U, T, Ts...>::Value != (SIZE_T)-1, "The TLuaVariant is not declared to hold the type passed to IsType<>");
		return UnrealLua::Variant::TIsType<U, T, Ts...>::IsSame(TypeIndex);
	}

	/** Get a reference to the held value. Bad things can happen if this is called on a variant that does not hold the type asked for */
	template <typename U>
	U& Get()
	{
		constexpr SIZE_T Index = UnrealLua::Variant::TParameterPackTypeIndex<U, T, Ts...>::Value;
		static_assert(Index != (SIZE_T)-1, "The TLuaVariant is not declared to hold the type passed to Get<>");

		check(Index == TypeIndex);
		// The intermediate step of casting to void* is used to avoid warnings due to use of reinterpret_cast between related types if U and the storage class are related
		// This was specifically encountered when U derives from TAlignedBytes
		return *reinterpret_cast<U*>(reinterpret_cast<void*>(&UnrealLua::Variant::CastToStorage(*this).Storage));
	}

	/** Get a reference to the held value. Bad things can happen if this is called on a variant that does not hold the type asked for */
	template <typename U>
	const U& Get() const
	{
		// Temporarily remove the const qualifier so we can implement Get in one location.
		return const_cast<TLuaVariant*>(this)->template Get<U>();
	}

	/** Get a pointer to the held value if the held type is the same as the one specified */
	template <typename U>
	U* TryGet()
	{
		constexpr SIZE_T Index = UnrealLua::Variant::TParameterPackTypeIndex<U, T, Ts...>::Value;
		static_assert(Index != (SIZE_T)-1, "The TLuaVariant is not declared to hold the type passed to TryGet<>");
		// The intermediate step of casting to void* is used to avoid warnings due to use of reinterpret_cast between related types if U and the storage class are related
		// This was specifically encountered when U derives from TAlignedBytes
		return Index == (SIZE_T)TypeIndex ? reinterpret_cast<U*>(reinterpret_cast<void*>(&UnrealLua::Variant::CastToStorage(*this).Storage)) : nullptr;
	}

	/** Get a pointer to the held value if the held type is the same as the one specified */
	template <typename U>
	const U* TryGet() const
	{
		// Temporarily remove the const qualifier so we can implement TryGet in one location.
		return const_cast<TLuaVariant*>(this)->template TryGet<U>();
	}

	/** Set a specifically-typed value into the variant */
	template <typename U>
	void Set(typename TIdentity<U>::Type&& Value)
	{
		Emplace<U>(MoveTemp(Value));
	}

	/** Set a specifically-typed value into the variant */
	template <typename U>
	void Set(const typename TIdentity<U>::Type& Value)
	{
		Emplace<U>(Value);
	}

	/** Set a specifically-typed value into the variant using in-place construction */
	template <typename U, typename... TArgs>
	void Emplace(TArgs&&... Args)
	{
		constexpr SIZE_T Index = UnrealLua::Variant::TParameterPackTypeIndex<U, T, Ts...>::Value;
		static_assert(Index != (SIZE_T)-1, "The TLuaVariant is not declared to hold the type passed to Emplace<>");

		UnrealLua::Variant::TDestructorLookup<T, Ts...>::Destruct(TypeIndex, &UnrealLua::Variant::CastToStorage(*this).Storage);
		::new((void*)&UnrealLua::Variant::CastToStorage(*this).Storage) U(Forward<TArgs>(Args)...);
		TypeIndex = (uint8)Index;
	}

	/** Lookup the index of a type in the template parameter pack at compile time. */
	template <typename U>
	static constexpr SIZE_T IndexOfType()
	{
		constexpr SIZE_T Index = UnrealLua::Variant::TParameterPackTypeIndex<U, T, Ts...>::Value;
		static_assert(Index != (SIZE_T)-1, "The TLuaVariant is not declared to hold the type passed to IndexOfType<>");
		return Index;
	}

			
	static constexpr SIZE_T SizeOfType(const uint64 index)
	{
		constexpr SIZE_T Sizes[] = { sizeof(Ts)... };
		return Sizes[index];
	}
	
	/** Returns the currently held type's index into the template parameter pack */
	SIZE_T GetIndex() const
	{
		return (SIZE_T)TypeIndex;
	}
	
	void MarkAsScriptValue()
	{
		this->bIsScriptValue = true;
	}
	
	void ClearScriptValue()
	{
		this->bIsScriptValue = false;
	}
	
	bool IsScriptValue() const 
	{
		return this->bIsScriptValue;
	}
	
private:
#if UE_TVARIANT_TRIVIAL_DESTRUCTOR_USING_CONCEPTS
	/** Index into the template parameter pack for the type held. */
	uint8 TypeIndex;
#else
	using Super::TypeIndex;
#endif
};

/** Apply a visitor function to the list of variants */
template <
	typename Func,
	typename... Variants
	UE_REQUIRES((TIsLuaVariant_V<std::decay_t<Variants>> && ...))
>
decltype(auto) Visit(Func&& Callable, Variants&&... Args)
{
	constexpr SIZE_T NumPermutations = (1 * ... * (TLuaVariantSize_V<std::decay_t<Variants>>));

	return UnrealLua::Variant::VisitImpl(
		UnrealLua::Variant::EncodeIndices(Args...),
		Forward<Func>(Callable),
		TMakeIntegerSequence<SIZE_T, NumPermutations>{},
		TMakeIntegerSequence<SIZE_T, sizeof...(Variants)>{},
		Forward<Variants>(Args)...
	);
}

/**
 * Serialization function for TLuaVariant. 
 *
 * In order for a TLuaVariant to be serializable, each type in its template parameter pack must:
 *   1. Have a default constructor. This is required because when reading the type from an archive, it must be default constructed before being loaded.
 *   2. Implement the `FArchive& operator<<(FArchive&, T&)` function. This is required to serialize the actual type that's stored in TLuaVariant.
 */
template <typename... Ts>
inline FArchive& operator<<(typename UnrealLua::Variant::TAlwaysFArchive<TLuaVariant<Ts...>>::Type& Ar, TLuaVariant<Ts...>& Variant)
{
	if (Ar.IsLoading())
	{
		uint8 Index;
		Ar << Index;
		check(Index < sizeof...(Ts));

		UnrealLua::Variant::TLuaVariantLoadFromArchiveLookup<Ts...>::Load((SIZE_T)Index, Ar, Variant);
	}
	else
	{
		uint8 Index = (uint8)Variant.GetIndex();
		Ar << Index;
		Visit([&Ar](auto& StoredValue)
		{
			Ar << StoredValue;
		}, Variant);
	}
	return Ar;
}
