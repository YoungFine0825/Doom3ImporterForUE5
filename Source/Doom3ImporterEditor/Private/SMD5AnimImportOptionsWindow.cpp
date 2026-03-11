#include "SMD5AnimImportOptionsWindow.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "PropertyCustomizationHelpers.h"
#include "MD5AnimImportOptions.h"
#include "Misc/MessageDialog.h"

SMD5AnimImportOptionsWindow::SMD5AnimImportOptionsWindow()
{
	ImportOptions = nullptr;
	WidgetWindow = nullptr;
}

SMD5AnimImportOptionsWindow::~SMD5AnimImportOptionsWindow()
{
	ImportOptions = nullptr;
	WidgetWindow = nullptr;
}

void SMD5AnimImportOptionsWindow::Construct(const FArguments& InArgs)
{
	ImportOptions = InArgs._ImportOptions;
	WidgetWindow = InArgs._WidgetWindow;

	this->ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(10)
		[
			SNew(STextBlock)
			.Text(FText::FromString("Select Skeleton to Import"))
			.Font(FAppStyle::GetFontStyle("StandardDialog.LargeFont"))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(10)
		[
			//资产选择入口
			SNew(SObjectPropertyEntryBox)
			.AllowedClass(USkeleton::StaticClass())
			.ObjectPath_Lambda([this]()
			{
				if(ImportOptions->Skeleton != nullptr)
				{
					return ImportOptions->Skeleton->GetPathName();
				}
				else
				{
					return FString("Unselected");
				}
			})
			.OnObjectChanged_Lambda([this](const FAssetData& AssetData)
			{
				ImportOptions->Skeleton = Cast<USkeleton>(AssetData.GetAsset());
			})
		]
		//底部按钮
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Right)
		.Padding(10.0f)
		[
			SNew(SHorizontalBox)
			//”导入“按钮
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0,0,10.0,0)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.Text(FText::FromString("Import"))
				.OnClicked(this, &SMD5AnimImportOptionsWindow::OnImport)
				.ButtonStyle(FAppStyle::Get(),"PrimaryButton")// 蓝色高亮按钮
			]
			//”取消“按钮
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.Text(FText::FromString("Cancel"))
				.OnClicked(this, &SMD5AnimImportOptionsWindow::OnCancel)
			]
		]
	];
}

FReply SMD5AnimImportOptionsWindow::OnImport()
{
	if(ImportOptions->Skeleton != nullptr)
	{
		bShouldImport = true;
		if(WidgetWindow.IsValid())
		{
			WidgetWindow->RequestDestroyWindow();
		}
	}
	else
	{
		FMessageDialog::Open(EAppMsgType::Ok,
		FText::FromString(TEXT("Please select a skeleton to import!")),
		FText::FromString(TEXT("Warning"))
		);
	}
	return FReply::Handled();
}	


