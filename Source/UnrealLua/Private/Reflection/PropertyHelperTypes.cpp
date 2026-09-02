#include "Reflection/PropertyHelperTypes.h"
#include "Reflection/PropertyMapping.h"


FName FSetLuaScriptUObjectMemberPropertyWrapperParams::GetMappingFName() const
{
	return PropMapping.GetMappingFName();
}
