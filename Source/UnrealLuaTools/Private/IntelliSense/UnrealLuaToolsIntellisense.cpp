// Fill out your copyright notice in the Description page of Project Settings.


#include "IntelliSense/UnrealLuaToolsIntellisense.h"

#include <locale>

#include "InputKeyEventArgs.h"
#include "Widgets/Text/SMultiLineEditableText.h"

void UUnrealLuaToolsIntellisense::NotifyKeyHit(const FInputKeyEventArgs& inputEvent)
{
	SMultiLineEditableText* text = nullptr;
	if (inputEvent.Key == EKeys::Colon)
	{
		FTextLocation location = text->GetCursorLocation();
		int32 offset = location.GetOffset();
		FString line;
		
		text->GetCurrentTextLine(line);

		TCHAR* textPtr = GetData(line) + offset;
		while (offset > 0)
		{
			--textPtr;
			--offset;
			TCHAR chr = *textPtr;
			//if (!TChar<TCHAR>::IsAlnum(chr) || chr != TCHAR('_') && )
		}
	}
	else if (inputEvent.Key == EKeys::Period)
	{
		
	}
}
