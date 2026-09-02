#pragma once
#include "Templates/SharedPointer.h"

class SWindow;

namespace UnrealLuaTools::ShowWindowUtility
{
	UNREALLUATOOLS_API bool MakeModalWindow(TSharedRef<SWindow> newWindow, TSharedPtr<SWindow> parentWindow = nullptr);
}
