#include "Interface/LuaScriptStructBase.h"

#include "LuaTypes/LuaSharedStruct.h"
#include "UObjectRegistry/LuaUObjectItem.h"
#include "UObjectRegistry/UnrealLuaUObjectRegistry.h"
#include "UObjectRegistry/LuaUObjectRegistry.h"


FLuaScriptStructMemory::FLuaScriptStructMemory(const UScriptStruct* ss, const void* memToCopyFrom)
	: FLuaGCObject(), ScriptStruct(ss)
{
	ScriptStruct->InitializeStruct(this->GetMemory());
		
	if (memToCopyFrom)
	{
		ScriptStruct->CopyScriptStruct(this->GetMemory(), memToCopyFrom);
	}
}


FLuaScriptStructMemory::~FLuaScriptStructMemory()
{
	ScriptStruct->DestroyStruct(this->GetMemory());
}

void FLuaScriptStructMemory::AddReferencedObjects(FReferenceCollector& Collector)
{
	//LUA_LOG("Adding references in %s", *GetNameSafe(this->GetScriptStruct()));
	if(this->ScriptStruct && this->GetMemory())
	{
	
		Collector.AddReferencedObject(this->ScriptStruct);
		Collector.AddReferencedObjects(this->ScriptStruct, this->GetMemory(), nullptr);
		//Collector.AddPropertyReferencesWithStructARO(this->GetScriptStruct(), this->GetMemory());
	}
}

uint8* FLuaScriptStructMemory::GetMemory() const
{
	return Align(const_cast<uint8*>(this->Data), ScriptStruct->GetMinAlignment());
}

const UScriptStruct* FLuaScriptStructMemory::GetScriptStruct() const
{
	return this->ScriptStruct;
}

void FLuaScriptStructMemory::AddRef()
{
	this->RefCount++;
	verifyf(this->RefCount >= 0, TEXT("RefCount of FLuaScriptStructMemory %p is %d"), this, this->RefCount);
}

int32 FLuaScriptStructMemory::RemoveRef()
{
	this->RefCount--;
	verifyf(this->RefCount >= 0, TEXT("RefCount of FLuaScriptStructMemory %p is %d"), this, this->RefCount);
	return this->RefCount;
}

FLuaScriptStructBase::~FLuaScriptStructBase()
{
	
}
FLuaScriptStructBase::FLuaScriptStructBase()
	: PropertyMapping(nullptr)
{
	//UnrealLua::GCRegistry::RegisterObject(this);
}

FLuaScriptStructBase::FLuaScriptStructBase(const FLuaScriptStructBase& other)
	: PropertyMapping(other.PropertyMapping)
{
}

FLuaScriptStructBase::FLuaScriptStructBase(FLuaScriptStructBase&& other) noexcept
	: PropertyMapping(other.PropertyMapping)
{
}

FLuaScriptStructBase::FLuaScriptStructBase(const UScriptStruct* metaStruct)
{
	this->UpdatePropertyMapping(metaStruct);
}

void* FLuaScriptStructBase::GetMemory() const
{
	checkNoEntry();
	return nullptr;
}

sol::object FLuaScriptStructBase::Lua_Copy(sol::this_state) const
{
	checkNoEntry();
	return sol::nil;
}

const UScriptStruct* FLuaScriptStructBase::GetScriptStruct() const
{
	checkNoEntry();
	return nullptr;
}

bool FLuaScriptStructBase::IsReference() const
{
	return false;
}

/*
bool FLuaScriptStructBase::IsValid() const
{
	return this->GetMemory() != nullptr && this->GetScriptStruct() != nullptr;
}
*/

void FLuaScriptStructBase::UpdatePropertyMapping(const UScriptStruct* metaStruct)
{
	if(metaStruct)
	{
		FLuaUObjectItem& superStruct = UnrealLua::UObjectRegistry::GetMetaObjectItem(metaStruct);
		this->PropertyMapping = superStruct.PropertyMapping.GetPtr<FUStructPropertyMapping>();
		verify(this->PropertyMapping != nullptr);
	}
	else
	{
		this->PropertyMapping = nullptr;
	}
}

void FLuaScriptStructBase::AddReferencedObjects(FReferenceCollector& Collector)
{
	//LUA_LOG("Adding references in %s", *GetNameSafe(this->GetScriptStruct()));
	if(this->GetScriptStruct() && this->GetMemory())
	{
		//TObjectPtr<const UScriptStruct> ss = this->GetScriptStruct();
		TObjectPtr<const UScriptStruct> ss = Cast<UScriptStruct>(this->PropertyMapping->OwningField.Get());		
		
		Collector.AddReferencedObjects(ss, this->GetMemory(), nullptr);
		//Collector.AddPropertyReferencesWithStructARO(this->GetScriptStruct(), this->GetMemory());
	}
	this->PropertyMapping->AddReferencedObjects(Collector);
}

bool FLuaScriptStructBase::HasUObjectReferences()
{
	TArray<const FStructProperty*> props;
	for(TFieldIterator<FProperty> prop(GetScriptStruct()); prop; ++prop)
	{
		if(prop->ContainsObjectReference(props))
		{
			return true;
		}
	}
	return false;
}

bool FLuaScriptStructBase::__le(sol::object other, sol::this_state lua) const
{
	if (other.get_type() == sol::type::table)
	{
		const UScriptStruct* ss = this->GetScriptStruct();
		void* mem = this->GetMemory();
		if (ss && mem)
		{
			sol::table tbl = other.as<sol::table>();
			UnrealLua::PropertyHelper::InitializeStructFromTable(ss, mem, tbl, false);
			return true;
		}
	}
	return false;
}

bool FLuaScriptStructBase::IsValid() const
{
	return GetScriptStruct() && GetMemory() && this->PropertyMapping != nullptr;
}
