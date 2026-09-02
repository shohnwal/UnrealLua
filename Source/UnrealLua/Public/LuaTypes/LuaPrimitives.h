#pragma once
#include "CoreMinimal.h"
#include "Reflection/PropertyHelperTypes.h"
#include "sol/sol.hpp"
#include <string>

struct UNREALLUA_API FLuaPrimitiveCPPType
{
	FLuaPrimitiveCPPType()
	{		
	}
	FLuaPrimitiveCPPType(ELuaSupportedClassCastFlags type)
		: Type(type)
	{		
	}
	ELuaSupportedClassCastFlags Type = ELuaSupportedClassCastFlags::LUA_CASTCLASS_None;
	
	sol::object operator()(sol::object arg, const sol::this_state lua) const;

	std::string tostring() const;
	bool Matches(sol::object& toCheck);
};

namespace UnrealLua
{
	namespace LuaTypes
	{
		namespace Primitives
		{
			void RegisterPrimitiveWrappers(sol::state_view& lua);
		}
		namespace TypeInfo
		{
			UNREALLUA_API std::string UType_Stack(sol::stack_object obj, sol::stack_object innerCheck, sol::this_state lua);
			UNREALLUA_API std::string UType(sol::object obj, sol::object innerCheck, sol::this_state lua);
			UNREALLUA_API std::string UType(sol::object obj, bool bWithInner = true, int32 wantedPropIndex = -1);
			UNREALLUA_API std::string UTypeInternal(sol::object value, bool bWithInner, int32 wantedPropIndex);
			UNREALLUA_API sol::object UInnerType(sol::object& obj, sol::this_state lua);
			
			UNREALLUA_API std::string UType(UFunction* func, bool fullsignature = false);
		}
	}
}