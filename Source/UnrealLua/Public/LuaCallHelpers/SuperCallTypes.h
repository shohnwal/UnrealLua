#pragma once
#include "CoreTypes.h"

namespace UnrealLua::LuaScriptCall
{
	
	bool IsLuaSuperCallRequested();
	void AcceptLuaSuperCall();
	void MarkLuaSuperCallDone();

	enum ELuaSuperCallStatus : uint8
	{
		Requested,
		InProgress,
		Done
	};
	
	struct UNREALLUA_API FLuaSuperCallInfo
	{
		FLuaSuperCallInfo();
		~FLuaSuperCallInfo();
		
		void MarkInProgress();
		void MarkDone();
		bool IsDone();

		ELuaSuperCallStatus SuperCallStatus;
	};
}
