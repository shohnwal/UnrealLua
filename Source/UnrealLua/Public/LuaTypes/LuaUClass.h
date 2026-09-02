#pragma once
#include "CoreMinimal.h"
#include "sol/sol.hpp"
#include "UObject/SoftObjectPtr.h"

class UNREALLUA_API FLuaUClass
{
public:
	static void RegisterUsertype(sol::state_view& lua);
	
	FLuaUClass();
	explicit FLuaUClass(UClass* obj);
	explicit FLuaUClass(const UClass* obj);
	FLuaUClass(sol::object obj);
	FLuaUClass(sol::string_view str);
	FLuaUClass(const FLuaUClass& other);
	FLuaUClass(const FLuaUClass* other);
	FLuaUClass(const FSoftClassPath& path);
	FLuaUClass(FLuaUClass&& other) noexcept;
	FLuaUClass& operator=(const FLuaUClass& other);
	~FLuaUClass();

	bool operator==(const FLuaUClass& other) const
	{
		return this->Class == other.Class;
	}
		
	sol::object operator()(sol::object outerObj, const sol::this_state lua) const;
	bool Valid() const;
	void Set(UClass* Class);

	static int __index(lua_State* lua);
	static void __newindex(FLuaUClass* self, sol::object key, sol::object value, sol::this_state lua);

	UClass* TryLoadClass() const;
	const FSoftClassPath GetSoftClassPath() const;
	/*
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override { return TEXT("FLuaUClass");}
	*/
	void Extend(sol::table extensionTbl);
private:
	TSharedPtr<FSoftClassPath> Class;
	//UClass* Class = nullptr;
	//TAssetPtr<UClass> Path;

};