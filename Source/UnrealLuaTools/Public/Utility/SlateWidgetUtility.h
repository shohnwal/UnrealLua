#pragma once
#include "Layout/ChildrenBase.h"
#include "Widgets/SWidget.h"

class SWidget;

namespace UnrealLuaTools::SlateWidgetUtility
{
	template<typename T, typename U>
	const T::Slot& GetSlotForWidget(T* parent, U* child )
	{
		FChildren* children = parent->GetChildren();
		for (int32 childIndex = 0; childIndex < children->Num(); ++childIndex)
		{
			if (children->GetChildAt(childIndex) == child)
			{
				return children->GetSlotAt(childIndex);
			}
		}
		return T::Slot();
	}
}
