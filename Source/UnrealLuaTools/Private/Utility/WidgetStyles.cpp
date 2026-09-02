#include "Utility/WidgetStyles.h"

#include "IntelliSense/UnrealLuaSyntaxLayoutMarshaller.h"
#include "Styling/SlateStyleMacros.h"
#include "Styling/SlateWidgetStyle.h"
#include "Styling/UMGCoreStyle.h"
#include "UIWidgets/SMultiTabEdtitableLuaScriptSwitcher.h"




namespace UnrealLuaTools
{
	FEditableTextBoxStyle MonospacedCodingTextStyle = {}; 
	FSimpleMulticastDelegate OnStyleChanged = {};
	
	TUniquePtr<FLuaSyntaxTextStyle> LuaSyntax = nullptr;

}
void UnrealLuaTools::SlateStyles::Initialize()
{
	constexpr int32 SourceCodeFontSize = 14;
	
	MonospacedCodingTextStyle = FCoreStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>("NormalEditableTextBox");
	MonospacedCodingTextStyle.TextStyle.Font.bForceMonospaced = false;
	MonospacedCodingTextStyle.TextStyle.Font.MonospacedWidth = 1.f;
	MonospacedCodingTextStyle.SetPadding(FMargin(4.0f, 4.0f, 4.0f, 4.0f));
	MonospacedCodingTextStyle.TextStyle.Font.LetterSpacing = 1;
	MonospacedCodingTextStyle.FocusedForegroundColor = FLinearColor(0.68,0.68,0.68,1);
	MonospacedCodingTextStyle.ForegroundColor = FLinearColor(0.68,0.68,0.68,1);
	MonospacedCodingTextStyle.ReadOnlyForegroundColor = FLinearColor(0.4,0.4,0.4,1);
	MonospacedCodingTextStyle.TextStyle.Font.Size = SourceCodeFontSize;
	
	MonospacedCodingTextStyle.BackgroundColor = FLinearColor(0.11,0.11,0.11,1);
	MonospacedCodingTextStyle.BackgroundImageNormal.TintColor = FLinearColor(0.11,0.11,0.11,1);
	MonospacedCodingTextStyle.BackgroundImageHovered.TintColor = FLinearColor(0.11,0.11,0.11,1);
	MonospacedCodingTextStyle.BackgroundImageFocused.TintColor = FLinearColor(0.11,0.11,0.11,1);
	
	
	FSlateFontInfo SourceCodeFont = DEFAULT_FONT("Mono", SourceCodeFontSize);
	
	static const FTextBlockStyle NormalText = FTextBlockStyle()
		.SetFont(DEFAULT_FONT("Regular", FUMGCoreStyle::RegularTextSize))
		.SetColorAndOpacity(FSlateColor::UseForeground())
		.SetShadowOffset(FVector2f::ZeroVector)
		.SetShadowColorAndOpacity(FLinearColor::Black)
		.SetHighlightColor(FLinearColor(0.02f, 0.3f, 0.0f));
		//.SetHighlightShape(BOX_BRUSH("Common/TextBlockHighlightShape", FMargin(3.f /8.f)));
	
	FTextBlockStyle NormalSourceCodeText = FTextBlockStyle(NormalText)
		.SetFont(SourceCodeFont);
	const FTextBlockStyle SourceCodeErrorText = FTextBlockStyle(NormalSourceCodeText)
		//.SetUnderlineBrush(IMAGE_BRUSH("Old/White", FVector2f(8.f,8.f), FLinearColor::Red, ESlateBrushTileType::Both))
		.SetColorAndOpacity(FLinearColor::Red);
	
	LuaSyntax.Reset(new FLuaSyntaxTextStyle(
		FTextBlockStyle(NormalSourceCodeText).SetColorAndOpacity(FLinearColor(FColor(220, 220, 220))),
		FTextBlockStyle(NormalSourceCodeText).SetColorAndOpacity(FLinearColor(FColor(150, 156, 214))),
		FTextBlockStyle(NormalSourceCodeText).SetColorAndOpacity(FLinearColor(FColor(86, 156, 214))),
		FTextBlockStyle(NormalSourceCodeText).SetColorAndOpacity(FLinearColor(FColor(214, 157, 133))),
		FTextBlockStyle(NormalSourceCodeText).SetColorAndOpacity(FLinearColor(FColor(181, 206, 168))),
		FTextBlockStyle(NormalSourceCodeText).SetColorAndOpacity(FLinearColor(FColor(150, 150, 150))),
		FTextBlockStyle(NormalSourceCodeText).SetColorAndOpacity(FLinearColor(FColor(188, 98, 171))),
		FTextBlockStyle(NormalSourceCodeText).SetColorAndOpacity(FLinearColor(FColor(214, 90, 90)))
	));
}
FSimpleMulticastDelegate& UnrealLuaTools::SlateStyles::GetOnStyleChangedDelegate()
{
	return UnrealLuaTools::OnStyleChanged;
}

const FEditableTextBoxStyle* UnrealLuaTools::SlateStyles::GetEditableTextBoxStyle()
{
	return &MonospacedCodingTextStyle;
}

void UnrealLuaTools::SlateStyles::SetEditableTextBoxFontSize(float fontSize)
{
	const FVector2f limits = GetEditableTextBoxFontSizeLimits();
	fontSize = FMath::Clamp(fontSize, limits.X,limits.Y);
	int32 currentFontSize = MonospacedCodingTextStyle.TextStyle.Font.Size;
	if (currentFontSize != static_cast<int32>(fontSize))
	{
		MonospacedCodingTextStyle.TextStyle.Font.Size = fontSize;
		LuaSyntax->AnnotationStyle.Font.Size = fontSize;
		LuaSyntax->CommentTextStyle.Font.Size = fontSize;
		LuaSyntax->ErrorTextStyle.Font.Size = fontSize;
		LuaSyntax->KeywordTextStyle.Font.Size = fontSize;
		LuaSyntax->NormalTextStyle.Font.Size = fontSize;
		LuaSyntax->NumberTextStyle.Font.Size = fontSize;
		LuaSyntax->OperatorTextStyle.Font.Size = fontSize;
		LuaSyntax->StringTextStyle.Font.Size = fontSize;
	
		GetOnStyleChangedDelegate().Broadcast();	
	}
}

const FLuaSyntaxTextStyle& UnrealLuaTools::SlateStyles::GetLuaSyntaxStyle()
{
	return *UnrealLuaTools::LuaSyntax.Get();
}

FVector2f UnrealLuaTools::SlateStyles::GetEditableTextBoxFontSizeLimits()
{
	return FVector2f(8,96);
}

float UnrealLuaTools::SlateStyles::GetEditableTextBoxFontSize()
{
	return MonospacedCodingTextStyle.TextStyle.Font.Size;
}
