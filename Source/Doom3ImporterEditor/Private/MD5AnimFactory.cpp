#include "MD5AnimFactory.h"
#include "MD5ImportUtil.h"
#include "MD5AnimImportOptions.h"
#include "SMD5AnimImportOptionsWindow.h"
#include "Interfaces/IMainFrameModule.h"
//UE5
#include "AssetRegistry/AssetRegistryModule.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimData/AnimDataModel.h"
#include "AnimDataController.h"
#include "Rendering/SkeletalMeshLODModel.h"
#include "Animation/AnimData/IAnimationDataController.h"
//Assimp
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

UMD5AnimFactory::UMD5AnimFactory()
{
	bEditorImport = true;
	bText = false;// 虽然 MD5 是文本，但由于我们用 Assimp 统一处理，设为 false 即可
	SupportedClass = UAnimSequence::StaticClass();
	Formats.Add(TEXT("md5anim;Doom 3 Animation"));
	// 设置优先级
	// 如果有多个工厂支持同一个后缀，值越大优先级越高。通常设为高一点防止被通用导入器拦截。
	ImportPriority = DefaultImportPriority + 10;
}

bool UMD5AnimFactory::FactoryCanImport(const FString& Filename)
{
	const FString Extension = FPaths::GetExtension(Filename);
	if (Extension == TEXT("md5anim"))
	{
		return true;
	}
	return false;
}

UObject* UMD5AnimFactory::FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled)
{
	//assimp加载动画文件
	Assimp::Importer* Importer = new Assimp::Importer();
	const aiScene* Scene = MD5ImportUtil::LoadAssimpSceneFromFile(Importer,TCHAR_TO_UTF8(*Filename));
	if(!Scene || !Scene->HasMeshes())
	{
		delete Importer;
		return nullptr;
	}
	//打开选择骨骼资产弹窗
	UMD5AnimImportOptions* MD5AnimImportOptions = NewObject<UMD5AnimImportOptions>(GetTransientPackage(),NAME_None,RF_Transactional);
	TSharedPtr<SWindow> ParentWindow;
	if(FModuleManager::Get().IsModuleLoaded("MainFrame"))
	{
		IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");
		ParentWindow = MainFrame.GetParentWindow();
	}
	TSharedRef<SWindow> Window = SNew(SWindow)
		.ClientSize(FVector2D(450.0f,550.f))
		.Title(FText::FromString("MD5 Anim Import Options"));
	TSharedPtr<SMD5AnimImportOptionsWindow> MD5AnimOptionsWindow;
	Window->SetContent(
		SAssignNew(MD5AnimOptionsWindow, SMD5AnimImportOptionsWindow)
		.ImportOptions(MD5AnimImportOptions)
		.WidgetWindow(Window)
	);
	FSlateApplication::Get().AddModalWindow(Window,ParentWindow,false);
	if(!MD5AnimOptionsWindow->ShouldImport() || MD5AnimImportOptions->Skeleton == nullptr)
	{
		delete Importer;
		return nullptr;
	}
	USkeleton* TargetSkeleton = MD5AnimImportOptions->Skeleton;
	if(!TargetSkeleton)
	{
		UE_LOG(LogTemp,Error,TEXT("UMD5AnimFactory::FactoryCreateFile - Failed to load Skeleton"));
		delete Importer;
		return nullptr;
	}
	//
	UAnimSequence* NewAnim = NewObject<UAnimSequence>(InParent,InClass,InName,Flags);
	NewAnim->SetSkeleton(TargetSkeleton);
	//
	aiAnimation* Anim = Scene->mAnimations[0];
	double Duration = Anim->mDuration / Anim->mTicksPerSecond;
	int32 NumFrames = FMath::CeilToInt(Anim->mDuration);
	float ModelScale = 2.5f;
	// 以骨骼资产的参考姿势的根骨格定义补偿变换
	FQuat RefRootRotation = TargetSkeleton->GetReferenceSkeleton().GetRefBonePose()[0].GetRotation();
	//
	IAnimationDataModel::FReimportScope ReimportScope(NewAnim->GetDataModel());
	IAnimationDataController& Controller = NewAnim->GetController();
	Controller.InitializeModel();//很重要，要先初始化
	Controller.OpenBracket(FText::FromString("Importing Animation"));
	{
		Controller.RemoveAllBoneTracks();
		Controller.SetFrameRate(FFrameRate(Anim->mTicksPerSecond,1));
		Controller.SetNumberOfFrames(NumFrames);
		for(uint32 i = 0;i < Anim->mNumChannels;++i)
		{
			aiNodeAnim* Channel = Anim->mChannels[i];
			FName BoneName(Channel->mNodeName.C_Str());
			int32 BoneIndex = TargetSkeleton->GetReferenceSkeleton().FindBoneIndex(BoneName);
			if(BoneIndex != INDEX_NONE)
			{
				bool bIsRoot = BoneIndex == 0;// 判断是否为根骨骼
				bool bAddSuccess = Controller.AddBoneCurve(BoneName);
				if(!bAddSuccess)
				{
					UE_LOG(LogTemp,Error,TEXT("AddBoneCurve failed for Bone '%s' !!!!"),*BoneName.ToString());
				}
				TArray<FVector3f> PosKeys;
				TArray<FQuat4f> RotKeys;
				TArray<FVector3f> ScaleKeys;
				FQuat4f CurrentFrameOffset = FQuat4f::Identity;
				for(uint32 k = 0;k < Channel->mNumPositionKeys;++k)
				{
					aiVector3f Pos = Channel->mPositionKeys[k].mValue;
					aiQuaternion Rot = Channel->mRotationKeys[k].mValue;
					//
					FVector3f FinalPos = MD5ImportUtil::aiVec3ToFVec3(Pos) * ModelScale;
					FQuat4f FinalRot = MD5ImportUtil::aiQuatToFQuat(Rot);
					if(bIsRoot)
					{
						if(k == 0)
						{
							// 计算第一帧动画旋转与 RefPose 旋转之间的差值 (Offset)
							// 以后每一帧都应用这个 Offset
							CurrentFrameOffset = FQuat4f(RefRootRotation) * FinalRot.Inverse();
						}
						// --- 根骨骼特殊补偿 ---
						// 1. 旋转补偿：将修正旋转应用到原始旋转上
						// 注意乘法顺序：RootFix * InitialRot 表示在局部空间前置修正
						FQuat4f CorrectedRot = CurrentFrameOffset * FinalRot;
						RotKeys.Add(CorrectedRot);
						// 2. 位移补偿：如果旋转了根骨骼，位移方向也必须跟着旋转
						FVector3f CorrectedPos = CurrentFrameOffset.RotateVector(FinalPos);
						PosKeys.Add(CorrectedPos);
					}
					else
					{
						// 非根骨骼保持正常转换
						PosKeys.Add(FinalPos);
						RotKeys.Add(FinalRot);
					}
					ScaleKeys.Add(FVector3f(1, 1, 1));
				}
				bool bSetKeysSuccess = Controller.SetBoneTrackKeys(BoneName, PosKeys, RotKeys, ScaleKeys);
				if(!bSetKeysSuccess)
				{
					UE_LOG(LogTemp,Error,TEXT("SetBoneTrackKeys failed for Bone '%s' !!!!"),*BoneName.ToString());
				}
			}
		}
		Controller.NotifyPopulated();
	}
	Controller.CloseBracket();
	//
	NewAnim->bEnableRootMotion = true;
	NewAnim->RootMotionRootLock = ERootMotionRootLock::Zero;
	NewAnim->Modify();
	NewAnim->MarkPackageDirty();
	NewAnim->PostEditChange();
	// Notify the asset registry
	FAssetRegistryModule::AssetCreated(NewAnim);
	return NewAnim;
}

bool UMD5AnimFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
	return true;
}

void UMD5AnimFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{

}

int32 UMD5AnimFactory::GetPriority() const
{
	return ImportPriority;
}

EReimportResult::Type UMD5AnimFactory::Reimport( UObject* Obj )
{
	return EReimportResult::Failed;
}