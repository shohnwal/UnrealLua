
#include "LuaValue/LuaValue.h"

#include "Reflection/FunctionDescr.h"
#include "UnrealLua.h"
#include "sol/sol.hpp"
#include "LuaContext/ScopedLuaContext.h"
#include "LuaValue/LuaValueType.h"
#include "Reflection/PropertyHelper_ToString.h"
#include "UObjectRegistry/LuaUObjectItem.h"
#include "UObjectRegistry/LuaUObjectRegistry.h"
#include "Utility/UnrealVersion.h"

#pragma message ("TODO : Right now we assume same Lua state for everything. Still need to add checks for mismatching Lua states in " __FILE__)

FLuaUFunctionReference::FLuaUFunctionReference(const FFunctionDescr* func, sol::function luaFunc)
	: LuaFunc(luaFunc), Func(func)
{
}

FLuaUFunctionReference::FLuaUFunctionReference(const FFunctionDescr* func)
	: LuaFunc(), Func(func)
{
}

int FLuaUFunctionReference::PushValue(sol::this_state lua) const
{
	if(this->LuaFunc.valid())
	{
		return sol::stack::push(lua, this->LuaFunc);
	}
	//UnrealLua::LuaScriptCall::CurrentFunc = this->Func;
	//return sol::stack::push(lua, sol::light(this->Func));
	return sol::stack::push(lua, this->Func);
}

sol::object FLuaUFunctionReference::GetValue(sol::this_state lua) const
{
	if(this->LuaFunc.valid())
	{
		return this->LuaFunc;
	}
	return UnrealLua::LightUserdata::MakeFFunctionDescrReferenceObject(lua, this->Func);
}

void FLuaValue::SetDead()
{
	this->Emplace<UnrealLua::DeadValue>();
}

void FLuaValue::MarkAsScriptValue()
{
	this->Data.MarkAsScriptValue();
}

bool FLuaValue::IsScriptValue() const
{
	return this->Data.IsScriptValue();
}

void FLuaValue::ClearIsScriptValue()
{
	this->Data.ClearScriptValue();
}

void FLuaValue::ConvertLuaObjectsToHandles()
{
	switch (this->GetTypeIndex())
	{
	default:
		break;
	case LuaValueData::IndexOfType<sol::function>():
		{
			sol::function func = this->Get<sol::function>();
			this->Emplace<FLuaFunctionHandle>(FLuaFunctionHandle::MakeHandle(func));
			break;
		}
	case LuaValueData::IndexOfType<sol::table>():
		{
			sol::table tbl = this->Get<sol::table>();
			this->Emplace<FLuaTableHandle>(FLuaTableHandle::MakeHandle(tbl));
			break;
		}
	}
}


//The prop arg is either
//- the UFunction input parameter from UUnrealLuaUtilityBlueprintFunctionLibrary::execSetLuaScriptValue
//- The prop is the input param type
//or
//- the prop from FLuaValueReplicator::ServerProcessRegisteredLuaScriptOwner
//- The prop is the UObject member prop
//ATTENTION:
//void* InputValueAddress is already the correct memory location, with offset
//
ESetLuaValueResult FLuaValue::SetValueFromPropertySource(const FProperty* prop, const void* inputValueAddress)
{
	//@TODO : Add support for Array, Map and Set
	if(prop->IsA<FBoolProperty>())
	{
		bool data;
		prop->CopySingleValue(&data, inputValueAddress);
		this->GetData().Emplace<bool>(data);
		return ESetLuaValueResult::Success;
	}
	else if(const FNumericProperty* numProp = CastField<FNumericProperty>(prop))
	{
		if(numProp->IsEnum())
		{
			int64 val = numProp->GetSignedIntPropertyValue(inputValueAddress);;
			
			UEnum* uenum = numProp->GetIntPropertyEnum();
			FLuaUObjectItem& enumItem = UnrealLua::UObjectRegistry::GetMetaObjectItem(uenum);
			FLuaUEnumEntry* entry = enumItem.PropertyMapping.Get<FLuaUEnumMapping>().GetEnumEntryByNumberValue(val);

			this->GetData().Emplace<FLuaUEnumEntry*>(entry);			
		}
		else if(numProp->IsInteger())
		{
			int64 val = numProp->GetSignedIntPropertyValue(inputValueAddress);;
			this->GetData().Emplace<int64>(val);
		}
		else if(numProp->IsFloatingPoint())
		{
			double val = numProp->GetFloatingPointPropertyValue(inputValueAddress);
			this->GetData().Emplace<double>(val);
		}
		return ESetLuaValueResult::Success;
	}
	else if(const FObjectProperty* oProp = CastField<FObjectProperty>(prop))
	{
		UObject* val = oProp->GetPropertyValue(inputValueAddress);
		if(IsValid(val) && !val->HasAnyFlags(RF_BeginDestroyed | RF_MirroredGarbage))
		{
			this->GetData().Emplace<TObjectPtr<UObject>>(val);
		}
		else
		{
			this->GetData().Emplace<sol::nil_t>();
		}
		return ESetLuaValueResult::Success;
	}
	else if(const FStrProperty* strProp = CastField<FStrProperty>(prop))
	{
		const FString& str = strProp->GetPropertyValue(inputValueAddress);
		this->GetData().Emplace<std::string>(StringCast<char>(*str).Get());
		return ESetLuaValueResult::Success;
	}
	else if(const FNameProperty* nameProp = CastField<FNameProperty>(prop))
	{
		const FName& str = nameProp->GetPropertyValue(inputValueAddress);
		this->GetData().Emplace<std::string>(StringCast<char>(*str.ToString()).Get());
		return ESetLuaValueResult::Success;
	}
	else if(const FStructProperty* structProp = CastField<FStructProperty>(prop))
	{
		UScriptStruct* ss = structProp->Struct;
		if(ss == UnrealLua::StaticPackages::VectorStruct)
		{
			const FVector* data = static_cast<const FVector*>(inputValueAddress);
			this->GetData().Emplace<FVector>(*data);
			return ESetLuaValueResult::Success;
		}
		else if(ss == UnrealLua::StaticPackages::Vector2DStruct )
		{
			const FVector2D* data = static_cast<const FVector2D*>(inputValueAddress);
			this->GetData().Emplace<FVector2D>(*data);
			return ESetLuaValueResult::Success;
		}
		else if(ss == UnrealLua::StaticPackages::RotatorStruct)
		{
			const FRotator* data = static_cast<const FRotator*>(inputValueAddress);
			this->GetData().Emplace<FRotator>(*data);
			return ESetLuaValueResult::Success;
		}
		else if(ss == UnrealLua::StaticPackages::TransformStruct)
		{
			const FTransform* data = static_cast<const FTransform*>(inputValueAddress);
			this->GetData().Emplace<TLuaVariantPtr<FTransform>>(MakePimpl<FTransform, EPimplPtrMode::DeepCopy>(*data));
			return ESetLuaValueResult::Success;
		}
		else if(ss == UnrealLua::StaticPackages::InstancedStruct)
		{
			const FInstancedStruct* data = static_cast<const FInstancedStruct*>(inputValueAddress);
			FLuaInstancedStruct temp{data, true};
			this->GetData().Emplace<FLuaInstancedStruct>(temp.Copy());
			return ESetLuaValueResult::Success;
		}
		else if(ss == UnrealLua::StaticPackages::SharedStruct)
		{
			const FSharedStruct* data = static_cast<const FSharedStruct*>(inputValueAddress);
			this->GetData().Emplace<FLuaSharedStruct>(data);
			return ESetLuaValueResult::Success;
		}
		else if(ss == UnrealLua::StaticPackages::LuaTable)
		{
			const FLuaTableHandle* data = static_cast<const FLuaTableHandle*>(inputValueAddress);
			if (data->IsValid())
			{
				this->Emplace<FLuaTableHandle>(*data);
			}
			else
			{
				this->Emplace<sol::nil_t>();
			}
			return ESetLuaValueResult::Success;
		}
		else if(ss == UnrealLua::StaticPackages::LuaFunction)
		{
			const FLuaFunctionHandle* data = static_cast<const FLuaFunctionHandle*>(inputValueAddress);
			if (data->IsValid())
			{
				this->Emplace<FLuaFunctionHandle>(*data);
			}
			else
			{
				this->Emplace<sol::nil_t>();
			}
			return ESetLuaValueResult::Success;
		}
		else if(ss == UnrealLua::StaticPackages::LuaCoroutine)
		{
			const FLuaCoroutineHandle* data = static_cast<const FLuaCoroutineHandle*>(inputValueAddress);
			if (data->IsValid())
			{
				this->Emplace<FLuaCoroutineHandle>(*data);
			}
			else
			{
				this->Emplace<sol::nil_t>();
			}
			return ESetLuaValueResult::Success;
		}
		else if (ss == UnrealLua::StaticPackages::LuaDelegate)
		{
			FLuaScriptMulticastDelegate& mcDel = this->Emplace_GetRef<FLuaScriptMulticastDelegate>();
			const FLuaDelegate* del = static_cast<const FLuaDelegate*>(inputValueAddress);
			if (del->IsBound())
			{
				mcDel.AddDynamicListener(*del);				
			}
			return ESetLuaValueResult::Success;
		}
		else if(ss == UnrealLua::StaticPackages::LuaValue)
		{
			const FLuaValue* data = static_cast<const FLuaValue*>(inputValueAddress);
			*this = *data;
			return ESetLuaValueResult::Success;
		}
		else
		{
			this->GetData().Emplace<FLuaScriptStruct>(FLuaScriptStruct{ss, inputValueAddress, false});
			return ESetLuaValueResult::Success;
		}
	}
	return ESetLuaValueResult::Error;
}

FString FLuaValue::ToValueString() const
{
	FString output;

	switch(this->GetData().GetIndex())
	{
		case LuaValueData::IndexOfType<FLuaFunctionHandle>():
		{
			const FLuaFunctionHandle& handle = this->Get<FLuaFunctionHandle>();
			if (handle.IsValid())
			{
				sol::function func = this->Get<FLuaFunctionHandle>().GetFunction();
				lua_Debug data{};
				lua_getinfo(func.lua_state(), "S", &data);
				output.Append(data.source, data.srclen);				
			}
			else
			{
				output.Append("Invalid Lua function handle");
			}
			break;
		}

		case LuaValueData::IndexOfType<FLuaUFunctionReference>():
		{
			const FLuaUFunctionReference& func = this->Get<FLuaUFunctionReference>();
			FString str = func.Func->Func->GetName();
			output = "UFunction: ";
			output += str;
			output += ", LuaFunc: ";
			if(func.LuaFunc.valid())
			{
				//lua_Debug data{};
				//lua_getinfo(func.luaFunc.lua_state(), "S", &data);
				
				//sol::bytecode code = func.luaFunc.dump();
				//output.Append(code.as_string_view().data());
				//output.Append(data.short_src);
			}
			else
			{
				output += "nil";
			}
			break;
		}
		case LuaValueData::IndexOfType<int64>():
		{
			int64 val = this->Get<int64>();
			output.Appendf(TEXT("%lld"), val);
			break;
		}
		case LuaValueData::IndexOfType<double>():
		{
			double val = this->Get<double>();
			output.Appendf(TEXT("%g"), val);
			break;
		}
		case LuaValueData::IndexOfType<bool>():
		{
			bool val = this->Get<bool>();
			if (val)
			{
				output.Appendf(TEXT("true"));	
			}
			else
			{
				output.Appendf(TEXT("false"));
			}
			break;
		}
		case LuaValueData::IndexOfType<sol::nil_t>(): [[fallthrough]];
		case LuaValueData::IndexOfType<std::nullptr_t>():
		{
			break;
		}
		case LuaValueData::IndexOfType<std::string>():
		{
			const std::string& val = this->Get<std::string>();
			output.Appendf(TEXT("%hs"), val.c_str());
			break;
		}
		case LuaValueData::IndexOfType<FVector2D>():
		{
			const FVector2D& val = this->Get<FVector2D>();
			output += "{" + val.ToString() + "}";
			break;
		}
		case LuaValueData::IndexOfType<FVector>():
			{
			const FVector& val = this->Get<FVector>();
			output += "{" + val.ToString() + "}";
			break;
		}
		case LuaValueData::IndexOfType<FRotator>():
		{
			const FRotator& val = this->Get<FRotator>();
			output += "{" + val.ToString() + "}";
			break;
		}
		case LuaValueData::IndexOfType<TObjectPtr<UObject>>():
		{
			const TObjectPtr<UObject>& val = this->Get<TObjectPtr<UObject>>();
			output += GetNameSafe(val);
			break;
		}
		case LuaValueData::IndexOfType<FLuaUClass>():
		{
			const FLuaUClass& val = this->Get<FLuaUClass>();
			output += val.GetSoftClassPath().ToString();
			break;
		}
		case LuaValueData::IndexOfType<FLuaUStruct>():
		{
			const FLuaUStruct& val = this->Get<FLuaUStruct>();
			output += val.GetPath().ToString();
			break;
		}
		case LuaValueData::IndexOfType<FPropertyReferenceWrapper>():
		{
			const FPropertyReferenceWrapper& wrapper = this->Get<FPropertyReferenceWrapper>();
			UObject* owner = wrapper.Owner.Get(); 
			if (IsValid(owner))
			{
				void* memPtr = wrapper.Prop->ContainerPtrToValuePtr<void>(owner);
				wrapper.Prop->ExportText_Direct(output, memPtr, memPtr, nullptr, PPF_None, nullptr);
			}
			else
			{
				output = "Invalid Property Owner";
			}
			break;
		}
		case LuaValueData::IndexOfType<FLuaUEnumEntry*>():
		{
			FLuaUEnumEntry* entry = this->Get<FLuaUEnumEntry*>();
			output += entry->ToString().c_str();
			break;
		}
		case LuaValueData::IndexOfType<FLuaTableHandle>(): [[fallthrough]];
		case LuaValueData::IndexOfType<FLuaCoroutineHandle>(): [[fallthrough]];
		case LuaValueData::IndexOfType<TLuaVariantPtr<FTransform>>():[[fallthrough]];
		case LuaValueData::IndexOfType<FLuaRPCFunction>():[[fallthrough]];
		case LuaValueData::IndexOfType<FLuaScriptStruct>():[[fallthrough]];
		case LuaValueData::IndexOfType<FLuaArray>():[[fallthrough]];
		case LuaValueData::IndexOfType<FLuaSet>():[[fallthrough]];
		case LuaValueData::IndexOfType<FLuaMap>():[[fallthrough]];
		case LuaValueData::IndexOfType<FLuaSharedStruct>():[[fallthrough]];
		case LuaValueData::IndexOfType<FLuaInstancedStruct>():[[fallthrough]];
		default:
			{
				output = "unimplemented";
			}
	}

	return output;
}

FString FLuaValue::GetTypeString() const
{
	switch(this->GetData().GetIndex())
	{
		case LuaValueData::IndexOfType<FLuaFunctionHandle>():[[fallthrough]];
		case LuaValueData::IndexOfType<sol::function>():
		{
			return "Lua Function";		
		}
		case LuaValueData::IndexOfType<FLuaPrimitiveCPPType>():
		{
			return FString{this->Get<FLuaPrimitiveCPPType>().tostring().c_str()};
		}
		case LuaValueData::IndexOfType<FLuaUFunctionReference>():
		{
			const FLuaUFunctionReference& func = this->Get<FLuaUFunctionReference>();
			if(func.LuaFunc.valid())
			{
				return "UFunction, Lua Function";
			}
			else
			{
				return "UFunction";
			}
			break;
		}
		case LuaValueData::IndexOfType<int64>():
		{
			return "Integer";
		}
		case LuaValueData::IndexOfType<double>():
		{
			return "Float";
		}
		case LuaValueData::IndexOfType<bool>():
		{
			return "Bool";
		}
		case LuaValueData::IndexOfType<sol::nil_t>(): [[fallthrough]];
		case LuaValueData::IndexOfType<std::nullptr_t>():
		{
			return "Nil";
		}
		case LuaValueData::IndexOfType<std::string>():
		{
			return "String";
		}
		case LuaValueData::IndexOfType<FVector2D>():
		{
			return "FVector2D";
		}
		case LuaValueData::IndexOfType<FVector>():
		{
			return "FVector";
		}
		case LuaValueData::IndexOfType<FRotator>():
		{
			return "FRotator";
		}
		case LuaValueData::IndexOfType<TObjectPtr<UObject>>():
		{
			UClass* uclass = this->Get<TObjectPtr<UObject>>().GetClass();
			return "UObject (" + uclass->GetName() + ")";
		}
		case LuaValueData::IndexOfType<FLuaUClass>():
		{
			FString path = this->Get<FLuaUClass>().GetSoftClassPath().ToString();
			return "UClass (" + path + ")";
		}
		case LuaValueData::IndexOfType<FLuaUStruct>():
		{
			FString path = this->Get<FLuaUStruct>().GetPath().ToString();
			return "UScriptStruct (" + path + ")";
		}
		case LuaValueData::IndexOfType<FPropertyReferenceWrapper>():
		{
			FString type =	this->Get<FPropertyReferenceWrapper>().Prop->GetCPPType();
			return "FProperty (" + type + ")";
		}
		case LuaValueData::IndexOfType<FLuaUEnumEntry*>():
		{
			FLuaUEnumEntry* entry = this->Get<FLuaUEnumEntry*>();
			return "Enum value (" + FString(entry->ToString().c_str()) + ")";
		}
		case LuaValueData::IndexOfType<FLuaTableHandle>(): [[fallthrough]];
		case LuaValueData::IndexOfType<sol::table>():
		{
			return "Lua Table";
		}
		case LuaValueData::IndexOfType<FLuaCoroutineHandle>():
		{
			return "Lua Coroutine";
		}
		case LuaValueData::IndexOfType<TLuaVariantPtr<FTransform>>():
		{
			return "FTransform";
		}
		case LuaValueData::IndexOfType<FLuaRPCFunction>():
		{
			return "Lua Function (RPC)";
		}
		case LuaValueData::IndexOfType<FLuaScriptStruct>():
		{
			const UScriptStruct* uss = this->Get<FLuaScriptStruct>().GetScriptStruct();
			return "ScriptStruct (" + GetNameSafe(uss) + ")";
		}
		case LuaValueData::IndexOfType<FLuaArray>():
		{
			return "Lua TArray<>";
		}
		case LuaValueData::IndexOfType<FLuaSet>():
		{
			return "Lua TSet<>";
		}
		case LuaValueData::IndexOfType<FLuaMap>():
		{
			return "Lua TMap<>";
		}
		case LuaValueData::IndexOfType<FLuaSharedStruct>():
		{
			const UScriptStruct* uss = this->Get<FLuaScriptStruct>().GetScriptStruct();
			return "TSharedStruct<" + GetNameSafe(uss) + ">";
		}
		case LuaValueData::IndexOfType<FLuaInstancedStruct>():
		{
				const UScriptStruct* uss = this->Get<FLuaScriptStruct>().GetScriptStruct();
				return "TInstancedStruct<" + GetNameSafe(uss) + ">";
		}
		default:
			{
				return "Unknown type";
			}
	}

}


FString FLuaValue::ToStringForStructBuilderEditor() const
{
	//@TODO : Improve, so returned strings can be used as values in script editor
	FString output;

	switch(this->GetData().GetIndex())
	{
		case LuaValueData::IndexOfType<FLuaFunctionHandle>():
		{
			const FLuaFunctionHandle& handle = this->Get<FLuaFunctionHandle>();
			if (handle.IsValid())
			{
				sol::function func = this->Get<FLuaFunctionHandle>().GetFunction();
				lua_Debug data{};
				lua_getinfo(func.lua_state(), "S", &data);
				output.Append(data.source, data.srclen);				
			}
			else
			{
				output.Append("Invalid Lua function handle");
			}
			break;
		}

		case LuaValueData::IndexOfType<FLuaUFunctionReference>():
		{
			const FLuaUFunctionReference& func = this->Get<FLuaUFunctionReference>();
			FString str = func.Func->Func->GetName();
			output = "UFunction: ";
			output += str;
			output += ", LuaFunc: ";
			if(func.LuaFunc.valid())
			{
				//lua_Debug data{};
				//lua_getinfo(func.luaFunc.lua_state(), "S", &data);
				
				//sol::bytecode code = func.luaFunc.dump();
				//output.Append(code.as_string_view().data());
				//output.Append(data.short_src);
			}
			else
			{
				output += "nil";
			}
			break;
		}
		case LuaValueData::IndexOfType<int64>():
		{
			int64 val = this->Get<int64>();
			output.Appendf(TEXT("%lld"), val);
			break;
		}
		case LuaValueData::IndexOfType<double>():
		{
			double val = this->Get<double>();
			output.Appendf(TEXT("%g"), val);
			break;
		}
		case LuaValueData::IndexOfType<bool>():
		{
			bool val = this->Get<bool>();
			if (val)
			{
				output.Appendf(TEXT("true"));	
			}
			else
			{
				output.Appendf(TEXT("false"));
			}
			break;
		}
		case LuaValueData::IndexOfType<sol::nil_t>(): [[fallthrough]];
		case LuaValueData::IndexOfType<std::nullptr_t>():
		{
			output = "nil";
			break;
		}
		case LuaValueData::IndexOfType<std::string>():
		{
			const std::string& val = this->Get<std::string>();
			output.Appendf(TEXT("\"%hs\""), val.c_str());
			break;
		}
		case LuaValueData::IndexOfType<FVector2D>():
		{
			const FVector2D& val = this->Get<FVector2D>();
			output += "{" + val.ToString() + "}";
			break;
		}
		case LuaValueData::IndexOfType<FVector>():
			{
			const FVector& val = this->Get<FVector>();
			output += "{" + val.ToString() + "}";
			break;
		}
		case LuaValueData::IndexOfType<FRotator>():
		{
			const FRotator& val = this->Get<FRotator>();
			output += "{" + val.ToString() + "}";
			break;
		}
		case LuaValueData::IndexOfType<TObjectPtr<UObject>>():
		{
			const TObjectPtr<UObject>& val = this->Get<TObjectPtr<UObject>>();
			output = "nil";
			break;
		}
		case LuaValueData::IndexOfType<FLuaUClass>():
		{
			const FLuaUClass& val = this->Get<FLuaUClass>();
			output += val.GetSoftClassPath().ToString();
			break;
		}
		case LuaValueData::IndexOfType<FLuaUStruct>():
		{
			const FLuaUStruct& val = this->Get<FLuaUStruct>();
			output += val.GetPath().ToString();
			break;
		}
		case LuaValueData::IndexOfType<FPropertyReferenceWrapper>():
		{
			const FPropertyReferenceWrapper& wrapper = this->Get<FPropertyReferenceWrapper>();
			UObject* owner = wrapper.Owner.Get(); 
			if (IsValid(owner))
			{
				FGetPropertyValueAsLuaSyntaxStringParams params{wrapper.Prop, owner, false};
				output = UnrealLua::PropertyHelper::GetPropertyValueAsLuaSyntaxValidString_InContainer(params);
			}
			else
			{
				output = "nil";
			}
			break;
		}
		case LuaValueData::IndexOfType<FLuaUEnumEntry*>():
		{
			FLuaUEnumEntry* entry = this->Get<FLuaUEnumEntry*>();
			output += entry->ToString().c_str();
			break;
		}
		case LuaValueData::IndexOfType<FLuaTableHandle>():
		{
			return "{}";
		}
		case LuaValueData::IndexOfType<FLuaCoroutineHandle>():
		{
			return "<coroutine>_notsupportedvalue";
		}
		case LuaValueData::IndexOfType<TLuaVariantPtr<FTransform>>():
		{
			const FTransform* transform = this->Get<TLuaVariantPtr<FTransform>>().Get();
#if UNREALLUA_UE_VERSION_OLDER_THAN(5,8,0)
			FStringBuilderBase builder;
#else
			TStringBuilder<128> builder;
#endif
			builder << "FTransform{{";
			FString trstr = transform->ToString();
			trstr.ReplaceInline(TEXT("|"), TEXT("},{"));
			builder << "}}";
			return builder.ToString();
		}
		case LuaValueData::IndexOfType<FLuaRPCFunction>():
		{
			return "<LuaRPCFunction>_notsupportedvalue";
		}
		case LuaValueData::IndexOfType<FLuaScriptStruct>():
		{
			return this->Get<FLuaScriptStruct>().ToLuaSyntaxValueString();
		}
		case LuaValueData::IndexOfType<FLuaArray>():
		{
			return "<FLuaArray>_notsupportedvalue";
		}
		case LuaValueData::IndexOfType<FLuaSet>():
		{
			return "<FLuaSet>_notsupportedvalue";
		}
		case LuaValueData::IndexOfType<FLuaMap>():
		{
			return "<FLuaMap>_notsupportedvalue";
		}
		case LuaValueData::IndexOfType<FLuaSharedStruct>():
		{
			return "<TSharedStruct>_notsupportedvalue";
		}
		case LuaValueData::IndexOfType<FLuaInstancedStruct>():
		{
			return "<TInstancedStruct>_notsupportedvalue";
		}
		default:
			{
				output = "<?>";
			}
	}

	return output;
}

FString FLuaValue::GetLuaSyntaxValidValueString(bool containersAsTable) const
{
		//@TODO : Improve, so returned strings can be used as values in script editor
	FString output;

	switch(this->GetData().GetIndex())
	{
		case LuaValueData::IndexOfType<FLuaFunctionHandle>():
		{
			const FLuaFunctionHandle& handle = this->Get<FLuaFunctionHandle>();
			if (handle.IsValid())
			{
				sol::function func = this->Get<FLuaFunctionHandle>().GetFunction();
				lua_Debug data{};
				lua_getinfo(func.lua_state(), "S", &data);
				output.Append(data.source, data.srclen);				
			}
			else
			{
				output.Append("nil");
			}
			break;
		}

		case LuaValueData::IndexOfType<FLuaUFunctionReference>():
		{
			const FLuaUFunctionReference& func = this->Get<FLuaUFunctionReference>();
			FString str = func.Func->Func->GetName();
			break;
		}
		case LuaValueData::IndexOfType<int64>():
		{
			int64 val = this->Get<int64>();
			output = FString::FromInt(val);
			break;
		}
		case LuaValueData::IndexOfType<double>():
		{
			double val = this->Get<double>();
			output = FString::SanitizeFloat(val);
			break;
		}
		case LuaValueData::IndexOfType<bool>():
		{
			bool val = this->Get<bool>();
			output = val ? TEXT("true") : TEXT("false"); 	
			break;
		}
		case LuaValueData::IndexOfType<sol::nil_t>(): [[fallthrough]];
		case LuaValueData::IndexOfType<std::nullptr_t>():
		{
			output = "nil";
			break;
		}
		case LuaValueData::IndexOfType<std::string>():
		{
			const std::string& val = this->Get<std::string>();
			output.Appendf(TEXT("\"%hs\""), val.c_str());
			break;
		}
		case LuaValueData::IndexOfType<FVector2D>():
		{
			const FVector2D& val = this->Get<FVector2D>();
			output += "{" + val.ToString() + "}";
			break;
		}
		case LuaValueData::IndexOfType<FVector>():
			{
			const FVector& val = this->Get<FVector>();
			output += "{" + val.ToString() + "}";
			break;
		}
		case LuaValueData::IndexOfType<FRotator>():
		{
			const FRotator& val = this->Get<FRotator>();
			output += "{" + val.ToString() + "}";
			break;
		}
		case LuaValueData::IndexOfType<TObjectPtr<UObject>>():
		{
			const TObjectPtr<UObject>& val = this->Get<TObjectPtr<UObject>>();
			output = "nil";
			break;
		}
		case LuaValueData::IndexOfType<FLuaUClass>():
		{
			const FLuaUClass& val = this->Get<FLuaUClass>();
			output += val.GetSoftClassPath().ToString();
			break;
		}
		case LuaValueData::IndexOfType<FLuaUStruct>():
		{
			const FLuaUStruct& val = this->Get<FLuaUStruct>();
			output += val.GetPath().ToString();
			break;
		}
		case LuaValueData::IndexOfType<FPropertyReferenceWrapper>():
		{
			const FPropertyReferenceWrapper& wrapper = this->Get<FPropertyReferenceWrapper>();
			UObject* owner = wrapper.Owner.Get(); 
			if (IsValid(owner))
			{
				FGetPropertyValueAsLuaSyntaxStringParams params{wrapper.Prop, owner, containersAsTable};
				output = UnrealLua::PropertyHelper::GetPropertyValueAsLuaSyntaxValidString_InContainer(params);
			}
			else
			{
				output = "nil";
			}
			break;
		}
		case LuaValueData::IndexOfType<FLuaUEnumEntry*>():
		{
			FLuaUEnumEntry* entry = this->Get<FLuaUEnumEntry*>();
			output += entry->ToString().c_str();
			break;
		}
		case LuaValueData::IndexOfType<FLuaTableHandle>():
		{
			return "{}";
		}
		case LuaValueData::IndexOfType<FLuaCoroutineHandle>():
		{
			return "{} --[[coroutine]]";
		}
		case LuaValueData::IndexOfType<TLuaVariantPtr<FTransform>>():
		{
			const FTransform* transform = this->Get<TLuaVariantPtr<FTransform>>().Get();
#if UNREALLUA_UE_VERSION_OLDER_THAN(5,8,0)
			FStringBuilderBase builder;
#else
			TStringBuilder<128> builder;
#endif
			builder << "FTransform{{";
			FString trstr = transform->ToString();
			trstr.ReplaceInline(TEXT("|"), TEXT("},{"));
			builder << "}}";
			return builder.ToString();
		}
		case LuaValueData::IndexOfType<FLuaRPCFunction>():
		{
			return "<LuaRPCFunction>_notsupportedvalue";
		}
		case LuaValueData::IndexOfType<FLuaScriptStruct>():
		{
			return this->Get<FLuaScriptStruct>().ToLuaSyntaxValueString();
		}
		case LuaValueData::IndexOfType<FLuaArray>():
		{
			return "<FLuaArray>_notsupportedvalue";
		}
		case LuaValueData::IndexOfType<FLuaSet>():
		{
			return "<FLuaSet>_notsupportedvalue";
		}
		case LuaValueData::IndexOfType<FLuaMap>():
		{
			return "<FLuaMap>_notsupportedvalue";
		}
		case LuaValueData::IndexOfType<FLuaSharedStruct>():
		{
			return "<TSharedStruct>_notsupportedvalue";
		}
		case LuaValueData::IndexOfType<FLuaInstancedStruct>():
		{
			return "<TInstancedStruct>_notsupportedvalue";
		}
		default:
			{
				output = "<?>";
			}
	}

	return output;
}

bool FLuaValue::PostGCHandleUObjectPtrs()
{
	if(this->IsType<TObjectPtr<UObject>>())
	{
		UObject* obj = this->Get<TObjectPtr<UObject>>();
		if(!IsValid(obj) || obj->HasAnyFlags(RF_BeginDestroyed | RF_MirroredGarbage))
		{
			this->Emplace<sol::nil_t>();
			return true;
		}
	}
	return false;
}

void FLuaValue::CleanUpForLuaState(sol::this_state lua)
{
	switch(this->GetData().GetIndex())
	{
	case LuaValueData::IndexOfType<sol::function>():
		{
			const sol::function& func = this->Get<sol::function>();
			if (func.lua_state() == lua)
			{
				this->Data.Emplace<sol::nil_t>();
			}
			break;
		}
	case LuaValueData::IndexOfType<sol::table>():
		{
			const sol::table& ref = this->Get<sol::table>();
			if (ref.lua_state() == lua)
			{
				this->Data.Emplace<sol::nil_t>();
			}
			break;
		}
	case LuaValueData::IndexOfType<FLuaRPCFunction>():
		{
			const FLuaRPCFunction& f = this->Get<FLuaRPCFunction>();
			if(f.LuaFunc.lua_state() == lua)
			{
				this->Data.Emplace<sol::nil_t>();	
			}
			break;
		}
	case LuaValueData::IndexOfType<FLuaUFunctionReference>():
		{
			FLuaUFunctionReference& f = this->GetMutable<FLuaUFunctionReference>();
			if(f.LuaFunc.lua_state() == lua)
			{
				f.LuaFunc = sol::nil;	
			}
			break;
		}
	case LuaValueData::IndexOfType<FLuaTableHandle>():
		{
			const FLuaTableHandle& ref = this->GetMutable<FLuaTableHandle>();
			if (!ref.IsValid() || ref.GetTable().lua_state() == lua)
			{
				this->Data.Emplace<sol::nil_t>();
			}
			break;
		}
	case LuaValueData::IndexOfType<FLuaFunctionHandle>():
		{
			const FLuaFunctionHandle& ref = this->Get<FLuaFunctionHandle>();
			if (!ref.IsValid() || ref.GetFunction().lua_state() == lua)
			{
				this->Data.Emplace<sol::nil_t>();
			}
			break;
		}
	case LuaValueData::IndexOfType<FLuaCoroutineHandle>():
		{
			const FLuaCoroutineHandle& ref = this->Get<FLuaCoroutineHandle>();
			if (!ref.IsValid() || ref.GetCoroutine().lua_state() == lua)
			{
				this->Data.Emplace<sol::nil_t>();
			}
			break;
		}
	default:
		break;
	}	
}

FLuaDelegateHandle FLuaValue::AddDelegateListener(const FLuaDelegate& delToAdd)
{
	if (this->IsPropertyOrUFunctionReference())
	{
		return {};
	}
	//If this Lua value is uninitialized or nil, allow automatically converting it to a multicast delegate
	switch (this->GetData().GetIndex())
	{
	case LuaValueData::IndexOfType<std::nullptr_t>(): [[fallthrough]];
	case LuaValueData::IndexOfType<sol::nil_t>():
		{
			this->Data.Emplace<FLuaScriptDelegate>();
			break;
		}
	default:
		break;
	}
	
	if (this->GetData().GetIndex() == LuaValueData::IndexOfType<FLuaScriptDelegate>())
	{
		FLuaScriptDelegate& del = this->Data.Get<FLuaScriptDelegate>();
		return del.Add(delToAdd);
	}
	return {};	
}

FLuaDelegateHandle FLuaValue::AddMulticastDelegateListener(const FLuaDelegate& delToAdd)
{
	if (this->IsPropertyOrUFunctionReference())
	{
		return {};
	}
	//If this Lua value is uninitialized or nil, allow automatically converting it to a multicast delegate
	switch (this->GetData().GetIndex())
	{
		case LuaValueData::IndexOfType<std::nullptr_t>(): [[fallthrough]];
		case LuaValueData::IndexOfType<sol::nil_t>():
			{
				this->Data.Emplace<FLuaScriptMulticastDelegate>();
				break;
			}
		default:
			break;
	}
	
	if (this->GetData().GetIndex() == LuaValueData::IndexOfType<FLuaScriptMulticastDelegate>())
	{
		FLuaScriptMulticastDelegate& mcdel = this->Data.Get<FLuaScriptMulticastDelegate>();
		return mcdel.AddDynamicListener(delToAdd);
	}
	return {};
}

bool FLuaValue::UnbindMulticastDelegateListener(const FLuaDelegate& delToRemove)
{
	if (this->IsType<FLuaScriptMulticastDelegate>())
	{
		this->GetMutable<FLuaScriptMulticastDelegate>().RemoveDynamicListener(delToRemove);
		return true;
	}
	return false;
}

bool FLuaValue::UnbindMulticastDelegateListener(FLuaDelegateHandle handle)
{
	if (this->IsType<FLuaScriptMulticastDelegate>())
	{
		this->GetMutable<FLuaScriptMulticastDelegate>().RemoveHandle(handle);
		return true;
	}
	else if (this->IsType<FLuaScriptDelegate>())
	{
		this->GetMutable<FLuaScriptDelegate>().RemoveHandle(handle);
		return true;
	}
	return false;
}

bool FLuaValue::BroadcastLuaDelegate(const TArray<FLuaValue>& args)
{
	if (this->IsType<FLuaScriptMulticastDelegate>())
	{
		this->Get<FLuaScriptMulticastDelegate>().Execute(args);
		return true;
	}
	return false;
}


/*
static ELuaValueType luaValueIndexToType[30] = 
{
	ELuaValueType::Nil,			//std::nullptr_t,
	ELuaValueType::Nil,			//sol::nil_t,
	ELuaValueType::Property,	//FPropertyReferenceWrapper, 
	ELuaValueType::Boolean,		//bool,
	ELuaValueType::Integer,			//int64,
	ELuaValueType::Float,		//double,
	ELuaValueType::String,			//std::string,
	ELuaValueType::Vector2,			//FVector2D,
	ELuaValueType::Vector2,			//FVector,
	ELuaValueType::Rotator,			//FRotator,
	ELuaValueType::UObject,			//TObjectPtr<UObject>,
	ELuaValueType::Transform,			//FTransformContainer,
	ELuaValueType::LuaTable,			//sol::table,
	ELuaValueType::LuaFunction,			//FLuaRPCFunction,
	ELuaValueType::LuaFunction,			//sol::function,
	ELuaValueType::UClass,			//FLuaUClass,
	ELuaValueType::UScriptStruct,			//FLuaUStruct,
			//FLuaUEnumEntry,
			//FLuaUFunctionReference,
			//FLuaScriptStruct,
			//FLuaSharedStruct,
			//FLuaInstancedStruct,
			//FLuaArray,
			//FLuaMap,
			//FLuaSet,
			//FLuaTableHandle,
			//FLuaFunctionHandle,
			//FLuaCoroutineHandle,
			//FLuaScriptDelegate,
			//FLuaScriptMulticastDelegate
};
*/
ELuaValueType FLuaValue::GetType() const
{
	switch(this->GetData().GetIndex())
	{
	case LuaValueData::IndexOfType<std::nullptr_t>(): [[fallthrough]];
	case LuaValueData::IndexOfType<sol::nil_t>():
		{
			return ELuaValueType::Nil;
		}
	case LuaValueData::IndexOfType<FPropertyReferenceWrapper>():
		{
			return ELuaValueType::Property;
		}
	case LuaValueData::IndexOfType<bool>():
		{
			return ELuaValueType::Boolean;
		}
	case LuaValueData::IndexOfType<int64>():
		{
			return ELuaValueType::Integer;
		}
	case LuaValueData::IndexOfType<double>():
		{
			return ELuaValueType::Float;
		}
	case LuaValueData::IndexOfType<std::string>():
		{
			return ELuaValueType::String;
		}
	case LuaValueData::IndexOfType<FVector2D>():
		{
			return ELuaValueType::Vector2D;
		}
	case LuaValueData::IndexOfType<FVector>():
		{
			return ELuaValueType::Vector;
		}
	case LuaValueData::IndexOfType<FRotator>():
		{
			return ELuaValueType::Rotator;
		}
	case LuaValueData::IndexOfType<TObjectPtr<UObject>>():
		{
			return ELuaValueType::UObject;
		}
	case LuaValueData::IndexOfType<FLuaUClass>():
		{
			return ELuaValueType::UClass;
		}
	case LuaValueData::IndexOfType<FLuaUStruct>():
		{
			return ELuaValueType::UScriptStruct;
		}
	case LuaValueData::IndexOfType<TLuaVariantPtr<FTransform>>():
		{
			return ELuaValueType::Transform;
		}
	case LuaValueData::IndexOfType<FLuaUEnumEntry*>():
		{
			return ELuaValueType::UEnum;
		}
	case LuaValueData::IndexOfType<FLuaScriptStruct>():
		{
			return ELuaValueType::ScriptStruct;
		}
	case LuaValueData::IndexOfType<FLuaArray>():
		{
			return ELuaValueType::Array;
		}
	case LuaValueData::IndexOfType<FLuaSet>():
		{
			return ELuaValueType::Set;
		}
	case LuaValueData::IndexOfType<FLuaMap>():
		{
			return ELuaValueType::Map;
		}
	case LuaValueData::IndexOfType<FLuaUFunctionReference>():
		{
			return ELuaValueType::UFunction;
		}
	case LuaValueData::IndexOfType<FLuaRPCFunction>():
		{
			return ELuaValueType::LuaFunction;
		}
	case LuaValueData::IndexOfType<FLuaSharedStruct>():
		{
			return ELuaValueType::SharedStruct;
		}
	case LuaValueData::IndexOfType<FLuaInstancedStruct>():
		{
			return ELuaValueType::InstancedStruct;
		}
	case LuaValueData::IndexOfType<FLuaFunctionHandle>():
		{
			return ELuaValueType::LuaFunction;
		}
	case LuaValueData::IndexOfType<FLuaTableHandle>():
		{
			return ELuaValueType::LuaTable;
		}
	case LuaValueData::IndexOfType<FLuaCoroutineHandle>():
		{
			return ELuaValueType::Coroutine;
		}
	default:
		{
			return ELuaValueType::Nil;
		}
	}
}

sol::object FLuaValue::GetValue(sol::this_state lua) const
{
	//@TODO : These only need to run for FLuaArray and FLuaScriptStruct-types... perhaps give these types a "FProperty* ReferenceProp" so they know they are a reference, instead of "bOwnsMemory"? 
	//UnrealLua::PropertyHelper::HandleGetPropertyNetBehavior(owner, this->Prop);

	sol::object result{sol::nil};
	switch(this->GetData().GetIndex())
	{
	default:
		{
			checkNoEntry();
			break;
		}
	case LuaValueData::IndexOfType<std::nullptr_t>():
		break;
	case LuaValueData::IndexOfType<sol::nil_t>():
		break;
	case LuaValueData::IndexOfType<FPropertyReferenceWrapper>():
		{
			const FPropertyReferenceWrapper& wrapper = this->GetData().Get<FPropertyReferenceWrapper>(); 
			FProperty* prop = wrapper.Prop;
			UObject* owner = wrapper.Owner;
			if(!IsValid(owner))
			{
				return result;
			}
			FPlatformMisc::Prefetch(prop);
			FGetPropertyValueParams params{prop, owner, 0, lua};
			result = UnrealLua::PropertyHelper::GetPropertyValue_InContainer(params);
			break;
		}
	case LuaValueData::IndexOfType<bool>():
		{
			bool val = this->GetData().Get<bool>();
			result = sol::object(lua, sol::in_place, val);
			break;
		}
	case LuaValueData::IndexOfType<int64>():
		{
			auto val = this->GetData().Get<int64>();
			result = sol::object(lua, sol::in_place, val);
			break;
		}
	case LuaValueData::IndexOfType<double>():
		{
			auto val = this->GetData().Get<double>();
			result = sol::object(lua, sol::in_place, val);
			break;
		}
	case LuaValueData::IndexOfType<std::string>():
		{
			auto& val = this->GetData().Get<std::string>();
			result = sol::object(lua, sol::in_place, val.c_str());
			break;
		}
	case LuaValueData::IndexOfType<FVector2D>():
		{
			const FVector2D& val = this->GetData().Get<FVector2D>();
			result = sol::object(lua, sol::in_place, val);
			break;
		}
	case LuaValueData::IndexOfType<FVector>():
		{
			const FVector& val = this->GetData().Get<FVector>();
			result = sol::object(lua, sol::in_place, val);
			break;
		}
	case LuaValueData::IndexOfType<FRotator>():
		{
			const FRotator& val = this->GetData().Get<FRotator>();
			result = sol::object(lua, sol::in_place, val);
			break;
		}
	case LuaValueData::IndexOfType<TObjectPtr<UObject>>():
		{
			UObject* val = this->GetData().Get<TObjectPtr<UObject>>();
			if(IsValid(val) && !val->HasAnyFlags(RF_BeginDestroyed | RF_MirroredGarbage))
			{
				result = sol::make_object(lua, val);
				verify(UnrealLua::IsUObject(result));
			}
			else
			{
				const_cast<FLuaValue*>(this)->GetData().Emplace<sol::nil_t>();
			}
			break;
		}
	case LuaValueData::IndexOfType<TLuaVariantPtr<FTransform>>():
		{
			const FTransform* val = this->GetData().Get<TLuaVariantPtr<FTransform>>().Get();
			result = sol::object(lua, sol::in_place_type<FTransform>, *val);
			break;
		}
	case LuaValueData::IndexOfType<FLuaRPCFunction>():
		{
			const FLuaRPCFunction& ref = this->GetData().Get<FLuaRPCFunction>();
			result = ref.GetValue(lua);
			break;
		}
	case LuaValueData::IndexOfType<FLuaUEnumEntry*>():
		{	
			FLuaUEnumEntry* entry = this->GetData().Get<FLuaUEnumEntry*>();
			result = UnrealLua::LightUserdata::GetUEnumValueAsTaggedLightUserdata(entry, lua);
			break;
		}
	case LuaValueData::IndexOfType<FLuaUFunctionReference>():
		{
			const FLuaUFunctionReference& ref = this->GetData().Get<FLuaUFunctionReference>();
			result = ref.GetValue(lua);
			break;
		}
	case LuaValueData::IndexOfType<FLuaScriptStruct>():
		{
			constexpr int32 s = sizeof(sol::object);
			constexpr int32 r = sizeof(sol::reference);
			constexpr int32 b = sizeof(sol::basic_reference<false>);
			constexpr int32 sl = sizeof(sol::stateless_reference);
			constexpr int32 i = sizeof(int);
			const FLuaScriptStruct& ref = this->GetData().Get<FLuaScriptStruct>();
			result = sol::object(lua, sol::in_place_type<FLuaScriptStruct>, ref);
			break;
		}
	case LuaValueData::IndexOfType<FLuaInstancedStruct>():
		{
			const FLuaInstancedStruct& ref = this->GetData().Get<FLuaInstancedStruct>();
			result = sol::object(lua, sol::in_place_type<FLuaInstancedStruct>, ref);
			break;
		}
	case LuaValueData::IndexOfType<FLuaSharedStruct>():
		{
			const FLuaSharedStruct& ref = this->GetData().Get<FLuaSharedStruct>();
			result = sol::object(lua, sol::in_place_type<FLuaSharedStruct>, ref);
			break;
		}
	case LuaValueData::IndexOfType<FLuaArray>():
		{
			const FLuaArray& ref = this->GetData().Get<FLuaArray>();
			result = sol::object(lua, sol::in_place_type<FLuaArray>, ref);
			break;
		}
	case LuaValueData::IndexOfType<FLuaMap>():
		{
			const FLuaMap& ref = this->GetData().Get<FLuaMap>();
			result = sol::object(lua, sol::in_place_type<FLuaMap>, ref);
			break;
		}
	case LuaValueData::IndexOfType<FLuaSet>():
		{
			const FLuaSet& ref = this->GetData().Get<FLuaSet>();
			result = sol::object(lua, sol::in_place_type<FLuaSet>, ref);
			break;
		}
	case LuaValueData::IndexOfType<sol::table>():
		{
			const sol::table& ref = this->Get<sol::table>();
			result = sol::object(ref);
			break;
		}
	case LuaValueData::IndexOfType<FLuaTableHandle>():
		{
			const FLuaTableHandle& ref = this->Get<FLuaTableHandle>();
			result = sol::object(lua, ref.GetTable());
			break;
		}
	case LuaValueData::IndexOfType<sol::function>():
		{
			const sol::function& ref = this->Get<sol::function>();
			result = sol::object(ref);
			break;
		}
	case LuaValueData::IndexOfType<FLuaFunctionHandle>():
		{
			const FLuaFunctionHandle& ref = this->Get<FLuaFunctionHandle>();
			result = sol::object(lua, ref.GetFunction());
			break;
		}
	case LuaValueData::IndexOfType<FLuaCoroutineHandle>():
		{
			const FLuaCoroutineHandle& ref = this->Get<FLuaCoroutineHandle>();
			result = sol::object(lua, ref.GetCoroutine());
			break;
		}
	/*
	case LuaValueData::IndexOfType<FMulticastDelegatePropertyProxy>():
		{
			const FMulticastDelegatePropertyProxy& ref = this->GetData().Get<FMulticastDelegatePropertyProxy>();
			result = sol::object(lua, sol::in_place_type<FMulticastDelegatePropertyProxy>, ref);
			break;
		}
	case LuaValueData::IndexOfType<FSingleDelegatePropertyProxy>():
		{
			const FMulticastDelegatePropertyProxy& ref = this->GetData().Get<FMulticastDelegatePropertyProxy>();
			result = sol::object(lua, sol::in_place_type<FMulticastDelegatePropertyProxy>, ref);
			break;
		}
	*/
	}
	return result;
}

int FLuaValue::PushValue(sol::this_state lua) const
{
	switch(this->GetData().GetIndex())
	{
	default:
		{
			return sol::stack::push(lua, sol::nil);
		}
	case LuaValueData::IndexOfType<std::nullptr_t>(): [[fallthrough]];
	case LuaValueData::IndexOfType<sol::nil_t>():
		return sol::stack::push(lua, sol::nil);
	case LuaValueData::IndexOfType<FPropertyReferenceWrapper>():
		{
			FPropertyReferenceWrapper& wrapper = this->GetData().Get<FPropertyReferenceWrapper>();

			FProperty* prop = wrapper.Prop;
			UObject* owner = wrapper.Owner;
			FPlatformMisc::Prefetch(prop);
			FPushPropertyValueParams params{prop, owner, 0, lua};
			return UnrealLua::PropertyHelper::GetPropertyValue_InContainer(params);
		}
	case LuaValueData::IndexOfType<bool>():
		{
			bool val = this->GetData().Get<bool>();
			return sol::stack::push(lua, val);
		}
	case LuaValueData::IndexOfType<int64>():
		{
			int64 val = this->GetData().Get<int64>();
			return sol::stack::push(lua, val);
		}
	case LuaValueData::IndexOfType<double>():
		{
			double val = this->GetData().Get<double>();
			return sol::stack::push(lua, val);
		}
	case LuaValueData::IndexOfType<std::string>():
		{
			std::string val = this->GetData().Get<std::string>();
			return sol::stack::push(lua, val);
		}
	case LuaValueData::IndexOfType<FVector2D>():
		{
			FVector2D val = this->GetData().Get<FVector2D>();			
			return sol::stack::push(lua, val);
		}
	case LuaValueData::IndexOfType<FVector>():
		{
			FVector val = this->GetData().Get<FVector>();
			return sol::stack::push(lua, val);
		}
	case LuaValueData::IndexOfType<FRotator>():
		{
			FRotator val = this->GetData().Get<FRotator>();
			return sol::stack::push(lua, val);
		}
	case LuaValueData::IndexOfType<TObjectPtr<UObject>>():
		{
			UObject* val = this->GetData().Get<TObjectPtr<UObject>>();
			if(val && !val->HasAnyFlags(RF_BeginDestroyed | RF_MirroredGarbage))
			{
				return sol::stack::push(lua, val);	
			}
			else
			{
				this->GetData().Emplace<sol::nil_t>();
				return sol::stack::push(lua, sol::nil);				
			}
		}
	case LuaValueData::IndexOfType<TLuaVariantPtr<FTransform>>():
		{
			const FTransform* val = this->GetData().Get<TLuaVariantPtr<FTransform>>().Get();
			return sol::stack::push(lua, *val);
		}
	case LuaValueData::IndexOfType<FLuaRPCFunction>():
		{
			const FLuaRPCFunction& ref = this->GetData().Get<FLuaRPCFunction>();
			return sol::stack::push(lua, ref.GetValue(lua));
		}
	case LuaValueData::IndexOfType<FLuaUEnumEntry*>():
		{
			FLuaUEnumEntry* entry = this->GetData().Get<FLuaUEnumEntry*>();
			return UnrealLua::LightUserdata::PushUEnumValueAsTaggedLightUserdata(entry, lua);
		}
	case LuaValueData::IndexOfType<FLuaUFunctionReference>():
		{
			const FLuaUFunctionReference& ref = this->GetData().Get<FLuaUFunctionReference>();
			return sol::stack::push(lua, ref.GetValue(lua));
		}
	case LuaValueData::IndexOfType<FLuaScriptStruct>():
		{
			const FLuaScriptStruct& ref = this->GetData().Get<FLuaScriptStruct>();
			return sol::stack::push(lua, ref);
		}
	case LuaValueData::IndexOfType<FLuaInstancedStruct>():
		{
			const FLuaInstancedStruct& ref = this->GetData().Get<FLuaInstancedStruct>();
			return sol::stack::push(lua, FLuaInstancedStruct{ref, true});
		}
	case LuaValueData::IndexOfType<FLuaSharedStruct>():
		{
			const FLuaSharedStruct& ref = this->GetData().Get<FLuaSharedStruct>();
			return sol::stack::push(lua, ref);
		}
	case LuaValueData::IndexOfType<FLuaArray>():
		{
			const FLuaArray& ref = this->GetData().Get<FLuaArray>();
			return sol::stack::push(lua, ref);
		}
	case LuaValueData::IndexOfType<FLuaMap>():
		{
			const FLuaMap& ref = this->GetData().Get<FLuaMap>();
			return sol::stack::push(lua, ref);
		}
	case LuaValueData::IndexOfType<FLuaSet>():
		{
			const FLuaSet& ref = this->GetData().Get<FLuaSet>();
			return sol::stack::push(lua, ref);
		}
	case LuaValueData::IndexOfType<sol::table>():
		{
			const sol::table& ref = this->Get<sol::table>();
			return ref.push();
		}
	case LuaValueData::IndexOfType<FLuaTableHandle>():
		{
			const FLuaTableHandle& ref = this->Get<FLuaTableHandle>();
			return sol::stack::push(lua, ref.GetTable());
		}
	case LuaValueData::IndexOfType<sol::function>():
		{
			const sol::function& ref = this->Get<sol::function>();
			return ref.push();
		}
	case LuaValueData::IndexOfType<FLuaFunctionHandle>():
		{
			const FLuaFunctionHandle& ref = this->Get<FLuaFunctionHandle>();
			return sol::stack::push(lua, ref.GetFunction());
		}
	case LuaValueData::IndexOfType<FLuaCoroutineHandle>():
		{
			const FLuaCoroutineHandle& ref = this->Get<FLuaCoroutineHandle>();
			return sol::stack::push(lua, ref.GetCoroutine());
		}
	}
	
}

//Used by UUnrealLuaUtility and FFunctionDescr to write to Blueprint graph
ESetLuaValueResult FLuaValue::WriteValueToPropertyMemoryAddress_WithPropertyTypeCheck(FProperty* targetPropertyToWriteTo, void* memAddressToWriteTo) const
{
	switch(this->GetData().GetIndex())
	{
	default:
		{
			unimplemented();
			targetPropertyToWriteTo->InitializeValue(memAddressToWriteTo);
			return ESetLuaValueResult::Error;
		}
	case LuaValueData::IndexOfType<FPropertyReferenceWrapper>():
		{
			targetPropertyToWriteTo->InitializeValue(memAddressToWriteTo);
				
			FPropertyReferenceWrapper& wrapper = this->GetData().Get<FPropertyReferenceWrapper>();
			FProperty* prop = wrapper.Prop;
			UObject* owner = wrapper.Owner;
			if(!IsValid(owner))
			{
				return ESetLuaValueResult::Error;
			}
			if(prop->SameType(targetPropertyToWriteTo))
			{
				void* ownerPropAddress = prop->ContainerPtrToValuePtr<void>(owner);
				targetPropertyToWriteTo->CopyCompleteValue(targetPropertyToWriteTo, ownerPropAddress);
				return ESetLuaValueResult::Success;
			}
			return ESetLuaValueResult::Error;
		}
	case LuaValueData::IndexOfType<std::nullptr_t>(): [[fallthrough]];
	case LuaValueData::IndexOfType<sol::nil_t>():
		{
			targetPropertyToWriteTo->InitializeValue(memAddressToWriteTo);
			return ESetLuaValueResult::Error;	
		}
	case LuaValueData::IndexOfType<bool>():
		{
			if(FBoolProperty* bProp = CastField<FBoolProperty>(targetPropertyToWriteTo))
			{
				bool val = this->GetData().Get<bool>();
				bProp->SetPropertyValue(memAddressToWriteTo, val);
				return ESetLuaValueResult::Success;	
			}
			targetPropertyToWriteTo->InitializeValue(memAddressToWriteTo);
			return ESetLuaValueResult::Error;
		}
	case LuaValueData::IndexOfType<int64>():
		{
			int64 val = this->GetData().Get<int64>();
			if(FNumericProperty* targetProp = CastField<FNumericProperty>(targetPropertyToWriteTo))
			{
				if(targetProp->IsInteger())
				{
					targetProp->SetIntPropertyValue(memAddressToWriteTo, val);	
				}
				else
				{
					double dval = val;
					targetProp->SetFloatingPointPropertyValue(memAddressToWriteTo, dval);
				}
				return ESetLuaValueResult::Success;	
			}
			targetPropertyToWriteTo->InitializeValue(memAddressToWriteTo);
			return ESetLuaValueResult::Error;
		}
	case LuaValueData::IndexOfType<double>():
		{
			double val = this->GetData().Get<double>();
			if(FNumericProperty* targetProp = CastField<FNumericProperty>(targetPropertyToWriteTo))
			{
				if(targetProp->IsInteger())
				{
					int64 ival = val;
					targetProp->SetIntPropertyValue(memAddressToWriteTo, ival);	
				}
				else
				{
					targetProp->SetFloatingPointPropertyValue(memAddressToWriteTo, val);
				}
				return ESetLuaValueResult::Success;	
			}
			targetPropertyToWriteTo->InitializeValue(memAddressToWriteTo);
			return ESetLuaValueResult::Error;
		}
	case LuaValueData::IndexOfType<std::string>():
		{
			auto& val = this->GetData().Get<std::string>();
			if(FStrProperty* sProp = CastField<FStrProperty>(targetPropertyToWriteTo))
			{
				FString str = val.c_str();
				sProp->SetPropertyValue(memAddressToWriteTo, str);
				return ESetLuaValueResult::Success;
			}
			else if(FNameProperty* nProp = CastField<FNameProperty>(targetPropertyToWriteTo))
			{
				FString str = val.c_str();
				nProp->SetPropertyValue(memAddressToWriteTo, *str);
				return ESetLuaValueResult::Success;
			}
			targetPropertyToWriteTo->InitializeValue(memAddressToWriteTo);
			return ESetLuaValueResult::Error;
		}
	case LuaValueData::IndexOfType<FLuaScriptStruct>():
		{
			if(FStructProperty* sprop = CastField<FStructProperty>(targetPropertyToWriteTo))
			{
				FLuaScriptStruct& lss = this->GetData().Get<FLuaScriptStruct>();
				const UScriptStruct* ss = lss.GetScriptStruct();
				if(ss && ss->IsChildOf(sprop->Struct))
				{
					sprop->CopySingleValue(memAddressToWriteTo, lss.GetMemory());
					return ESetLuaValueResult::Success;
				}
			}
			targetPropertyToWriteTo->InitializeValue(memAddressToWriteTo);
			return ESetLuaValueResult::Error;
		}
	case LuaValueData::IndexOfType<FLuaInstancedStruct>():
		{
			if(FStructProperty* sprop = CastField<FStructProperty>(targetPropertyToWriteTo))
			{
				if(sprop->Struct == UnrealLua::StaticPackages::InstancedStruct)
				{
					FInstancedStruct* shared = static_cast<FInstancedStruct*>(memAddressToWriteTo);
					FLuaInstancedStruct& lss = this->GetData().Get<FLuaInstancedStruct>();
					const UScriptStruct* ss = lss.GetScriptStruct();
					if(ss)
					{
						shared->InitializeAs(ss, static_cast<uint8*>(lss.GetMemory()));
						return ESetLuaValueResult::Success;
					}						
				}
			}
			targetPropertyToWriteTo->InitializeValue(memAddressToWriteTo);
			return ESetLuaValueResult::Error;
		}
	case LuaValueData::IndexOfType<FLuaSharedStruct>():
		{
			if(FStructProperty* sprop = CastField<FStructProperty>(targetPropertyToWriteTo))
			{
				if(sprop->Struct == UnrealLua::StaticPackages::SharedStruct)
				{
					FSharedStruct* shared = static_cast<FSharedStruct*>(memAddressToWriteTo);
					FLuaSharedStruct& lss = this->GetData().Get<FLuaSharedStruct>();
					const UScriptStruct* ss = lss.GetScriptStruct();
					if(ss)
					{
						shared->InitializeAs(ss, static_cast<uint8*>(lss.GetMemory()));
						return ESetLuaValueResult::Success;
					}						
				}
			}
			targetPropertyToWriteTo->InitializeValue(memAddressToWriteTo);
			return ESetLuaValueResult::Error;
		}
	case LuaValueData::IndexOfType<FVector2D>():
		{
			if(FStructProperty* sprop = CastField<FStructProperty>(targetPropertyToWriteTo))
			{
				FVector2D& val = this->GetData().Get<FVector2D>();
				if(sprop->Struct == UnrealLua::StaticPackages::Vector2DStruct)
				{
					targetPropertyToWriteTo->CopySingleValue(memAddressToWriteTo, &val);
					return ESetLuaValueResult::Success;
				}
				else if(sprop->Struct == UnrealLua::StaticPackages::VectorStruct)
				{
					FVector vec{val.X, val.Y, 0};
					targetPropertyToWriteTo->CopySingleValue(memAddressToWriteTo, &vec);
					return ESetLuaValueResult::Success;
				}
			}
			targetPropertyToWriteTo->InitializeValue(memAddressToWriteTo);
			return ESetLuaValueResult::Error;
		}
	case LuaValueData::IndexOfType<FVector>():
		{
			if(FStructProperty* sprop = CastField<FStructProperty>(targetPropertyToWriteTo))
			{
				FVector& val = this->GetData().Get<FVector>();
				if(sprop->Struct == UnrealLua::StaticPackages::VectorStruct)
				{
						
					targetPropertyToWriteTo->CopySingleValue(memAddressToWriteTo, &val);
					return ESetLuaValueResult::Success;
				}
				else if(sprop->Struct == UnrealLua::StaticPackages::Vector2DStruct)
				{
					FVector2D vec{val.X, val.Y};
					targetPropertyToWriteTo->CopySingleValue(memAddressToWriteTo, &vec);
					return ESetLuaValueResult::Success;
				}
			}
			targetPropertyToWriteTo->InitializeValue(memAddressToWriteTo);
			return ESetLuaValueResult::Error;
		}
	case LuaValueData::IndexOfType<FRotator>():
		{
			if(FStructProperty* sprop = CastField<FStructProperty>(targetPropertyToWriteTo))
			{
				FRotator& val = this->GetData().Get<FRotator>();
				if(sprop->Struct == UnrealLua::StaticPackages::RotatorStruct)
				{
					targetPropertyToWriteTo->CopySingleValue(memAddressToWriteTo, &val);
					return ESetLuaValueResult::Success;
				}
				else if(sprop->Struct == UnrealLua::StaticPackages::VectorStruct)
				{
					FVector vec{val.Vector()};
					targetPropertyToWriteTo->CopySingleValue(memAddressToWriteTo, &vec);
					return ESetLuaValueResult::Success;
				}
			}
			targetPropertyToWriteTo->InitializeValue(memAddressToWriteTo);
			return ESetLuaValueResult::Error;
		}
	case LuaValueData::IndexOfType<TObjectPtr<UObject>>():
		{
			UObject* val = this->GetData().Get<TObjectPtr<UObject>>();
			if(FObjectProperty* oprop = CastField<FObjectProperty>(targetPropertyToWriteTo))
			{
				oprop->SetPropertyValue(memAddressToWriteTo, val);
				return ESetLuaValueResult::Success;
			}
			targetPropertyToWriteTo->InitializeValue(memAddressToWriteTo);
			return ESetLuaValueResult::Error;
		}
	case LuaValueData::IndexOfType<TLuaVariantPtr<FTransform>>():
		{
			FTransform* val = this->GetData().Get<TLuaVariantPtr<FTransform>>().Get();
			if(FStructProperty* sprop = CastField<FStructProperty>(targetPropertyToWriteTo))
			{
				if(sprop->Struct == UnrealLua::StaticPackages::TransformStruct)
				{
					targetPropertyToWriteTo->CopySingleValue(memAddressToWriteTo, val);
					return ESetLuaValueResult::Success;
				}
				else if(sprop->Struct == UnrealLua::StaticPackages::RotatorStruct)
				{
					FRotator rot = val->Rotator();
					targetPropertyToWriteTo->CopySingleValue(memAddressToWriteTo, &rot);
					return ESetLuaValueResult::Success;
				}
				else if(sprop->Struct == UnrealLua::StaticPackages::VectorStruct)
				{
					FVector vec{val->GetLocation()};
					targetPropertyToWriteTo->CopySingleValue(memAddressToWriteTo, &vec);
					return ESetLuaValueResult::Success;
				}
			}
			targetPropertyToWriteTo->InitializeValue(memAddressToWriteTo);
			return ESetLuaValueResult::Error;
		}
		case LuaValueData::IndexOfType<FLuaRPCFunction>():
			{
				unimplemented();
				targetPropertyToWriteTo->InitializeValue(memAddressToWriteTo);
				return ESetLuaValueResult::Error;
			}
		case LuaValueData::IndexOfType<sol::function>():
			{
				if (FStructProperty* sprop = CastField<FStructProperty>(targetPropertyToWriteTo))
				{
					const sol::function& func = this->Get<sol::function>();
					if (sprop->Struct == UnrealLua::StaticPackages::LuaFunction)
					{
						FLuaFunctionHandle* valuePtr = static_cast<FLuaFunctionHandle*>(memAddressToWriteTo);
						*valuePtr = FLuaFunctionHandle::MakeHandle(func);
						return ESetLuaValueResult::Success;
					}
					else if (sprop->Struct == UnrealLua::StaticPackages::LuaValue)
					{
						FLuaValue* valuePtr = static_cast<FLuaValue*>(memAddressToWriteTo);
						valuePtr->Emplace<FLuaFunctionHandle>(FLuaFunctionHandle::MakeHandle(func));
						return ESetLuaValueResult::Success;
					}
				}
				return ESetLuaValueResult::Error;
			}
		case LuaValueData::IndexOfType<FLuaFunctionHandle>():
			{
				if (FStructProperty* sprop = CastField<FStructProperty>(targetPropertyToWriteTo))
				{
					const FLuaFunctionHandle& thisHandle = this->Get<FLuaFunctionHandle>();
					if (sprop->Struct == UnrealLua::StaticPackages::LuaFunction)
					{
						FLuaFunctionHandle* valuePtr = static_cast<FLuaFunctionHandle*>(memAddressToWriteTo);
						*valuePtr = thisHandle ;
						return ESetLuaValueResult::Success;
					}
					else if (sprop->Struct == UnrealLua::StaticPackages::LuaValue)
					{
						FLuaValue* valuePtr = static_cast<FLuaValue*>(memAddressToWriteTo);
						valuePtr->Emplace<FLuaFunctionHandle>(thisHandle );
						return ESetLuaValueResult::Success;
					}
				}
				return ESetLuaValueResult::Error;
			}
		case LuaValueData::IndexOfType<FLuaUEnumEntry*>():
			{
				FLuaUEnumEntry* val = this->GetData().Get<FLuaUEnumEntry*>();
				if(FEnumProperty* eprop = CastField<FEnumProperty>(targetPropertyToWriteTo))
				{
					if(eprop->GetEnum() == val->uenum)
					{
						int64 value = val->Value;
						eprop->GetUnderlyingProperty()->SetIntPropertyValue(memAddressToWriteTo, value);
						return ESetLuaValueResult::Success;
					}
				}
				targetPropertyToWriteTo->InitializeValue(memAddressToWriteTo);
				return ESetLuaValueResult::Error;
			}
		
		case LuaValueData::IndexOfType<sol::table>():
			{
				if(FStructProperty* sprop = CastField<FStructProperty>(targetPropertyToWriteTo))
				{
					const sol::table& ref = this->Get<sol::table>();
					if(sprop->Struct == UnrealLua::StaticPackages::LuaTable)
					{
						FLuaTableHandle* valuePtr = static_cast<FLuaTableHandle*>(memAddressToWriteTo);
						*valuePtr = FLuaTableHandle::MakeHandle(ref);
						return ESetLuaValueResult::Success;
					}
					else if (sprop->Struct == UnrealLua::StaticPackages::LuaValue)
					{
						FLuaValue* valuePtr = static_cast<FLuaValue*>(memAddressToWriteTo);
						valuePtr->Emplace<FLuaTableHandle>(FLuaTableHandle::MakeHandle(ref));
						return ESetLuaValueResult::Success;
					}
				}
				targetPropertyToWriteTo->InitializeValue(memAddressToWriteTo);
				return ESetLuaValueResult::Error;
			}
		case LuaValueData::IndexOfType<FLuaTableHandle>():
			{
				if(FStructProperty* sprop = CastField<FStructProperty>(targetPropertyToWriteTo))
				{
					FLuaTableHandle& ref = this->GetData().Get<FLuaTableHandle>();
					if(sprop->Struct == UnrealLua::StaticPackages::LuaTable)
					{
						FLuaTableHandle* valuePtr = static_cast<FLuaTableHandle*>(memAddressToWriteTo);
						*valuePtr = ref;
						return ESetLuaValueResult::Success;
					}
					else if (sprop->Struct == UnrealLua::StaticPackages::LuaValue)
					{
						FLuaValue* valuePtr = static_cast<FLuaValue*>(memAddressToWriteTo);
						valuePtr->Emplace<FLuaTableHandle>(ref);
						return ESetLuaValueResult::Success;
					}
				}
				targetPropertyToWriteTo->InitializeValue(memAddressToWriteTo);
				return ESetLuaValueResult::Error;
			}
		case LuaValueData::IndexOfType<FLuaCoroutineHandle>():
			{
				if(FStructProperty* sprop = CastField<FStructProperty>(targetPropertyToWriteTo))
				{
					FLuaCoroutineHandle& ref = this->GetData().Get<FLuaCoroutineHandle>();
					if(sprop->Struct == UnrealLua::StaticPackages::LuaCoroutine)
					{
						FLuaCoroutineHandle* valuePtr = static_cast<FLuaCoroutineHandle*>(memAddressToWriteTo);
						*valuePtr = ref;
						return ESetLuaValueResult::Success;
					}
					else if (sprop->Struct == UnrealLua::StaticPackages::LuaValue)
					{
						FLuaValue* valuePtr = static_cast<FLuaValue*>(memAddressToWriteTo);
						valuePtr->Emplace<FLuaCoroutineHandle>(ref);
						return ESetLuaValueResult::Success;
					}
				}
				targetPropertyToWriteTo->InitializeValue(memAddressToWriteTo);
				return ESetLuaValueResult::Error;
			}
		case LuaValueData::IndexOfType<FLuaUFunctionReference>():
			{
				unimplemented();
				targetPropertyToWriteTo->InitializeValue(memAddressToWriteTo);
				return ESetLuaValueResult::Error;
			}
	case LuaValueData::IndexOfType<FLuaArray>():
		{
			FLuaArray& luaArr = this->GetData().Get<FLuaArray>();
			if (FArrayProperty* arrProp = CastField<FArrayProperty>(targetPropertyToWriteTo))
			{
				if (arrProp->Inner->SameType(luaArr.GetInner()))
				{
					FScriptArray* targetScriptArray = static_cast<FScriptArray*>(memAddressToWriteTo);
					FLuaArray::Copy(targetScriptArray, arrProp->Inner, luaArr.GetScriptArray(), luaArr.GetInner());
					return ESetLuaValueResult::Success;
				}
			}
			else if (FStructProperty* sprop = CastField<FStructProperty>(targetPropertyToWriteTo))
			{
				//if (sprop->Struct == UnrealLua::Packages::LuaArray)
				return ESetLuaValueResult::Error;
			}
			else if (targetPropertyToWriteTo->SameType(luaArr.GetInner()))
			{
				if (!luaArr.Lua_IsEmpty())
				{
					targetPropertyToWriteTo->CopySingleValue(memAddressToWriteTo, luaArr.GetData(0));
					return ESetLuaValueResult::Success;
				}
				else
				{
					targetPropertyToWriteTo->InitializeValue(memAddressToWriteTo);
					return ESetLuaValueResult::Error;
				}
			}
			return ESetLuaValueResult::Error;
		}
		//@TODO : Add support for Map and Set

	}
}

bool FLuaValue::AddStructReferencedObjects(FReferenceCollector& collector)
{
	switch(this->GetData().GetIndex())
	{
	case LuaValueData::IndexOfType<TObjectPtr<UObject>>():
		{
			TObjectPtr<UObject>& ptr = this->GetMutable<TObjectPtr<UObject>>();
			
			collector.AddReferencedObject(ptr);
			//LUA_LOG("FLuaValue referencing UObject %s", *GetNameSafe(ptr))
			if(!IsValid(ptr) || ptr->HasAnyFlags(RF_BeginDestroyed | RF_MirroredGarbage))
			{
				//LUA_LOG("FLuaValue : Invalidating TObjectPtr")
				this->GetData().Emplace<sol::nil_t>();
				//@TODO : broadcast
			}
			break;
		}
	case LuaValueData::IndexOfType<FLuaUFunctionReference>():
		{
			FLuaUFunctionReference& ref = this->GetData().Get<FLuaUFunctionReference>();
			collector.AddReferencedObject(const_cast<FFunctionDescr*>(ref.Func)->Func);
			break;
		}
	case LuaValueData::IndexOfType<FPropertyReferenceWrapper>():
		{
			FPropertyReferenceWrapper& wrapper = this->GetData().Get<FPropertyReferenceWrapper>();
			collector.AddReferencedObject(wrapper.Owner);
			break;
		}
	case LuaValueData::IndexOfType<FLuaScriptStruct>():
		{
			this->GetMutable<FLuaScriptStruct>().AddReferencedObjects(collector);
			break;
		}
	case LuaValueData::IndexOfType<FLuaInstancedStruct>():
		{
			this->GetMutable<FLuaInstancedStruct>().AddReferencedObjects(collector);
			break;
		}
	case LuaValueData::IndexOfType<FLuaSharedStruct>():
		{
			this->GetMutable<FLuaSharedStruct>().AddReferencedObjects(collector);
			break;
		}
	case LuaValueData::IndexOfType<FLuaArray>():
		{
			//Done vlua LuaGCObject in UnrealLuaGarbageCollector
			break;
		}
	case LuaValueData::IndexOfType<FLuaMap>():
		{
			//Done vlua LuaGCObject in UnrealLuaGarbageCollector
			break;
		}
	case LuaValueData::IndexOfType<FLuaSet>():
		{
			//Done vlua LuaGCObject in UnrealLuaGarbageCollector
			break;
		}
	case LuaValueData::IndexOfType<FLuaUEnumEntry*>():
		{
			this->GetMutable<FLuaUEnumEntry*>()->AddReferencedObjects(collector);
			break;
		}
	default:
		break;
		//FProperties are handed by the engine anyway are collected seperately by UnrealLuaEngineSubsystem
	}
	return true;
}

bool FLuaValue::AddStructReferencedObjects(FReferenceCollector& collector, const TObjectPtr<UObject>& owner)
{
	switch(this->GetData().GetIndex())
	{
	case LuaValueData::IndexOfType<TObjectPtr<UObject>>():
		{
			TObjectPtr<UObject>& ptr = this->GetMutable<TObjectPtr<UObject>>();
			
			collector.AddReferencedObject(ptr);
			LUA_LOG("FLuaValue owner %s referencing UObject %s", *GetNameSafe(owner), *GetNameSafe(ptr))
			if(!IsValid(ptr) || ptr->HasAnyFlags(RF_BeginDestroyed | RF_MirroredGarbage))
			{
				LUA_LOG("FLuaValue : Invalidating TObjectPtr for UObject %s", *GetNameSafe(ptr))
				this->GetData().Emplace<sol::nil_t>();
				//@TODO : broadcast
			}
			break;
		}
	case LuaValueData::IndexOfType<FLuaUFunctionReference>():
		{
			FLuaUFunctionReference& ref = this->GetData().Get<FLuaUFunctionReference>();
			collector.AddReferencedObject(const_cast<FFunctionDescr*>(ref.Func)->Func);
			break;
		}
	case LuaValueData::IndexOfType<FPropertyReferenceWrapper>():
		{
			FPropertyReferenceWrapper& wrapper = this->GetData().Get<FPropertyReferenceWrapper>();
			collector.AddReferencedObject(wrapper.Owner);
			break;
		}
	case LuaValueData::IndexOfType<FLuaScriptStruct>():
		{
			this->GetMutable<FLuaScriptStruct>().AddReferencedObjects(collector);
			break;
		}
	case LuaValueData::IndexOfType<FLuaInstancedStruct>():
		{
			this->GetMutable<FLuaInstancedStruct>().AddReferencedObjects(collector);
			break;
		}
	case LuaValueData::IndexOfType<FLuaSharedStruct>():
		{
			this->GetMutable<FLuaSharedStruct>().AddReferencedObjects(collector);
			break;
		}
	case LuaValueData::IndexOfType<FLuaArray>():
		{
			//this->Get<FLuaArray>().AddReferencedObjects(collector);
			break;
		}
	case LuaValueData::IndexOfType<FLuaMap>():
		{
			//this->Get<FLuaMap>().AddReferencedObjects(collector);
			break;
		}
	case LuaValueData::IndexOfType<FLuaSet>():
		{
			//this->Get<FLuaSet>().AddReferencedObjects(collector);
			break;
		}
	case LuaValueData::IndexOfType<FLuaUEnumEntry*>():
		{
			this->GetMutable<FLuaUEnumEntry*>()->AddReferencedObjects(collector);
			break;
		}
	default:
		break;
		//FProperties are handed by the engine anyway are collected seperately by UnrealLuaEngineSubsystem
	}
	return true;
}


bool FLuaValue::NetSerialize(FArchive& ar, UPackageMap* map, bool& bOutSuccess)
{
	uint8 index = this->GetData().GetIndex();
	ar << index;

	switch(index)
	{
	case LuaValueData::IndexOfType<std::nullptr_t>(): [[fallthrough]];
	case LuaValueData::IndexOfType<sol::nil_t>():
		{
			if(ar.IsLoading())
			{
				this->GetData().Emplace<sol::nil_t>(sol::nil);	
			}
			break;
		}
	case LuaValueData::IndexOfType<bool>():
		{
			if(ar.IsLoading())
			{
				bool value;
				ar << value;
				this->GetData().Emplace<bool>(value);				
			}
			else
			{
				bool value = this->GetData().Get<bool>();
				ar << value;
			}
			break;
		}
	case LuaValueData::IndexOfType<int64>():
		{
			if(ar.IsLoading())
			{
				int64 value;
				ar << value;
				this->GetData().Emplace<int64>(value);				
			}
			else
			{
				int64 value = this->GetData().Get<int64>();
				ar << value;
			}
			break;
		}
	case LuaValueData::IndexOfType<double>():
		{
			if(ar.IsLoading())
			{
				double value;
				ar << value;
				this->GetData().Emplace<double>(value);	
			}
			else
			{
				double value = this->GetData().Get<double>();
				ar << value;
			}
			break;
		}
	case LuaValueData::IndexOfType<std::string>():
		{
			if(ar.IsLoading())
			{
				FString value;
				ar << value;
				this->GetData().Emplace<std::string>(StringCast<char>(*value).Get());
			}
			else
			{
				std::string& value = this->GetData().Get<std::string>();
				FString valStr = value.c_str();
				ar << valStr;
			}
			break;
		}
	case LuaValueData::IndexOfType<FVector2D>():
		{
			if(ar.IsLoading())
			{
				FVector2D value;
				ar << value;
				this->GetData().Emplace<FVector2D>(value);
			}
			else
			{
				FVector2D& value = this->GetData().Get<FVector2D>();
				ar << value;
			}
			break;
		}
	case LuaValueData::IndexOfType<FVector>():
		{
			if(ar.IsLoading())
			{
				FVector value;
				ar << value;
				this->GetData().Emplace<FVector>(value);
			}
			else
			{
				FVector& value = this->GetData().Get<FVector>();
				ar << value;
			}
			break;
		}
	case LuaValueData::IndexOfType<FRotator>():
		{
			if(ar.IsLoading())
			{
				FRotator value;
				ar << value;
				this->GetData().Emplace<FRotator>(value);				
			}
			else
			{
				FRotator& value = this->GetData().Get<FRotator>();
				ar << value;
			}
			break;
		}
	case LuaValueData::IndexOfType<TObjectPtr<UObject>>():
		{
			if(ar.IsLoading())
			{
				UObject* value;
				ar << value;
				if(value == nullptr || value->HasAnyFlags(RF_BeginDestroyed | RF_MirroredGarbage))
				{
					this->GetData().Emplace<sol::nil_t>(sol::nil);	
				}
				else
				{
					this->GetData().Emplace<TObjectPtr<UObject>>(value);
				}
			}
			else
			{
				UObject* value = this->GetData().Get<TObjectPtr<UObject>>().Get();
				if(IsValid(value) && !value->HasAnyFlags(RF_BeginDestroyed | RF_MirroredGarbage))
				{
					ar << value;	
				}
				else
				{
					value = nullptr;
					ar << value;
				}
			}
			break;
		}
	case LuaValueData::IndexOfType<FLuaUClass>():
		{
			if(ar.IsLoading())
			{
				FSoftClassPath path;
				ar << path;
				this->GetData().Emplace<FLuaUClass>(path);	
			}
			else
			{
				FSoftClassPath path = this->GetData().Get<FLuaUClass>().GetSoftClassPath();
				ar << path;
			}
			break;
		}
	case LuaValueData::IndexOfType<FLuaUStruct>():
		{
			if(ar.IsLoading())
			{
				FSoftObjectPath path;
				ar << path;
				this->GetData().Emplace<FLuaUStruct>(path);	
			}
			else
			{
				FSoftObjectPath path = this->GetData().Get<FLuaUStruct>().GetPath();
				ar << path;
			}
			break;
		}
	case LuaValueData::IndexOfType<TLuaVariantPtr<FTransform>>():
		{
			if(ar.IsLoading())
			{
				FTransform value;
				ar << value;
				this->GetData().Emplace<TLuaVariantPtr<FTransform>>(MakePimpl<FTransform, EPimplPtrMode::DeepCopy>(value));
			}
			else
			{
				FTransform* value = this->GetData().Get<TLuaVariantPtr<FTransform>>().Get();
				FTransform t = *value;
				ar << t;
			}
			break;
		}
	case LuaValueData::IndexOfType<FLuaUEnumEntry*>():
		{
			if(ar.IsLoading())
			{
				UEnum* uenum;
				int64 value;
				ar << uenum;
				ar << value;
				
				FLuaUObjectItem& enumItem = UnrealLua::UObjectRegistry::GetMetaObjectItem(uenum);
				FLuaUEnumEntry* entry = enumItem.PropertyMapping.Get<FLuaUEnumMapping>().GetEnumEntryByNumberValue(value);
				this->Emplace<FLuaUEnumEntry*>(entry);
			}
			else
			{
				FLuaUEnumEntry* value = this->Get<FLuaUEnumEntry*>();
				ar << value->uenum;
				ar << value->Value;
			}
			break;
		}
	case LuaValueData::IndexOfType<FLuaScriptStruct>():
		{
			if(ar.IsLoading())
			{
				UScriptStruct* ss; 
				ar << ss;
				FLuaScriptStruct temp{ss};
				ss->SerializeBin(ar, temp.GetMemory());
				this->GetData().Emplace<FLuaScriptStruct>(temp);	
			}
			else
			{
				FLuaScriptStruct& value = this->GetData().Get<FLuaScriptStruct>();
				UScriptStruct* ss = const_cast<UScriptStruct*>(value.GetScriptStruct()); 
				ar << ss;
				ss->SerializeBin(ar, value.GetMemory());
			}
			break;
		}
	case LuaValueData::IndexOfType<FLuaArray>():
		{
			unimplemented();
			if(ar.IsLoading())
			{
				FLuaArray& value = this->GetData().Get<FLuaArray>();
				
				int32 num = value.Num();
				ar << num;

				//@TODO : better property serialization
				//@TODO : Also needs to serialize specific datatype, in case of UObject property, U Class property, Structproperty, etc
				FProperty* inner = value.GetInner();
				FString innerstr = inner->GetNameCPP(); 
				ar << innerstr;

				ar.Serialize(value.GetData(), num * inner->GetElementSize());
			}
			else
			{
				FLuaArray& value = this->GetData().Get<FLuaArray>();
				
				int32 num = value.Num();
				ar << num;

				//@TODO : better property serialization
				//@TODO : Also needs to serialize specific datatype, in case of UObject property, U Class property, Structproperty, etc
				FProperty* inner = value.GetInner();
				FString innerstr = inner->GetNameCPP(); 
				ar << innerstr;

				ar.Serialize(value.GetData(), num * inner->GetElementSize());
			}
			break;
		}
	case LuaValueData::IndexOfType<FLuaSharedStruct>():
		{
			if(ar.IsLoading())
			{
				UScriptStruct* ss; 
				ar << ss;
				FLuaSharedStruct temp{ss};
				ss->SerializeBin(ar, temp.GetMemory());
				this->GetData().Emplace<FLuaSharedStruct>(temp);	
			}
			else
			{
				FLuaSharedStruct& value = this->GetData().Get<FLuaSharedStruct>();
				UScriptStruct* ss = const_cast<UScriptStruct*>(value.GetScriptStruct()); 
				ar << ss;
				ss->SerializeBin(ar, value.GetMemory());
			}
			break;
		}
	case LuaValueData::IndexOfType<FLuaInstancedStruct>():
		{
			if(ar.IsLoading())
			{
				UScriptStruct* ss; 
				ar << ss;
				FLuaInstancedStruct temp{ss};
				ss->SerializeBin(ar, temp.GetMemory());
				this->GetData().Emplace<FLuaInstancedStruct>(MoveTemp(temp));	
			}
			else
			{
				FLuaInstancedStruct& value = this->GetData().Get<FLuaInstancedStruct>();
				UScriptStruct* ss = const_cast<UScriptStruct*>(value.GetScriptStruct()); 
				ar << ss;
				ss->SerializeBin(ar, value.GetMemory());
			}
			break;
		}
	}
	return true;
}
