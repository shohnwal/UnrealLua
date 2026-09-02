// Fill out your copyright notice in the Description page of Project Settings.


#include "UIWidgets/LuaScriptEditorTextBox/LuaScriptTextLayout.h"


TSharedRef< FLuaScriptTextLayout > FLuaScriptTextLayout::Create(SWidget* InOwner, FTextBlockStyle InDefaultTextStyle)
{
	LLM_SCOPE_BYTAG(UI_Text);
	TSharedRef< FLuaScriptTextLayout > Layout = MakeShareable( new FLuaScriptTextLayout(InOwner, MoveTemp(InDefaultTextStyle)) );
	Layout->AggregateChildren();

	return Layout;
}


void FLuaScriptTextLayout::UpdateLayout()
{
	FSlateTextLayout::UpdateLayout();
	this->OnLayoutChanged.ExecuteIfBound();
}
