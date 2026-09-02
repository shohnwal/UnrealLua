// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include <limits>
#include "sol/sol.hpp"
/**
 * 
 */

struct FGetPropertyValueParams;

template<typename TNumberType, typename TProperty>
requires std::derived_from<TProperty, TProperty_Numeric<TNumberType>>
struct UNREALLUA_API FNumericPropertyDescr
{
	static bool IsCompatibleType(FProperty* prop, const sol::object& luaValue)
	{
		switch(luaValue.get_type())
		{
			case sol::type::number :
				return true;
			case sol::type::boolean :
				return true;
			default:
				return false;
		}
	}
	static sol::object GetPropertyValue(const FGetPropertyValueParams& params)
	{
		TProperty* prop = CastField<TProperty>(params.Prop);
		TNumberType val = prop->GetPropertyValue(params.MemoryPtr);
		
		//Limit : +- ~9 quadrillion / 9 billiarden :
		//This is due to Lua represending all numberas via double
		//and double having a mantissa of 53 bits, thus
		//only being able to represent integers with a max
		//of 53 bits
		if constexpr (std::is_same_v<TNumberType, int64>)
		{
			int64 value = val;
			const int64 max = (1LL << std::numeric_limits<double>::digits);// ((1LL << 53));
			const int64 min = -(1LL << std::numeric_limits<double>::digits); //-((1LL << 53));
			val = (FMath::Clamp<int64>(value, min, max));
		}
		else if constexpr(std::is_same_v<TNumberType, uint64>)
		{
			uint64 value = val;
			const uint64 max = (1LL << std::numeric_limits<double>::digits); //((1ULL << 53));
			const uint64 min = 0;
			val = (FMath::Clamp<uint64>(value, 0, max));
		}
		return sol::make_object<TNumberType>(params.Lua, val);
	}

	static int GetPropertyValue(FPushPropertyValueParams& params)
	{
		TProperty* prop = CastField<TProperty>(params.Prop);
		TNumberType val = prop->GetPropertyValue(params.MemoryPtr);
		
		//Limit : +- ~9 quadrillion / 9 billiarden :
		//This is due to Lua represending all numberas via double
		//and double having a mantissa of 53 bits, thus
		//only being able to represent integers with a max
		//of 53 bits
		if constexpr (std::is_same_v<TNumberType, int64>)
		{
			int64 value = val;
			const int64 max = (1LL << std::numeric_limits<double>::digits);// ((1LL << 53));
			const int64 min = -(1LL << std::numeric_limits<double>::digits); //-((1LL << 53));
			val = (FMath::Clamp<int64>(value, min, max));
		}
		else if constexpr(std::is_same_v<TNumberType, uint64>)
		{
			uint64 value = val;
			const uint64 max = (1LL << std::numeric_limits<double>::digits); //((1ULL << 53));
			const uint64 min = 0;
			val = (FMath::Clamp<uint64>(value, 0, max));
		}
		return sol::stack::push(params.Lua, val);
	}

	template<typename LUAOBJ>
	static void SetPropertyValue(const TSetPropertyValueParams<LUAOBJ>& params)
	{
		TProperty* prop = CastFieldChecked<TProperty>(params.Prop);
		TNumberType val = 0;
		
		params.Prop->InitializeValue(params.MemoryPtr);
		if(!params.LuaValue.valid())
		{
			return;
		}
		sol::type type = params.LuaValue.get_type(); 
		if (type == sol::type::number)
		{
			val = params.LuaValue.template as<TNumberType>();
		}
		else if (type == sol::type::string)
		{
			sol::string_view strv = params.LuaValue.template as<sol::string_view>();
			FString str = strv.data();
			if (str.IsNumeric())
			{
				double parsed = FCString::Atod(*str);
				val = static_cast<TNumberType>(parsed);	
			}
		}
		
		prop->SetPropertyValue(params.MemoryPtr, val);
	}

	static FString GetPropertyValueAsLuaSyntaxValidString(const FGetPropertyValueAsLuaSyntaxStringParams& params)
	{
		if (params.MemoryPtr == nullptr)
		{
			return FString::FromInt(0);
		}
		TProperty* prop = CastFieldChecked<TProperty>(params.Prop);
		TNumberType val = prop->GetPropertyValue(params.MemoryPtr);
		
		//Limit : +- ~9 quadrillion / 9 billiarden :
		//This is due to Lua represending all numberas via double
		//and double having a mantissa of 53 bits, thus
		//only being able to represent integers with a max
		//of 53 bits
		if constexpr (std::is_same_v<TNumberType, int64>)
		{
			int64 value = val;
			const int64 max = (1LL << std::numeric_limits<double>::digits);// ((1LL << 53));
			const int64 min = -(1LL << std::numeric_limits<double>::digits); //-((1LL << 53));
			val = (FMath::Clamp<int64>(value, min, max));
		}
		else if constexpr(std::is_same_v<TNumberType, uint64>)
		{
			uint64 value = val;
			const uint64 max = (1LL << std::numeric_limits<double>::digits); //((1ULL << 53));
			const uint64 min = 0;
			val = (FMath::Clamp<uint64>(value, 0, max));
		}
		
		if constexpr(std::is_floating_point_v<TNumberType>)
		{
			return FString::SanitizeFloat(val);
		}
		else if constexpr(std::is_integral_v<TNumberType>)
		{
			return FString::FromInt(val);
		}
		return FString::FromInt(0);
	}

private:
	FNumericPropertyDescr() { }
};