#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Input/Reply.h"

class UMD5AnimImportOptions;

class SMD5AnimImportOptionsWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMD5AnimImportOptionsWindow):_ImportOptions(nullptr),_WidgetWindow(nullptr) {}
		SLATE_ARGUMENT(UMD5AnimImportOptions*,ImportOptions)
		SLATE_ARGUMENT(TSharedPtr<SWindow>,WidgetWindow)
	SLATE_END_ARGS()

public:
	SMD5AnimImportOptionsWindow();
	~SMD5AnimImportOptionsWindow();

	void Construct(const FArguments& InArgs);

	bool ShouldImport() const {return bShouldImport;}

private:
	
	FReply OnImport();
	
	FReply OnCancel()
	{
		bShouldImport = false;
		if(WidgetWindow.IsValid())
		{
			WidgetWindow->RequestDestroyWindow();
		}
		return FReply::Handled();
	}
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override
	{
		if(InKeyEvent.GetKey() == EKeys::Enter)  return OnImport();
		if(InKeyEvent.GetKey() == EKeys::Escape) return OnCancel();
		return FReply::Unhandled();
	}
private:
	TWeakObjectPtr<UMD5AnimImportOptions> ImportOptions;
	TSharedPtr<SWindow> WidgetWindow;
	bool bShouldImport;
};