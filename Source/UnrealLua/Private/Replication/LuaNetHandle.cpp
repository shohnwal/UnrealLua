#include "Replication/LuaNetHandle.h"

#include "Interface/LuaScriptable.h"
#include "UObjectRegistry/LuaUObjectRegistry.h"
#include "UObjectRegistry/UnrealLuaUObjectRegistry.h"


FLuaNetHandle::FLuaNetHandle(UObject* obj): HandleValue(0)
{
	UnrealLua::UObjectRegistry::GetLuaNetHandleForObject(obj);
}

bool FLuaNetHandle::NetSerialize(FArchive& ar, UPackageMap* map, bool& bOutSuccess)
{
	ar << this->HandleValue;
	return true;
}

bool FRegisteredLuaNetObjectInfo::NetSerialize(FArchive& ar, UPackageMap* map, bool& bOutSuccess)
{
	if(ar.IsSaving())
	{
		bool bUseNetHandle = this->LuaNetHandle.IsValid();
		ar.SerializeBits(&bUseNetHandle, 1);
		if(bUseNetHandle)
		{
			ar << this->LuaNetHandle.HandleValue;
		}
		else
		{
			ar << this->RegisteredObject;
		}
	}
	else if(ar.IsLoading())
	{
		bool bUseNetHandle = false;
		ar.SerializeBits(&bUseNetHandle, 1);
		if(bUseNetHandle)
		{
			this->RegisteredObject = nullptr;
			FLuaNetHandle netHandle;
			ar << netHandle.HandleValue;
			verify(netHandle.IsValid());
			this->LuaNetHandle = netHandle;
		}
		else
		{
			this->LuaNetHandle = {};
			ar << this->RegisteredObject;
		}
	}
	return true;
}

