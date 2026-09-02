#pragma once
#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "UObject/LazyObjectPtr.h"
#include "UObject/ScriptInterface.h"
#include "UObject/SoftObjectPtr.h"

template<typename U>
concept IsUClassPtr = requires(U t) { std::remove_pointer_t<U>::StaticClass();}
	&& std::is_pointer_v<U>
	&& std::is_base_of_v<UClass, std::remove_pointer_t<U>>;

template<typename U>
concept UObjectPtrTypename = requires(U t) { std::remove_pointer_t<U>::StaticClass();}
&& std::is_pointer_v<U>
&& (std::is_base_of_v<UObject, std::remove_pointer_t<U>> || std::is_same_v<UObject, std::remove_pointer_t<U>>)
&& !std::is_base_of_v<UClass, std::remove_pointer_t<U>>
&& !std::is_base_of_v<UEnum, std::remove_pointer_t<U>>
&& !std::is_base_of_v<UScriptStruct, std::remove_pointer_t<U>>;

template<typename U>
concept UObjectTypename = requires { U::StaticClass();}
&& !std::is_pointer_v<U>
&& (std::is_base_of_v<UObject, U> || std::is_same_v<UObject, U>)
&& !std::is_base_of_v<UClass, U>
&& !std::is_base_of_v<UEnum, U>
&& !std::is_base_of_v<UScriptStruct, U>;

// These simple helpers aren't enough to truly detect all UEnums and UStructs,
// we would also need specializations for non-UHT generated UStructs,to make
// this edge case more obvious I have chosen these names:
template <typename T, typename = void>
struct TIsNativeEnum : std::false_type { };

template <typename T>
struct TIsNativeEnum <T, std::void_t<decltype(&T::StaticEnum)>> : std::true_type {};

template <typename T, typename = void>
struct TIsNativeUStruct : std::false_type { };
	
template <typename T>
struct TIsNativeUStruct <T, std::void_t<decltype(&T::StaticStruct)>> : std::true_type {};
	/*
		FProperty <-> CPPType mappings, encoded in TCPPTypeToPropertyType:
		FInt8Property	<-> int8
		FInt16Property	<-> int16
		FIntProperty	<-> int32
		FInt64Property	<-> int64
		FByteProperty	<-> uint8
		FUInt16Property	<-> uint16
		FUInt32Property	<-> uint32
		FUInt64Property	<-> uint64
		FFloatProperty	<-> float
		FDoubleProperty	<-> double
		FBoolProperty	<-> bool
		FStrProperty	<-> FString
		FNameProperty	<-> FName
		FTextProperty	<-> FText
		FObjectProperty	<-> TObjectPtr
		FClassProperty	<-> TSubclassOf
		FSoftObjectProperty	<-> TSoftObjectPtr
		FSoftClassProperty	<-> TSoftClassPtr
		FWeakObjectProperty	<-> TWeakObjectPtr
		FLazyObjectProperty	<-> TLazyObjectPtr
		FSetProperty	<-> TSet
		FArrayProperty	<-> TArray
		FMapProperty	<-> TMap
		FOptionalProperty	<-> TOptional
		FInterfaceProperty	<-> TScriptInterface
		FMulticastInlineDelegateProperty	<-> TMulticastDelegate
		FMulticastSparseDelegateProperty	<-> TSparseDynamicDelegate
		FDelegateProperty	<-> TScriptDelegate
		FEnumProperty	<-> UEnum
		FStructProperty	<-> UStruct
	*/
	template<typename T>
	struct TypeToProp
	{
 		using PropertyType = std::conditional< 
			TIsNativeUStruct<T>::value, FStructProperty,
				std::conditional<TIsNativeEnum<T>::value, FEnumProperty, std::false_type>>;
	};

	template<> struct TypeToProp<int8> { using PropertyType = FInt8Property; };
	template<> struct TypeToProp<int16> { using PropertyType = FInt16Property; };
	template<> struct TypeToProp<int32> { using PropertyType = FIntProperty; };
	template<> struct TypeToProp<int64> { using PropertyType = FInt64Property; };
	template<> struct TypeToProp<uint8> { using PropertyType = FByteProperty; };
	template<> struct TypeToProp<uint16> { using PropertyType = FUInt16Property; };
	template<> struct TypeToProp<uint32> { using PropertyType = FUInt32Property; };
	template<> struct TypeToProp<uint64> { using PropertyType = FUInt64Property; };
	template<> struct TypeToProp<float> { using PropertyType = FFloatProperty; };
	template<> struct TypeToProp<double> { using PropertyType = FDoubleProperty; };
	template<> struct TypeToProp<bool> { using PropertyType = FBoolProperty; };
	template<> struct TypeToProp<FString> { using PropertyType = FStrProperty; };
	template<> struct TypeToProp<FName> { using PropertyType = FNameProperty; };
	template<> struct TypeToProp<FText> { using PropertyType = FTextProperty; };
	template<UObjectPtrTypename T> struct TypeToProp<T> { using PropertyType = FObjectProperty; };
	template<typename T> struct TypeToProp<TObjectPtr<T>> { using PropertyType = FObjectProperty; };
	template<typename T> struct TypeToProp<TSubclassOf<T>> { using PropertyType = FClassProperty; };
	template<typename T> struct TypeToProp<TSoftObjectPtr<T>> { using PropertyType = FSoftObjectProperty; };
	template<typename T> struct TypeToProp<TSoftClassPtr<T>> { using PropertyType = FSoftClassProperty; };
	template<typename T> struct TypeToProp<TWeakObjectPtr<T>> { using PropertyType = FWeakObjectProperty; };
	template<typename T> struct TypeToProp<TLazyObjectPtr<T>> { using PropertyType = FLazyObjectProperty; };
	template<typename T> struct TypeToProp<TArray<T>> { using PropertyType = FArrayProperty; };
	template<typename K, typename V> struct TypeToProp<TMap<K, V>> { using PropertyType = FMapProperty; };
	template<typename T> struct TypeToProp<TOptional<T>> { using PropertyType = FOptionalProperty; };
	template<typename T> struct TypeToProp<TScriptInterface<T>> { using PropertyType = FInterfaceProperty; };
	template<typename T> struct TypeToProp<TMulticastDelegate<T>> { using PropertyType = FMulticastInlineDelegateProperty; };
	template<typename A, typename B, typename C> struct TypeToProp<TSparseDynamicDelegate<A,B,C>> { using PropertyType = FMulticastSparseDelegateProperty; };
	template<typename T> struct TypeToProp<TScriptDelegate<T>> { using PropertyType = FDelegateProperty; };
