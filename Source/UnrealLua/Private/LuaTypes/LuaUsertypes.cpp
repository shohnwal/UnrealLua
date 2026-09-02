// Fill out your copyright notice in the Description page of Project Settings.


#include "LuaTypes/LuaUsertypes.h"
#include "Utility/LuaLogMacros.h"
#include "Reflection/FunctionDescr.h"
#include "LuaCoreDelegates.h"
#include "LuaContext/LuaImportRegistry.h"
#include "UnrealLua.h"
#include "LuaTypes/LuaScriptStruct.h"
#include "LuaTypes/LuaUClass.h"
#include "LuaTypes/LuaUStruct.h"
#include "Reflection/PropertyDescr/MulticastDelegatePropertyDescr.h"
#include "Reflection/PropertyDescr/SingleDelegatePropertyDescr.h"
#include "LuaTypes/LuaUserdataTypeTraits.h"
//#include "sol/variadic_args.hpp"

TArray<TFunction<void(sol::state_view&)>> FLuaUsertypes::LuaUserTypesCallbacks = {};

void FLuaUsertypes::RegisterLuaUserTypes(sol::state& lua)
{
	LUA_LOG("Registering usertypes")
	
	lua.new_usertype<FRotator>(
		"FRotator",
		//"new", sol::no_constructor,
		//sol::constructors<FRotator(float, float, float)>(),
		"new", sol::no_constructor,
		//),
		sol::call_constructor, sol::factories
		(
			[]() { return FRotator(0, 0, 0); },
			[](FRotator const& v) { return FRotator(v); },
			[](FVector const& v) { return v.Rotation(); },
			[](const sol::table& v)
			{
				if (v.size() > 0)
				{
					return FRotator{
						v[1].get_or<float, float>(0),
						v[2].get_or<float, float>(0),
						v[3].get_or<float, float>(0)};
				}
				else
				{
					return FRotator{
						v["Pitch"].get_or<float, float>(0),
						v["Yaw"].get_or<float, float>(0),
						v["Roll"].get_or<float, float>(0)};
				}
			},
			[](sol::variadic_args va)
			{
				if (va.size() == 1)
				{
					return FRotator(va[0].get<double>(), 0, 0);
				}
				else if (va.size() == 2)
				{
					return FRotator(va[0].get<double>(), va[1].get<double>(), 0);
				}
				else
				{
					return FRotator(va[0].get<double>(), va[1].get<double>(), va[2].get<double>());
				}
			}
		),
		"Yaw", &FRotator::Yaw,
		"Z", &FRotator::Yaw,
		"Pitch", &FRotator::Pitch,
		"Y", &FRotator::Pitch,
		"Roll", &FRotator::Roll,
		"X", &FRotator::Roll,
		"GetForwardVector", &FRotator::Vector,
		sol::meta_function::to_string, [](const FRotator& self) { return std::string{StringCast<char>(*self.ToString()).Get()}; }
	);

	lua.new_usertype<FTransform>(
		"FTransform",
		"new", sol::no_constructor,
		sol::call_constructor, sol::factories
		(
			[]() { return FTransform(FTransform::Identity); },
			[](FTransform& other) { return FTransform(other);},
			[](FVector& loc) { return FTransform{loc};},
			[](FRotator& rot) { return FTransform{rot};},
			[](FVector& loc, FRotator& rot) { return FTransform{rot, loc};},
			[](FVector& loc, FRotator& rot, FVector& scal) { return FTransform{rot, loc, scal};}
		),
		"SetLocation", &FTransform::SetLocation,
		"GetLocation", &FTransform::GetLocation,
		"GetRotation", &FTransform::GetRotation,
		"SetRotation", &FTransform::SetRotation,
		"SetScale3D", &FTransform::SetScale3D,
		"GetScale3D", &FTransform::GetScale3D,
		sol::meta_function::equal_to, [](FTransform& self, FTransform& other) -> bool { return self.Equals(other); },
		sol::meta_function::less_than, [](FTransform& self, FTransform& other) -> bool { return self.Equals(other); },
		sol::meta_function::less_than_or_equal_to, [](FTransform& self, FTransform& other) -> bool { return self.Equals(other); },
		"ToString", [](FTransform& self) { return std::string(TCHAR_TO_UTF8(*self.ToString())); },
		sol::meta_function::to_string, [](FTransform* self) { return std::string{StringCast<char>(*self->ToString()).Get()}; }
	);

	lua.new_usertype<FVector2D>(
		"FVector2D",
		"new", sol::no_constructor,
		//sol::constructors<FVector2D(float, float)>(),
		sol::call_constructor, sol::factories(
			[]() { return FVector2D(0, 0); },
			[](FVector2D const& v) { return FVector2D{ v }; },
			[](FVector const& v) { return FVector2D{ v }; },
			[](sol::table const& v)
			{
				if (v.size() > 0)
				{
					return FVector2D{
						v[1].get_or<float, float>(0),
						v[2].get_or<float, float>(0)};					
				}
				else
				{
					return FVector2D{
						v["X"].get_or<float, float>(0),
						v["Y"].get_or<float, float>(0)};
				}
			},
			[](sol::variadic_args va)
		{
			if (va.size() == 1) { return FVector2D(va[0].get<float>(), 0); }
			else if (va.size() >= 2) { return FVector2D(va[0].get<float>(), va[1].get<float>()); }
			else { return FVector2D(0, 0); }
		}
		),
		"Size", &FVector2D::Size,
		sol::meta_function::addition, sol::resolve<FVector2D(const FVector2D&) const>(&FVector2D::operator+),
		sol::meta_function::multiplication, sol::overload(
			sol::resolve<FVector2D(const FVector2D&) const>(&FVector2D::operator*),
			sol::resolve<FVector2D(double) const>(&FVector2D::operator*)
		),
		sol::meta_function::equal_to, [](const FVector2D& self, const FVector2D& other) { return self.Equals(other);},
		sol::meta_function::less_than, [](const FVector2D& self, const FVector2D& other) { return self.ComponentwiseAllLessThan(other);},
		sol::meta_function::less_than_or_equal_to, [](const FVector2D& self, const FVector2D& other) { return self.ComponentwiseAllLessOrEqual(other);},
		"Normalize", &FVector2D::Normalize,
		"X", &FVector2D::X,
		"Y", &FVector2D::Y,
		"Distance", [](sol::object self, sol::object other)
		{
			if(self.is<FVector2D>() && other.is<FVector2D>())
			{
				return FVector2D::Distance(self.as<FVector2D&>(), other.as<FVector2D&>());
			}
			return 0.0;
		},
		sol::meta_function::to_string, [](FVector2D* self) { return std::string{StringCast<char>(*self->ToString()).Get()}; }
	);
	
	lua.new_usertype<FVector>(
		"FVector",
		//"new", sol::no_constructor,
		//sol::constructors<FVector(float, float, float)>(),
		sol::call_constructor, sol::factories
		(
			[]() -> FVector { return FVector::ZeroVector; },
			[](FVector const& v) -> FVector { return FVector{ v }; },
			[](FRotator const& r) -> FVector { return r.Vector(); },
			[](sol::table const& v)
			{
				if (v.size() > 0)
				{
					return FVector{
						v[1].get_or<float, float>(0),
						v[2].get_or<float, float>(0),				
						v[3].get_or<float, float>(0)};					
				}
				else
				{
					return FVector{
						v["X"].get_or<float, float>(0),
						v["Y"].get_or<float, float>(0),
						v["Z"].get_or<float, float>(0)};
				}
			},
			[](sol::variadic_args va) -> FVector
			{
				if (va.size() == 1) { return FVector(va[0].get<float>(), 0, 0); }
				else if (va.size() == 2) { return FVector(va[0].get<float>(), va[1].get<float>(), 0); }
				else { return FVector(va[0].get<float>(), va[1].get<float>(), va[2].get<float>()); }
			}
		),
		"Size", &FVector::Size,
		sol::meta_function::addition, sol::overload (
			[](const FVector& self, const FVector& other) -> FVector { return self.operator+(other); },
			[](const FVector& self, double scalar) -> FVector { return {self.X + scalar, self.Y + scalar, self.Z + scalar}; },
			[](const FVector& self, int scalar) -> FVector { return {self.X + scalar, self.Y + scalar, self.Z + scalar}; },
			[](const FVector& self, const FVector2D& other) -> FVector { return self + FVector{other.X, other.Y, 0}; },
			[](const FVector& self, const FRotator& other) -> FVector { return self.operator+(other.Vector()); },
			[](const FVector& self, sol::variadic_args) -> FVector { return self; }
		),
		sol::meta_function::subtraction, sol::overload (
			[](const FVector& self, const FVector& other) -> FVector { return self.operator-(other); },
			[](const FVector& self, double scalar) -> FVector { return {self.X - scalar, self.Y - scalar, self.Z - scalar}; },
			[](const FVector& self, int scalar) -> FVector { return {self.X - scalar, self.Y - scalar, self.Z - scalar}; },
			[](const FVector& self, const FVector2D& other) -> FVector { return self - FVector{other.X, other.Y, 0}; },
			[](const FVector& self, const FRotator& other) -> FVector { return self.operator-(other.Vector()); },
			[](const FVector& self, sol::variadic_args) -> FVector { return self; }
		),
		sol::meta_function::multiplication, sol::overload(
			[](const FVector& self, const FVector& other) -> FVector { return self.operator*(other); },
			[](const FVector& self, double scalar) -> FVector { return self.operator*(scalar); },
			[](const FVector& self, int scalar) -> FVector { return self.operator*(scalar); },
			[](const FVector& self, const FVector2D& other) -> FVector { return self.operator*(FVector{other.X, other.Y, 0}); },
			[](const FVector& self, const FRotator& other) -> FVector { return self.operator*(other.Vector()); },
			[](const FVector& self, sol::variadic_args) -> FVector { return self; }
		),
		sol::meta_function::division, sol::overload(
			[](const FVector& self, const FVector& other) -> FVector { return self.operator/(other); },
			[](const FVector& self, double scalar) -> FVector { return self.operator/(scalar); },
			[](const FVector& self, int scalar) -> FVector { return self.operator/(scalar); },
			[](const FVector& self, const FVector2D& other) -> FVector { return self.operator/(FVector{other.X, other.Y, 0}); },
			[](const FVector& self, const FRotator& other) -> FVector { return self.operator/(other.Vector()); },
			[](const FVector& self, sol::variadic_args) -> FVector { return self; }
		),
		"Normalize", &FVector::Normalize,
		"GetNormal", &FVector::GetSafeNormal,
		sol::meta_function::less_than, [](const FVector& self, const FVector& other) { return self.X < other.X && self.Y < other.Y && self.Z < other.Z;},
		sol::meta_function::less_than_or_equal_to, [](const FVector& self, const FVector& other) { return self.X <= other.X && self.Y <= other.Y && self.Z <= other.Z;},
		"X", &FVector::X,
		"Y", &FVector::Y,
		"Z", &FVector::Z,
		"Distance", [](sol::object self, sol::object other)
		{
			if(self.is<FVector>() && other.is<FVector>())
			{
				return FVector::Distance(self.as<FVector&>(), other.as<FVector&>());
			}
			return 0.0;
		},
		sol::meta_function::to_string, [](const FVector& self) -> std::string { return std::string(StringCast<char>(*self.ToString()).Get());}, 
		"ToString", [](const FVector& self) -> std::string { return std::string(TCHAR_TO_UTF8(*self.ToString()));}
	);
	lua.new_usertype<FMulticastDelegatePropertyProxy>(
		"MulticastDelegatePropertyProxy",
		"new", sol::no_constructor,
		"Add", &FMulticastDelegatePropertyProxy::Add,
		"AddUnique", &FMulticastDelegatePropertyProxy::AddUnique,
		"Remove", &FMulticastDelegatePropertyProxy::Remove,
		"Broadcast", &FMulticastDelegatePropertyProxy::Broadcast
	);
	lua.new_usertype<FSingleDelegatePropertyProxy>(
		"DelegatePropertyProxy",
		"new", sol::no_constructor,
		"Bind", &FSingleDelegatePropertyProxy::Bind,
		"Unbind", &FSingleDelegatePropertyProxy::Unbind,
		"IsBound", &FSingleDelegatePropertyProxy::IsBound,
		"Execute", &FSingleDelegatePropertyProxy::Execute
	);

	lua.new_usertype<FLuaScriptStructBase>(
		"FLuaScriptStructBase",
		"new", sol::no_constructor
	);
	LUA_LOG("All core usertypes registered")
	sol::state_view view{lua};
	FLuaCoreDelegates::OnRegisterLuaUsertypes.Broadcast(view);
	LUA_LOG("All usertypes registered")
}

bool FLuaUsertypes::Is(sol::object objToCheck, sol::object typeToCheck, sol::this_state lua_)
{
	if(!objToCheck.valid() || !typeToCheck.valid())
	{
		return true;
	}
	if(typeToCheck.get_type() != sol::type::string)
	{
		sol::state_view lua{lua_};
		if(UObject* obj = UnrealLua::GetUObject(objToCheck))
		{
			if(!obj)
			{
				return false;
			}
			
			FLuaImportRegistry& reg = FLuaImportRegistry::Get();
			sol::object type = reg.__index(typeToCheck, lua_);
			if(!type.is<FLuaUClass>())
			{
				return false;
			}

			FLuaUClass& lclazz = type.as<FLuaUClass&>(); 
		
			UClass* clazz = lclazz.TryLoadClass();
			if(!clazz)
			{
				return false;
			}
			bool isResult = obj->IsA(clazz); 
			return isResult;
		}
		else if(objToCheck.is<FLuaScriptStruct>())
		{
			const UScriptStruct* ss = objToCheck.as<FLuaScriptStruct&>().GetScriptStruct();
			if(!ss)
			{
				return false;
			}
			FLuaImportRegistry& reg = FLuaImportRegistry::Get();
			sol::object type = reg.__index(typeToCheck, lua_);
			if(!type.is<FLuaUStruct>())
			{
				return false;
			}
			FLuaUStruct& strct = type.as<FLuaUStruct&>();
			return ss->IsChildOf(Cast<UScriptStruct>(strct.TryLoad()));
		}
	}
	return false;
}

/*
UObject* sol_lua_get(UObject*, lua_State* L, int index, sol::stack::record& tracking) {
	const int absolute_index = lua_absindex(L, index);
	
	if (sol::stack::check_usertype<FLuaUObject>(L, index)) {
		FLuaUObject& ns = sol::stack::get_usertype<FLuaUObject>(L, index, tracking);
		tracking.use(1);
		return ns.Object;
	}
	tracking.use(1);
	return nullptr;


	// Get the first element
	const FLuaUObject& obj{sol::stack::get<FLuaUObject&>(L, absolute_index)};
	
	return obj.Object;

}

int sol_lua_push(lua_State* L, UObject* obj) {
	// amount will be 1: int pushes 1 item
	return sol::stack::push(L, FLuaUObject{obj});	
}

template <typename Handler>
bool sol_lua_check(UObject*, lua_State* L, int index, Handler&& handler, sol::stack::record& tracking) {
	// indices can be negative to count backwards from the top of the stack,
	// rather than the bottom up
	// to deal with this, we adjust the index to
	// its absolute position using the lua_absindex function
	int absolute_index = lua_absindex(L, index);
	// Check first and second second index for being the proper types
	bool success = sol::stack::check<FLuaUObject>(L, absolute_index, handler);
	tracking.use(1);
	return success;
}
*/