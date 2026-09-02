#include "LuaTypes/LuaMap.h"

#include "LuaCoreDelegates.h"
#include "Config/UnrealLuaConstants.h"
#include "LuaTypes/LuaArray.h"
#include "Reflection/PropertyHelper.h"
#include "Reflection/PropertyHelperTypes.h"
#include "Reflection/PropertyDescr/FMapPropertyDescr.h"

static const FDelegateHandle fLuaMapLuaTypeHandle = FLuaCoreDelegates::OnRegisterLuaUsertypes.AddStatic(&FLuaMap::RegisterUsertype);

FLuaMapKeyValuePropertyPair::~FLuaMapKeyValuePropertyPair()
{
	delete Key;
	delete Value;
}

FLuaScriptMap::FLuaScriptMap(FLuaMapKeyValuePropertyPair* properties)
	: FScriptMap()
{
	verify(properties != nullptr)
	verify(properties->Key != nullptr);
	verify(properties->Value != nullptr);
	this->Properties.Reset(properties);
	TArray<const FStructProperty*> encounteredStructProps;
	FProperty* key = Properties->Key;
	FProperty* value = Properties->Value;
	this->bContainsObjectReferences =
		(UnrealLua::PropertyHelper::CanPropertyContainObjectReferences(key) && key->ContainsObjectReference(encounteredStructProps))
	||	(UnrealLua::PropertyHelper::CanPropertyContainObjectReferences(value) && value->ContainsObjectReference(encounteredStructProps));
}

FLuaScriptMap::~FLuaScriptMap()
{
	verify(RefCount == 0);
	FLuaMapKeyValuePropertyPair* properties = this->Properties.Get();
	this->Properties.Reset();
	this->RefCount = 0;
}

void FLuaScriptMap::AddReferencedObjects(FReferenceCollector& Collector)
{
	if(!this->bContainsObjectReferences)
	{
		FProperty* keyProp = Properties->Key;
		FProperty* valueProp = Properties->Value;
		FScriptMapHelper helper = FScriptMapHelper::CreateHelperFormInnerProperties(keyProp, valueProp, this);

		for (int32 index = this->Num() - 1; index >= 0; index--)
		{
			if(!helper.IsValidIndex(index))
			{
				continue;
			}
			void* ptr = helper.GetKeyPtr(index);
			void* vptr = helper.GetValuePtr(index);
			// If AddReferencedObject collect obj
			// remove it from this array
			if (UnrealLua::PropertyHelper::AddRefByProperty(Collector, keyProp, ptr))
			{
				//PropertyHelper will null out any invalid objects
				//helper.RemoveAt(index);
			}
			else if (UnrealLua::PropertyHelper::AddRefByProperty(Collector, valueProp, vptr))
			{
				//PropertyHelper will null out any invalid objects
				//helper.RemoveAt(index);
			};
		}
	}
}

void FLuaScriptMap::AddRef()
{
	this->RefCount++;
	verifyf(this->RefCount >= 0, TEXT("RefCount of FLuaScriptMap %p is %d"), this, this->RefCount);
}

int32 FLuaScriptMap::RemoveRef()
{
	this->RefCount--;
	verifyf(this->RefCount >= 0, TEXT("RefCount of FLuaScriptMap %p is %d"), this, this->RefCount);
	return this->RefCount;
}

void FLuaMap::RegisterUsertype(sol::state_view& lua)
{
	sol::usertype<FLuaMap> ut = lua.new_usertype<FLuaMap>(
	"TMap",
	"new", sol::no_constructor,
	sol::call_constructor, [](sol::object keyType, sol::object valueType, sol::stack_object initial, sol::this_state lua) ->sol::object
	{
		FProperty* keyprop = UnrealLua::PropertyHelper::CreateNewProperty(keyType,  UnrealLua::PropertyNames::NAME_UnrealLuaMapKeyProperty);
		FProperty* valueprop = UnrealLua::PropertyHelper::CreateNewProperty(valueType, UnrealLua::PropertyNames::NAME_UnrealLuaMapValueProperty);
		if(!keyprop || !valueprop)
		{
			delete keyprop;
			delete valueprop;
			return sol::nil;
		}
		return sol::object(lua, sol::in_place_type<FLuaMap>, keyprop, valueprop, initial, lua);
	},
	"Add", &FLuaMap::Lua_Add,
	"Num", &FLuaMap::Lua_Num,
	"Find", &FLuaMap::Lua_Find,
	"Empty", &FLuaMap::Lua_Empty,
	"Clear", &FLuaMap::Lua_Empty,
	"Remove",&FLuaMap::Lua_Remove,
	"Contains", &FLuaMap::Lua_Contains,
	"ToTable", &FLuaMap::Lua_ToTable,
	"Copy", &FLuaMap::Lua_Copy,
	"IsReference", &FLuaMap::IsUPropertyReference,
	sol::meta_function::length, &FLuaMap::Lua_Num,
	sol::meta_function::index, &FLuaMap::__Index,
	sol::meta_function::pairs, &FLuaMap::__pairs,
	sol::meta_function::new_index, [](FLuaMap& arr, sol::stack_object key, sol::stack_object newvalue, sol::this_state lua) { return arr.Lua_Add(key, newvalue, lua); }
	);
}

FScriptMapHelper CreateHelperFormInnerProperties(FProperty* InKeyProperty, FProperty* InValProperty, const void *InMap)
{
	return FScriptMapHelper::CreateHelperFormInnerProperties(const_cast<FProperty*>(InKeyProperty), const_cast<FProperty*>(InValProperty), InMap);
}

FLuaMap::FLuaMap()
	: Properties()
	, Data()
{
	checkNoEntry();
}

//getter from FMapProperty
//Can be a reference, if referencing a UProperty, or a copy if it's a UFunction return value
//Must construct properties manually
FLuaMap::FLuaMap(FMapProperty* prop, FScriptMap* otherScriptMap, bool bAsRef)
	: Properties()
	, Data()
{
	this->SetProperties(
		UnrealLua::PropertyHelper::CreateNewProperty(prop->KeyProp,  UnrealLua::PropertyNames::NAME_UnrealLuaMapKeyProperty),
		UnrealLua::PropertyHelper::CreateNewProperty(prop->ValueProp,  UnrealLua::PropertyNames::NAME_UnrealLuaMapValueProperty)
	);
	this->FixupProperties();
	if(bAsRef)
	{
		this->Data.Emplace<FScriptMap*>(otherScriptMap);
	}
	else
	{
		this->Data.Emplace<FLuaScriptMap*>(new FLuaScriptMap(this->Properties));
		this->AddRef();
		verify(this->GetScriptMap()->Num() == 0);
		this->Clone(otherScriptMap, this->GetScriptMap());
	}
}

//Constructor when pushing FLuaMap reference via sol::stack::push, the properties are ours and pre-constructed
FLuaMap::FLuaMap(FProperty* keyProp, FProperty* valueProp, FScriptMap* otherScriptMap, bool bAsRef)
	: Properties(nullptr)
	, Data()
{
	this->SetProperties(keyProp, valueProp);
	this->FixupProperties();
	if(bAsRef)
	{
		this->Data.Emplace<FScriptMap*>(otherScriptMap);
	}
	else
	{
		this->Data.Emplace<FLuaScriptMap*>(new FLuaScriptMap(this->Properties));
		this->AddRef();
		auto helper = FScriptMapHelper::CreateHelperFormInnerProperties(this->GetKeyProperty(),this->GetValueProperty(), this->GetScriptMap());
		helper.EmptyValues();
		verify(this->GetScriptMap()->Num() == 0);
		this->Clone(otherScriptMap, this->GetScriptMap());
	}
}

//Constructor when using Lua call constructor
//properties have been pre-constructed to make sure
//before this map gets constructed, it'll be valid
FLuaMap::FLuaMap(FProperty* keyProp, FProperty* valueProp, sol::object initial, sol::this_state lua)
	: Properties()
	, Data()
{
	this->SetProperties(keyProp, valueProp);
	verify(this->GetKeyProperty()->GetFName() == UnrealLua::PropertyNames::NAME_UnrealLuaMapKeyProperty);
	verify(this->GetValueProperty()->GetFName() == UnrealLua::PropertyNames::NAME_UnrealLuaMapValueProperty);
	this->Data.Emplace<FLuaScriptMap*>(new FLuaScriptMap(this->Properties));
	this->AddRef();
	verify(this->GetScriptMap()->Num() == 0);
	this->FixupProperties();
	
	if (initial.get_type() == sol::type::table)
	{
		sol::table tbl = initial;
		tbl.for_each([this, &lua](sol::object key, sol::object value)
		{
			this->Lua_Add(key, value, lua);
		});
	}
}

//Copy constructor
FLuaMap::FLuaMap(const FLuaMap& other)
	: Properties(other.Properties)
	, Data(other.Data)
{

	if(this->OwnsMemory())
	{
		this->FixupProperties();
		this->AddRef();	
	}
	else
	{
		if (other.GetKeyProperty()->GetFName() == UnrealLua::PropertyNames::NAME_UnrealLuaMapKeyProperty)
		{
			//This Inner is a manually created one, so we need to create a new one for the new array
			verify(other.GetValueProperty()->GetFName() == UnrealLua::PropertyNames::NAME_UnrealLuaMapValueProperty);
			verify(other.GetKeyProperty()->Owner == nullptr && other.GetValueProperty()->Owner == nullptr);
			
			FProperty* keyProp = UnrealLua::PropertyHelper::CreateNewProperty(other.GetKeyProperty(), UnrealLua::PropertyNames::NAME_UnrealLuaMapKeyProperty);
			FProperty* valProp = UnrealLua::PropertyHelper::CreateNewProperty(other.GetKeyProperty(), UnrealLua::PropertyNames::NAME_UnrealLuaMapValueProperty);
			this->Properties = new FLuaMapKeyValuePropertyPair(keyProp, valProp);
		}
		else
		{
			//this is a UProperty reference
			verify(other.GetKeyProperty()->Owner != nullptr)
			verify(other.GetValueProperty()->Owner != nullptr)
		}
	}
}

FLuaMap::FLuaMap(FLuaMap&& other) noexcept
	: Properties(other.Properties)
	, Data(other.Data) 
{
	if(this->OwnsMemory())
	{
		//must add ref BEFORE decreasing ref from other, to keep data alive
		this->AddRef();
	}
	other.Reset();
}

FLuaMap::~FLuaMap()
{
	this->Reset();
}

void FLuaMap::Reset()
{
	this->RemoveRef();
	if (this->Data.IsType<FScriptMap*>())
	{
		
	}
	this->Data.Emplace<std::nullptr_t>();
	this->Properties = nullptr;
}

FScriptMap* FLuaMap::GetScriptMap() const
{
	return this->OwnsMemory() ? this->Data.Get<FLuaScriptMap*>() : this->Data.Get<FScriptMap*>();
}

void FLuaMap::SetProperties(FProperty* key, FProperty* value)
{
	this->Properties = new FLuaMapKeyValuePropertyPair{key, value};
}

void FLuaMap::FixupProperties()
{
//	FScriptMapLayout MapLayout = FScriptMap::GetScriptLayout(this->KeyProp->GetElementSize(), this->KeyProp->GetMinAlignment(), this->ValueProp->GetElementSize(), this->ValueProp->GetMinAlignment());
	//UEProperty_Private::FProperty_DoNotUse::Unsafe_AlterOffset(*this->ValueProp, MapLayout.ValueOffset);
	//this->ValueProp->SetOffset_Internal(MapLayout.ValueOffset);
}

//ATTENTION : Assumes Inner-type compatibility has been checked beforehand!

void FLuaMap::Clone(const FScriptMap* src,FScriptMap* dest) {
	if(!src || !dest)
	{
		return;
	}
	FProperty* keyProp = this->GetKeyProperty();
	FProperty* valProp = this->GetValueProperty();
	FScriptMapHelper dstHelper = CreateHelperFormInnerProperties(keyProp, valProp, dest);
	FScriptMapLayout MapLayout = FScriptMap::GetScriptLayout(keyProp->GetElementSize(), valProp->GetMinAlignment(), valProp->GetElementSize(), valProp->GetMinAlignment());
	//src->Empty(src->Num(), MapLayout);
	
	for (int32 index = dest->Num() - 1; index >= 0; index--)
	{
		dest->RemoveAt(index,MapLayout);
	}
	
	verify(dest->Num() == 0)
	if(src->Num() == 0)
	{
		return;
	}
	FScriptMapHelper srcHelper = CreateHelperFormInnerProperties(keyProp, valProp, src);
	for (auto n = 0; n < srcHelper.GetMaxIndex(); n++)
	{
		if (srcHelper.IsValidIndex(n))
		{
			auto keyPtr = srcHelper.GetKeyPtr(n);
			auto valuePtr = srcHelper.GetValuePtr(n);
			dstHelper.AddPair(keyPtr, valuePtr);
		}
	}
}

int32 FLuaMap::Lua_Num() const
{
	FProperty* keyProp = this->GetKeyProperty();
	FProperty* valProp = this->GetValueProperty();
	FScriptMapHelper helper = CreateHelperFormInnerProperties(keyProp, valProp, this->GetScriptMap());
	return helper.Num();
}

sol::object FLuaMap::AddDefault(sol::object key_o)
{
	FProperty* keyProp = this->GetKeyProperty();
	FProperty* valueProp = this->GetValueProperty();
	if(!UnrealLua::FMapPropertyDescr::IsCompatibleKey(keyProp, key_o))
	{
		return sol::nil;
	}
	FScriptMapHelper helper = CreateHelperFormInnerProperties(keyProp, valueProp, this->GetScriptMap());
	
	FDefaultConstructedPropertyElement key{keyProp};
	TSetPropertyValueParams params{keyProp, key.GetObjAddress(),0, key_o};
	UnrealLua::PropertyHelper::SetPropertyValue_Direct(params);
	
	FDefaultConstructedPropertyElement val{valueProp};
	helper.AddPair(&key, val.GetObjAddress());
	return sol::nil;
}

void FLuaMap::Lua_Add(sol::object key, sol::object value, sol::this_state lua)
{
	if(key == sol::nil)
	{
		return;
	}
	if(value == sol::nil)
	{
		return;
	}
	FProperty* keyProp = this->GetKeyProperty();
	FProperty* valueProp = this->GetValueProperty();
	if(!UnrealLua::FMapPropertyDescr::IsCompatibleTypePair(keyProp, valueProp, key, value))
	{
		return;
	}
	FScriptMapHelper helper = CreateHelperFormInnerProperties(keyProp, valueProp, this->GetScriptMap());

	FDefaultConstructedPropertyElement tempKey(keyProp);
	FDefaultConstructedPropertyElement tempValue(valueProp);

	TSetPropertyValueParams keyParams{keyProp, tempKey.GetObjAddress(),0, key};
	UnrealLua::PropertyHelper::SetPropertyValue_Direct(keyParams);
	TSetPropertyValueParams valParams{valueProp, tempValue.GetObjAddress(),0, value};
	UnrealLua::PropertyHelper::SetPropertyValue_Direct(valParams);

	helper.AddPair(tempKey.GetObjAddress(), tempValue.GetObjAddress());
}

sol::object FLuaMap::Lua_Find(sol::object key, sol::this_state lua)
{
	if(key == sol::nil)
	{
		return sol::nil;
	}
	FProperty* keyProp = this->GetKeyProperty();
	FProperty* valueProp = this->GetValueProperty();
	if(!UnrealLua::FMapPropertyDescr::IsCompatibleKey(keyProp, key))
	{
		return sol::nil;
	}
	FScriptMapHelper helper = CreateHelperFormInnerProperties(keyProp, valueProp, this->GetScriptMap());

	FDefaultConstructedPropertyElement tempKey(keyProp);
	
	TSetPropertyValueParams keyparams{keyProp, tempKey.GetObjAddress(), 0, key};
	UnrealLua::PropertyHelper::SetPropertyValue_Direct(keyparams);
	
	uint8* valuePtr= helper.FindValueFromHash(tempKey.GetObjAddress());//helper.FindMapPairPtrWithKey(key);//helper.FindMapPairPtrFromHash(key);
	if(!valuePtr)
	{
		return sol::nil;
	}
	FGetPropertyValueParams getValParams{helper.ValueProp, valuePtr,0, lua};
	return UnrealLua::PropertyHelper::GetPropertyValue(getValParams);
}

bool FLuaMap::Lua_Remove(sol::object key, sol::this_state lua)
{
	if(key == sol::nil)
	{
		return false;
	}
	FProperty* keyProp = this->GetKeyProperty();
	FProperty* valueProp = this->GetValueProperty();
	if(!UnrealLua::FMapPropertyDescr::IsCompatibleKey(keyProp, key))
	{
		return false;
	}
	FScriptMapHelper helper = CreateHelperFormInnerProperties(keyProp, valueProp, this->GetScriptMap());

	FDefaultConstructedPropertyElement tempKey(keyProp);
	
	TSetPropertyValueParams keyparams{keyProp, tempKey.GetObjAddress(), 0, key};
	UnrealLua::PropertyHelper::SetPropertyValue_Direct(keyparams);
	
	return helper.RemovePair(tempKey.GetObjAddress());
}

bool FLuaMap::Lua_Contains(sol::object key, sol::this_state lua)
{
	if(key == sol::nil)
	{
		return false;
	}
	FProperty* keyProp = this->GetKeyProperty();
	FProperty* valueProp = this->GetValueProperty();
	if(!UnrealLua::FMapPropertyDescr::IsCompatibleKey(keyProp, key))
	{
		return false;
	}
	FScriptMapHelper helper = CreateHelperFormInnerProperties(keyProp, valueProp, this->GetScriptMap());

	FDefaultConstructedPropertyElement tempKey(keyProp);
	
	TSetPropertyValueParams keyparams{keyProp, tempKey.GetObjAddress(), 0, key};
	UnrealLua::PropertyHelper::SetPropertyValue_Direct(keyparams);

	uint8* pair = helper.FindMapPairPtrWithKey(tempKey.GetObjAddress());//helper.FindMapPairPtrFromHash(key);

	return pair != nullptr;
}

void FLuaMap::Lua_Empty()
{
	FProperty* keyProp = this->GetKeyProperty();
	FProperty* valueProp = this->GetValueProperty();
	FScriptMapLayout MapLayout = FScriptMap::GetScriptLayout(keyProp->GetElementSize(), keyProp->GetMinAlignment(), valueProp->GetElementSize(), valueProp->GetMinAlignment());
	FScriptMap* scriptMap = this->GetScriptMap();
	for (int32 index = scriptMap->Num() - 1; index >= 0; index--)
	{
		scriptMap->RemoveAt(index,MapLayout);
	}
}

sol::table FLuaMap::Lua_ToTable(sol::this_state lua_s)
{
	int32 num = this->GetScriptMap()->Num();
	sol::state_view lua = lua_s;
	sol::table results = lua.create_table();
	FProperty* keyProp = this->GetKeyProperty();
	FProperty* valueProp = this->GetValueProperty();
	FScriptMapHelper helper = CreateHelperFormInnerProperties(keyProp, valueProp, this->GetScriptMap());

	for(FScriptMapHelper::FIterator it = helper.CreateIterator(); it; ++it)
	{
		int32 index = it.GetInternalIndex();
		//uint8* pair = helper.GetPairPtr(index);
		uint8* valuePtr = helper.GetValuePtr(index);
		uint8* keyPtr = helper.GetKeyPtr(index);

		FGetPropertyValueParams getValParams{helper.ValueProp, valuePtr,0, lua_s};
		sol::object value = UnrealLua::PropertyHelper::GetPropertyValue(getValParams);
		FGetPropertyValueParams getKeyParams{helper.KeyProp, keyPtr,0, lua_s};
		sol::object key = UnrealLua::PropertyHelper::GetPropertyValue(getKeyParams);
		results[key] = value;
	}
	return results;
}

void FLuaMap::Copy(FScriptMap* dest, FProperty* keyProp, FProperty* valueProp, FScriptMap* src)
{
	FScriptMapHelper SrcMapHelper = FScriptMapHelper::CreateHelperFormInnerProperties (keyProp, valueProp, src);
	FScriptMapHelper DestMapHelper = FScriptMapHelper::CreateHelperFormInnerProperties (keyProp, valueProp, dest);

	int32 Num = SrcMapHelper.Num();
	DestMapHelper.EmptyValues(Num);

	if (Num == 0)
	{
		return;
	}

	for (int32 SrcIndex = 0; Num; ++SrcIndex)
	{
		if (SrcMapHelper.IsValidIndex(SrcIndex))
		{
			int32 DestIndex = DestMapHelper.AddDefaultValue_Invalid_NeedsRehash();

			uint8* SrcData  = SrcMapHelper .GetPairPtr(SrcIndex);
			uint8* DestData = DestMapHelper.GetPairPtr(DestIndex);

			keyProp  ->CopyCompleteValue_InContainer(DestData, SrcData);
			valueProp->CopyCompleteValue_InContainer(DestData, SrcData);

			--Num;
		}
	}

	DestMapHelper.Rehash();
}

FLuaMap FLuaMap::Lua_Copy() const
{
	FProperty* keyProp = UnrealLua::PropertyHelper::CreateNewProperty(this->GetKeyProperty(),  UnrealLua::PropertyNames::NAME_UnrealLuaMapKeyProperty);
	FProperty* valueProp = UnrealLua::PropertyHelper::CreateNewProperty(this->GetValueProperty(),  UnrealLua::PropertyNames::NAME_UnrealLuaMapValueProperty);
	verify(keyProp != nullptr);
	verify(valueProp != nullptr);
	return FLuaMap{keyProp, valueProp, this->GetScriptMap(), false};
}

bool FLuaMap::IsValid() const
{
	return this->GetScriptMap() != nullptr && this->Properties != nullptr;
}

FProperty* FLuaMap::GetKeyProperty() const
{
	return this->Properties->Key;
}

FProperty* FLuaMap::GetValueProperty() const
{
	return this->Properties->Value;
}

sol::object FLuaMap::__Index(sol::object key, sol::this_state lua)
{
	return this->Lua_Find(key, lua);
	/*
	int32 index = key.as<int32>();
	index--; //Lua Index correction
	return this->__IndexInternal(index, lua);
	*/
}

sol::object FLuaMap::__IndexInternal(int32 index, sol::this_state lua)
{
	FProperty* keyProp = this->GetKeyProperty();
	FProperty* valueProp = this->GetValueProperty();
	FScriptMapHelper helper = CreateHelperFormInnerProperties(keyProp, valueProp, this->GetScriptMap());

	if(!helper.IsValidIndex(index))
	{
		return {};
	}

	uint8* valPtr = helper.GetValuePtr(index);
	
	if(!valPtr)
	{
		return {};
	}
	FGetPropertyValueParams getValueParams{helper.ValueProp, valPtr,0, lua};
	return UnrealLua::PropertyHelper::GetPropertyValue(getValueParams);
}

std::tuple<sol::object, sol::object> FLuaMap::__IndexPairInternal(int32 index, sol::this_state lua)
{
	FProperty* keyProp = this->GetKeyProperty();
	FProperty* valueProp = this->GetValueProperty();
	FScriptMapHelper helper = CreateHelperFormInnerProperties(keyProp, valueProp, this->GetScriptMap());

	if(!helper.IsValidIndex(index))
	{
		return {sol::nil, sol::nil};
	}
	//TScriptContainerIterator<FScriptMapHelper> it = helper.CreateIterator(index);

	//int32 trueIndex = it.GetLogicalIndex();
	uint8* valuePtr = helper.GetValuePtr(index);
	uint8* keyPtr = helper.GetKeyPtr(index);
	
	if(!valuePtr || !keyPtr)
	{
		return {sol::nil, sol::nil};
	}

	FGetPropertyValueParams getKeyParams{helper.KeyProp, keyPtr,0, lua};
	sol::object key = UnrealLua::PropertyHelper::GetPropertyValue(getKeyParams);
	FGetPropertyValueParams getValParams{helper.ValueProp, valuePtr,0, lua};
	sol::object value = UnrealLua::PropertyHelper::GetPropertyValue(getValParams);
	
	return {key, value};
}

int FLuaMap::__next(lua_State* L)
{
	// this gets called
	// to start the first iteration, and every
	// iteration there after
	// the state you passed in pairs is argument 1
	// the key value is argument 2
	// we do not care about the key value here
	lua_iterator_state& it_state = sol::stack::get<sol::user<lua_iterator_state>>(L, 1);
	auto& it = it_state.it;
	if (it.GetLogicalIndex() == it_state.last.GetLogicalIndex()) {
		return sol::stack::push(L, sol::lua_nil);
	}
	int32 iteratorindex = it.GetLogicalIndex();
	// 2 values are returned (pushed onto the stack):
	// the key and the value
	// the state is left alone

	auto [key, value] = it_state.owner.__IndexPairInternal(iteratorindex, L);
	
	int pushed = sol::stack::push(L, key);
	pushed += sol::stack::push(L, value);
	++it;
	return pushed;
}

int FLuaMap::__pairs(lua_State* L)
{
	FLuaMap& mt = sol::stack::get<FLuaMap&>(L, 1);
	lua_iterator_state it_state(mt);
	// pairs expects 3 returns:
	// the "next" function on how to advance,
	// the "table" itself or some state,
	// and an initial key value (can be nil)

	// next function controls iteration
	int pushed = sol::stack::push(L, &FLuaMap::__next);
	pushed += sol::stack::push<sol::user<lua_iterator_state>>(L, std::move(it_state));
	pushed += sol::stack::push(L, sol::lua_nil);
	return pushed;
}

bool FLuaMap::OwnsMemory() const
{
	return this->Data.IsType<FLuaScriptMap*>();
}

void FLuaMap::AddRef()
{
	verify(this->Data.IsType<FLuaScriptMap*>());
	this->Data.Get<FLuaScriptMap*>()->AddRef();
}

int32 FLuaMap::RemoveRef()
{
	if(this->OwnsMemory())
	{
		if(this->Data.Get<FLuaScriptMap*>()->RemoveRef() == 0)
		{
			this->Lua_Empty();
			delete this->Data.Get<FLuaScriptMap*>();
			this->Data.Emplace<std::nullptr_t>();
		}
	}
	return -1;
}

bool FLuaMap::IsUPropertyReference() const
{
	return !this->OwnsMemory();
}