#pragma once

namespace UnrealLua
{
	namespace Compilation
	{
		//When trying to find a UProperty on an UObject, also look through child UStruct properties to find a matching property
		/* Example:
			-- C++:
			struct FMyStruct
			{
				UPROPERTY()
				int X;
			}

			class UMyClass
			{
				UPROPERTY()
				FMyStruct myStruct;
			}

			--Lua :
			--self is an object of UMyClass
			function MyScript:Func()
				--Set MyClass.MyStruct.X to 123
				self.X = 123
				--Read 123 from MyClass.MyStruct.X
				print(self.X)  
			end
		 */
		constexpr bool WITH_SUBSTRUCT_PROPERTIES_ACCESS = false;

		//Whether the server will kick clients if the client tries to server-rpc a lua script function that does not exist on the server
		constexpr bool WITH_SERVER_RPC_FUNCTION_VALIDATE = false;

		//How many entries for preparing and recording UFunction input parameters in a FFuncDescr should be preallocated
		//constexpr uint8 NUM_PREALLOCATED_FUNCDESCR_ARGVALS = 4;

		//Allow Structs to inherit and implement FBlueprintLibrarySupportedScriptStruct to allow them pseudo-function calls
		constexpr bool  WITH_SCRIPTSTRUCT_FUNCTION_LIBS = true;

		//if a UFunction has no output/return params, return calling object
		//This allows function chaining self:DoThis(x):DoThat(y,z):AndThat(w)
		constexpr bool  WITH_UFUNCTION_CHAINING = true;

		//Allows named access and overriding calls of subobjects (components) from within a single script
		/*
			Usage : When declaring script
		
			local myActor = {}
			function myActor:ReceiveBeginPlay()
			end

			--Create overrides for subobject named MovementComponent
			myActor.MovementComponent = {}
			function myActor.MovementComponent:ReceiveBeginPlay()
			end

			return myActor
		*/
		constexpr bool  WITH_SUBOBJECT_ACCESS = false;

		//periodically scan loaded lua script files whether they were modified
		//and if so, attempt to reload them
		constexpr bool WITH_AUTO_HOTRELOAD = false;

		/*
		 * Whether to correct the index of Unreal containers by subtracting 1 from the asked index:
		 * In Lua:
		 * local arr = TArray("int32")
		 * arr[1] = 123
		 * 
		 * If ZERO_INDEXING_CORRECTION_FOR_UE_CONTAINERS is true, this will become
		 * FLuaArray[0] = 123
		 * otherwise, if ZERO_INDEXING_CORRECTION_FOR_UE_CONTAINERS is false, this will become
		 * FLuaArray[1] = 123
		 */
		constexpr bool ZERO_INDEXING_CORRECTION_FOR_UE_CONTAINERS = true;
		
//		constexpr bool WITH_LIGHTUSERDATA_UOBJECTS = false;
		
		//constexpr bool LUASCRIPTINSTANCE_USE_FLUASCRIPTVALUES_INSTEAD_OF_LUATABLE = true;
	}
}