#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Input/Reply.h"

class UMD5MeshImportOptions;
struct FMD5MeshImportEntry;

class SMD5MeshImportOptionsWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMD5MeshImportOptionsWindow)
		: _ImportOptions(nullptr)
		,_WidgetWindow(nullptr)
	{}
		SLATE_ARGUMENT(UMD5MeshImportOptions*,ImportOptions)
		SLATE_ARGUMENT(TSharedPtr<SWindow>,WidgetWindow)
	SLATE_END_ARGS()

public:
	SMD5MeshImportOptionsWindow();
	~SMD5MeshImportOptionsWindow();

	void Construct(const FArguments& InArgs);

	//用户是否点击了导入按钮
	bool ShouldImport() const {return bConfirmImport;}

private:

	TSharedRef<SWidget> MakeSubmeshListRow(FMD5MeshImportEntry& InEntry);

	FReply OnSetAllImport(bool bOutput);
	
	FReply OnImport()
	{
		bConfirmImport = true;
		if(WidgetWindow.IsValid())
		{
			WidgetWindow->RequestDestroyWindow();
		}
		return FReply::Handled();
	}

	FReply OnCancel()
	{
		bConfirmImport = false;
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
	TWeakObjectPtr<UMD5MeshImportOptions> ImportOptions;
	TSharedPtr<SWindow> WidgetWindow;
	bool bConfirmImport;
};
