#pragma once
#include "Framework/Text/SlateTextRun.h"

class FLuaSyntaxTextRunVariable;
struct FUnrealLuaSyntaxVariable;

enum class ELuaSyntaxTextRunType : uint8
{
	None,
	KeywordLocal,
	Variable,
	FunctionKeyword,
	FunctionParamsParenthesis,
	String,
	Number,
	WhiteSpace,
	Annotation,		// A meta annotation: <---@<AnnotationType>>, for example ---@Type or ---@Param
	TypeAnnotation,  // The type name of a type meta annosation:---@Type <type>, for example ---@Type FVector
	Assignment, //Assignment operator
	Period,
	Colon,
	Boolean,
};

class UNREALLUATOOLS_API FUnrealLuaSyntaxTextRun : public ISlateRun, public TSharedFromThis< FUnrealLuaSyntaxTextRun >
{
public:
	static TSharedRef<FUnrealLuaSyntaxTextRun> Create(const FRunInfo& InRunInfo, const TSharedRef< const FString >& InText, const FTextBlockStyle& Style, const FTextRange& InRange, const TSharedPtr<FUnrealLuaSyntaxTextRun>& previous);
	
	static FString RunInfoName;
	
	virtual ~FUnrealLuaSyntaxTextRun() override = default;
	
	virtual FString GetRunName() const { return "GenericRun"; }
	virtual ELuaSyntaxTextRunType GetRunType() const { return ELuaSyntaxTextRunType::None;}
	
	//goes back the run chain, trying to find the current variable or field
	virtual TSharedPtr<FUnrealLuaSyntaxVariable> FindCurrentVariableOrField() const;
	virtual TSharedPtr<FLuaSyntaxTextRunVariable> FindCurrentVariableRun();
	virtual TSharedPtr<FLuaSyntaxTextRunVariable> FindTopOwningVariableRun();
	virtual TSharedPtr<FUnrealLuaSyntaxTextRun> GetNonWhiteSpacedRun();

	bool IsRunType(ELuaSyntaxTextRunType runType) const { return this->GetRunType() == runType;}
	bool HasPreviousRunType(ELuaSyntaxTextRunType previousRuntype) const;

	//virtual TSharedPtr<FLuaSyntaxTextRunAssignment> FindAssignment() const { return nullptr; }
	FUnrealLuaSyntaxTextRun(const FRunInfo& InRunInfo, const TSharedRef<const FString>& InText,
	const FTextBlockStyle& InStyle, const FTextRange& InRange, const TSharedPtr<FUnrealLuaSyntaxTextRun>& previous)
		: PreviousRun(previous)
		, RunInfo( InRunInfo )
		, Text( InText )
		, Style( InStyle )
		, Range( InRange )
		#if TEXT_LAYOUT_DEBUG
		, DebugSlice( FString( Text->Len(), **Text ) )
		#endif
	{
		this->RunInfo.Name = RunInfoName;
	}
public:
	virtual FTextRange GetTextRange() const override;
	virtual void SetTextRange( const FTextRange& Value ) override;
	
	virtual bool IsAssignable() const { return false; }
	
	template<typename T>
	TSharedPtr<T> As() const { return StaticCastSharedRef<T>(this->SharedThis(this));}
	template<typename T>
	TSharedPtr<T> As() { return StaticCastSharedRef<T>(this->SharedThis(this));}
	template<typename T>
	TSharedPtr<T> PreviousAs() const{ return this->PreviousRun.IsValid() ? this->PreviousRun.Pin()->As<T>() : nullptr; }
	template<typename T>
	TSharedPtr<T> PreviousAs() { return this->PreviousRun.IsValid() ? this->PreviousRun.Pin()->As<T>() : nullptr; }
	
	virtual int16 GetBaseLine( float Scale ) const override;
	virtual int16 GetMaxHeight( float Scale ) const override;
	virtual FVector2D Measure( int32 StartIndex, int32 EndIndex, float Scale, const FRunTextContext& TextContext ) const override;
	virtual int8 GetKerning(int32 CurrentIndex, float Scale, const FRunTextContext& TextContext) const override;
	virtual FVector2d GetOutlineSize(int32 StartIndex, int32 EndIndex, float Scale) const override;
	virtual FVector2d GetShadowSize(int32 StartIndex, int32 EndIndex, float Scale) const override;

	virtual TSharedRef< ILayoutBlock > CreateBlock( int32 StartIndex, int32 EndIndex, FVector2D Size, const FLayoutBlockTextContext& TextContext, const TSharedPtr< IRunRenderer >& Renderer ) override;

	virtual int32 OnPaint(const FPaintArgs& PaintArgs, const FTextArgs& TextArgs, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	virtual const TArray< TSharedRef<SWidget> >& GetChildren() override;

	virtual void ArrangeChildren( const TSharedRef< ILayoutBlock >& Block, const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const override;

	virtual void BeginLayout() override {}
	virtual void EndLayout() override {}

	virtual int32 GetTextIndexAt( const TSharedRef< ILayoutBlock >& Block, const FVector2D& Location, float Scale, ETextHitPoint* const OutHitPoint = nullptr ) const override;

	virtual FVector2D GetLocationAt(const TSharedRef< ILayoutBlock >& Block, int32 Offset, float Scale) const override;

	virtual void Move(const TSharedRef<FString>& NewText, const FTextRange& NewRange) override;
	virtual TSharedRef<IRun> Clone() const override;

	virtual void AppendTextTo(FString& Text) const override;
	virtual void AppendTextTo(FString& AppendToText, const FTextRange& PartialRange) const override;

	virtual const FRunInfo& GetRunInfo() const override;

	virtual ERunAttributes GetRunAttributes() const override;
	
	TWeakPtr<FUnrealLuaSyntaxTextRun> PreviousRun = nullptr;
	TWeakPtr<FUnrealLuaSyntaxTextRun> NextRun = nullptr;
	FRunInfo RunInfo;
	TSharedRef< const FString > Text;
	const FTextBlockStyle& Style;
	FTextRange Range;
};
