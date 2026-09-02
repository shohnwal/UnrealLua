#pragma once
#include "UObject/Object.h"
#include "UObject/Class.h"
#include "UObject/Stack.h"


namespace UnrealLua
{	
	inline bool IsParentCall(FFrame& currentFrame)
	{
		UFunction* currentFunc = currentFrame.CurrentNativeFunction;
		FFrame* previousFrame = currentFrame.PreviousFrame;
		if (!previousFrame)
		{
			return false;
		}
		UFunction* previousFunc = previousFrame->CurrentNativeFunction;
		bool isParentFunc = previousFunc == currentFunc->GetSuperStruct(); 
		return isParentFunc;
	}
	
	/** Helper function to zero the return value in case of a fatal (runaway / infinite recursion) error */
	void ClearReturnValue(FProperty* ReturnProp, RESULT_DECL);

	void SkipFunction(FFrame& Stack, RESULT_DECL, UFunction* Function);
	
	void ReadParams(FFrame& Stack, uint8* funcMemory, UFunction* Function);
	
	inline void ProcessLocalScriptFunction(UObject* Context, FFrame& Stack, RESULT_DECL)
	{
		UFunction* Function = (UFunction*)Stack.Node;
		// No POD struct can ever be stored in this buffer. 
		MS_ALIGN(16) uint8 Buffer[MAX_SIMPLE_RETURN_VALUE_SIZE] GCC_ALIGN(16);

		// Execute the bytecode
		while (*Stack.Code != EX_Return && !Stack.bAbortingExecution)
		{
			Stack.Step(Stack.Object, Buffer);
		}

		if (!Stack.bAbortingExecution)
		{
			// Step over the return statement and evaluate the result expression
			Stack.Code++;

			if (*Stack.Code != EX_Nothing)
			{
				Stack.Step(Stack.Object, RESULT_PARAM);
			}
			else
			{
				Stack.Code++;
			}
		}
		else
		{
			// If we have a return property, return a zeroed value in it
			FProperty* ReturnProp = (Function)->GetReturnProperty();
			ClearReturnValue(ReturnProp, RESULT_PARAM);
		}
	}

	// Helper function to set up a script function, and then execute it using ExecFtor. This is 
	// a template function because we use alloca to allocate space for parameters and results,
	// but we also have two hotpaths: normal function calls which must call GetFunctionCallspace,
	// and normal bytecode functions that are local only!
	template<typename Exec>
	void ProcessScriptFunction(UObject* Context, UFunction* Function, FFrame& Stack, RESULT_DECL, Exec ExecFtor)
	{
		//check(!Function->HasAnyFunctionFlags(FUNC_Native));

		// Allocate any temporary memory the script may need via AllocA. This AllocA dependency, along with
		// the desire to inline calls to our Execution function are the reason for this template function:
		FFrame NewStack(Context, Function, nullptr, &Stack, Function->ChildProperties);
		UE_VSTACK_MAKE_FRAME(ProcessScriptFunctionBookmark, NewStack.CachedThreadVirtualStackAllocator);

		uint8* FrameMemory = Function->GetOuterUClassUnchecked()->GetPersistentUberGraphFrame(Context, Function);

		bool bUsePersistentFrame = (nullptr != FrameMemory);
		if (!bUsePersistentFrame)
		{
			FrameMemory = (uint8*)UE_VSTACK_ALLOC_ALIGNED(NewStack.CachedThreadVirtualStackAllocator, Function->PropertiesSize, Function->GetMinAlignment());
			if (Function->PropertiesSize)
			{
				FMemory::Memzero(FrameMemory, Function->PropertiesSize);
			}
		}

		/* 
			Allocate space for return value bookkeeping - rarely used by bytecode functions, 
			but necessary in cases where a bytecode function's signature needs to match 
			a native function:
		 */
		if( Function->ReturnValueOffset != MAX_uint16 )
		{
			FProperty* ReturnProperty = Function->GetReturnProperty();
			if(ensure(ReturnProperty))
			{
				FOutParmRec* RetVal = (FOutParmRec*)UE_VSTACK_ALLOC(NewStack.CachedThreadVirtualStackAllocator, sizeof(FOutParmRec));

				/* Our context should be that we're in a variable assignment to the return value, so ensure that we have a valid property to return to */
				check(RESULT_PARAM != NULL);
				RetVal->PropAddr = (uint8*)RESULT_PARAM;
				RetVal->Property = ReturnProperty;
				NewStack.OutParms = RetVal;
			}
		}
		
		NewStack.Locals = FrameMemory;
		FOutParmRec** LastOut = &NewStack.OutParms;
			
		for (FProperty* Property = (FProperty*)(Function->ChildProperties); *Stack.Code != EX_EndFunctionParms; Property = (FProperty*)Property->Next)
		{
			checkfSlow(Property, TEXT("NULL Property in Function %s"), *Function->GetPathName()); 

			Stack.MostRecentPropertyAddress = nullptr;
			Stack.MostRecentPropertyContainer = nullptr;

			// Skip the return parameter case, as we've already handled it above
			const bool bIsReturnParam = ((Property->PropertyFlags & CPF_ReturnParm) != 0);
			if( bIsReturnParam )
			{
				continue;
			}

			if (Property->PropertyFlags & CPF_OutParm)
			{
				// evaluate the expression for this parameter, which sets Stack.MostRecentPropertyAddress to the address of the property accessed
				Stack.Step(Stack.Object, NULL);

				CA_SUPPRESS(6263)
				FOutParmRec* Out = (FOutParmRec*)UE_VSTACK_ALLOC(NewStack.CachedThreadVirtualStackAllocator, sizeof(FOutParmRec));
				// set the address and property in the out param info
				// warning: Stack.MostRecentPropertyAddress could be NULL for optional out parameters
				// if that's the case, we use the extra memory allocated for the out param in the function's locals
				// so there's always a valid address
				ensureMsgf(Stack.MostRecentPropertyAddress, TEXT("MostRecentPropertyAddress was null. Blueprint callstack:\n%s"), *Stack.GetScriptCallstack()); // possible problem - output param values on local stack are neither initialized nor cleaned.
				Out->PropAddr = (Stack.MostRecentPropertyAddress != NULL) ? Stack.MostRecentPropertyAddress : Property->ContainerPtrToValuePtr<uint8>(NewStack.Locals);
				Out->Property = Property;

				// add the new out param info to the stack frame's linked list
				if (*LastOut)
				{
					(*LastOut)->NextOutParm = Out;
					LastOut = &(*LastOut)->NextOutParm;
				}
				else
				{
					*LastOut = Out;
				}
			}
			else
			{
				// copy the result of the expression for this parameter into the appropriate part of the local variable space
				uint8* Param = Property->ContainerPtrToValuePtr<uint8>(NewStack.Locals);
				checkSlow(Param);

				Property->InitializeValue_InContainer(NewStack.Locals);

				Stack.Step(Stack.Object, Param);
			}
		}
		Stack.Code++;
		// set the next pointer of the last item to NULL to mark the end of the list
		if (*LastOut)
		{
			(*LastOut)->NextOutParm = NULL;
		}

		if (!bUsePersistentFrame)
		{
			// Initialize any local properties that aren't CPF_ZeroConstruct:
			for (FProperty* LocalProp = Function->FirstPropertyToInit; LocalProp != nullptr; LocalProp = (FProperty*)(LocalProp->PostConstructLinkNext))
			{
				LocalProp->InitializeValue_InContainer(NewStack.Locals);
			}
		}

		// Execute the code.
		ExecFtor( Context, NewStack, RESULT_PARAM );


		if (!bUsePersistentFrame)
		{
			// destruct properties on the stack, except for out params since we know we didn't use that memory
			for (FProperty* Destruct = Function->DestructorLink; Destruct; Destruct = Destruct->DestructorLinkNext)
			{
				if (!Destruct->HasAnyPropertyFlags(CPF_OutParm))
				{
					Destruct->DestroyValue_InContainer(NewStack.Locals);
				}
			}
		}

		// propagate abort flag up the stack
		Stack.bAbortingExecution |= NewStack.bAbortingExecution;
	}
}
