#include "Utility/ParseUObjectToLua.h"

#include "Config/UnrealLuaConstants.h"
#include "Reflection/PropertyHelper_ToString.h"
#include "Reflection/PropertyHelper_Utility.h"
#include "UObject/PropertyTypeName.h"

FString UnrealLua::ParseUtility::ParseUFunctionToLuaFunctionTemplateString(const FString& tableName, UFunction* func)
{
	FString funcNameStr = func->GetName();
	
	FStringBuilderBase fileBuilder;
	FStringBuilderBase argsBuilder;
	FStringBuilderBase returnBuilder;
	
	TArray<FProperty*> inputParams;
	TArray<FProperty*> outParams;
	
	//Sort UFunctions
	for (TFieldIterator<FProperty> propIt(func); propIt; ++propIt)
	{
		
		FProperty* prop = *propIt;
		if (UnrealLua::PropertyHelper::IsInputParameter(prop))
		{
			inputParams.Add(prop);
		}
		if (UnrealLua::PropertyHelper::IsOutputParameter(prop))
		{
			outParams.Add(prop);
		}
		if (prop->HasAnyPropertyFlags(CPF_ReturnParm))
		{
			outParams.EmplaceAt(0, prop);
		}
	}
	
	//Parse input parms
	for (int32 index = 0; index < inputParams.Num(); index++)
	{
		FProperty* prop = inputParams[index];
		argsBuilder << prop->GetName();
		if (index < inputParams.Num() - 1)
		{
			argsBuilder << ", ";
		}
	}
	
	//Parse output parms
	for (int32 index = 0; index < outParams.Num(); index++)
	{
		FProperty* prop = outParams[index];
		returnBuilder << prop->GetName();
		if (index < outParams.Num() - 1)
		{
			returnBuilder << ", ";
		}
	}
	FString args = argsBuilder.ToString();
	FString returns = returnBuilder.ToString();
	
	//function signature
	fileBuilder << "function Script:" << funcNameStr << "(";
	fileBuilder << args;
	fileBuilder << ")\n";
	
	//self call line : local a, b, c = self:Super("FuncName", c, d)
	if (func->GetFName() == UnrealLua::PropertyNames::NAME_Tick || func->GetFName() == UnrealLua::PropertyNames::NAME_ReceiveTick)
	{
		fileBuilder << "-- No super call needed for Tick function\n";
	}
	else
	{
		fileBuilder << "\t";
		if (!returns.IsEmpty())
		{
			fileBuilder << "local " << returns << " = ";
		}
		fileBuilder << "self:Super(\"" << funcNameStr << "\"";
		if (!args.IsEmpty())
		{
			fileBuilder << ", ";
			fileBuilder << args;
		}
		fileBuilder << ")\n";	
	}
	
	//optional return values line
	if (!returns.IsEmpty())
	{
		fileBuilder << "\treturn " << returns << "\n";	
	}
	//func end
	fileBuilder << "end\n";
	
	return fileBuilder.ToString();
}

FString UnrealLua::ParseUtility::ParsePropertyToLuaFunctionTemplateString(const FString& tableName, FProperty* prop)
{
	FString propLine = "--\t" + tableName + "." + prop->GetName() + " =\n";
	return propLine;
}

FString UnrealLua::ParseUtility::ParseUFunctionToLuaFunctionAnnontation(UFunction* func, bool signatureOnly, bool withFuncName, bool withAnnotations)
{
	FStringBuilderBase content;
	
	FString funcName = withFuncName ?  func->GetName() : "";
	if (withAnnotations)
	{
		content << "---@param self " <<"\n";
	}
		
	FString self = "self";
	FString paramNames = "";
	for(TFieldIterator<FProperty> prop(func); prop; ++prop)
	{
		FString propName = prop->GetName();
			
		if(prop->HasAllPropertyFlags(CPF_Parm) && !prop->HasAnyPropertyFlags(CPF_ReturnParm))
		{
			FString outParmPostFix = "";
			if(prop->HasAnyPropertyFlags(CPF_ConstParm))
			{
				outParmPostFix += " const";
			}
			else if(prop->HasAnyPropertyFlags(CPF_OutParm))
			{
				outParmPostFix += " out";
			}
			if(prop->HasAnyPropertyFlags(CPF_ReferenceParm))
			{
				outParmPostFix += " ref";
			}
			if(!outParmPostFix.IsEmpty())
			{
				outParmPostFix = outParmPostFix.TrimStart();
				outParmPostFix = FString::Printf(TEXT(" (%s)"),*outParmPostFix);
			}
			if (withAnnotations)
			{
				content << "---@param " << propName << " " << UnrealLua::PropertyHelper::GetPropertyTypeName(*prop, true) << "?" << outParmPostFix << "\n";
			}
			if (!paramNames.IsEmpty())
			{
				paramNames += ", ";
			}
			paramNames += propName;
		}
	}

	FProperty* retprop = func->GetReturnProperty();
	if (retprop && withAnnotations)
	{
		FString propName = retprop->GetName();
		content << "---@return " << UnrealLua::PropertyHelper::GetPropertyTypeName(retprop, true) << " " << "\n";
	}
		
	content << "function " << funcName << "(" << self;
	if (!paramNames.IsEmpty())
	{
		content << ", " << paramNames;
	}
	content << ")";
	
	if(!signatureOnly)
	{
		content << "\n";
		content << "  self:Super(\"" << func->GetName() << "\"";
		if (!paramNames.IsEmpty())
		{
			content << ", " << paramNames;
		}
		content << ")";
		content << "\n";
		if (retprop)
		{
			FGetPropertyValueAsLuaSyntaxStringParams params{retprop, nullptr, false, 0};
			FString returnValue = UnrealLua::PropertyHelper::GetPropertyValueAsLuaSyntaxValidString(params);
			content << "  return " << returnValue << "\n";
		}
		else
		{
			content << "\n";
		}
		content << "end";		
	}
	
	return content.ToString();
}
