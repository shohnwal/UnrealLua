#include "LuaCallHelpers/SuperCallTypes.h"

#include "Containers/Array.h"
#include "Misc/AssertionMacros.h"

namespace UnrealLua::LuaScriptCall
{
	thread_local TArray<FLuaSuperCallInfo*> SuperCallInfo = {};
}

bool UnrealLua::LuaScriptCall::IsLuaSuperCallRequested()
{
	return SuperCallInfo.IsEmpty() ? false : SuperCallInfo.Top()->SuperCallStatus == ELuaSuperCallStatus::Requested;
}

void UnrealLua::LuaScriptCall::AcceptLuaSuperCall()
{
	verify(!SuperCallInfo.IsEmpty())
	SuperCallInfo.Top()->MarkInProgress();
}

void UnrealLua::LuaScriptCall::MarkLuaSuperCallDone()
{
	verify(!SuperCallInfo.IsEmpty())
	SuperCallInfo.Top()->MarkDone();
}

UnrealLua::LuaScriptCall::FLuaSuperCallInfo::FLuaSuperCallInfo() 
	: SuperCallStatus(ELuaSuperCallStatus::Requested)
{
	SuperCallInfo.Add(this);
}

UnrealLua::LuaScriptCall::FLuaSuperCallInfo::~FLuaSuperCallInfo()
{
	verify(SuperCallInfo.Top() == this);
	SuperCallInfo.Pop();
}

void UnrealLua::LuaScriptCall::FLuaSuperCallInfo::MarkInProgress()
{
	verify(this->SuperCallStatus == ELuaSuperCallStatus::Requested);
	this->SuperCallStatus = ELuaSuperCallStatus::InProgress;
}

void UnrealLua::LuaScriptCall::FLuaSuperCallInfo::MarkDone()
{
	verify(this->SuperCallStatus == ELuaSuperCallStatus::InProgress);
	this->SuperCallStatus = ELuaSuperCallStatus::Done;
}

bool UnrealLua::LuaScriptCall::FLuaSuperCallInfo::IsDone()
{
	return this->SuperCallStatus == ELuaSuperCallStatus::Done;
}
