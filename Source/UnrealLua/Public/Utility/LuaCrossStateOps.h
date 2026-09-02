// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "sol/sol.hpp"


/**
 * 
 */

namespace UnrealLua::CrossLuaState
{
  sol::object CopyLuaObjectToAnotherLuaState(sol::object& from, sol::this_state to);
}