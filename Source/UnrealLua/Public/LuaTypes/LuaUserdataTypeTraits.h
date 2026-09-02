// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "sol/sol.hpp"
/**
 * 
 */

namespace sol {
	template <>
	struct is_container<FString> : std::false_type {};
}

namespace sol {
	template <>
	struct is_automagical<FVector2D> : std::false_type {};
}

inline bool operator==(FTransform& lhs, const FTransform& rhs)
{
	return lhs.Equals(rhs);
}
inline bool operator==(FText& lhs, const FText& rhs)
{
	return lhs.EqualTo(rhs);
}

inline bool operator<(FName& lhs, const FName& rhs)
{
	return lhs.FastLess(rhs);
}
/*
uint32 GetTypeHash(const sol::function& func)
{
	return func.registry_index();
}
*/

/*
uint32 GetTypeHash(const std::string& str)
{
	return FCrc::StrCrc32<char>(str.data());
}
*/
/*
bool operator==(const std::string& str, const std::string& other)
{
	return str._Equal(other);
}
*/

/*
inline bool operator==(FName& lhs, const FName& rhs)
{
	return lhs.IsEqual(rhs);
}
*/


