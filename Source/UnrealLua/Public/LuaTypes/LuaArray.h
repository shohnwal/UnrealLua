// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interface/LuaGCObject.h"
#include "Misc/TVariant.h"
#include "sol/sol.hpp"
#include "Utility/LuaLogMacros.h"

/**
 * 
 */

struct UNREALLUA_API FLuaScriptArray : public FScriptArray, public FLuaGCObject
{
	FLuaScriptArray(FProperty* inner);
	virtual ~FLuaScriptArray() override;
	TUniquePtr<FProperty> Inner = nullptr;
	int32 RefCount = 0;
	bool bContainsObjectReferences = false;
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	void AddRef();
	int32 RemoveRef();
};

struct UNREALLUA_API FLuaArray
{
	static void RegisterUsertype(sol::state_view& pairs);
public:
	FLuaArray();

	// As reference used when getting a UPROPERTY
	// Create copy for passing tarray from Blueprint to Lua as a function argument / function return
	FLuaArray(FArrayProperty& prop, FScriptArray* referencedArray, bool bAsReference);

	FLuaArray(FProperty& inner, FScriptArray* otherScriptArray, bool bAsReference);

	FLuaArray(FProperty& inner, sol::object initial, sol::this_state lua);
	
	FLuaArray(const FLuaArray&);
	FLuaArray(FLuaArray&&) noexcept;
	~FLuaArray();
	void Reset();

	FLuaArray& operator=(const FLuaArray& other)
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

	bool operator==(const FLuaArray& other) const
	{
		return this->GetScriptArray() == other.GetScriptArray();
	}
	
	FLuaArray& operator()(sol::variadic_args args)
	{
		LUA_LOG("Called array!")
		if (args.size() == 1)
		{
			sol::stack_object arg = args[0];
			if (arg.get_type() == sol::type::table)
			{
				sol::table tbl = arg.as<sol::table>();
				this->InitFromTable(tbl);
			}
		}
		return *this;
	}
	void InitFromTable(const sol::table& Tbl);

	FProperty* GetInner() const;
	FScriptArray* GetScriptArray() const;

	//virtual void SetThisPropertyReference(UObject* owner, const sol::object& value, sol::this_state lua) override;

	//explicit FLuaArray(sol::stack_object type, sol::this_state lua);

	/*
	using value_type = decltype(data)::value_type;
	using iterator = TScriptContainerIterator<FScriptArray>;
	using size_type = decltype(data)::size_;
	*/

	static void LuaToTArrayIndexCorrection(int32& index);
	static sol::object __index(const FLuaArray* self, sol::object key, sol::this_state lua);
	sol::object operator[](sol::object key) const;
	int32 Lua_Add(sol::object obj);
	sol::object Lua_Get(int32 index, sol::this_state lua);
	sol::object Lua_Set(int32 index, sol::object value_o, sol::this_state lua);
	void Lua__Remove(int32 index, int32 count);
	sol::object Lua_AddAt(int32 index, sol::object obj);
	void Lua__RemoveAt(int32 index) const;
	bool Lua_IsValidIndex(int32 index) const;
	void Lua_Clear();
	int32 Lua_Num() const;
	int32 Num() const { return Lua_Num(); }
	int32 Lua_Find(sol::object toSearch, sol::this_state lua);
	int32 Lua_FindLast(sol::object toSearch, sol::this_state lua);
	bool Lua_Contains(sol::object toSearch, sol::this_state lua);

	bool Lua_IsEmpty() const;


	bool IsValid() const;
	FLuaArray& Lua_ForEach(sol::protected_function func, sol::variadic_args args);
	FLuaArray Lua_Copy() const;
	FLuaArray& Lua_KeepAll(sol::protected_function func, sol::variadic_args args, const sol::this_state lua);
	FLuaArray& Lua_RemoveAll(sol::protected_function func, sol::variadic_args args, const sol::this_state lua);
	FLuaArray& Filter_Internal(sol::protected_function& func, sol::variadic_args& args, bool bShouldKeep, const sol::this_state lua);
	sol::variadic_results Any(sol::object num, bool bInplace);
	void Push(sol::object obj, sol::this_state lua);
	sol::object Lua_Pop(sol::this_state lua);
	sol::object Lua_Top(sol::this_state lua);
	sol::object Lua_Last(int32 indexFromEnd, sol::this_state lua);

	static void Copy(FScriptArray* destArray, FProperty* destinnerProp, const FScriptArray* srcArray, FProperty* srcInnerProp);
	//void CloneArray(FScriptArray* destArray, FProperty* p, const FScriptArray* srcArray);
	void MoveArray(FScriptArray* destArray, FProperty& innerProperty, FScriptArray* srcArray);

	sol::table Lua_ToTable(sol::this_state lua_s);

	uint8* GetMemPtrForIndex(int32 index) const;
	uint8* AddDefaultGetMemPtr();
	void ConstructDefaultItems(int32 index, int32 count);
private:
	FProperty* Inner = nullptr;
	TVariant<std::nullptr_t, FScriptArray*, FLuaScriptArray*> Data;

public:
	bool OwnsMemory() const;
	void AddRef();
	int32 RemoveRef();
	bool IsUPropertyReference() const;
	
	/**
 * Templated iterator to go through script helper containers that may contain invalid entries
 * that are not part of the valid number of elements (i.e. GetMaxIndex() != Num() ).
 * The iterator
 *  - will advance to the first valid entry on creation and when incremented
 *  - can be dereferenced to an internal index to be used with methods like Get<Item>Ptr or Get<Item>PtrWithoutCheck
 *  - can also be used directly with methods like Get<Item>PtrChecked
 *  - can return the associated logical index (number of valid visited entries) by calling GetLogicalIndex()
 */
	struct UNREALLUA_API FIterator
	{
		explicit FIterator(const FLuaArray& InContainer) : Container(InContainer)
		{
			Advance();
		}

		explicit FIterator(const FLuaArray& InContainer, const int32 InLogicalIndex) : Container(InContainer)
		{
			const int32 MaxIndex = Container.Num();
			if (MaxIndex == Container.Num())
			{
				InternalIndex = InLogicalIndex;
				LogicalIndex = InLogicalIndex;
				return;
			}

			do
			{
				Advance();
			}
			while (LogicalIndex < InLogicalIndex && InternalIndex < MaxIndex);
		}

		FIterator& operator++()
		{
			Advance();
			return *this;
		}

		FIterator operator++(int)
		{
			const FIterator Temp(*this);
			Advance();
			return Temp;
		}

		explicit operator bool() const
		{
			return Container.IsValidIndex(InternalIndex);
		}

		int32 GetInternalIndex() const
		{
			return InternalIndex;
		}

		int32 GetLogicalIndex() const
		{
			return LogicalIndex;
		}

		UE_DEPRECATED(5.4, "Use Iterator directly, GetInternalIndex or GetLogicalIndex instead.")
		int32 operator*() const
		{
			return InternalIndex;
		}

	private:
		const FLuaArray& Container;
		int32 InternalIndex = INDEX_NONE;
		int32 LogicalIndex = INDEX_NONE;

		void Advance()
		{
			++InternalIndex;
			const int32 MaxIndex = Container.Num();
			while (InternalIndex < MaxIndex && !Container.IsValidIndex(InternalIndex))
			{
				++InternalIndex;
			}

			++LogicalIndex;
		}
	};
	
	using iterator = FIterator;
	using size_type = int32;
	
	FIterator begin() const { return FIterator{*this, 0}; }
	FIterator end() const { return FIterator{*this, this->Lua_Num()}; }

	int32 size() const { return this->Lua_Num(); }
	
	struct lua_iterator_state {
		typedef FLuaArray::iterator it_t;
		it_t it;
		it_t last;
		FLuaArray& owner;

		lua_iterator_state(FLuaArray& mt)
		: it(mt), last(mt, mt.Num()), owner(mt) {
		}
	};
	static int __next(lua_State* L);
	static int __pairs(lua_State* L);
	static int __ipairs(lua_State* L);
	    
	/**
 * Check the validity of an index
 *
 * @param Index - the index
 * @return - true if the index is valid, false otherwise
 */
	bool IsValidIndex(int32 Index) const
	{
		return Index >= 0 && Index < this->GetScriptArray()->Num();
	}

	/**
 * Add an element to the array
 *
 * @param Item - the element
 * @return - the index of the added element
 */
	int32 Add(const void *Item);

	/**
	 * Add a unique element to the array
	 *
	 * @param Item - the element
	 * @return - the index of the added element
	 */
	int32 AddUnique(const void *Item)
	{
		int32 Index = Find(Item);
		if (Index == INDEX_NONE)
		{
			Index = Add(Item);
		}
		return Index;
	}

	/**
	 * Add N defaulted elements to the array
	 *
	 * @param Count - number of elements
	 * @return - the index of the first element added
	 */
	int32 AddDefaulted(int32 Count = 1);

	/**
	 * Add N uninitialized elements to the array
	 *
	 * @param Count - number of elements
	 * @return - the index of the first element added
	 */
	int32 AddUninitialized(int32 Count = 1);

	/**
	 * Find an element
	 *
	 * @param Item - the element
	 * @return - the index of the element
	 */
	int32 Find(const void *Item) const;

	/**
	 * Insert an element
	 *
	 * @param Item - the element
	 * @param Index - the index
	 */
	void Insert(const void *Item, int32 Index);

	/**
	 * Remove the i'th element
	 *
	 * @param Index - the index
	 */
	void Lua_Remove(int32 Index);

	/**
	 * Remove all elements equals to 'Item'
	 *
	 * @param Item - the element
	 * @return - number of elements that be removed
	 */
	int32 RemoveItem(const void *Item)
	{
		int32 NumRemoved = 0;
		int32 Index = Find(Item);
		while (Index != INDEX_NONE)
		{
			++NumRemoved;
			Lua_Remove(Index);
			Index = Find(Item);
		}
		return NumRemoved;
	}

	/**
	 * Empty the array
	 */
	void Clear();

	/**
	 * Reserve space for N elements
	 *
	 * @param Size - the element
	 * @return - whether the operation succeed
	 */
	bool Reserve(int32 Size);

	/**
	 * Resize the array to new size
	 *
	 * @param NewSize - new size of the array
	 */
	void Resize(int32 NewSize);

	/**
	 * Get value of the i'th element
	 *
	 * @param Index - the index
	 * @param OutItem - the element in the 'Index'
	 */
	void Get(int32 Index, void *OutItem) const;

	/**
	 * Set new value for the i'th element
	 *
	 * @param Index - the index
	 * @param Item - the element to be set
	 */
	void Set(int32 Index, const void *Item);

	/**
	 * Swap two elements
	 *
	 * @param A - the first index
	 * @param B - the second index
	 */
	void Swap(int32 A, int32 B);

	/**
 * Shuffle the elements
 */
	void Lua_Shuffle();

	/**
 * Append another array
 *
 * @param SourceArray - the array to be appended
 */
	void Append(const FLuaArray &SourceArray);

	/**
 * Get address of the i'th element
 *
 * @param Index - the index
 * @return - the address of the i'th element
 */
	uint8* GetData(int32 Index);

	const uint8* GetData(int32 Index) const;

	/**
 * Get the address of the allocated memory
 *
 * @return - the address of the allocated memory
 */
	void* GetData()
	{
		return GetScriptArray()->GetData();
	}

	const void* GetData() const
	{
		return GetScriptArray()->GetData();
	}
	private:
		/**
		 * Construct n elements
		 */
		void Construct(int32 Index, int32 Count = 1);

	/**
		 * Destruct n elements
		 */
		void Destruct(int32 Index, int32 Count = 1);
};

inline bool FLuaArray::Lua_IsEmpty() const
{
	return this->GetScriptArray() == nullptr || this->Num() == 0;
}

/*
namespace sol {
	template <>
	struct is_container<FLuaArray> : std::true_type {};
	
	template <>
	struct usertype_container<FLuaArray> {

		using iterator = TScriptContainerIterator<FScriptArray>;
		
		static iterator begin(lua_State*, FLuaArray& self)
		{
			FScriptArrayHelper helper = FScriptArrayHelper::CreateHelperFormInnerProperty(self.Inner, self.ScriptArray);
			return 
		}
		static int pairs(lua_State*)
		{
			
			
		}
		// see below for implemetation details
	};
}
*/