#pragma once
#include "LuaCoroutine.h"
#include "LuaFunction.h"
#include "LuaTable.h"
#include "LuaValueType.h"
#include "Config/UnrealLuaConfig.h"
#include "Utility/LuaLogMacros.h"
#include "Config/UnrealLuaConstants.h"
#include "Replication/LuaRPCFunctions.h"
#include "LuaTypes/LuaArray.h"
#include "LuaTypes/LuaDelegate.h"
#include "LuaTypes/LuaInstancedStruct.h"
#include "LuaTypes/LuaMap.h"
#include "LuaTypes/LuaScriptStruct.h"
#include "LuaTypes/LuaSet.h"
#include "LuaTypes/LuaEnum.h"
#include "LuaTypes/LuaPrimitives.h"
#include "LuaTypes/LuaSharedStruct.h"
#include "LuaTypes/LuaUClass.h"
#include "LuaTypes/LuaUStruct.h"
#include "LuaValue/TLuaVariant.h"
#include "Reflection/FunctionDescr.h"
#include "Reflection/PropertyHelper.h"
#include "sol/sol.hpp"
#include "UObject/ObjectPtr.h"
#include "UObject/Object.h"
#include "LuaTypes/LuaLightUserdata.h"
#include "LuaTypes/LuaMulticastDelegate.h"
#include "LuaValue.generated.h"

struct FSetLuaScriptUObjectMemberPropertyWrapperParams;


enum ESetLuaValueResult : uint8
{
	Success = 0,
	Error = 1 << 0,
	TickFunctionModified = 1 << 1
};
ENUM_CLASS_FLAGS(ESetLuaValueResult)


struct UNREALLUA_API FLuaUFunctionReference
{
	FLuaUFunctionReference(const FFunctionDescr* func, sol::function luaFunc);
	FLuaUFunctionReference(const FFunctionDescr* func);
	bool operator==(const FLuaUFunctionReference& other) const
	{
		return this->Func == other.Func;	
	}


	sol::function LuaFunc; //16
	const FFunctionDescr* Func; //24
	
	int PushValue(sol::this_state lua) const;
	sol::object GetValue(sol::this_state lua) const;
};

struct UNREALLUA_API FPropertyReferenceWrapper
{
	TObjectPtr<UObject> Owner;
	FProperty* Prop;
	
	template<typename LUAOBJ>
	void SetUObjectMemberProperty(UObject* owner, const LUAOBJ& newValue)
	{
		FProperty* prop = this->Prop;
		TSetPropertyValueParams params{prop, owner, 0, newValue};
		UnrealLua::PropertyHelper::SetPropertyValue_InContainer(params);
		UnrealLua::PropertyHelper::HandleSetPropertyNetBehavior(owner, prop);
	}
	
	void SetUObjectMemberProperty(UObject* owner, const FProperty* funcprop, const void* inputValueAddress)
	{
		FProperty* memberprop = this->Prop;
		if(memberprop->SameType(funcprop))
		{
			memberprop->SetValue_InContainer(owner, inputValueAddress);
		}
		else
		{
			memberprop->InitializeValue_InContainer(owner);
		}
		UnrealLua::PropertyHelper::HandleSetPropertyNetBehavior(owner, memberprop);
	}

	//sol::object LuaReference = {};
};

namespace UnrealLua
{
	enum DeadValue
	{
		Dead
	};
}

//Note : FName is 12 byte in Debug build, 8 byte in Shipping build
typedef TLuaVariant<
	std::nullptr_t,
	sol::nil_t,
	FPropertyReferenceWrapper, 
	FLuaPrimitiveCPPType,
	bool,
	int64,	
	double,
	std::string,
	FVector2D,
	FVector,
	FRotator,
	TObjectPtr<UObject>,
	TLuaVariantPtr<FTransform>,
	FLuaUClass,
	FLuaUStruct,
	FLuaUEnumEntry*,
	FLuaUFunctionReference,
	FLuaScriptStruct,
	FLuaSharedStruct,
	FLuaInstancedStruct,
	FLuaArray,
	FLuaMap,
	FLuaSet,
	sol::function,
	FLuaRPCFunction,
	FLuaFunctionHandle,  
	sol::table,
	FLuaTableHandle,
	FLuaCoroutineHandle,
	FLuaScriptDelegate,
	FLuaScriptMulticastDelegate,
	UnrealLua::DeadValue
> LuaValueData;

static_assert(sizeof(LuaValueData) <= 40);



USTRUCT(BlueprintType)
struct UNREALLUA_API FLuaValue
{
	GENERATED_BODY()
	FLuaValue();
	//nullptr_t means "uninitialized"
	FLuaValue(std::nullptr_t);
	FLuaValue(sol::nil_t);
	FLuaValue(const sol::stack_object& obj);
	FLuaValue(const sol::object& obj);
	FLuaValue(const sol::object& obj, const sol::string_view& key);
	//FLuaValue(const sol::object& obj, ELuaValueType Type, const sol::string_view& key);
	FLuaValue(FFunctionDescr* func);
	FLuaValue(FFunctionDescr* func, sol::function luaFunc);
	FLuaValue(UObject* container, FProperty* prop);
	FLuaValue(UObject* value);
	FLuaValue(FProperty* prop, const void* inputValueAddress);
	FLuaValue(const FSetLuaScriptUObjectMemberPropertyWrapperParams& params);
	FLuaValue(const FSetLuaScriptUObjectMemberPropertyWrapperParams& params, const sol::function& func);
	FLuaValue(const FSetLuaScriptUObjectMemberPropertyWrapperParams& params, const sol::object& obj);
	FLuaValue(FLuaValue&& other) noexcept;
	FLuaValue(const FLuaValue& other);
	~FLuaValue();
	bool operator==(const FLuaValue& other) const;
	bool Equals(const FLuaValue& other) const;
	FLuaValue& operator=(const FLuaValue& other);

	bool operator!=(const FLuaValue& other) const
	{
		return !(*this == other);
	}

	FLuaValue MakeCopy(bool copyComplexTypes = true, bool getNotReplicatedAsNil = false) const;

	template<typename T>
	inline bool IsType() const
	{
		return this->GetData().IsType<T>();
	}

	template<typename T>
	inline const T& Get() const
	{
		return (this)->GetData().Get<T>();
	}
	
	template<typename T>
	inline T& GetMutable() const
	{
		return (this)->GetData().Get<T>();
	}
	
	bool IsPropertyReference() const;
	bool IsUFunctionReference() const;
	bool IsPropertyOrUFunctionReference() const;
	
	
	SIZE_T GetTypeIndex() const
	{
		return const_cast<FLuaValue*>(this)->Data.GetIndex();
	}
	
	bool CanBeReplicated() const;

	//Used only by LuaScriptValue::SetScriptValue
	template<typename T>
	ESetLuaValueResult SetValue(const sol::basic_object<T>& newValue, const sol::string_view& key);
	//Used by 
	//- LuaScriptValue::SetScriptValue
	//- UUnrealLuaUtility::execMakeLuaValue
	ESetLuaValueResult SetValue(const FProperty* prop, const void* inputValueAddress);
	//used by 
	//- LuaScriptValue::ChangeToPropertyReference
	//- FLuaScriptValue::SetScriptValue(const FLuaValue& source)
	//- FStructPropertyDescr::SetLuaValuePropertyValue
	ESetLuaValueResult SetValue(const FLuaValue& source);
	void SetDead();
	
	void MarkAsScriptValue();
	bool IsScriptValue() const;
	void ClearIsScriptValue();
	
	void ConvertLuaObjectsToHandles();

	template<typename T, typename ...Args>
	inline void Emplace(Args&&... args)
	{
		this->GetData().Emplace<T>(std::forward<Args>(args)...);
	}
private:
	template<typename T> 
	ESetLuaValueResult SetLuaValue(const sol::basic_object<T>& newValue, const std::string_view& key);
	ESetLuaValueResult SetValueFromPropertySource(const FProperty* prop, const void* inputValueAddress);
	
	
	template<typename T, typename ...Args>
	inline T& Emplace_GetRef(Args&&... args)
	{
		this->GetData().Emplace<T>(std::forward<Args>(args)...);
		return this->GetData().Get<T>();
	}

	LuaValueData& GetData() const
	{
		return const_cast<FLuaValue*>(this)->Data;
	}
public:
	
	bool IsEqualToPropertyValue(FProperty* prop, void* valueAddress) const;

	FString ToValueString() const;
	FString ToStringForStructBuilderEditor() const;
	FString GetTypeString() const;
	FString GetLuaSyntaxValidValueString(bool containersAsTable) const;

	//Set invalid UObject references to nil
	//@return bool - Whether object was set to nil
	bool PostGCHandleUObjectPtrs();
	void CleanUpForLuaState(sol::this_state lua);

	FLuaDelegateHandle AddDelegateListener(const FLuaDelegate& LuaDelegate);
	FLuaDelegateHandle AddMulticastDelegateListener(const FLuaDelegate& delToAdd);
	bool UnbindMulticastDelegateListener(const FLuaDelegate& delToRemove);
	bool UnbindMulticastDelegateListener(FLuaDelegateHandle handle);
	bool BroadcastLuaDelegate(const TArray<FLuaValue>& args);

public:
	sol::object GetValue(sol::this_state lua) const;
	int PushValue(sol::this_state lua) const;
	ESetLuaValueResult WriteValueToPropertyMemoryAddress_WithPropertyTypeCheck(FProperty* targetPropertyToWriteTo, void* memAddressToWriteTo) const;
	
	ELuaValueType GetType() const;
	bool IsNil() const
	{
		return this->GetData().IsType<sol::nil_t>() || this->GetData().IsType<std::nullptr_t>();
	}
	bool IsDead() const
	{
		return this->GetData().IsType<UnrealLua::DeadValue>();
	}
	bool IsInitialized() const
	{
		return !this->GetData().IsType<std::nullptr_t>();
	}
	bool HasLuaReference() const
	{
		return this->GetData().IsType<FLuaTableHandle>() || this->GetData().IsType<FLuaFunctionHandle>();
	}
	FName GetKeyName() const;

	bool AddStructReferencedObjects(FReferenceCollector& collector);
	bool AddStructReferencedObjects(FReferenceCollector& collector, const TObjectPtr<UObject>& owner);
private:
	ESetLuaValueResult HandleSettingFuncReference(const FLuaValue& other) const;
	ESetLuaValueResult HandleSettingFuncReference(const sol::nil_t nil) const;
	ESetLuaValueResult HandleSettingFuncReference(const sol::stack_object& newValue) const;
	ESetLuaValueResult HandleSettingFuncReference(const sol::object& newValue) const;
	ESetLuaValueResult HandleSettingFuncReference(const sol::function& newValue) const;
public:
	bool NetSerialize(FArchive& ar, UPackageMap* map, bool& bOutSuccess);
private:
	LuaValueData Data = {};
};

static_assert(sizeof(FLuaValue) == 40);

template<>
struct TStructOpsTypeTraits<FLuaValue> : public TStructOpsTypeTraitsBase2<FLuaValue>
{
	enum
	{
		WithZeroConstructor = true,
		WithCopy = true,
		WithNetSerializer = true,
		WithAddStructReferencedObjects = true
	};
};
/*
#if UE_BUILD_SHIPPING
static_assert(sizeof(FLuaValue) == 40);
#else
static_assert(sizeof(FLuaValue) == 40);
#endif
*/

inline FLuaValue& FLuaValue::operator=(const FLuaValue& other)
{
	if(this != &other)
	{
		bool isScriptValue = this->IsScriptValue();
		this->SetValue(other);
		if (isScriptValue)
		{
			this->MarkAsScriptValue();
		}
	}
	return *this;
}

inline FLuaValue::FLuaValue()
{
	this->GetData().Emplace<std::nullptr_t>();
}

inline FLuaValue::FLuaValue(std::nullptr_t)
{
	this->GetData().Emplace<std::nullptr_t>();
}

inline FLuaValue::FLuaValue(sol::nil_t)
{
	this->GetData().Emplace<sol::nil_t>();
}

inline FLuaValue::FLuaValue(const sol::stack_object& obj)
{
	const std::string emptykey = "";
	this->SetLuaValue(obj, emptykey);	
}

inline FLuaValue::FLuaValue(const sol::object& obj)
{
	const std::string emptykey = "";
	this->SetLuaValue(obj, emptykey);
}

inline FLuaValue::FLuaValue(const sol::object& obj, const sol::string_view& key)
{
	this->SetLuaValue(obj, key);
}
/*
inline FLuaValue::FLuaValue(const sol::object& obj, ELuaValueType Type, const sol::string_view& key)
{
	if(!obj.valid())
	{
		this->GetData().Emplace<sol::nil_t>();
	}
	this->SetLuaValue(obj, Type, key);
}
*/

inline FLuaValue::FLuaValue(FFunctionDescr* func)
{
	this->GetData().Emplace<FLuaUFunctionReference>(func, sol::nil);
}

inline FLuaValue::FLuaValue(FFunctionDescr* func, sol::function luaFunc)
{
	this->GetData().Emplace<FLuaUFunctionReference>(func, luaFunc);
}

inline FLuaValue::FLuaValue(UObject* container, FProperty* prop)
{
	verify(container->GetClass()->IsChildOf(prop->GetOwnerClass()))
	this->GetData().Emplace<FPropertyReferenceWrapper>(container, prop);
}

inline FLuaValue::FLuaValue(UObject* value)
{
	if (IsValid(value))
	{
		this->GetData().Emplace<TObjectPtr<UObject>>(value);	
	}
	else
	{
		this->GetData().Emplace<sol::nil_t>();
	}
}

inline FLuaValue::FLuaValue(FProperty* prop, const void* inputValueAddress)
{
	this->GetData().Emplace<std::nullptr_t>();
	this->SetValueFromPropertySource(prop, inputValueAddress);
}

inline FLuaValue::FLuaValue(const FSetLuaScriptUObjectMemberPropertyWrapperParams& params)
{
	UObject* owner = params.ScriptOwner;
	if(params.PropMapping.IsFunction())
	{
		this->GetData().Emplace<FLuaUFunctionReference>(params.PropMapping.GetFunction());
	}
	else
	{
		FProperty* prop = params.PropMapping.GetProperty();
		this->GetData().Emplace<FPropertyReferenceWrapper>(owner, prop);	
	}
}

inline FLuaValue::FLuaValue(const FSetLuaScriptUObjectMemberPropertyWrapperParams& params, const sol::function& luaFunc)
{
	verify(params.PropMapping.IsFunction());
	this->GetData().Emplace<FLuaUFunctionReference>(params.PropMapping.GetFunction(), luaFunc);
}

inline FLuaValue::FLuaValue(const FSetLuaScriptUObjectMemberPropertyWrapperParams& params, const sol::object& obj)
	: FLuaValue(params)
{
	verify(params.PropMapping.IsProperty());
	TSetPropertyValueParams setparams{params.PropMapping.GetProperty(), params.ScriptOwner, 0, obj};
	UnrealLua::PropertyHelper::SetPropertyValue_InContainer(setparams);
}

inline FLuaValue::FLuaValue(FLuaValue&& other) noexcept
	: Data(MoveTemp(other.Data))
{
	other.GetData().Emplace<sol::nil_t>();	
}

inline FLuaValue::FLuaValue(const FLuaValue& other)
	: Data(other.Data)
{
}

inline FLuaValue::~FLuaValue()
{
	this->GetData().Emplace<UnrealLua::DeadValue>();
}

inline bool FLuaValue::Equals(const FLuaValue& other) const
{
	if(this->GetData().GetIndex() != other.GetData().GetIndex())
	{
		if(this->IsType<FPropertyReferenceWrapper>() || other.IsType<FPropertyReferenceWrapper>())
		{
			FProperty* prop = nullptr;
			UObject* propOwner = nullptr;
			const FLuaValue* valueVal = nullptr;
			if(this->IsType<FPropertyReferenceWrapper>())
			{
				prop = this->Get<FPropertyReferenceWrapper>().Prop;
				propOwner = this->Get<FPropertyReferenceWrapper>().Owner;
				valueVal = &other;
			}
			else
			{
				prop = other.Get<FPropertyReferenceWrapper>().Prop;
				propOwner = other.Get<FPropertyReferenceWrapper>().Owner;
				valueVal = this;
			}
			verify(prop != nullptr);
			verify(IsValid(propOwner));

			void* memData = prop->ContainerPtrToValuePtr<void>(propOwner);

			return valueVal->IsEqualToPropertyValue(prop, memData);
		}
		return false;
	}

	//must be same type
	switch(this->GetData().GetIndex())
	{
		case LuaValueData::IndexOfType<FPropertyReferenceWrapper>():
		{
			//other must also be FProperty*
			FProperty* myProp = this->Get<FPropertyReferenceWrapper>().Prop;
			UObject* myowner = this->Get<FPropertyReferenceWrapper>().Owner;
			FProperty* otherProp = other.Get<FPropertyReferenceWrapper>().Prop;
			UObject* otherOwner = other.Get<FPropertyReferenceWrapper>().Owner;
			if(myowner == otherOwner && myProp == otherProp)
			{
				return true;
			}
			if(!myProp->SameType(otherProp))
			{
				return false;
			}
			return myProp->Identical(myProp->ContainerPtrToValuePtr<void>(myowner), otherProp->ContainerPtrToValuePtr<void>(otherOwner));
		}
	case LuaValueData::IndexOfType<std::nullptr_t>(): [[fallthrough]];
	case LuaValueData::IndexOfType<sol::nil_t>():
		{
			return true;
		}
	case LuaValueData::IndexOfType<bool>():
		{
			const bool& ref = this->Get<bool>();
			const bool& oref = other.Get<bool>();
			return ref == oref;
		}
	case LuaValueData::IndexOfType<int64>():
		{
			const int64& ref = this->Get<int64>();
			const int64& oref = other.Get<int64>();
			return ref == oref;
		}
	case LuaValueData::IndexOfType<double>():
		{
			const double& ref = this->Get<double>();
			const double& oref = other.Get<double>();
			return FMath::IsNearlyEqual(ref, oref);
		}
	case LuaValueData::IndexOfType<std::string>():
		{
			const std::string& ref = this->Get<std::string>();
			const std::string& oref = other.Get<std::string>();
			return ref == oref;
		}
	case LuaValueData::IndexOfType<FVector2D>():
		{
			const FVector2D& ref = this->Get<FVector2D>();
			const FVector2D& oref = other.Get<FVector2D>();
			return ref == oref;
		}
	case LuaValueData::IndexOfType<FVector>():
		{
			const FVector& ref = this->Get<FVector>();
			const FVector& oref = other.Get<FVector>();
			return ref == oref;
		}
	case LuaValueData::IndexOfType<FRotator>():
		{
			const FRotator& ref = this->Get<FRotator>();
			const FRotator& oref = other.Get<FRotator>();
			return ref == oref;
		}
	case LuaValueData::IndexOfType<TLuaVariantPtr<FTransform>>():
		{
			//Can just overwrite data
			const FTransform* ref = this->Get<TLuaVariantPtr<FTransform>>().Get();
			const FTransform* oref = other.Get<TLuaVariantPtr<FTransform>>().Get();
			return ref->Equals(*oref);
		}
	case LuaValueData::IndexOfType<TObjectPtr<UObject>>():
		{
			//Can just overwrite data
			const TObjectPtr<UObject>& ref = this->Get<TObjectPtr<UObject>>();
			const TObjectPtr<UObject>& oref = other.Get<TObjectPtr<UObject>>();
			return ref == oref;
		}
	case LuaValueData::IndexOfType<FLuaTableHandle>():
		{
			const FLuaTableHandle& thisHandle = this->Get<FLuaTableHandle>();
			const FLuaTableHandle& otherHandle = other.Get<FLuaTableHandle>();
			return thisHandle == otherHandle;
		}
	case LuaValueData::IndexOfType<FLuaUEnumEntry*>():
		{
			//Can just overwrite data
			const FLuaUEnumEntry* ref = this->Get<FLuaUEnumEntry*>();
			const FLuaUEnumEntry* oref = other.Get<FLuaUEnumEntry*>();
			return *ref == *oref;
		}
	case LuaValueData::IndexOfType<FLuaUStruct>():
		{
			//Can just overwrite data
			const FLuaUStruct& ref = this->Get<FLuaUStruct>();
			const FLuaUStruct& oref = other.Get<FLuaUStruct>();
			return ref == oref;
		}
	//case LuaValueData::IndexOfType<FMulticastDelegatePropertyProxy>(): [[fallthrough]];
	//case LuaValueData::IndexOfType<FSingleDelegatePropertyProxy>(): [[fallthrough]];
	case LuaValueData::IndexOfType<FLuaUClass>():
		{
			//Can just overwrite data
			const FLuaUClass& ref = this->Get<FLuaUClass>();
			const FLuaUClass& oref = other.Get<FLuaUClass>();
			return ref == oref;
		}
	case LuaValueData::IndexOfType<FLuaArray>():
		{
			//Can just overwrite data
			const FLuaArray& arr = this->Get<FLuaArray>();
			const FLuaArray& oarr = other.Get<FLuaArray>();
			return arr == oarr;
		}
	case LuaValueData::IndexOfType<FLuaMap>():
		{
			//Can just overwrite data
			const FLuaMap& map = this->Get<FLuaMap>();
			const FLuaMap& omap = other.Get<FLuaMap>();
			return map == omap;	
		}
	case LuaValueData::IndexOfType<FLuaSet>():
		{
			const FLuaSet& set = this->GetData().Get<FLuaSet>();
			const FLuaSet& oset = other.GetData().Get<FLuaSet>();
			return set == oset;
		}
	case LuaValueData::IndexOfType<FLuaSharedStruct>():
		{
			const FLuaSharedStruct& strct = this->Get<FLuaSharedStruct>();
			const FLuaSharedStruct& ostrct = other.Get<FLuaSharedStruct>();
			return strct.GetScriptStruct() == ostrct.GetScriptStruct() && strct.GetScriptStruct()->CompareScriptStruct(strct.GetMemory(), ostrct.GetMemory(), 0);	
		}
	case LuaValueData::IndexOfType<FLuaInstancedStruct>():
		{
			const FLuaInstancedStruct& strct = this->Get<FLuaInstancedStruct>();
			const FLuaInstancedStruct& ostrct = other.Get<FLuaInstancedStruct>();
			return strct.GetScriptStruct() == ostrct.GetScriptStruct() && strct.GetScriptStruct()->CompareScriptStruct(strct.GetMemory(), ostrct.GetMemory(), 0);
		}
	case LuaValueData::IndexOfType<FLuaScriptStruct>():
		{
			//Can just overwrite data
			const FLuaScriptStruct& strct = this->Get<FLuaScriptStruct>();
			const FLuaScriptStruct& ostrct = other.Get<FLuaScriptStruct>();
			return strct.GetScriptStruct() == ostrct.GetScriptStruct() && strct.GetScriptStruct()->CompareScriptStruct(strct.GetMemory(), ostrct.GetMemory(), 0);
		}
	case LuaValueData::IndexOfType<FLuaRPCFunction>():
		{
			//Can just overwrite data
			const FLuaRPCFunction& ref = this->Get<FLuaRPCFunction>();
			const FLuaRPCFunction& oref = other.Get<FLuaRPCFunction>();
			return ref.FuncName == oref.FuncName;
		}
	case LuaValueData::IndexOfType<FLuaFunctionHandle>():
		{
			const FLuaFunctionHandle& handle = this->Get<FLuaFunctionHandle>(); 
			const FLuaFunctionHandle& otherHandle = other.Get<FLuaFunctionHandle>();
			const sol::function& ref = handle.GetFunction();
			return handle.GetFunction() == otherHandle.GetFunction();
		}
	case LuaValueData::IndexOfType<FLuaUFunctionReference>():
		{
			const FLuaUFunctionReference& ref = this->Get<FLuaUFunctionReference>();
			const FLuaUFunctionReference& oref = other.Get<FLuaUFunctionReference>();
			return ref.Func == oref.Func;
		}
	}	
	return false;
}

inline bool FLuaValue::operator==(const FLuaValue& other) const
{
	return this->Equals(other);
}

/**
	@param copyComplexTypes			If true, duplicate complex structs
									If false, create a shared reference to the following data types, with the same memory as what is currently in this FLuaValue.
									Note that FLuaInstancedStruct always will get copied. The folloing datatypes can have a shared reference:
									- FLuaArray
									- FLuaSet
									- FLuaMap
									- FScriptStruct
									- FLuaSharedStruct
									
	@param getNotReplicatedAsNil	If true and the type contained in this FLuaValue is not able to be replicated, return a nil-FLuaValue
	@return FLuaValue				LuaValue with either a copy or shared ref of the data
*/
inline FLuaValue FLuaValue::MakeCopy(bool copyComplexTypes, bool getNotReplicatedAsNil) const
{
	FLuaValue ret{nullptr};
	if(getNotReplicatedAsNil)
	{
		if(!this->CanBeReplicated())
		{
			//early out if a value can not be replicated but is required to do so
			return ret;
		}		
	}

	switch(this->GetData().GetIndex())
	{
	case LuaValueData::IndexOfType<FPropertyReferenceWrapper>():
		{
			const FPropertyReferenceWrapper& wrapper = this->GetData().Get<FPropertyReferenceWrapper>();
			const FProperty* prop = wrapper.Prop;
			if(IsValid(wrapper.Owner))
			{
				ret.SetValueFromPropertySource(prop, prop->ContainerPtrToValuePtr<void>(wrapper.Owner));
			}
			break;
		}
	case LuaValueData::IndexOfType<std::nullptr_t>(): [[fallthrough]];
	case LuaValueData::IndexOfType<sol::nil_t>(): [[fallthrough]];
	case LuaValueData::IndexOfType<bool>(): [[fallthrough]];
	case LuaValueData::IndexOfType<int64>(): [[fallthrough]];
	case LuaValueData::IndexOfType<double>(): [[fallthrough]];
	case LuaValueData::IndexOfType<std::string>(): [[fallthrough]];
	case LuaValueData::IndexOfType<FVector2D>(): [[fallthrough]];
	case LuaValueData::IndexOfType<FVector>(): [[fallthrough]];
	case LuaValueData::IndexOfType<FRotator>(): [[fallthrough]];
	case LuaValueData::IndexOfType<TLuaVariantPtr<FTransform>>():  [[fallthrough]];
	case LuaValueData::IndexOfType<TObjectPtr<UObject>>(): [[fallthrough]];
	case LuaValueData::IndexOfType<FLuaUEnumEntry*>(): [[fallthrough]];
	case LuaValueData::IndexOfType<FLuaUStruct>(): [[fallthrough]];
	case LuaValueData::IndexOfType<FLuaTableHandle>(): [[fallthrough]];
	case LuaValueData::IndexOfType<FLuaFunctionHandle>(): [[fallthrough]];
	case LuaValueData::IndexOfType<FLuaCoroutineHandle>(): [[fallthrough]];
	//case LuaValueData::IndexOfType<FMulticastDelegatePropertyProxy>(): [[fallthrough]];
	//case LuaValueData::IndexOfType<FSingleDelegatePropertyProxy>(): [[fallthrough]];
	case LuaValueData::IndexOfType<FLuaUClass>():
		{
			//Can just overwrite data
			ret.Data = this->Data;	
			break;	
		}
	case LuaValueData::IndexOfType<FLuaArray>():
		{
			//Can just overwrite data
			const FLuaArray& arr = this->GetData().Get<FLuaArray>();
			if(copyComplexTypes)
			{
				ret.GetData().Emplace<FLuaArray>(arr.Lua_Copy());	
			}
			else
			{
				ret.GetData().Emplace<FLuaArray>(arr);
			}
			
			break;	
		}
	case LuaValueData::IndexOfType<FLuaMap>():
		{
			//Can just overwrite data
			const FLuaMap& map = this->GetData().Get<FLuaMap>();
			if(copyComplexTypes)
			{
				ret.GetData().Emplace<FLuaMap>(map.Lua_Copy());
			}
			else
			{
				ret.GetData().Emplace<FLuaMap>(map);
			}
			break;	
		}
	case LuaValueData::IndexOfType<FLuaSet>():
		{
			const FLuaSet& set = this->GetData().Get<FLuaSet>();
			//Can just overwrite data
			if(copyComplexTypes)
			{
				ret.GetData().Emplace<FLuaSet>(set.Lua_Copy());
			}
			else
			{
				ret.GetData().Emplace<FLuaSet>(set);	
			}
			break;	
		}
	case LuaValueData::IndexOfType<FLuaSharedStruct>():
		{
			const FLuaSharedStruct& strct = this->GetData().Get<FLuaSharedStruct>();
			//Can just overwrite data
			if(copyComplexTypes)
			{
				ret.GetData().Emplace<FLuaSharedStruct>(strct.Copy());
			}
			else
			{
				ret.GetData().Emplace<FLuaSharedStruct>(strct);
			}
			break;	
		}
	case LuaValueData::IndexOfType<FLuaInstancedStruct>():
		{
			//instanced structs are ALWAYS gotten as copy
			const FLuaInstancedStruct& strct = this->GetData().Get<FLuaInstancedStruct>();
			ret.GetData().Emplace<FLuaInstancedStruct>(strct.Copy());
			break;	
		}
	case LuaValueData::IndexOfType<FLuaScriptStruct>():
		{
			//Can just overwrite data
			const FLuaScriptStruct& strct = this->GetData().Get<FLuaScriptStruct>();
			if(copyComplexTypes)
			{
				FLuaScriptStruct ss = strct.MakeCopy();
				ret.GetData().Emplace<FLuaScriptStruct>(MoveTemp(ss));
			}
			else
			{
				ret.GetData().Emplace<FLuaScriptStruct>(strct);
			}
			break;	
		}
	case LuaValueData::IndexOfType<FLuaRPCFunction>():
		{
			//Can just overwrite data
			ret.Data = this->Data;
			break;
		}
	case LuaValueData::IndexOfType<FLuaUFunctionReference>():
		{
			ret.Data = this->Data;
			break;
		}
	}
	ret.ClearIsScriptValue();
	return ret;
}

inline bool FLuaValue::IsPropertyReference() const
{
	return this->GetData().IsType<FPropertyReferenceWrapper>();
}

inline bool FLuaValue::IsUFunctionReference() const
{
	return this->GetData().IsType<FLuaUFunctionReference>();
}

inline bool FLuaValue::IsPropertyOrUFunctionReference() const
{
	return this->IsPropertyReference() || this->IsUFunctionReference();
}

inline bool FLuaValue::CanBeReplicated() const
{
	switch(this->GetData().GetIndex())
	{
	default :
		{
			//by default, a script value can not be replicated
			//unless explicitly indicated below
			return false;
		}
	case LuaValueData::IndexOfType<FPropertyReferenceWrapper>():
		{
			//@TODO : Can't replicate certain properties (Array, Map, Set, Delegates)
			FProperty* prop = this->GetData().Get<FPropertyReferenceWrapper>().Prop;
			if(prop->IsA<FNumericProperty>()
				|| prop->IsA<FObjectProperty>()
				|| prop->IsA<FBoolProperty>() 
				|| prop->IsA<FStrProperty>()
				|| prop->IsA<FClassProperty>()
				|| prop->IsA<FNameProperty>()
				|| prop->IsA<FEnumProperty>()
				|| prop->IsA<FStructProperty>())
			{
				return true;
			}
			return false;
		}
	case LuaValueData::IndexOfType<std::nullptr_t>(): [[fallthrough]];
	case LuaValueData::IndexOfType<sol::nil_t>(): [[fallthrough]];
	case LuaValueData::IndexOfType<bool>(): [[fallthrough]];
	case LuaValueData::IndexOfType<int64>(): [[fallthrough]];
	case LuaValueData::IndexOfType<double>(): [[fallthrough]];
	case LuaValueData::IndexOfType<std::string>(): [[fallthrough]];
	case LuaValueData::IndexOfType<FVector2D>(): [[fallthrough]];
	case LuaValueData::IndexOfType<FVector>(): [[fallthrough]];
	case LuaValueData::IndexOfType<FRotator>(): [[fallthrough]];
	case LuaValueData::IndexOfType<TLuaVariantPtr<FTransform>>():  [[fallthrough]];
	case LuaValueData::IndexOfType<TObjectPtr<UObject>>(): [[fallthrough]];
	case LuaValueData::IndexOfType<FLuaUClass>(): [[fallthrough]];
	case LuaValueData::IndexOfType<FLuaSharedStruct>(): [[fallthrough]];
	case LuaValueData::IndexOfType<FLuaInstancedStruct>(): [[fallthrough]];
	case LuaValueData::IndexOfType<FLuaUEnumEntry*>(): [[fallthrough]];
	case LuaValueData::IndexOfType<FLuaScriptStruct>(): [[fallthrough]];
	case LuaValueData::IndexOfType<FLuaUStruct>():
		{
			return true;
		}
	}
}

template<typename T>
inline ESetLuaValueResult FLuaValue::SetValue(const sol::basic_object<T>& newValue, const sol::string_view& key)
{
	ESetLuaValueResult result = ESetLuaValueResult::Success;
	switch(this->GetData().GetIndex())
	{
	case LuaValueData::IndexOfType<FPropertyReferenceWrapper>():
		{
			FPropertyReferenceWrapper& wrapper = this->GetData().Get<FPropertyReferenceWrapper>();
			UObject* owner = wrapper.Owner;
			if(!IsValid(owner))
			{
				result = ESetLuaValueResult::Error;
				break;
			}
			wrapper.SetUObjectMemberProperty(owner, newValue);
			break;
		}
	case LuaValueData::IndexOfType<std::nullptr_t>(): [[fallthrough]];
	case LuaValueData::IndexOfType<sol::nil_t>(): [[fallthrough]];
	case LuaValueData::IndexOfType<bool>(): [[fallthrough]];
	case LuaValueData::IndexOfType<int64>(): [[fallthrough]];
	case LuaValueData::IndexOfType<double>(): [[fallthrough]];
	case LuaValueData::IndexOfType<std::string>(): [[fallthrough]];
	case LuaValueData::IndexOfType<FVector2D>(): [[fallthrough]];
	case LuaValueData::IndexOfType<FVector>(): [[fallthrough]];
	case LuaValueData::IndexOfType<FRotator>(): [[fallthrough]];
	case LuaValueData::IndexOfType<TLuaVariantPtr<FTransform>>():  [[fallthrough]];
	case LuaValueData::IndexOfType<TObjectPtr<UObject>>(): [[fallthrough]];
	case LuaValueData::IndexOfType<FLuaUClass>(): [[fallthrough]];
	case LuaValueData::IndexOfType<FLuaArray>(): [[fallthrough]];
	case LuaValueData::IndexOfType<FLuaMap>(): [[fallthrough]];
	case LuaValueData::IndexOfType<FLuaSet>(): [[fallthrough]];
	case LuaValueData::IndexOfType<FLuaSharedStruct>(): [[fallthrough]];
	case LuaValueData::IndexOfType<FLuaInstancedStruct>(): [[fallthrough]];
	case LuaValueData::IndexOfType<FLuaUEnumEntry*>(): [[fallthrough]];
	case LuaValueData::IndexOfType<FLuaScriptStruct>(): [[fallthrough]];
	case LuaValueData::IndexOfType<FLuaCoroutineHandle>(): [[fallthrough]];
	case LuaValueData::IndexOfType<FLuaTableHandle>(): [[fallthrough]];
	case LuaValueData::IndexOfType<sol::table>(): [[fallthrough]];
	case LuaValueData::IndexOfType<FLuaPrimitiveCPPType>(): [[fallthrough]];
	//case LuaValueData::IndexOfType<FMulticastDelegatePropertyProxy>(): [[fallthrough]];
	//case LuaValueData::IndexOfType<FSingleDelegatePropertyProxy>(): [[fallthrough]];
	case LuaValueData::IndexOfType<FLuaUStruct>():
		{
			result = this->SetLuaValue(newValue, key);
			break;
		}
	case LuaValueData::IndexOfType<FLuaRPCFunction>():
		{
			result = this->SetLuaValue(newValue, key);
			break;
		}
	case LuaValueData::IndexOfType<sol::function>(): [[fallthrough]];
	case LuaValueData::IndexOfType<FLuaFunctionHandle>():
		{
			result = this->SetLuaValue(newValue, key);
			break;
		}
	case LuaValueData::IndexOfType<FLuaUFunctionReference>():
		{
			result = this->HandleSettingFuncReference(newValue);
			break;
		}
	default: ;
	}
	return result;
}

inline ESetLuaValueResult FLuaValue::SetValue(const FProperty* inputProps, const void* inputValueAddress)
{
	//The prop is the source input parameter, NOT a property of the UObject which owns this LuaValue!
	
	ESetLuaValueResult result = ESetLuaValueResult::Success;
	switch(this->GetTypeIndex())
	{
	default :
		{
			result = this->SetValueFromPropertySource(inputProps, inputValueAddress);
			break;			
		}
	//Handle case for FProperty wrappers
	case LuaValueData::IndexOfType<FPropertyReferenceWrapper>():
		{
			FPropertyReferenceWrapper& wrapper = this->GetData().Get<FPropertyReferenceWrapper>();
			UObject* owner = wrapper.Owner;
			if(!IsValid(owner))
			{
				result = ESetLuaValueResult::Error;
			}
			else
			{
				wrapper.SetUObjectMemberProperty(owner, inputProps, inputValueAddress);
			}
			break;
		}
	//Handle case for UFunction wrappers
	//Ufunction wrappers only allow setting a Lua function handle
	case LuaValueData::IndexOfType<FLuaUFunctionReference>():
		{
			if (const FStructProperty* sprop = CastField<FStructProperty>(inputProps))
			{
				if (sprop->Struct == UnrealLua::StaticPackages::LuaFunction)
				{
					const FLuaFunctionHandle* functionHandle = static_cast<const FLuaFunctionHandle*>(inputValueAddress);
					sol::function func = functionHandle->GetFunction();
					this->GetMutable<FLuaUFunctionReference>().LuaFunc = func;
				}
			}
			break;
		}
	case LuaValueData::IndexOfType<FLuaFunctionHandle>():
		{
			result = this->SetValueFromPropertySource(inputProps, inputValueAddress);
			break;
		}
	case LuaValueData::IndexOfType<FLuaRPCFunction>():
		{
			result = this->SetValueFromPropertySource(inputProps, inputValueAddress);
			break;
		}
	}
	return result;
}

inline ESetLuaValueResult FLuaValue::SetValue(const FLuaValue& other)
{
	ESetLuaValueResult result = ESetLuaValueResult::Success;
	
	if (this->IsType<FPropertyReferenceWrapper>())
	{
		const FPropertyReferenceWrapper& wrapper = this->Get<FPropertyReferenceWrapper>();
		FProperty* thisProp = wrapper.Prop;
		UObject* thisOwner = wrapper.Owner;
	
		//Can't modify a FProperty*-type script value
		if(other.GetData().IsType<FPropertyReferenceWrapper>())
		{
			const FPropertyReferenceWrapper& otherWrapper = other.GetData().Get<FPropertyReferenceWrapper>();
			FProperty* otherProp = otherWrapper.Prop;
			UObject* otherOwner = otherWrapper.Owner;

			if(thisProp == otherProp && thisOwner == otherOwner)
			{
				//early out
			}
			else if(thisProp->SameType(otherProp))
			{
				thisProp->CopyCompleteValue(thisProp->ContainerPtrToValuePtr<void>(thisOwner), otherProp->ContainerPtrToValuePtr<void>(otherOwner));
			}
			else
			{
				//not matching properties
				result = ESetLuaValueResult::Error;
			}
		}
		else
		{
			result |= other.WriteValueToPropertyMemoryAddress_WithPropertyTypeCheck(thisProp, thisProp->ContainerPtrToValuePtr<void>(thisOwner));
		}
	}
	else if (this->IsType<FLuaUFunctionReference>())
	{
		result |= this->HandleSettingFuncReference(other);	
	}
	else
	{
		//Handle depending on what incoming value is
		switch (other.GetTypeIndex())
		{
		default:
			{
				//Can just overwrite data
				this->Data = other.Data;
				break;	
			}
		case LuaValueData::IndexOfType<FLuaArray>():
			{
				//Can just overwrite data
				const FLuaArray& arr = other.GetData().Get<FLuaArray>();
				this->GetData().Emplace<FLuaArray>(arr.Lua_Copy());
				break;	
			}
		case LuaValueData::IndexOfType<FLuaMap>():
			{
				//Can just overwrite data
				const FLuaMap& map = other.GetData().Get<FLuaMap>();
				this->GetData().Emplace<FLuaMap>(map.Lua_Copy());
				break;	
			}
		case LuaValueData::IndexOfType<FLuaSet>():
			{
				//Can just overwrite data
				const FLuaSet& set = other.GetData().Get<FLuaSet>();
				this->GetData().Emplace<FLuaSet>(set.Lua_Copy());
				break;	
			}
		case LuaValueData::IndexOfType<FLuaSharedStruct>():
			{
				//Can just overwrite data
				const FLuaSharedStruct& strct = other.GetData().Get<FLuaSharedStruct>();
				this->GetData().Emplace<FLuaSharedStruct>(strct.Copy());
				break;	
			}
		case LuaValueData::IndexOfType<FLuaInstancedStruct>():
			{
				//Can just overwrite data
				const FLuaInstancedStruct& strct = other.GetData().Get<FLuaInstancedStruct>();
				this->GetData().Emplace<FLuaInstancedStruct>(strct.Copy());
				break;	
			}
		case LuaValueData::IndexOfType<FLuaScriptStruct>():
			{
				//Can just overwrite data
				const FLuaScriptStruct& strct = other.GetData().Get<FLuaScriptStruct>();
				this->GetData().Emplace<FLuaScriptStruct>(strct.MakeCopy());
				break;	
			}
		case LuaValueData::IndexOfType<FLuaRPCFunction>():
			{
				//Can just overwrite data
				this->Data = other.Data;
				break;
			}
		case LuaValueData::IndexOfType<FLuaFunctionHandle>():
			{
				if (this->IsScriptValue())
				{
					sol::function otherFunc = other.Get<FLuaFunctionHandle>().GetFunction();
					if (otherFunc.valid())
					{
						this->Emplace<sol::function>(otherFunc);
					}
					else
					{
						this->Emplace<sol::nil_t>();
					}
				}
				else
				{
					this->Data = other.Data;
				}
				break;
			}
		case LuaValueData::IndexOfType<FLuaUFunctionReference>():
			{
				this->Data = other.Data;
				break;
			}
		}
	}

	return result;
}

inline bool FLuaValue::IsEqualToPropertyValue(FProperty* prop, void* propertyValueAddress) const
{
	switch(this->GetData().GetIndex())
	{
	case LuaValueData::IndexOfType<FPropertyReferenceWrapper>():
		{
			FProperty* myProp = this->Get<FPropertyReferenceWrapper>().Prop;
			UObject* myowner = this->Get<FPropertyReferenceWrapper>().Owner;
			void* myMem = myProp->ContainerPtrToValuePtr<void>(myowner);
			if(myMem == propertyValueAddress)
			{
				return true;
			}
			if(!myProp->SameType(prop))
			{
				return false;
			}
			return myProp->Identical(myMem, prop->ContainerPtrToValuePtr<void>(propertyValueAddress));
		}
	case LuaValueData::IndexOfType<bool>():
		{
			if(FBoolProperty* bProp = CastField<FBoolProperty>(prop))
			{
				bool val1 = this->Get<bool>();
				bool val2 = bProp->GetPropertyValue(propertyValueAddress);
				return prop->Identical(&val1, &val2);
			}
			return false;
		}
	case LuaValueData::IndexOfType<FLuaUEnumEntry*>():
		{
			const FLuaUEnumEntry* ref = this->Get<FLuaUEnumEntry*>();
			if(const FNumericProperty* numProp = CastField<FNumericProperty>(prop))
			{
				if(numProp->IsEnum())
				{
					return ref->uenum == numProp->GetIntPropertyEnum() && ref->Value == numProp->GetSignedIntPropertyValue(propertyValueAddress);	
				}
				return false;
			}
			else if(const FEnumProperty* eprop = CastField<FEnumProperty>(prop))
			{
				return ref->uenum == eprop->GetEnum() && ref->Value == eprop->GetUnderlyingProperty()->GetSignedIntPropertyValue(propertyValueAddress);
			}
			return false;
		}
	case LuaValueData::IndexOfType<int64>():
		{
			int64 value = this->Get<int64>();
			if(const FNumericProperty* numProp = CastField<FNumericProperty>(prop))
			{
				if(numProp->IsInteger())
				{
					return numProp->GetSignedIntPropertyValue(propertyValueAddress) == value;
				}
			}
			return false;
		}
	case LuaValueData::IndexOfType<double>():
		{
			double value = this->Get<double>();
			if(const FNumericProperty* numProp = CastField<FNumericProperty>(prop))
			{
				if(numProp->IsFloatingPoint())
				{
					double propVal = numProp->GetFloatingPointPropertyValue(propertyValueAddress);
					return FMath::IsNearlyEqual(propVal, value);
				}
			}
			return false;
		}
	case LuaValueData::IndexOfType<TObjectPtr<UObject>>():
		{
			if(const FObjectProperty* oProp = CastField<FObjectProperty>(prop))
			{
				UObject* val1 = this->Get<TObjectPtr<UObject>>();
				UObject* val2 = oProp->GetObjectPropertyValue(propertyValueAddress);
				return oProp->Identical(val1, val2, 0);							
			}
			return false;
		}
	case LuaValueData::IndexOfType<std::string>():
		{
			if(const FStrProperty* strProp = CastField<FStrProperty>(prop))
			{
				const FString& val2 = strProp->GetPropertyValue(propertyValueAddress);
				FString val1 = this->Get<std::string>().c_str();
				return strProp->Identical(&val1, &val2);
			}
			else if(const FNameProperty* nameProp = CastField<FNameProperty>(prop))
			{
				const FName& val2 = nameProp->GetPropertyValue(propertyValueAddress);
				FString val1 = this->Get<std::string>().c_str();
				return nameProp->Identical(&val1, &val2);
			}
			return false;
		}
	case LuaValueData::IndexOfType<FLuaScriptStruct>():
		{
			const FLuaScriptStruct& ref = this->Get<FLuaScriptStruct>();
			if(const FStructProperty* structProp = CastField<FStructProperty>(prop))
			{
				UScriptStruct* pss = structProp->Struct;
				if(pss != ref.GetScriptStruct())
				{
					return false;
				}
				return pss->CompareScriptStruct(ref.GetMemory(), propertyValueAddress, 0);
			}
			return false;
		}
			
	case LuaValueData::IndexOfType<FLuaInstancedStruct>():
		{
			const FLuaInstancedStruct& ref = this->Get<FLuaInstancedStruct>();
			if(const FStructProperty* structProp = CastField<FStructProperty>(prop))
			{
				UScriptStruct* pss = structProp->Struct;
				if(pss != UnrealLua::StaticPackages::InstancedStruct)
				{
					return false;
				}		
				FInstancedStruct* propValue = static_cast<FInstancedStruct*>(propertyValueAddress);
				return propValue->Identical(ref.GetInstancedStruct(), 0);
			}
			return false;
		}
	case LuaValueData::IndexOfType<FLuaSharedStruct>():
		{
			const FLuaSharedStruct& ref = this->Get<FLuaSharedStruct>();
			if(const FStructProperty* structProp = CastField<FStructProperty>(prop))
			{
				UScriptStruct* pss = structProp->Struct;
				if(pss != UnrealLua::StaticPackages::SharedStruct)
				{
					return false;
				}		
				FSharedStruct* propValue = static_cast<FSharedStruct*>(propertyValueAddress);
				return propValue->Identical(&ref.SharedStruct, 0);
			}
			return false;
		}
	case LuaValueData::IndexOfType<FVector>():
		{
			const FVector& vec = this->Get<FVector>();
			if(const FStructProperty* structProp = CastField<FStructProperty>(prop))
			{
				UScriptStruct* ss = structProp->Struct;
				if(ss == UnrealLua::StaticPackages::VectorStruct)
				{
					const FVector* data = static_cast<const FVector*>(propertyValueAddress);
					return vec == *data;
				}
			}
			return false;
		}
	case LuaValueData::IndexOfType<FVector2D>():
		{
			const FVector2D& vec = this->Get<FVector2D>();
			if(const FStructProperty* structProp = CastField<FStructProperty>(prop))
			{
				UScriptStruct* ss = structProp->Struct;
				if(ss == UnrealLua::StaticPackages::Vector2DStruct)
				{
					const FVector2D* data = static_cast<const FVector2D*>(propertyValueAddress);
					return vec == *data;
				}
			}
			return false;
		}
	case LuaValueData::IndexOfType<FRotator>():
		{
			const FRotator& vec = this->Get<FRotator>();
			if(const FStructProperty* structProp = CastField<FStructProperty>(prop))
			{
				UScriptStruct* ss = structProp->Struct;
				if(ss == UnrealLua::StaticPackages::RotatorStruct)
				{
					const FRotator* data = static_cast<const FRotator*>(propertyValueAddress);
					return vec == *data;
				}
			}
			return false;
		}
	case LuaValueData::IndexOfType<TLuaVariantPtr<FTransform>>():
		{
			const FTransform* vec = this->Get<TLuaVariantPtr<FTransform>>().Get();
			if(const FStructProperty* structProp = CastField<FStructProperty>(prop))
			{
				UScriptStruct* ss = structProp->Struct;
				if(ss == UnrealLua::StaticPackages::TransformStruct)
				{
					const FTransform* data = static_cast<const FTransform*>(propertyValueAddress);
					return vec->Equals(*data);
				}
			}
			return false;
		}
	case LuaValueData::IndexOfType<FLuaArray>():
		{
			const FLuaArray& val = this->Get<FLuaArray>();
			if(const FArrayProperty* arrProp = CastField<FArrayProperty>(prop))
			{
				if(!arrProp->Inner->SameType(val.GetInner()))
				{
					return false;
				}
				return arrProp->Identical(propertyValueAddress, val.GetScriptArray(), 0);
			}
			return false;
		}
	case LuaValueData::IndexOfType<FLuaMap>():
		{
			const FLuaMap& val = this->Get<FLuaMap>();
			if(const FMapProperty* mapProp = CastField<FMapProperty>(prop))
			{
				if(!mapProp->KeyProp->SameType(val.GetKeyProperty()) || !mapProp->ValueProp->SameType(val.GetValueProperty()))
				{
					return false;
				}
				return mapProp->Identical(propertyValueAddress, val.GetScriptMap(), 0);
			}
			return false;
		}
	case LuaValueData::IndexOfType<FLuaSet>():
		{
			const FLuaSet& val = this->Get<FLuaSet>();
			if(const FSetProperty* setProp = CastField<FSetProperty>(prop))
			{
				if(!setProp->ElementProp->SameType(val.GetInner()))
				{
					return false;
				}
				return setProp->Identical(propertyValueAddress, val.GetScriptSet(), 0);
			}
			return false;
		}
	case LuaValueData::IndexOfType<sol::table>():
		{
			const sol::table& item = this->Get<sol::table>();
			if(const FStructProperty* structProp = CastField<FStructProperty>(prop))
			{
				UScriptStruct* ss = structProp->Struct;
				if(ss == UnrealLua::StaticPackages::LuaTable)
				{
					const FLuaTableHandle* data = static_cast<const FLuaTableHandle*>(propertyValueAddress);
					return data->GetTable() == item;
				}
			}
			return false;
		}
	case LuaValueData::IndexOfType<FLuaTableHandle>():
		{
			const FLuaTableHandle& item = this->Get<FLuaTableHandle>();
			if(const FStructProperty* structProp = CastField<FStructProperty>(prop))
			{
				UScriptStruct* ss = structProp->Struct;
				if(ss == UnrealLua::StaticPackages::LuaTable)
				{
					const FLuaTableHandle* data = static_cast<const FLuaTableHandle*>(propertyValueAddress);
					return data->IsValid() == item.IsValid() && item.GetTable() == data->GetTable();
				}
			}
			return false;
		}
	case LuaValueData::IndexOfType<FLuaCoroutineHandle>():
		{
			const FLuaCoroutineHandle& item = this->Get<FLuaCoroutineHandle>();
			if(const FStructProperty* structProp = CastField<FStructProperty>(prop))
			{
				UScriptStruct* ss = structProp->Struct;
				if(ss == UnrealLua::StaticPackages::LuaCoroutine)
				{
					const FLuaCoroutineHandle* data = static_cast<const FLuaCoroutineHandle*>(propertyValueAddress);
					return data->IsValid() == item.IsValid() && item.GetCoroutine() == data->GetCoroutine();
				}
			}
			return false;
		}
	case LuaValueData::IndexOfType<sol::function>():
		{
			const sol::function& item = this->Get<sol::function>();
			if(const FStructProperty* structProp = CastField<FStructProperty>(prop))
			{
				UScriptStruct* ss = structProp->Struct;
				if(ss == UnrealLua::StaticPackages::LuaFunction)
				{
					const FLuaFunctionHandle* data = static_cast<const FLuaFunctionHandle*>(propertyValueAddress);
					return data->GetFunction() == item;
				}
			}
			return false;
		}
	case LuaValueData::IndexOfType<FLuaFunctionHandle>():
		{
			const FLuaFunctionHandle& item = this->Get<FLuaFunctionHandle>();
			if(const FStructProperty* structProp = CastField<FStructProperty>(prop))
			{
				UScriptStruct* ss = structProp->Struct;
				if(ss == UnrealLua::StaticPackages::LuaFunction)
				{
					const FLuaFunctionHandle* data = static_cast<const FLuaFunctionHandle*>(propertyValueAddress);
					return data->IsValid() == item.IsValid() && item.GetFunction() == data->GetFunction();
				}
			}
			return false;
		}
	default:
		return false;
	}
}

template<typename T>
inline ESetLuaValueResult FLuaValue::SetLuaValue(const sol::basic_object<T>& newValue, const std::string_view& key)
{
	ESetLuaValueResult result = ESetLuaValueResult::Success;
	

	sol::type solType = newValue.valid() ? newValue.get_type() : sol::type::nil; 
	switch (solType)
	{
	case sol::type::nil:
		{
			this->GetData().Emplace<sol::nil_t>();
			break;
		}
	case sol::type::string:
		{
			this->GetData().Emplace<std::string>(newValue.template as<sol::string_view>());
			break;
		} 
	case sol::type::number:
		{
			if(newValue.template is<int>())
			{
				this->GetData().Emplace<int64>(newValue.template as<int64>());
				break;
			}
			else if(newValue.template is<double>())
			{
				this->GetData().Emplace<double>(newValue.template as<double>());
				break;
			}
			else
			{
				checkNoEntry();
			}
		}
	case sol::type::boolean:
		{
			this->GetData().Emplace<bool>(newValue.template as<bool>());
			break;
		}
	case sol::type::lightuserdata:
		{
			if (UnrealLua::LightUserdata::IsUObject(newValue))
			{
				UObject* obj = UnrealLua::LightUserdata::GetUObject(newValue);
				if(IsValid(obj) && !obj->HasAnyFlags(RF_BeginDestroyed | RF_MirroredGarbage))
				{
					this->GetData().Emplace<TObjectPtr<UObject>>(obj);
				}
				else
				{
					this->GetData().Emplace<sol::nil_t>();
				}				
			}
			else
			{
				this->GetData().Emplace<sol::nil_t>();
			}
			break;
		}
	case sol::type::userdata:
		if(newValue.template is<FVector>())
		{
			this->GetData().Emplace<FVector>(newValue.template as<FVector>());
			break;
		}
		else if(newValue.template is<FRotator>())
		{
			this->GetData().Emplace<FRotator>(newValue.template as<FRotator>());
			break;
		}
		else if(newValue.template is<FVector2D>())
		{
			this->GetData().Emplace<FVector2D>(newValue.template as<FVector2D>());
			break;
		}
		else if(newValue.template is<FTransform>())
		{
			FTransform& t = newValue.template as<FTransform&>(); 
			this->GetData().Emplace<TLuaVariantPtr<FTransform>>(MakePimpl<FTransform, EPimplPtrMode::DeepCopy>(t));
			break;
		}
		else if(newValue.template is<FLuaUClass>())
		{
			this->GetData().Emplace<FLuaUClass>(newValue.template as<FLuaUClass>());
			break;
		}
		else if(newValue.template is<FLuaUStruct>())
		{
			this->GetData().Emplace<FLuaUStruct>(newValue.template as<FLuaUStruct>());
			break;
		}
		else if(newValue.template is<FLuaScriptStruct>())
		{
			this->Data.Emplace<sol::nil_t>();
			FLuaScriptStruct& val = newValue.template as<FLuaScriptStruct&>();
			FLuaScriptStruct ss = val.MakeCopy();
			this->GetData().Emplace<FLuaScriptStruct>(ss);
			break;
		}
		else if(newValue.template is<FFunctionDescr>())
		{
			//shouldnt allow to set FFunctionDescr values
			checkNoEntry()
			break;
		}
		else if(newValue.template is<FLuaInstancedStruct>())
		{
			this->GetData().Emplace<FLuaInstancedStruct>(newValue.template as<FLuaInstancedStruct&>().Copy());
			break;
		}
		else if(newValue.template is<FLuaSharedStruct>())
		{
			this->GetData().Emplace<FLuaSharedStruct>(newValue.template as<FLuaSharedStruct>());
			break;
		}
		else if(newValue.template is<FLuaArray>())
		{
			FLuaArray& newArr = newValue.template as<FLuaArray&>();
			if(newArr.IsUPropertyReference())
			{
				this->GetData().Emplace<FLuaArray>(newArr.Lua_Copy());	
			}
			else
			{
				//Add reference
				this->GetData().Emplace<FLuaArray>(newArr);
			}
			break;
		}
		else if(newValue.template is<FLuaMap>())
		{
			FLuaMap& map = newValue.template as<FLuaMap&>();
			if(map.IsUPropertyReference())
			{
				this->GetData().Emplace<FLuaMap>(map.Lua_Copy());
			}
			else
			{
				this->GetData().Emplace<FLuaMap>(map);
			}
			break;
		}
		else if(newValue.template is<FLuaSet>())
		{
			FLuaSet& set = newValue.template as<FLuaSet&>();
			if(set.IsUPropertyReference())
			{
				this->GetData().Emplace<FLuaSet>(set.Lua_Copy());
			}
			else
			{
				this->GetData().Emplace<FLuaSet>(set);
			}
			break;
		}
		else if (newValue.template is<FLuaPrimitiveCPPType>())
		{
			FLuaPrimitiveCPPType type = newValue.template as<FLuaPrimitiveCPPType>();
			this->Emplace<FLuaPrimitiveCPPType>(type);
			break;
		}
	
	case sol::type::table:
		{
			this->Emplace<FLuaTableHandle>(FLuaTableHandle::MakeHandle(newValue.template as<sol::table>()));
			break;
		}
	case sol::type::function:
		{
			if (this->IsType<FLuaFunctionHandle>())
			{
				result |= this->HandleSettingFuncReference(newValue.template as<sol::function>());
			}
			else if(isupper(key[0]))
			{
				if(key.starts_with("SERVER_"))
				{
					this->GetData().Emplace<FLuaRPCFunction>(newValue.template as<sol::function>(), key);
				}
				else if(key.starts_with("CLIENT_"))
				{
					this->GetData().Emplace<FLuaRPCFunction>(newValue.template as<sol::function>(), key);
				}
				else if(key.starts_with("MULTICAST_"))
				{
					this->GetData().Emplace<FLuaRPCFunction>(newValue.template as<sol::function>(), key);
				}
				else
				{
					this->GetData().Emplace<sol::function>(newValue.template as<sol::function>());	
				}
			}
			else
			{
				this->GetData().Emplace<sol::function>(newValue.template as<sol::function>());	
			}
			break;
		}
	case sol::type::thread:
		{
			checkNoEntry()
			break;
		}
	default:
		{
			LUA_LOG_WARNING("Attempt to set LuaValue from lua value %hs, will emplace nil instead", UnrealLua::LuaTypes::TypeInfo::UType( newValue, sol::nil, newValue.lua_state()).c_str());
			result |= ESetLuaValueResult::Error;
			this->GetData().Emplace<sol::nil_t>();
			break;
		}
	}
	return result;
}


inline FName FLuaValue::GetKeyName() const
{
	switch(this->GetData().GetIndex())
	{
	case LuaValueData::IndexOfType<FPropertyReferenceWrapper>():
		{
			const FPropertyReferenceWrapper& wrapper = this->GetData().Get<FPropertyReferenceWrapper>();
			FProperty* prop = wrapper.Prop;
			UObject* owner = wrapper.Owner;
			return prop->GetFName();
		}
	case LuaValueData::IndexOfType<FLuaUFunctionReference>():
		{
			return this->GetData().Get<FLuaUFunctionReference>().Func->Func->GetFName();
		}
	default:
		{
			return NAME_None;					
		}
	}
}

inline ESetLuaValueResult FLuaValue::HandleSettingFuncReference(const FLuaValue& other) const
{
	if (other.IsType<sol::function>())
	{
		return this->HandleSettingFuncReference(other.Get<sol::function>());
	}
	else if (other.IsType<FLuaFunctionHandle>())
	{
		return this->HandleSettingFuncReference(other.Get<FLuaFunctionHandle>().GetFunction());
	}
	else if (other.IsType<FLuaRPCFunction>())
	{
		return this->HandleSettingFuncReference(other.Get<FLuaRPCFunction>().LuaFunc);
	}
	else
	{
		return this->HandleSettingFuncReference(sol::nil);
	}
}


inline ESetLuaValueResult FLuaValue::HandleSettingFuncReference(const sol::nil_t nil) const
{
	verify(this->GetData().IsType<FLuaUFunctionReference>());
	sol::function func{sol::nil};
	return this->HandleSettingFuncReference(func);
}

inline ESetLuaValueResult FLuaValue::HandleSettingFuncReference(const sol::stack_object& newValue) const
{
	verify(this->GetData().IsType<FLuaUFunctionReference>());
	
	if(!newValue.valid() || newValue.get_type() == sol::type::function)
	{
		return this->HandleSettingFuncReference(newValue.as<sol::function>());
	}
	return ESetLuaValueResult::Error;	
}

inline ESetLuaValueResult FLuaValue::HandleSettingFuncReference(const sol::object& newValue) const
{
	verify(this->GetData().IsType<FLuaUFunctionReference>());
	
	if(!newValue.valid() || newValue.get_type() == sol::type::function)
	{
		return this->HandleSettingFuncReference(newValue.as<sol::function>());
	}
	return ESetLuaValueResult::Error;
}

inline ESetLuaValueResult FLuaValue::HandleSettingFuncReference(const sol::function& newValue) const
{
	ESetLuaValueResult result = ESetLuaValueResult::Success;
	
	FLuaUFunctionReference& ref = this->GetMutable<FLuaUFunctionReference>();
	FName funcName = ref.Func->Func->GetFName();
	if (funcName == UnrealLua::PropertyNames::NAME_ReceiveTick || funcName == UnrealLua::PropertyNames::NAME_UserWidgetTick)
	{
		result |= ESetLuaValueResult::TickFunctionModified;
	}
	ref.LuaFunc = newValue;
	return result;	
}


DECLARE_DELEGATE_OneParam(FOnLuaScriptValueChangedNativeDelegate, FLuaValue);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnLuaScriptValueChangedDelegate, FLuaValue, luaValue);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLuaScriptValueChangedMulticastDelegate, FLuaValue);