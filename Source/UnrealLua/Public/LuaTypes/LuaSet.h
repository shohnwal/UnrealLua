// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/UnrealType.h"
#include "Interface/LuaGCObject.h"
#include "Misc/TVariant.h"
#include "sol/sol.hpp"

/**
 * 
 */

struct UNREALLUA_API FLuaScriptSet : public FScriptSet, public FLuaGCObject
{
	FLuaScriptSet(FProperty* inner);
	virtual ~FLuaScriptSet() override;
	TUniquePtr<FProperty> Inner = nullptr;
	int32 RefCount = 0;
	bool bContainsObjectReferences = false;
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	void AddRef();
	int32 RemoveRef();
};


struct UNREALLUA_API FLuaSet
{
	static void RegisterUsertype(sol::state_view& lua);

	FLuaSet();
	//explicit FLuaSet(sol::stack_object type, sol::this_state lua);
	explicit FLuaSet(FSetProperty* setProp, FScriptSet* referencedScriptSet, bool bAsReference);
	explicit FLuaSet(FProperty* inner, FScriptSet* otherContainer, bool bAsReference);
	explicit FLuaSet(FProperty* inner, sol::object initial, sol::this_state lua);
	FLuaSet(const FLuaSet& other);
	explicit FLuaSet(FLuaSet&& other) noexcept;
	~FLuaSet();

	void Reset();
	FScriptSet* GetScriptSet() const;

	FLuaSet& operator=(const FLuaSet& other)
	{
		if(this != &other)
		{
			this->Reset();
			this->Inner = other.Inner;
			this->Data = other.Data;
			if(this->OwnsMemory())
			{
				this->AddRef();
			}
		}
		return *this;
	}

	int32 Lua_Num() const;
	void Lua_Add(sol::object obj, sol::this_state lua);
	int32 Lua_Find(sol::object luaValue, sol::this_state lua);
	bool Lua_Remove(sol::object luaValue, sol::this_state lua);
	bool Lua_Contains(sol::object luaValue, sol::this_state lua);
	void Lua_RemoveAt(sol::object luaValue, sol::this_state lua);
	void Lua_Empty();

	void Append(const FLuaSet& toAppend);
	FProperty* GetInner() const;

	static void Copy(FScriptSet* src, FProperty* innerProp, FScriptSet* dest);
	void Clone(FScriptSet* dest) const;
	//void Clone(FProperty* prop, FScriptSet* src, FScriptSet* dest) const;
	FLuaSet Lua_Copy() const;
	sol::variadic_results Lua_Any(sol::object num);
	FLuaSet& Lua_KeepAll(sol::protected_function func, sol::variadic_args args, const sol::this_state lua);
	FLuaSet& Lua_RemoveAll(sol::protected_function func, sol::variadic_args args, const sol::this_state lua);
	bool Lua_IsEmpty() const;
	bool IsValid() const;

private:
	FLuaSet& Filter(sol::protected_function func, sol::variadic_args args, bool bShouldKeep, sol::this_state lua);
public:
	sol::table Lua_ToLuaTable(sol::this_state lua);

	sol::object __Index(sol::object key, sol::this_state lua);
	sol::object __IndexInternal(int32 index, sol::this_state lua);
	bool __NewIndex(sol::object key, sol::object value, sol::this_state lua);

private:
	FProperty* Inner = nullptr;
	TVariant<std::nullptr_t, FScriptSet*, FLuaScriptSet*> Data;
public:
	bool OwnsMemory() const;
	void AddRef();
	int32 RemoveRef();
	bool IsUPropertyReference() const;

	//using value_type = int32;
	using iterator = TScriptContainerIterator<FScriptSet>;
	using size_type = int32;
	
	TScriptContainerIterator<FScriptSet> begin() const { return TScriptContainerIterator<FScriptSet>{*this->GetScriptSet(), 0}; }
	TScriptContainerIterator<FScriptSet> end() const { return TScriptContainerIterator<FScriptSet>{*this->GetScriptSet(), this->Lua_Num()}; }

	int32 size() const { return this->Lua_Num(); }
	// automatically bound for obj == obj [ __eq ]
	bool operator==(const FLuaSet& right) const {
		return this->GetScriptSet() == right.GetScriptSet();
	}
	// automatically bound for obj < obj [ __lt ]
	bool operator<(const FLuaSet& right) const {
		return this->GetScriptSet() < right.GetScriptSet();
	}
	// automatically bound for obj <= obj [ __le ]
	bool operator<=(const FLuaSet& right) const {
		return this->GetScriptSet() <= right.GetScriptSet();
	}
	
	struct lua_iterator_state {
		typedef FLuaSet::iterator it_t;
		it_t it;
		it_t last;
		FLuaSet& owner;

		lua_iterator_state(FLuaSet& mt)
		: it(mt.begin()), last(mt.end()), owner(mt) {
		}
	};

	static int __next(lua_State* L);

	static int __pairs(lua_State* L);
	static int __ipairs(lua_State* L);
	//virtual void SetThisPropertyReference(UObject* owner, const sol::object& value, sol::this_state lua) override;
};



/*
namespace sol
{
	template <>
	struct is_container<FLuaSet> : std::true_type {};

	template <>
	struct usertype_container<FLuaSet> {
		static std::ptrdiff_t index_adjustment(lua_State* L, FLuaSet& self)
		{
			return 0;
		}
		
	};
} 
*/	