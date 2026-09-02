#include "Utility/UnrealLuaHash.h"

#include "Utility/LuaLogMacros.h"
#include "UnrealLua.h"
#include "sol/sol.hpp"
#include "LuaTypes/LuaScriptStruct.h"
#include "Reflection/FunctionDescr.h"
#include "LuaTypes/LuaArray.h"
#include "LuaTypes/LuaMap.h"
#include "LuaTypes/LuaSet.h"
#include "LuaTypes/LuaInstancedStruct.h"
#include "LuaTypes/LuaSharedStruct.h"
#include "LuaValue/LuaValueType.h"

/////////////////////////////////////////////////
/// Copy-pasted typedefs from Lua and LuaJit
/////////////////////////////////////////////////
///
//Lua String
namespace UnrealLua
{
	
	typedef struct TLuaString {
		struct GCObject *next;
		unsigned char tt;
		unsigned char marked;
		unsigned char extra;  /* reserved words for short strings; "has hash" for longs */
		unsigned char shrlen;  /* length for short strings */
		unsigned int hash;
		union {
			size_t lnglen;  /* length for long strings */
			struct TLuaString *hnext;  /* linked list for hash table */
		} u;
		char contents[1];
	} TLuaString;//TString;


//LuaJit String

/* -- String object ------------------------------------------------------- */

/* Memory and GC object sizes. */
typedef uint32_t MSize;

typedef uint32_t StrHash;	/* String hash value. */
typedef uint32_t StrID;		/* String ID. */

#define LJ_GC64 1
/* GCobj reference */
typedef struct GCRef {
#if LJ_GC64
	uint64_t gcptr64;	/* True 64 bit pointer. */
#else
	uint32_t gcptr32;	/* Pseudo 32 bit pointer. */
#endif
} GCRef;

/* String object header. String payload follows. */
typedef struct GCstr {
	GCRef nextgc; 
	int8_t marked;
	uint8_t gct;
	uint8_t reserved;	/* Used by lexer for fast lookup of reserved words. */
	uint8_t hashalg;	/* Hash algorithm. */
	StrID sid;		/* Interned string ID. */
	StrHash hash;		/* Hash of string. */
	MSize len;		/* Size of string. */
} GCstr;
}
ELuaValueType UnrealLua::HashUtility::GetLuaValueType(sol::object value)
{
	sol::type solType = value.valid() ? value.get_type() : sol::type::nil; 
	switch (solType)
	{
	case sol::type::nil:
		return ELuaValueType::Nil;
	case sol::type::string:
		return ELuaValueType::String; 
	case sol::type::number:
		{
			if(value.is<int>())
			{
				return ELuaValueType::Integer;
			}
			else if(value.is<double>())
			{
				return ELuaValueType::Float;
			}
			else
			{
				checkNoEntry();
			}
		}
		return ELuaValueType::Nil;
	case sol::type::boolean:
		return ELuaValueType::Boolean;
	case sol::type::lightuserdata:
		{
			if (UnrealLua::IsUObject(value))
			{
				return ELuaValueType::UObject;	
			}
			else if (FFunctionDescr* descr = UnrealLua::LightUserdata::GetFunctionDescr(value))
			{
				return ELuaValueType::UFunction;
			}
			return ELuaValueType::Nil;
		}
	case sol::type::userdata:
		if(value.is<FVector>())
		{
			return ELuaValueType::Vector;
		}
		else if(value.is<FRotator>())
		{
			return ELuaValueType::Rotator;
		}
		else if(value.is<FVector2D>())
		{
			return ELuaValueType::Vector2D;
		}
		else if(value.is<FTransform>())
		{
			return ELuaValueType::Transform;
		}
		else if(value.is<FLuaScriptStruct>())
		{
			return ELuaValueType::ScriptStruct;
		}
		else if(value.is<FFunctionDescr>())
		{
			return ELuaValueType::UFunction;
		}
		else if(value.is<FLuaInstancedStruct>())
		{
			return ELuaValueType::InstancedStruct;
		}
		else if(value.is<FLuaSharedStruct>())
		{
			return ELuaValueType::SharedStruct;
		}
		else if(value.is<FLuaArray>())
		{
			return ELuaValueType::Array;
		}
		else if(value.is<FLuaMap>())
		{
			return ELuaValueType::Map;
		}
		else if(value.is<FLuaSet>())
		{
			return ELuaValueType::Set;
		}
		return ELuaValueType::Nil;
	case sol::type::table:
		return ELuaValueType::LuaTable;
	case sol::type::function:
		return ELuaValueType::LuaFunction;
	case sol::type::thread:
		return ELuaValueType::Coroutine;
	default:
		return ELuaValueType::Nil;
	}
}

void UnrealLua::HashUtility::PrintLuaValue(sol::object value, FString msg)
{
	if(!value.valid())
	{
		FString message = msg + " nil";
		LUA_LOG("Print Lua value: %s", *message)	
	}
	else
	{
		sol::state_view lua{value.lua_state()};
		std::string val = lua["tostring"](value);
		FString message = msg + val.c_str();
		LUA_LOG("Print Lua value: %s", *message)
	}	
}
