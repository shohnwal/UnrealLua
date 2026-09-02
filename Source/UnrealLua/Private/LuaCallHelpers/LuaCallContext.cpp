// Fill out your copyright notice in the Description page of Project Settings.


#include "LuaCallHelpers/LuaCallContext.h"

int FLuaCallContext::__index(sol::stack_object key)
{
	return 0;
}

void FLuaCallContext::__newindex(sol::stack_object key, sol::stack_object value)
{
	return;
}
