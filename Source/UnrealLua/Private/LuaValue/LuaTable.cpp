#include "LuaValue/LuaTable.h"
#include "UnrealLua.h"
#include "LuaContext/ScopedLuaContext.h"

FLuaTable::FLuaTable(sol::table tbl)
	: Table(tbl)
{
	
}

void FLuaTable::Invalidate()
{
	this->Table = sol::nil;
}

FLuaValue FLuaTable::Index(const FLuaValue& key)
{
	if (this->Table.valid())
	{
		sol::object val = this->Table[key]; 
		return FLuaValue{val};
	}
	return FLuaValue{};
}

void FLuaTable::NewIndex(const FLuaValue& key, const FLuaValue& value)
{
	if (this->Table.valid())
	{
		this->Table[key] = value;
	}	
}

FLuaTableHandle::FLuaTableHandle(const TSharedPtr<FLuaTable>& tableSharedPtr)
	: LuaTableWrapper(tableSharedPtr)
{
}

FLuaTableHandle FLuaTableHandle::MakeHandle(const sol::table& tbl)
{
	verify(tbl.valid())
	FScopedLuaContext* ctx = FScopedLuaContext::GetLuaContextFromLuaState(tbl.lua_state());
	return ctx->CreateLuaTableHandleForTable(tbl);
}

void FLuaTableHandle::Invalidate()
{
	this->LuaTableWrapper.Reset();
}

bool FLuaTableHandle::operator==(const sol::table& table) const
{
	return this->GetTable() == table;
}

bool FLuaTableHandle::IsValid() const
{
	return this->LuaTableWrapper.IsValid() && this->LuaTableWrapper->Table.valid();
}

FLuaValue FLuaTableHandle::Index(const FLuaValue& key) const
{
	if (!this->IsValid())
	{
		return {};
	}
	return this->LuaTableWrapper->Index(key);
}

void FLuaTableHandle::NewIndex(const FLuaValue& key, const FLuaValue& value) const
{
	if (!this->IsValid())
	{
		return;
	}
	this->LuaTableWrapper->NewIndex(key, value);
}

sol::table FLuaTableHandle::GetTable() const
{
	if (!this->IsValid())
	{
		return sol::nil;
	}
	return this->LuaTableWrapper->Table;
}

FWeakLuaTableHandle::FWeakLuaTableHandle(TSharedPtr<FLuaTable>& tblContainer)
	: LuaTableContainer()
{
	verify(tblContainer.IsValid() && tblContainer->Table.valid())
	LuaTableContainer = tblContainer;
}

void FWeakLuaTableHandle::Invalidate()
{
	if (this->LuaTableContainer.IsValid())
	{
		this->LuaTableContainer.Pin()->Invalidate();
	}
	this->LuaTableContainer.Reset();
}