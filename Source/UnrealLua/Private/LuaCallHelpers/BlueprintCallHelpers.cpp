#include "LuaCallHelpers/BlueprintCallHelpers.h"
#include "Runtime/Launch/Resources/Version.h"

namespace UnrealLua
{
	void ClearReturnValue(FProperty* ReturnProp, void* const Z_Param__Result)
	{
		if (ReturnProp != NULL)
		{
			uint8* Data = (uint8*)RESULT_PARAM;
			for (int32 ArrayIdx = 0; ArrayIdx < ReturnProp->ArrayDim; ArrayIdx++, Data += ReturnProp->GetElementSize())
			{
				// Clear the property. This assumes that it has already been initialized, and that the caller will destroy it.
				ReturnProp->ClearValue(Data);
			}
		}
	}
	
	void ReadParams(FFrame& Stack, uint8* funcMemory, UFunction* Function)
	{
		if(!Stack.Code)
		{
			return;
		}
		uint8* Frame = (uint8*)funcMemory;
		//uint8* Frame = (uint8*) FMemory_Alloca(Function->PropertiesSize);
		FMemory::Memzero(Frame, Function->PropertiesSize);
		for (FProperty* Property = (FProperty*) (
#if ENGINE_MINOR_VERSION >= 25 || ENGINE_MAJOR_VERSION > 4
				 Function->ChildProperties
#else
				 Function->Children
#endif
			 );
			 Property && (*Stack.Code != EX_EndFunctionParms); Property = (FProperty*) (Property->Next))
		{
			Stack.MostRecentPropertyAddress = NULL;
			Stack.MostRecentPropertyContainer = nullptr;
			// evaluate the expression into our temporary memory space
			// it'd be nice to be able to skip the copy, but most native functions assume a non-NULL Result pointer
			// so we can only do that if we know the expression is an l-value (out parameter)
			Stack.Step(Stack.Object, (Property->PropertyFlags & CPF_OutParm) ? NULL : Property->ContainerPtrToValuePtr<uint8>(Frame));
		}
		checkSlow(*Stack.Code==EX_EndFunctionParms);
		// advance the code past EX_EndFunctionParms
		Stack.Code++;
	}

	
	void SkipFunction(FFrame& Stack, void* const Z_Param__Result, UFunction* Function)
	{
		if(!Stack.Code)
		{
			return;
		}
		uint8* Frame = (uint8*)FMemory_Alloca_Aligned(Function->PropertiesSize, Function->GetMinAlignment());
		uint8* ReturnValueAddress = Z_Param__Result ? ((uint8*)Frame + Function->ReturnValueOffset) : nullptr;
		//uint8* Frame = (uint8*) FMemory_Alloca(Function->PropertiesSize);
		FMemory::Memzero(Frame, Function->PropertiesSize);
		for (FProperty* Property = (FProperty*) (
#if ENGINE_MINOR_VERSION >= 25 || ENGINE_MAJOR_VERSION > 4
				 Function->ChildProperties
#else
				 Function->Children
#endif
			 );
			 Property && (*Stack.Code != EX_EndFunctionParms); Property = (FProperty*) (Property->Next))
		{
			Stack.MostRecentPropertyAddress = NULL;
			Stack.MostRecentPropertyContainer = nullptr;
			// evaluate the expression into our temporary memory space
			// it'd be nice to be able to skip the copy, but most native functions assume a non-NULL Result pointer
			// so we can only do that if we know the expression is an l-value (out parameter)
			Stack.Step(Stack.Object, (Property->PropertyFlags & CPF_OutParm) ? NULL : Property->ContainerPtrToValuePtr<uint8>(Frame));
		}
		checkSlow(*Stack.Code==EX_EndFunctionParms);
		// advance the code past EX_EndFunctionParms
		Stack.Code++;
		
		// destruct properties requiring it for which we had to use our temporary memory 
		// @warning: conditions for skipping DestroyValue() here must match conditions for passing NULL to Stack.Step() above
		for (FProperty* Destruct = Function->DestructorLink; Destruct; Destruct = Destruct->DestructorLinkNext)
		{
			if (!Destruct->HasAnyPropertyFlags(CPF_OutParm))
			{
				Destruct->DestroyValue_InContainer(Frame);
			}
		}

		FProperty* ReturnProp = Function->GetReturnProperty();
		if (ReturnProp != NULL)
		{
			ReturnProp->DestroyValue(ReturnValueAddress);
			FMemory::Memzero(ReturnValueAddress, ReturnProp->ArrayDim * ReturnProp->GetElementSize());
		}
	}
}