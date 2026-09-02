#pragma once

#include "PropertyHelperTypes.h"

namespace UnrealLua::PropertyHelper
{
	UNREALLUA_API FString GetPropertyValueAsLuaSyntaxValidString_InContainer(FGetPropertyValueAsLuaSyntaxStringParams& params);
	UNREALLUA_API FString GetPropertyValueAsLuaSyntaxValidString(FGetPropertyValueAsLuaSyntaxStringParams& params);
}
