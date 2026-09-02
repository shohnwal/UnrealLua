

#include "LuaTypes/LuaPrimitives.h"

#include "LuaCoreDelegates.h"
#include "LuaContext/LuaImportRegistry.h"
#include "LuaTypes/LuaSoftObjectWrapper.h"
#include "LuaContext/ScopedLuaContext.h"
#include "GameFramework/Actor.h"
#include "utility/to_string.hpp"
#include "UnrealLua.h"
#include "LuaTypes/TLuaSubclassOf.h"
#include "Reflection/PropertyDescr/MulticastDelegatePropertyDescr.h"
#include "Reflection/PropertyDescr/SingleDelegatePropertyDescr.h"

static const FDelegateHandle fLuaPrimitivesCPPTypeHandle = FLuaCoreDelegates::OnRegisterLuaUsertypes.AddStatic(&UnrealLua::LuaTypes::Primitives::RegisterPrimitiveWrappers);


template<typename T, typename U>
static sol::object GetClamped(sol::object arg, sol::this_state lua)
{
	T num = FMath::Clamp<U>((arg.get_type() == sol::type::number) ? arg.as<U>() : 0,std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
	return sol::make_object<T>(lua, num);	
}
sol::object FLuaPrimitiveCPPType::operator()(sol::object arg, const sol::this_state lua) const
{
	switch(this->Type)
	{
	case LUA_CASTCLASS_FInt8Property:
		return GetClamped<uint8, uint64>(arg, lua);
	case LUA_CASTCLASS_FByteProperty:
		return GetClamped<uint8, uint64>(arg, lua);
	case LUA_CASTCLASS_FIntProperty:
		return GetClamped<int64, uint64>(arg, lua);
	case LUA_CASTCLASS_FFloatProperty:
		return GetClamped<float, double>(arg, lua);
	case LUA_CASTCLASS_FUInt64Property:
		return GetClamped<uint64, uint64>(arg, lua);		
	case LUA_CASTCLASS_FUInt32Property:
		return GetClamped<uint32, uint64>(arg, lua);
	case LUA_CASTCLASS_FStrProperty:
		{
			sol::state_view luas{lua};
			return luas["tostring"](arg);	
		}
	case LUA_CASTCLASS_FBoolProperty:
		return sol::make_object<bool>(lua, arg.as<bool>());
	case LUA_CASTCLASS_FUInt16Property:
		return GetClamped<uint16, uint64>(arg, lua);
	case LUA_CASTCLASS_FInt64Property:
		return GetClamped<int64, int64>(arg, lua);
	case LUA_CASTCLASS_FInt16Property:
		return GetClamped<int16, int64>(arg, lua);
	case LUA_CASTCLASS_FDoubleProperty:
		return GetClamped<double, double>(arg, lua);
	default:
		checkNoEntry();
	}
	return sol::nil;
}

std::string FLuaPrimitiveCPPType::tostring() const
{
	switch(this->Type)
	{
	case LUA_CASTCLASS_FInt8Property:
		return "int8";
	case LUA_CASTCLASS_FByteProperty:
		return "byte";
	case LUA_CASTCLASS_FIntProperty:
		return "int32";
	case LUA_CASTCLASS_FFloatProperty:
		return "float";
	case LUA_CASTCLASS_FUInt64Property:
		return "uint64";		
	case LUA_CASTCLASS_FUInt32Property:
		return "uint32";
	case LUA_CASTCLASS_FStrProperty:
		return "string";
	case LUA_CASTCLASS_FBoolProperty:
		return "boolean";
	case LUA_CASTCLASS_FUInt16Property:
		return "uint16";
	case LUA_CASTCLASS_FInt64Property:
		return "int64";
	case LUA_CASTCLASS_FInt16Property:
		return "int16";
	case LUA_CASTCLASS_FDoubleProperty:
		return "double";
	case LUA_CASTCLASS_FNameProperty:
		return "name";
	case LUA_CASTCLASS_FTextProperty:
		return "text";
	default:
		return "unknown primitive!";
	}
}

bool FLuaPrimitiveCPPType::Matches(sol::object& toCheck)
{
	if (!toCheck.valid())
	{
		return false;
	}
	switch(this->Type)
	{
	case LUA_CASTCLASS_FInt8Property:
		return toCheck.is<int>();
	case LUA_CASTCLASS_FByteProperty:
		return toCheck.is<int>();
	case LUA_CASTCLASS_FIntProperty:
		return toCheck.is<int>();
	case LUA_CASTCLASS_FFloatProperty:
		return toCheck.is<float>();
	case LUA_CASTCLASS_FUInt64Property:
		return toCheck.is<int>();
	case LUA_CASTCLASS_FUInt32Property:
		return toCheck.is<int>();
	case LUA_CASTCLASS_FStrProperty:
		return toCheck.get_type() == sol::type::string;
	case LUA_CASTCLASS_FBoolProperty:
		return toCheck.get_type() == sol::type::boolean;
	case LUA_CASTCLASS_FUInt16Property:
		return toCheck.is<int>();
	case LUA_CASTCLASS_FInt64Property:
		return toCheck.is<int>();
	case LUA_CASTCLASS_FInt16Property:
		return toCheck.is<int>();
	case LUA_CASTCLASS_FDoubleProperty:
		return toCheck.is<double>();
	case LUA_CASTCLASS_FNameProperty:
		return toCheck.get_type() == sol::type::string;
	case LUA_CASTCLASS_FTextProperty:
		return toCheck.get_type() == sol::type::string;
	default:
		return false;
	}
}

void UnrealLua::LuaTypes::Primitives::RegisterPrimitiveWrappers(sol::state_view& lua)
{
	lua.new_usertype<FLuaPrimitiveCPPType>(
		"LuaPrimitiveCPPType",
		"new", sol::no_constructor,
		sol::meta_function::to_string, &FLuaPrimitiveCPPType::tostring 
	);
}

std::string UnrealLua::LuaTypes::TypeInfo::UType_Stack(sol::stack_object obj, sol::stack_object innerCheck, sol::this_state lua)
{
	return UType(obj, innerCheck, lua);	
}

FString GetNthPropTypeNameFromFunction(UFunction* func, int32 wantedPropIndex)
{
	FString name = "nil";
	FProperty* retProp = func->GetReturnProperty();
	if(wantedPropIndex == 0)
	{
		if(retProp)
		{
			name = UnrealLua::PropertyHelper::GetPropertyTypeName(retProp, true);
		}
	}
	else
	{
		int32 propCounter = 1;
		for(TFieldIterator<FProperty> it(func); it; ++it)
		{
			FProperty* prop = *it;							
			if(retProp == prop)
			{
				continue;
			}
			if(propCounter == wantedPropIndex)
			{
				name = UnrealLua::PropertyHelper::GetPropertyTypeName(prop, true);
				break;
			}
			propCounter++;
		}
	}
	return name;
}
namespace UnrealLua::LuaTypes::TypeInfo
{
	std::string GetFuncDescrDescr(FFunctionDescr* funcDescr, bool bWithInner, int32 wantedPropIndex)
	{
		FString name{"UFunction"};
		if(bWithInner)
		{
			FFunctionDescr& proxy = *funcDescr;
			UFunction* func = proxy.Func;
			if(wantedPropIndex >= 0)
			{
				name = GetNthPropTypeNameFromFunction(func, wantedPropIndex);
			}
			else
			{
				FString keyStr = "";
				FProperty* retProp = func->GetReturnProperty();
				if(retProp)
				{
					keyStr += UnrealLua::PropertyHelper::GetPropertyTypeName(retProp, true);
				}
				keyStr += "(";
				for(TFieldIterator<FProperty> it(func); it; ++it)
				{
					FProperty* prop = *it;
					if(retProp == prop)
					{
						continue;
					}
					keyStr += UnrealLua::PropertyHelper::GetPropertyTypeName(prop, true);
					if(prop->HasAllPropertyFlags(CPF_ReferenceParm))
					{
						keyStr += "&";
					}
					if(prop->HasAllPropertyFlags(CPF_OutParm))
					{
						keyStr += "& out";
					}
					if(prop->Next)
					{
						keyStr += ", ";
					}
				}
				keyStr += ")";
				name.Append("<" + keyStr + ">");	
			}
		}
		std::string str{StringCast<char>(*name).Get()};
		return str;
	}
	
	std::string UTypeInternal(sol::object value, bool bWithInner, int32 wantedPropIndex)
	{
		if(!value.valid())
		{
			return "nil";
		}
		sol::type type = value.get_type(); 
		switch(type)
		{
		case sol::type::nil:
			return "nil";
		case sol::type::string:
			return "string";
			/*
			if(bWithInner)
			{
				return "string : " + sol::utility::to_string(value);
			}
			else
			{
				return "string";	
			}
			*/
		case sol::type::number:
			{
				if(value.is<int>())
				{
					return "int64";
				}
				else
				{
					return "double";
				}
			}
		case sol::type::thread:
			return "thread";
		case sol::type::boolean:
			return "boolean";
		case sol::type::function:
			return "function";
		case sol::type::lightuserdata:
			if (UObject* obj = UnrealLua::LightUserdata::GetUObject(value))
			{
				if(IsValid(obj))
				{
					FString name = obj->GetClass()->GetName();
					if(obj->IsA<AActor>())
					{
						std::string str{StringCast<char>(*("A" + name)).Get()};
						return str;
					}
					else
					{
						std::string str{StringCast<char>(*("U" + name)).Get()};
						return str;
					}
				}
				return "nil";
			}
			else if(FFunctionDescr* ptr = UnrealLua::LightUserdata::GetFunctionDescr(value))
			{
				return GetFuncDescrDescr(ptr, bWithInner, wantedPropIndex);
			}
			return "lightuserdata";
		case sol::type::userdata:
			{
				if(value.is<FLuaPrimitiveCPPType>())
				{
					FLuaPrimitiveCPPType cppType = value.as<FLuaPrimitiveCPPType>();
					std::string cppStr = cppType.tostring(); 
					return cppStr;
				}
				else if(value.is<FFunctionDescr>())
				{
					checkNoEntry();
					return GetFuncDescrDescr(value.as<FFunctionDescr*>(), bWithInner, wantedPropIndex);
				}
				else if(value.is<FVector>())
				{
					return "FVector";
				}
				else if(value.is<FRotator>())
				{
					return "FRotator";
				}
				else if(value.is<FVector2D>())
				{
					return "FVector2D";
				}
				else if(value.is<FTransform>())
				{
					return "FTransform";
				}
				else if(value.is<FLuaScriptStruct>())
				{
					const FLuaScriptStruct& strct = value.as<FLuaScriptStruct&>();
					const UScriptStruct* ss = strct.GetScriptStruct();
					if(ss)
					{
						std::string str{StringCast<char>(*ss->GetStructCPPName()).Get()};
						return str;
					}
					else
					{
						return "FScriptStruct<__Garbage__>";
					}
				}
				else if(value.is<FLuaInstancedStruct>())
				{
					FString name{"TInstancedStruct"};
					if(bWithInner)
					{
						const FLuaInstancedStruct& is = value.as<FLuaInstancedStruct&>();
						const UScriptStruct* ss = is.GetScriptStruct();
						if(ss)
						{
							if(wantedPropIndex >= 0)
							{
								name = ss->GetStructCPPName();
							}
							else
							{
								FString inner = ss->GetStructCPPName();
								name.Append("<" + inner + ">");
							}
						}
						else
						{
							name.Append("<__Garbage__>");
						}
					}
					std::string str{StringCast<char>(*name).Get()};
					return str;;
				}
				else if(value.is<FLuaSharedStruct>())
				{
					FString name{"TSharedStruct"};
					if(bWithInner)
					{
						const FLuaSharedStruct& is = value.as<FLuaSharedStruct&>();
						const UScriptStruct* ss = is.GetScriptStruct();
						if(ss)
						{
							if(wantedPropIndex >= 0)
							{
								name = ss->GetStructCPPName();
							}
							else
							{

								FString inner = ss->GetStructCPPName();
								name.Append("<" + inner + ">");
							}
						}
						else
						{
							name.Append("<__Garbage__>");
						}
					}
					std::string str{StringCast<char>(*name).Get()};
					return str;;
				}
				else if(value.is<FLuaArray>())
				{
					FLuaArray& arr = value.as<FLuaArray&>();
					FString name{"TArray"};
					if(bWithInner)
					{
						FProperty* inner = arr.GetInner();
						if(inner)
						{
							FString innerStr = UnrealLua::PropertyHelper::GetPropertyTypeName(inner, true);
							if(wantedPropIndex >= 0)
							{
								name = innerStr;
							}
							else
							{
								name.Append("<" + innerStr + ">");	
							}							
						}
						else
						{
							name.Append("<__Garbage__>");
						}
					}
					std::string str{StringCast<char>(*name).Get()};
					return str;
				}
				else if(value.is<FLuaMap>())
				{
					FLuaMap& arr = value.as<FLuaMap&>();
					FString name{"TMap"};
					if(bWithInner)
					{
						FProperty* keyProp = arr.GetKeyProperty();
						FProperty* valueProp = arr.GetValueProperty();
						if(keyProp && valueProp)
						{
							FString keyStr = UnrealLua::PropertyHelper::GetPropertyTypeName(keyProp, true);
							FString valueStr = UnrealLua::PropertyHelper::GetPropertyTypeName(valueProp, true);
							if(wantedPropIndex == 0)
							{
								name = keyStr;
							}
							else if(wantedPropIndex >= 1)
							{
								name = valueStr;
							}
							else
							{
								name.Append("<" + keyStr + "," + valueStr + ">");						
							}	
						}
						else
						{
							name.Append("<__Garbage__>");
						}
					}
					std::string str{StringCast<char>(*name).Get()};
					return str;
				}
				else if(value.is<FLuaSet>())
				{
					FLuaSet& arr = value.as<FLuaSet&>();
					FString name{"TSet"};
					if(bWithInner)
					{
						FProperty* inner = arr.GetInner();
						if(inner)
						{
							FString innerStr = UnrealLua::PropertyHelper::GetPropertyTypeName(inner, true);
							if(wantedPropIndex >= 0)
							{
								name = innerStr;
							}
							else
							{
								name.Append("<" + innerStr + ">");	
							}
						}
						else
						{
							name.Append("<__Garbage__>");
						}
					}
					std::string str{StringCast<char>(*name).Get()};
					return str;
				}
				else if (value.is<TLuaSubclassOf>())
				{
					TLuaSubclassOf& lsubc = value.as<TLuaSubclassOf>();
					FLuaUClass& uclass = lsubc.ClassPath;
					FString fstr = uclass.GetSoftClassPath().ToString();
					std::string str = std::string{"TSubclassOf<"} + StringCast<char>(*fstr).Get() + ">";
					return str;
				}
				else if(value.is<FLuaUClass>())
				{
					FLuaUClass& uclass = value.as<FLuaUClass&>();
					FString fstr = uclass.GetSoftClassPath().ToString();;
					/*
					if(IsGarbageCollectingAndLockingUObjectHashTables())
					{
						fstr = 
					}
					else
					{
						UClass* clazz = uclass.TryLoadClass();
						if(!clazz)
						{
							return "nil";
						}
					
						if(clazz->IsChildOf<AActor>())
						{
							fstr = "A" + clazz->GetName();
						}
						else
						{
							fstr = "U" + clazz->GetName();
						}	
					}
					*/
					std::string str{StringCast<char>(*fstr).Get()};
					return str;
				}
				else if(value.is<FLuaUStruct>())
				{
					FLuaUStruct& strct = value.as<FLuaUStruct&>();
					FString fstr = strct.GetPath().ToString();
					std::string str{StringCast<char>(*fstr).Get()};
					return str;
				}
				else if(value.is<FLuaUEnumMapping>())
				{
					FLuaUEnumMapping& fenum = value.as<FLuaUEnumMapping&>();
					FString name = fenum.GetName();
					std::string str{StringCast<char>(*name).Get()};
					return str;
				}
				else if(value.is<FLuaUEnumEntry>())
				{
					const FLuaUEnumEntry& fenum = value.as<FLuaUEnumEntry&>();
					std::string str = fenum.ToString();
					return fenum.ToString();
				}
				else if(value.is<FLuaSoftObjectWrapper>())
				{
					FString name{"TSoftObjectPtr"};
					if(bWithInner)
					{
						FLuaSoftObjectWrapper& wrapper = value.as<FLuaSoftObjectWrapper&>();
						name.Append("<" + wrapper.Ptr.GetAssetName() + ">");
					}
					std::string str{StringCast<char>(*name).Get()};

					return str;
				}
				else if(value.is<FSingleDelegatePropertyProxy>())
				{
					FString name{ "FDelegate"};
					if(bWithInner)
					{
						FString keyStr = "";
						FSingleDelegatePropertyProxy& proxy = value.as<FSingleDelegatePropertyProxy&>();
						UFunction* func = proxy.Prop->SignatureFunction;
						FProperty* retProp = func->GetReturnProperty();
						if(wantedPropIndex >= 0)
						{
							name = GetNthPropTypeNameFromFunction(func, wantedPropIndex);
						}
						else
						{
							if(retProp)
							{
								keyStr += UnrealLua::PropertyHelper::GetPropertyTypeName(retProp, true);
							}
							keyStr += "(";
							for(TFieldIterator<FProperty> it(proxy.Prop->SignatureFunction); it; ++it)
							{
								FProperty* prop = *it;
								if(retProp == prop)
								{
									continue;
								}
								keyStr += UnrealLua::PropertyHelper::GetPropertyTypeName(prop, true);
								if(prop->Next)
								{
									keyStr += ", ";
								}
								keyStr += ")";
							}
							name.Append("<" + keyStr + ">");	
						}
					}
					std::string str{StringCast<char>(*name).Get()};
					return str;
				}
				else if(value.is<FMulticastDelegatePropertyProxy>())
				{
					FString name{ "FMulticastDelegate"};
					if(bWithInner)
					{
						FString keyStr = "";
						FMulticastDelegatePropertyProxy& proxy = value.as<FMulticastDelegatePropertyProxy&>();
						FProperty* retProp = proxy.Prop->SignatureFunction->GetReturnProperty();
						if(retProp)
						{
							keyStr += UnrealLua::PropertyHelper::GetPropertyTypeName(retProp, true);
						}
						keyStr += "(";
						for(TFieldIterator<FProperty> it(proxy.Prop->SignatureFunction); it; ++it)
						{
							FProperty* prop = *it;
							if(retProp == prop)
							{
								continue;
							}
							keyStr += UnrealLua::PropertyHelper::GetPropertyTypeName(prop, true);
							if(prop->Next)
							{
								keyStr += ", ";
							}
						}
						keyStr += ")";
						name.Append("<" + keyStr + ">");
					}
					std::string str{StringCast<char>(*name).Get()};
					return str;
				}
				else if(value.is<FLuaRPCFunction>())
				{
					FLuaRPCFunction& rpcFunc = value.as<FLuaRPCFunction&>();
					std::string ret = "LuaRPCFunction : ";
					ret.append(StringCast<char>(**rpcFunc.FuncName.Get()).Get());
					return ret;
				}
				else if(value.is<FLuaImportRegistry>())
				{
					return "FLuaImportRegistry";
				}
				else if(value.is<FScopedLuaContext>())
				{
					return "FLuaContext";
				}
				else if(value.is<luaL_Stream>())
				{
					return "File";
				}
				else
				{
					std::string str = sol::utility::to_string(value);
					return "unknown usertype : " + str;
				}
			}
		case sol::type::table:
			return "table";
		default:
			return "unknown type : " + std::to_string(static_cast<int>(type));
		}
	}

	std::string UType(UFunction* func, bool bWithInner)
	{
		FString name{"UFunction"};
		if(bWithInner)
		{
			FString keyStr = "";
			FProperty* retProp = func->GetReturnProperty();
			if(retProp)
			{
				keyStr += UnrealLua::PropertyHelper::GetPropertyTypeName(retProp, true);
			}
			keyStr += "(";
			for(TFieldIterator<FProperty> it(func); it; ++it)
			{
				FProperty* prop = *it;
				if(retProp == prop)
				{
					continue;
				}
				keyStr += UnrealLua::PropertyHelper::GetPropertyTypeName(prop, true);
				if(prop->HasAllPropertyFlags(CPF_ReferenceParm))
				{
					keyStr += "&";
				}
				if(prop->HasAllPropertyFlags(CPF_OutParm))
				{
					keyStr += "& out";
				}
				if(prop->Next)
				{
					keyStr += ", ";
				}
			}
			keyStr += ")";
			name.Append("<" + keyStr + ">");	
		}
		std::string str{StringCast<char>(*name).Get()};
		return str;
	}
}


std::string UnrealLua::LuaTypes::TypeInfo::UType(sol::object value, sol::object innerCheck, sol::this_state lua)
{
	if(!value.valid())
	{
		return "nil";
	}
	bool bWithInner = false;
	int32 wantedPropIndex = -1;
	if(innerCheck.is<int>())
	{
		wantedPropIndex = innerCheck.as<int>();
		bWithInner = true;
	}
	else if(innerCheck.is<bool>())
	{
		bWithInner = innerCheck.as<bool>();
	}

	return UTypeInternal(value, bWithInner, wantedPropIndex);
}

std::string UnrealLua::LuaTypes::TypeInfo::UType(sol::object value, bool bWithInner, int32 wantedPropIndex)
{
	return UTypeInternal(value, bWithInner, wantedPropIndex);
}

