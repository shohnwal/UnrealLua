#pragma once

#include "CoreMinimal.h"
#include "Interface/LuaGCObject.h"
#include "Misc/TVariant.h"
#include "UObject/UnrealType.h"
#include "sol/sol.hpp"


struct UNREALLUA_API FLuaMapKeyValuePropertyPair
{
	~FLuaMapKeyValuePropertyPair();
	FProperty* Key = nullptr;
	FProperty* Value = nullptr;
};

struct UNREALLUA_API FLuaScriptMap : public FScriptMap, public FLuaGCObject
{
	FLuaScriptMap(FLuaMapKeyValuePropertyPair* properties);
	virtual ~FLuaScriptMap() override;
	TUniquePtr<FLuaMapKeyValuePropertyPair> Properties = nullptr;
	int32 RefCount = 0;
	bool bContainsObjectReferences = false;
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	void AddRef();
	int32 RemoveRef();
};

struct UNREALLUA_API FLuaMap
{
	static void RegisterUsertype(sol::state_view& lua);
	
	FLuaMap();

	//reference mode, for returning a member FMapProperty
	FLuaMap(FMapProperty* prop, FScriptMap* otherScriptMap, bool bAsRef);

	//Used when pushing to Lua as well as whn using call constructor in Lua
	FLuaMap(FProperty* keyProp, FProperty* valueProp, FScriptMap* otherScriptMap, bool bAsRef);

	FLuaMap(FProperty* keyProp, FProperty* valueProp, sol::object initial, sol::this_state lua);

	//copy constructor
	FLuaMap(const FLuaMap&);

	//Move constructor
	explicit FLuaMap(FLuaMap&&) noexcept;

	~FLuaMap();

	void Reset();

	FScriptMap* GetScriptMap() const;

	FLuaMap& operator=(const FLuaMap& other)
	{
		if(this != &other)
		{
			this->Reset();
			this->Properties = other.Properties;
			this->Data = other.Data;
			if(this->OwnsMemory())
			{
				this->AddRef();
			}
		}
		return *this;
	}
	
	void SetProperties(FProperty* key, FProperty* value);
	void FixupProperties();
	void Clone(const FScriptMap* src, FScriptMap* dest);

	int32 Lua_Num() const;
	sol::object AddDefault(sol::object key);
	void Lua_Add(sol::object key, sol::object value, sol::this_state lua);
	sol::object Lua_Find(sol::object key, sol::this_state lua);
	bool Lua_Remove(sol::object key, sol::this_state lua);
	bool Lua_Contains(sol::object key, sol::this_state lua);
	void Lua_Empty();

	sol::table Lua_ToTable(sol::this_state lua_s);
	
	static void Copy(FScriptMap* dest, FProperty* keyProp, FProperty* valueProp, FScriptMap* src);
	FLuaMap Lua_Copy() const;
	bool IsValid() const;

private:
	FLuaMapKeyValuePropertyPair* Properties;

	TVariant<std::nullptr_t, FScriptMap*, FLuaScriptMap*> Data = {};
public:
	FProperty* GetKeyProperty() const;
	FProperty* GetValueProperty() const;

	sol::object __Index(sol::object key, sol::this_state lua);
	sol::object __IndexInternal(int32 index, sol::this_state lua);
	std::tuple<sol::object, sol::object> __IndexPairInternal(int32 index, sol::this_state lua);
	using iterator = TScriptContainerIterator<FScriptMap>;
	using size_type = int32;
	
	TScriptContainerIterator<FScriptMap> begin() const { return TScriptContainerIterator<FScriptMap>{*this->GetScriptMap(), 0}; }
	TScriptContainerIterator<FScriptMap> end() const { return TScriptContainerIterator<FScriptMap>{*this->GetScriptMap(), this->Lua_Num()}; }

	int32 size() const { return this->Lua_Num(); }

	// automatically bound for obj == obj [ __eq ]
	bool operator==(const FLuaMap& right) const {
		return this->GetScriptMap() == right.GetScriptMap();
	}
	// automatically bound for obj < obj [ __lt ]
	bool operator<(const FLuaMap& right) const {
		return this->GetScriptMap() < right.GetScriptMap();
	}
	// automatically bound for obj <= obj [ __le ]
	bool operator<=(const FLuaMap& right) const {
		return this->GetScriptMap() <= right.GetScriptMap();
	}

	struct lua_iterator_state {
		typedef FLuaMap::iterator it_t;
		it_t it;
		it_t last;
		FLuaMap& owner;

		lua_iterator_state(FLuaMap& mt)
		: it(mt.begin()), last(mt.end()), owner(mt) {
		}
	};

	static int __next(lua_State* L);

	static int __pairs(lua_State* L);
	
	bool OwnsMemory() const;
	void AddRef();
	int32 RemoveRef();
	bool IsUPropertyReference() const;
	//virtual void SetThisPropertyReference(UObject* owner, const sol::object& value, sol::this_state lua) override;
};

