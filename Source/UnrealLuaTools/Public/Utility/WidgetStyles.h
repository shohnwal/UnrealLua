#pragma once
#include "CoreMinimal.h"
#include "Styling/SlateTypes.h"
class FUnrealLuaSyntaxLayoutMarshaller;
struct FTextBlockStyle;
struct FEditableTextBoxStyle;

enum class EUnreaLLuaCodeStyle
{
	Normal,
	Operator,
	Keyword,
	String,
	Number,
	Comment,
	Annotation, 
	Error
};

struct FLuaSyntaxTextStyle
{
	FLuaSyntaxTextStyle(
		const FTextBlockStyle& InNormalTextStyle,
		const FTextBlockStyle& InOperatorTextStyle,
		const FTextBlockStyle& InKeywordTextStyle,
		const FTextBlockStyle& InStringTextStyle,
		const FTextBlockStyle& InNumberTextStyle,
		const FTextBlockStyle& InCommentTextStyle,
		const FTextBlockStyle& InAnnotationTextStyle,
		const FTextBlockStyle& InErrorTextStyle
		) :
		NormalTextStyle(InNormalTextStyle),
		OperatorTextStyle(InOperatorTextStyle),
		KeywordTextStyle(InKeywordTextStyle),
		StringTextStyle(InStringTextStyle),
		NumberTextStyle(InNumberTextStyle),
		CommentTextStyle(InCommentTextStyle),
		AnnotationStyle(InAnnotationTextStyle),
		ErrorTextStyle(InErrorTextStyle)
	{
	}

	FTextBlockStyle NormalTextStyle;
	FTextBlockStyle OperatorTextStyle;
	FTextBlockStyle KeywordTextStyle;
	FTextBlockStyle StringTextStyle;
	FTextBlockStyle NumberTextStyle;
	FTextBlockStyle CommentTextStyle;
	FTextBlockStyle AnnotationStyle;
	FTextBlockStyle ErrorTextStyle;
};


namespace UnrealLuaTools::SlateStyles
{
	UNREALLUATOOLS_API void Initialize();
	UNREALLUATOOLS_API FSimpleMulticastDelegate& GetOnStyleChangedDelegate();
	UNREALLUATOOLS_API const FEditableTextBoxStyle* GetEditableTextBoxStyle();
	UNREALLUATOOLS_API void SetEditableTextBoxFontSize(float fontSize);
	
	UNREALLUATOOLS_API const FLuaSyntaxTextStyle& GetLuaSyntaxStyle();
	UNREALLUATOOLS_API FVector2f GetEditableTextBoxFontSizeLimits();
	UNREALLUATOOLS_API float GetEditableTextBoxFontSize();
}
