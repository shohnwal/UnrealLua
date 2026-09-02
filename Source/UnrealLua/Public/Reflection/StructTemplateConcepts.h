#pragma once

#include "CoreMinimal.h"
#include "StructUtils/SharedStruct.h"
#include "StructUtils/InstancedStruct.h"
#include "LuaContext/LuaImportRegistry.h"
#include "LuaTypes/LuaEnum.h"
#include "LuaTypes/LuaLightUserdata.h"

struct FLuaValue;


template<typename T>
concept IsPushableScriptStruct = 	
					   !std::is_same_v<FSharedStruct, std::remove_cvref_t<std::remove_pointer_t<T>>>
					&& !std::is_same_v<FInstancedStruct, std::remove_cvref_t<std::remove_pointer_t<T>>>
					&& !std::is_same_v<FLuaValue, std::remove_cvref_t<std::remove_pointer_t<T>>>
					&& !std::is_same_v<FLuaUEnumMapping, std::remove_cvref_t<std::remove_pointer_t<T>>>
					&& !std::is_same_v<FLuaUEnumEntry, std::remove_cvref_t<std::remove_pointer_t<T>>>
					&& !std::is_same_v<FLuaImportRegistry, std::remove_cvref_t<std::remove_pointer_t<T>>>
					&& !std::derived_from<std::remove_const_t<T>, ILuaLightUserdata>
					&& requires(T t) { std::remove_cvref_t<std::remove_pointer_t<T>>::StaticStruct(); };

template<typename T>
concept IsLightUserdataStructPtr = std::is_pointer_v<T>
								&& !std::is_reference_v<T>
								&& std::derived_from<std::remove_const_t<std::remove_pointer_t<T>>, ILuaLightUserdata> 
								&& requires(T t) { std::remove_const_t<std::remove_pointer_t<T>>::StaticStruct(); };

template<typename T>
concept IsLightUserdataStruct = !std::is_pointer_v<T>
								&& !std::is_reference_v<T>
								&& std::derived_from<std::remove_const_t<T>, ILuaLightUserdata> 
								&& requires(T t) { std::remove_const_t<T>::StaticStruct(); };	

template<typename T>
concept IsUStruct = !std::is_pointer_v<T>
					&& !std::is_reference_v<T>
					&& IsPushableScriptStruct<T>;
					

template<typename T>
concept IsUStructPtr = std::is_pointer_v<T>
					&& !std::is_reference_v<T>
					&& IsPushableScriptStruct<T>;