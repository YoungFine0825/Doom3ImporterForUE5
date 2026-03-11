#include "SMD5MeshImportOptionsWindow.h"
#include "Modules/ModuleManager.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "EditorStyleSet.h"
#include "MD5MeshImportOptions.h"

SMD5MeshImportOptionsWindow::SMD5MeshImportOptionsWindow()
{
	ImportOptions = nullptr;
	WidgetWindow = nullptr;
}

SMD5MeshImportOptionsWindow::~SMD5MeshImportOptionsWindow()
{
	ImportOptions = nullptr;
	WidgetWindow = nullptr;
}


void SMD5MeshImportOptionsWindow::Construct(const FArguments& InArgs)
{
	ImportOptions = InArgs._ImportOptions;
	WidgetWindow = InArgs._WidgetWindow;
	
	//创建垂直列表容器
	TSharedRef<SVerticalBox> ListContent = SNew(SVerticalBox);
	for(FMD5MeshImportEntry& Entry : ImportOptions->SubmesheList)
	{
		ListContent->AddSlot().AutoHeight().Padding(2.0f,1.0f)
		[
			MakeSubmeshListRow(Entry)
		];
	}

	//构建UI布局
	this->ChildSlot
	[
		SNew(SBox)
		.WidthOverride(450.0f)
		.HeightOverride(550.0f)
		[
			SNew(SVerticalBox)
			//顶部标题
			+ SVerticalBox::Slot().AutoHeight().Padding(10.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString("Select Submeshes to Import"))
				.Font(FAppStyle::GetFontStyle("StandardDialog.LargeFont"))
			]

			//列表区域
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(10,0)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot() [ListContent]
				]
			]
			//全选/取消全选按钮
			+ SVerticalBox::Slot().AutoHeight().Padding(10,5)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton)
					.Text(FText::FromString("Select All"))
					.OnClicked(this, &SMD5MeshImportOptionsWindow::OnSetAllImport,true)
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton)
					.Text(FText::FromString("Deselect All"))
					.OnClicked(this, &SMD5MeshImportOptionsWindow::OnSetAllImport,false)
				]
			]
			//创建物理资产复选框
			+ SVerticalBox::Slot().AutoHeight().Padding(10)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(5,2)
				[
					SNew(SCheckBox)
					.IsChecked_Lambda([this]() {return this->ImportOptions->bCreatePhysicsAsset ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;})
					.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState){this->ImportOptions->bCreatePhysicsAsset = (NewState == ECheckBoxState::Checked);})
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString("Create Physics Asset"))
				]
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
					.OnClicked(this, &SMD5MeshImportOptionsWindow::OnImport)
					.ButtonStyle(FAppStyle::Get(),"PrimaryButton")// 蓝色高亮按钮
				]
				//”取消“按钮
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.Text(FText::FromString("Cancel"))
					.OnClicked(this, &SMD5MeshImportOptionsWindow::OnCancel)
				]
			]
		]
	];
}

TSharedRef<SWidget> SMD5MeshImportOptionsWindow::MakeSubmeshListRow(FMD5MeshImportEntry& InEntry)
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(5,2)
		[
			SNew(SCheckBox)
			.IsChecked_Lambda([&InEntry]() {return InEntry.bShouldImport ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;})
			.OnCheckStateChanged_Lambda([&InEntry](ECheckBoxState NewState) {InEntry.bShouldImport = (NewState == ECheckBoxState::Checked); })
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(InEntry.MeshName))
		];
}

FReply SMD5MeshImportOptionsWindow::OnSetAllImport(bool bOutput)
{
	if(ImportOptions.IsValid())
	{
		for(auto& Entry : ImportOptions->SubmesheList)
		{
			Entry.bShouldImport = bOutput;
		}
	}
	return FReply::Handled();
}



