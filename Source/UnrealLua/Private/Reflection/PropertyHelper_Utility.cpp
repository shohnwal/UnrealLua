#include "Reflection/PropertyHelper_Utility.h"

#include "Reflection/PropertyDescr/FObjectPropertyDescr.h"
#include "Reflection/PropertyDescr/MulticastDelegatePropertyDescr.h"
#include "Reflection/PropertyDescr/SingleDelegatePropertyDescr.h"
#include "Reflection/PropertyDescr/StructPropertyDescr.h"


bool UnrealLua::PropertyHelper::CanPropertyContainObjectReferences(FProperty* prop)
{
	if(prop->IsA<FObjectProperty>())
	{
		return true;
	}
	if(prop->IsA<FClassProperty>())
	{
		return true;
	}
	if(prop->IsA<FStructProperty>())
	{
		return true;
	}
	if (prop->IsA<FMulticastDelegateProperty>())
	{
		return true;
	}
	if (prop->IsA<FDelegateProperty>())
	{
		return true;
	}
	return false;
}

uint32 UnrealLua::PropertyHelper::AddRefByProperty(FReferenceCollector& collector, FProperty* prop, void* memory, bool container)
{
	if(FObjectProperty* objprop = CastField<FObjectProperty>(prop))
	{
		return FUObjectPropertyDescr::AddRef(objprop, memory, collector, container);
	}
	if(FClassProperty* classprop = CastField<FClassProperty>(prop))
	{
		return FUObjectPropertyDescr::AddRef(classprop, memory, collector, container);
	}
	if(FStructProperty* strProp = CastField<FStructProperty>(prop))
	{
		return FStructPropertyDescr::AddRef(collector, strProp, memory, container);
	}
	if (FMulticastDelegateProperty* p = CastField<FMulticastDelegateProperty>(prop))
	{
		return FMulticastDelegatePropertyDescr::AddRef(collector, p, memory, container);
	}
	if (FDelegateProperty* dp = CastField<FDelegateProperty>(prop))
	{
		return FSingleDelegatePropertyDescr::AddRef(collector, dp, memory, container);
	}
	/*
	switch (prop->GetCastFlags() & supportedPropTypeFlags)
    {
        case CASTCLASS_FObjectProperty :
             return FUObjectPropertyDescr::AddRef(CastField<FObjectProperty>(prop), memory, collector, container);
        default:
            break;
    }
    */
    return false;
}

uint32 UnrealLua::PropertyHelper::AddRefByStruct(FReferenceCollector& collector, const UScriptStruct* scriptStruct, void* data)
{
	uint32 numReferenced = 0;
	for (TFieldIterator<FProperty> it(scriptStruct); it; ++it)
	{
		void* actualMem = it->ContainerPtrToValuePtr<void>(data);
		numReferenced += UnrealLua::PropertyHelper::AddRefByProperty(collector, *it, actualMem);
	}
	return numReferenced;
}

FString UnrealLua::PropertyHelper::GetPropertyFlagsString(EPropertyFlags Flags)
{
	FString Result;

	if (Flags & EPropertyFlags::CPF_Edit)            Result += TEXT("Edit|");
	if (Flags & EPropertyFlags::CPF_EditConst)        Result += TEXT("EditConst|");
	if (Flags & EPropertyFlags::CPF_Parm) Result += TEXT("Parm|");
	if (Flags & EPropertyFlags::CPF_ReferenceParm) Result += TEXT("ReferenceParm|");
	if (Flags & EPropertyFlags::CPF_IsPlainOldData) Result += TEXT("IsPOD|");
	if (Flags & EPropertyFlags::CPF_OutParm) Result += TEXT("OutParm|");
	if (Flags & EPropertyFlags::CPF_ReturnParm) Result += TEXT("ReturnParm|");
	if (Flags & EPropertyFlags::CPF_ConstParm)    Result += TEXT("ConstParm|");
	if (Flags & EPropertyFlags::CPF_BlueprintVisible) Result += TEXT("BlueprintVisible|");
	if (Flags & EPropertyFlags::CPF_BlueprintReadOnly) Result += TEXT("BlueprintReadOnly|");
	if (Flags & EPropertyFlags::CPF_Config)          Result += TEXT("Config|");

	// Remove trailing "|"
	if (Result.Len() > 0)
		Result.RemoveAt(Result.Len() - 1);

	return Result;
}