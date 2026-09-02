#include "Utility/WindowUIUtility.h"

#include "Framework/Application/SlateApplication.h"
#if WITH_EDITOR
#include "Interfaces/IMainFrameModule.h"
#endif
bool UnrealLuaTools::ShowWindowUtility::MakeModalWindow(TSharedRef<SWindow> newWindow, TSharedPtr<SWindow> parentWindow)
{
	if (!parentWindow)
	{
#if WITH_EDITOR
		if (FModuleManager::Get().IsModuleLoaded("MainFrame"))
		{
			IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");
			parentWindow = MainFrame.GetParentWindow();
		}
#endif
		if (!parentWindow)
		{
			parentWindow = FSlateApplication::Get().FindBestParentWindowForDialogs(newWindow);	
		}		
	}
	if (!parentWindow)
	{
		return false;
	}
	FSlateApplication::Get().AddModalWindow(newWindow, parentWindow);
	return true;
}
