#include "MD5MeshFactory.h"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

//UE5
#include "Engine/SkeletalMesh.h"
#include "Rendering/SkeletalMeshModel.h"
#include "Rendering/SkeletalMeshLODModel.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/ARFilter.h"
#include "AssetNotifications.h"
#include "ImportUtils/SkeletalMeshImportUtils.h"
#include "ImportUtils/SkelImport.h"
#include "IMeshBuilderModule.h"
#include "EditorFramework/AssetImportData.h"
#include "MeshUtilities.h"
#include "MD5MeshImportOptions.h"
#include "SMD5MeshImportOptionsWindow.h"
#include "Interfaces/IMainFrameModule.h"

FVector3f aiVec3ToFVec3(aiVector3D& aiVec)
{
	return FVector3f{ aiVec.x, aiVec.y, aiVec.z };
}

UMD5MeshFactory::UMD5MeshFactory()
{
	bEditorImport = true;
	bText = false;// 虽然 MD5 是文本，但由于我们用 Assimp 统一处理，设为 false 即可
	SupportedClass = USkeletalMesh::StaticClass();
	Formats.Add(TEXT("md5mesh;Doom 3 Mesh"));
	// 设置优先级
	// 如果有多个工厂支持同一个后缀，值越大优先级越高。通常设为高一点防止被通用导入器拦截。
	ImportPriority = DefaultImportPriority + 10;
}

/*
 * 创建Asset对象辅助函数，源自官方FbxImporter
 */
UObject* UMD5MeshFactory::CreateAssetOfClass(UClass* AssetClass, FString ParentPackageName, FString ObjectName, bool bAllowReplace)
{
	// See if this sequence already exists.
	UObject* 	ParentPkg = CreatePackage( *ParentPackageName);
	FString 	ParentPath = FString::Printf(TEXT("%s/%s"), *FPackageName::GetLongPackagePath(*ParentPackageName), *ObjectName);
	UObject* 	Parent = CreatePackage( *ParentPath);
	// See if an object with this name exists
	UObject* Object = LoadObject<UObject>(Parent, *ObjectName, NULL, LOAD_NoWarn | LOAD_Quiet, NULL);

	// if object with same name but different class exists, warn user
	if ((Object != NULL) && (Object->GetClass() != AssetClass))
	{
		return NULL;
	}

	if (Object == NULL)
	{
		// add it to the set
		// do not add to the set, now create independent asset
		Object = NewObject<UObject>(Parent, AssetClass, *ObjectName, RF_Public | RF_Standalone);
		Object->MarkPackageDirty();
		// Notify the asset registry
		FAssetRegistryModule::AssetCreated(Object);
	}

	return Object;
}

bool UMD5MeshFactory::FactoryCanImport(const FString& Filename)
{
	const FString Extension = FPaths::GetExtension(Filename);
	if (Extension == TEXT("md5mesh"))
	{
		return true;
	}
	return false;
}

bool UMD5MeshFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
	UAssetImportData* SceneImportData = Cast<UAssetImportData>(Obj);
	if(SceneImportData)
	{
		FAssetImportInfo& ImportInfo = SceneImportData->SourceData;
		for (auto Element : ImportInfo.SourceFiles)
		{
			FString FileName = Element.RelativeFilename;
		}
	}
	return true;
}

void UMD5MeshFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
	UAssetImportData* SceneImportData = Cast<UAssetImportData>(Obj);
	if(SceneImportData)
	{
		FAssetImportInfo& ImportInfo = SceneImportData->SourceData;
		for (auto Element : ImportInfo.SourceFiles)
		{
			FString FileName = Element.RelativeFilename;
		}
	}
}

int32 UMD5MeshFactory::GetPriority() const
{
	return ImportPriority;
}

const aiScene* UMD5MeshFactory::LoadAssimpSceneFromFile(Assimp::Importer* Importer,const FString& Filename)
{
	const aiScene* Scene = Importer->ReadFile(TCHAR_TO_UTF8(*Filename),
		aiProcess_Triangulate
		|aiProcess_PopulateArmatureData
		|aiProcess_FlipUVs
		|aiProcess_GenSmoothNormals//生成平滑法线
		|aiProcess_GenSmoothNormals//修正反向法线
		);
	if (Scene == nullptr || !Scene->HasMeshes())
	{
		UE_LOG(LogTemp,Error,TEXT("Failed to read md5meshes %hs"),TCHAR_TO_UTF8(Importer->GetErrorString()));
	}
	return Scene;
}

EReimportResult::Type UMD5MeshFactory::Reimport( UObject* Obj )
{
	if (Obj->IsA(USkeletalMesh::StaticClass()))
	{
		USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(Obj);
		if (SkeletalMesh != nullptr && SkeletalMesh->GetAssetImportData() != nullptr)
		{
			UAssetImportData* ImportData = SkeletalMesh->GetAssetImportData();
			FString FileName = ImportData->SourceData.SourceFiles[0].RelativeFilename;
			Assimp::Importer* Importer = new Assimp::Importer();
			const aiScene* Scene = LoadAssimpSceneFromFile(Importer,TCHAR_TO_UTF8(*FileName));
			if(!Scene || !Scene->HasMeshes())
			{
				delete Importer;
				return EReimportResult::Failed;
			}
			else
			{
				UObject* Ret = CreateOrOverwriteMD5SkeletonMesh(SkeletalMesh,Scene,nullptr,nullptr,"",RF_NoFlags);
				delete Importer;
				if(Ret != nullptr)
				{
					return EReimportResult::Succeeded;
				}
			}
		}
	}
	return EReimportResult::Failed;
}

/*
 *导入.md5mesh文件 
 */
UObject* UMD5MeshFactory::FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled)
{
	Assimp::Importer* Importer = new Assimp::Importer();
	const aiScene* Scene = LoadAssimpSceneFromFile(Importer,TCHAR_TO_UTF8(*Filename));
	if(!Scene || !Scene->HasMeshes())
	{
		delete Importer;
		return nullptr;
	}
	UObject* ExistingObject = StaticFindObjectFast(UObject::StaticClass(), InParent, InName, false, RF_NoFlags, EInternalObjectFlags::Garbage);
	UObject* SkeletalMesh = nullptr;
	if(ExistingObject)
	{
		USkeletalMesh* ExistingSkelMesh = Cast<USkeletalMesh>(ExistingObject);
		SkeletalMesh = CreateOrOverwriteMD5SkeletonMesh(ExistingSkelMesh,Scene,InClass,InParent,InName,Flags);
	}
	else
	{
		SkeletalMesh = CreateOrOverwriteMD5SkeletonMesh(nullptr,Scene,InClass,InParent,InName,Flags);
	}
	delete Importer;
	return SkeletalMesh;
}

/*
 *
 * 
 */
UObject* UMD5MeshFactory::CreateOrOverwriteMD5SkeletonMesh(USkeletalMesh* ExistingSkelMesh, const aiScene* Scene, UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags)
{
	USkeletalMesh* SkeletalMesh = nullptr;
	// The import failed, we are marking the imported meshes for deletion in the next GC.
	auto FailureCleanup = [&]()
	{
		if (SkeletalMesh)
		{
			SkeletalMesh->ClearFlags(RF_Standalone);
			SkeletalMesh->Rename(NULL, GetTransientPackage(), REN_DontCreateRedirectors);
		}
	};
	if(!ExistingSkelMesh)
	{
		SkeletalMesh = NewObject<USkeletalMesh>(InParent,InClass,InName,Flags);
	}
	//选择需要导入的子网格
	UMD5MeshImportOptions* ImportOptions = NewObject<UMD5MeshImportOptions>(GetTransientPackage(), NAME_None, RF_Transactional);
	if(!GetMD5MeshImportOptions(ImportOptions,Scene))
	{
		FailureCleanup();
		return nullptr;
	}
	//导入中间数据结构
	Doom3Importer::FMD5MeshImportData ImportData;
	if(!FillImportDataFromAiScene(ImportData,ImportOptions,Scene))
	{
		FailureCleanup();
		return nullptr;
	}
	//
	int32 LodIndex = 0;
	// if(ExistingSkelMesh != nullptr)
	// {
	// 	FSkeletalMeshImportData::ReplaceSkeletalMeshGeometryImportData(ExistingSkelMesh, &ImportData, LodIndex);
	// 	FSkeletalMeshImportData::ReplaceSkeletalMeshRigImportData(ExistingSkelMesh, &ImportData, LodIndex);
	// }
	//
	FBox3f BoundingBox(ImportData.Points.GetData(), ImportData.Points.Num());
	const FVector3f BoundingBoxSize = BoundingBox.GetSize();
	//
	FSkeletalMeshBuildSettings BuildOptions;
	//Make sure the build option change in the re-import ui is reconduct
	BuildOptions.bUseFullPrecisionUVs = true;
	BuildOptions.bUseBackwardsCompatibleF16TruncUVs = false;
	BuildOptions.bUseHighPrecisionTangentBasis = false;
	BuildOptions.bRecomputeNormals = false;
	BuildOptions.bRecomputeTangents = false;
	BuildOptions.bRemoveDegenerates = false;
	BuildOptions.bUseMikkTSpace = false;
	TSharedPtr<FExistingSkelMeshData> ExistSkelMeshDataPtr;
	if (ExistingSkelMesh)
	{
		ExistingSkelMesh->PreEditChange(NULL);
		FSkeletalMeshLODInfo* LODInfoPtr = ExistingSkelMesh->GetLODInfo(LodIndex);
		if (LODInfoPtr)
		{
			// Full precision UV and High precision tangent cannot be change in the re-import options, it must not be change from the original data.
			BuildOptions.bUseFullPrecisionUVs = LODInfoPtr->BuildSettings.bUseFullPrecisionUVs;
			BuildOptions.bUseHighPrecisionTangentBasis = LODInfoPtr->BuildSettings.bUseHighPrecisionTangentBasis;
			BuildOptions.bUseBackwardsCompatibleF16TruncUVs = LODInfoPtr->BuildSettings.bUseBackwardsCompatibleF16TruncUVs;
			//Copy all the build option to reflect any change in the setting using the re-import UI
			LODInfoPtr->BuildSettings = BuildOptions;
		}
		//The backup of the skeletal mesh data empty the LOD array in the ImportedResource of the skeletal mesh
		//If the import fail after this step the editor can crash when updating the bone later since the LODModel will not exist anymore
		ExistSkelMeshDataPtr = SkeletalMeshImportUtils::SaveExistingSkelMeshData(ExistingSkelMesh, false, LodIndex);
	}
	if(SkeletalMesh == nullptr)
	{
		if (!ensure(ExistingSkelMesh != nullptr))
		{
			return nullptr;
		}
		SkeletalMesh = ExistingSkelMesh;
		//
		int32 PostEditChangeStackCounter = SkeletalMesh->GetPostEditChangeStackCounter();
		SkeletalMesh->SetPostEditChangeStackCounter(0);
		for (int32 LODIndex = 0, LODCount = SkeletalMesh->GetLODNum(); LODIndex < LODCount; ++LODIndex)
		{
			SkeletalMesh->ClearMeshDescriptionAndBulkData(LODIndex);
		}
		//
		FSkeletalMeshModel* ImportedResource = SkeletalMesh->GetImportedModel();
		ImportedResource->EmptyOriginalReductionSourceMeshData();
		ImportedResource->LODModels.Empty();
		SkeletalMesh->ResetLODInfo();
		SkeletalMesh->GetMaterials().Empty();
		SkeletalMesh->GetRefSkeleton().Empty();
		SkeletalMesh->SetSkeleton(nullptr);
		SkeletalMesh->SetPhysicsAsset(nullptr);
		SkeletalMesh->UnregisterAllMorphTarget();
		SkeletalMesh->ReleaseResources();
		SkeletalMesh->PostEditChange();
		SkeletalMesh->SetPostEditChangeStackCounter(PostEditChangeStackCounter);
	}
	//
	SkeletalMesh->PreEditChange(NULL);
	//Dirty the DDC Key for any imported Skeletal Mesh
	SkeletalMesh->InvalidateDeriveDataCacheGUID();
	FSkeletalMeshModel *ImportedResource = SkeletalMesh->GetImportedModel();
	ImportedResource->LODModels.Empty();
	ImportedResource->LODModels.Add(new FSkeletalMeshLODModel());
	FSkeletalMeshLODModel& LODModel = ImportedResource->LODModels[LodIndex];
	// process materials from import data
	SkeletalMeshImportUtils::ProcessImportMeshMaterials(SkeletalMesh->GetMaterials(), ImportData);
	// process reference skeleton from import data
	int32 SkeletalDepth = 0;
	USkeleton* ExistingSkeleton = nullptr;
	if(ExistSkelMeshDataPtr)
	{
		ExistingSkeleton = ExistSkelMeshDataPtr->ExistingSkeleton;
	}
	else
	{
		FString ObjectName = FString::Printf(TEXT("%s_Skeleton"), *SkeletalMesh->GetName());
		USkeleton* Skeleton = CreateAsset<USkeleton>(InParent->GetName(), ObjectName, true);
		if (!Skeleton)
		{
			// same object exists, try to see if it's skeleton, if so, load
			Skeleton = LoadObject<USkeleton>(InParent, *ObjectName);

			// if not skeleton, we're done, we can't create skeleton with same name
			// @todo in the future, we'll allow them to rename
			if (!Skeleton)
			{
				return SkeletalMesh;
			}
		}
		else
		{
			ExistingSkeleton = Skeleton;
		}
	}
	
	if (!SkeletalMeshImportUtils::ProcessImportMeshSkeleton(ExistingSkeleton, SkeletalMesh->GetRefSkeleton(), SkeletalDepth, ImportData))
	{
		UE_LOG(LogTemp,Error,TEXT("SkeletalMeshImportUtils.ProcessImportMeshSkeleton failed!"));
		FailureCleanup();
		return nullptr;
	}
	//
	// process bone influences from import data
	SkeletalMeshImportUtils::ProcessImportMeshInfluences(ImportData, SkeletalMesh->GetPathName());

	SkeletalMesh->ResetLODInfo();
	FSkeletalMeshLODInfo& NewLODInfo = SkeletalMesh->AddLODInfo();
	NewLODInfo.ReductionSettings.NumOfTrianglesPercentage = 1.0f;
	NewLODInfo.ReductionSettings.NumOfVertPercentage = 1.0f;
	NewLODInfo.ReductionSettings.MaxDeviationPercentage = 0.0f;
	NewLODInfo.LODHysteresis = 0.02f;

	//Store the original fbx import data the SkelMeshImportDataPtr should not be modified after this
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	SkeletalMesh->SaveLODImportedData(LodIndex, ImportData);
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

	SkeletalMesh->SetImportedBounds(FBoxSphereBounds((FBox)BoundingBox));
	SkeletalMesh->SetHasVertexColors(false);

	// Pass the number of texture coordinate sets to the LODModel.  Ensure there is at least one UV coord
	LODModel.NumTexCoords = FMath::Max<uint32>(1, ImportData.NumTexCoords);

	//创建RenderData
	if(!CreateRenderData(&ImportData,SkeletalMesh,ExistingSkelMesh,ExistSkelMeshDataPtr,BuildOptions,LodIndex))
	{
		UE_LOG(LogTemp,Error,TEXT("CreateRenderData failed!"));
		FailureCleanup();
		return nullptr;
	}

	if(ExistingSkeleton->MergeAllBonesToBoneTree( SkeletalMesh ))
	{
		ExistingSkeleton->UpdateReferencePoseFromMesh(SkeletalMesh);
		FAssetNotifications::SkeletonNeedsToBeSaved(ExistingSkeleton);
	}
	else
	{
		UE_LOG(LogTemp,Error,TEXT("USkeleton.MergeAllBonesToBoneTree failed!"));
	}
	
	if (SkeletalMesh->GetSkeleton() != ExistingSkeleton)
	{
		SkeletalMesh->SetSkeleton(ExistingSkeleton);
		SkeletalMesh->MarkPackageDirty();
	}
	return SkeletalMesh;
}

bool UMD5MeshFactory::GetMD5MeshImportOptions(UMD5MeshImportOptions* ImportOptions, const aiScene* Scene)
{
	int MeshCount = 0;
	for(unsigned int i = 0; i < Scene->mNumMeshes; ++i)
	{
		FString MeshName = Scene->mMeshes[i]->mName.C_Str();
		if(!MeshName.Equals("textures/common/shadow.msh"))//我们不需要用于渲染阴影的Mesh
		{
			FMD5MeshImportEntry Entry;
			Entry.Index = i;
			Entry.MeshName = MeshName;
			Entry.bShouldImport = true;
			ImportOptions->SubmesheList.Add(Entry);
			MeshCount++;
		}
	}
	TSharedPtr<SWindow> ParentWindow;
	if (FModuleManager::Get().IsModuleLoaded("MainFrame"))
	{
		IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");
		ParentWindow = MainFrame.GetParentWindow();
	}
	TSharedRef<SWindow> Window = SNew(SWindow)
		.ClientSize(FVector2D(450.f, 550.f))
		.Title(FText::FromString("MD5 Mesh Import Options"));
	
	TSharedPtr<SMD5MeshImportOptionsWindow> MD5MeshOptionsWindow;
	Window->SetContent
	(
		SAssignNew(MD5MeshOptionsWindow, SMD5MeshImportOptionsWindow)
		.ImportOptions(ImportOptions)
		.WidgetWindow(Window)
	);

	FSlateApplication::Get().AddModalWindow(Window, ParentWindow, false);

	if (!MD5MeshOptionsWindow->ShouldImport())
	{
		return false;
	}
	if(ImportOptions->SubmesheList.Num() <= 0)
	{
		return false;
	}
	return true;
}


/*
 * 填充FSkeletalMeshImportData数据结构
 */
bool UMD5MeshFactory::FillImportDataFromAiScene(Doom3Importer::FMD5MeshImportData& ImportData, UMD5MeshImportOptions* ImportOptions,const aiScene* Scene)
{
	ImportData.bHasNormals = true;
	ImportData.bHasTangents = true;
	ImportData.NumTexCoords = 1;//默认只包含一个纹理通道
	//找出需要导入的Mesh
	TArray<aiMesh*> ValidMeshes;
	for(int32 i = 0; i < ImportOptions->SubmesheList.Num(); i++)
	{
		if(ImportOptions->SubmesheList[i].bShouldImport)
		{
			int MeshIndex = ImportOptions->SubmesheList[i].Index;
			ValidMeshes.Add(Scene->mMeshes[MeshIndex]);
		}
	}
	int32 MeshCount = ValidMeshes.Num();
	if(MeshCount <= 0)
	{
		return false;
	}
	//
	float FinalScale = ImportOptions->ImportScale;
	//收集骨骼
	TArray<FString> BonesNames;
	TArray<FBoneImportData> Bones;
	ExtractBones(Scene->mRootNode,BonesNames,Bones,ImportOptions);
	//保存骨骼数据并判断父骨骼的索引
	for(int32 b = 0; b < Bones.Num(); ++b)
	{
		SkeletalMeshImportData::FBone UnrealBone;
		UnrealBone.Name = Bones[b].Name;
		UnrealBone.NumChildren = Bones[b].NumChildren;
		UnrealBone.BonePos = Bones[b].BonePos;
		UnrealBone.ParentIndex = INDEX_NONE;
		if(!Bones[b].ParentName.IsEmpty())
		{
			UnrealBone.ParentIndex = BonesNames.IndexOfByKey(Bones[b].ParentName);
		}
		else
		{
			UnrealBone.ParentIndex = INDEX_NONE;
		}
		ImportData.RefBonesBinary.Add(UnrealBone);
	}
	//读取Mesh数据
	ImportData.MeshInfos.Reserve(MeshCount);
	ImportData.Sections.Reserve(MeshCount);
	ImportData.Sections.SetNum(MeshCount);
	for(int32 m = 0; m < MeshCount; ++m)
	{
		aiMesh* Mesh = ValidMeshes[m];
		uint32 VertexOffset = ImportData.Points.Num();
		//
		SkeletalMeshImportData::FMeshInfo MeshInfo;
		MeshInfo.Name = Mesh->mName.C_Str();
		MeshInfo.NumVertices = Mesh->mNumVertices;
		MeshInfo.StartImportedVertex = VertexOffset;
		ImportData.MeshInfos.Add(MeshInfo);
		//
		Doom3Importer::FMD5MeshSectionInfo* SectionInfo = &ImportData.Sections[m];
		SectionInfo->NumVertices = Mesh->mNumVertices;
		SectionInfo->StartVertexIndex = VertexOffset;
		//顶点数据
		for(unsigned int v = 0;v < Mesh->mNumVertices;++v)
		{
			aiVector3d Vertex = Mesh->mVertices[v];
			ImportData.Points.Add(FVector3f(Vertex.x,Vertex.y,Vertex.z) * FVector3f(FinalScale, FinalScale,FinalScale));
			//
			SkeletalMeshImportData::FVertex VertexData;
			VertexData.VertexIndex = VertexOffset + v;
			VertexData.MatIndex = m;
			VertexData.Color = FColor::White;
			if(Mesh->HasTextureCoords(0))
			{
				VertexData.UVs[0] = FVector2f(Mesh->mTextureCoords[0][v].x, Mesh->mTextureCoords[0][v].y);
			}
			ImportData.Wedges.Add(VertexData);
		}
		//三角面数据
		SectionInfo->NumTriangles = Mesh->mNumFaces;
		SectionInfo->StartIndex = ImportData.Faces.Num();
		for(unsigned int f = 0;f < Mesh->mNumFaces;++f)
		{
			aiFace Face = Mesh->mFaces[f];
			SkeletalMeshImportData::FTriangle FaceData;
			FaceData.MatIndex = m;
			for(int32 index = 0; index < 3; index++)
			{
				//注意要反转顶点顺序
				uint32 VertexIndex = Face.mIndices[2 - index];
				FaceData.WedgeIndex[index] = VertexIndex + VertexOffset;
				if(Mesh->HasNormals())
				{
					FaceData.TangentZ[index] = aiVec3ToFVec3(Mesh->mNormals[VertexIndex]);
				}
				if(Mesh->HasTangentsAndBitangents())
				{
					FaceData.TangentX[index] = aiVec3ToFVec3(Mesh->mTangents[VertexIndex]);
					FaceData.TangentY[index] = aiVec3ToFVec3(Mesh->mBitangents[VertexIndex]);
				}
				else
				{
					FaceData.TangentX[index] = FVector3f(0.f, 0.f, 1.f);
					FaceData.TangentY[index] = FVector3f(0.f, 0.f, 1.f);
				}
			}
			ImportData.Faces.Add(FaceData);
		}
		//处理权重 (Influences)
		SectionInfo->BoneIndices.Reset();
		for (unsigned int b = 0; b < Mesh->mNumBones; ++b)
		{
			aiBone* Bone = Mesh->mBones[b];
			int32 BoneIndex = BonesNames.Find(Bone->mName.C_Str());
			for (unsigned int w = 0; w < Bone->mNumWeights; ++w)
			{
				SkeletalMeshImportData::FRawBoneInfluence Infl;
				Infl.BoneIndex = BoneIndex;
				Infl.Weight = Bone->mWeights[w].mWeight;
				Infl.VertexIndex = Bone->mWeights[w].mVertexId + VertexOffset;
				ImportData.Influences.Add(Infl);
			}
			//
			SectionInfo->BoneIndices.Add(BoneIndex);
		}
		//默认材质
		SkeletalMeshImportData::FMaterial NewMat;
		NewMat.MaterialImportName = FString::Printf(TEXT("M_MD5_Default%d"),m);
		ImportData.Materials.Add(NewMat);
	}
	return true;
}


void UMD5MeshFactory::ExtractBones(const aiNode* node,TArray<FString>& BonesNames,TArray<FBoneImportData>& BonesData,UMD5MeshImportOptions* ImportOptions)
{
	FString NodeName = node->mName.C_Str();
	if(!NodeName.StartsWith("<MD5_") && !BonesNames.Contains(NodeName))
	{
		int32 BoneIndex = BonesNames.Num();
		BonesNames.Add(NodeName);
		FBoneImportData BoneData;
		BoneData.Name = NodeName;
		if(node->mParent != nullptr)
		{
			BoneData.ParentName = node->mParent->mName.C_Str();
		}
		aiVector3D Pos,Scale;
		aiQuaternion Rotation;
		node->mTransformation.Decompose(Scale, Rotation, Pos);
		float FinalScale = ImportOptions->ImportScale;
		BoneData.BonePos.Transform.SetLocation(FVector3f(Pos.x, Pos.y, Pos.z) * FVector3f(FinalScale,FinalScale,FinalScale));
		BoneData.BonePos.Transform.SetRotation(FQuat4f(Rotation.x, Rotation.y, Rotation.z, Rotation.w));
		BonesData.Add(BoneData);
	}
	if(node->mNumChildren > 0)
	{
		for(uint32 c = 0; c < node->mNumChildren; ++c)
		{
			ExtractBones(node->mChildren[c],BonesNames,BonesData,ImportOptions);
		}		
	}
}

bool UMD5MeshFactory::CreateRenderData(
	Doom3Importer::FMD5MeshImportData* SkelMeshImportDataPtr,
	USkeletalMesh* SkeletalMesh,
	USkeletalMesh* ExistingSkelMesh,
	TSharedPtr<FExistingSkelMeshData> ExistSkelMeshDataPtr,
	FSkeletalMeshBuildSettings& BuildOptions,
	int ImportLODModelIndex)
{
	TArray<FVector3f> LODPoints;
	TArray<SkeletalMeshImportData::FMeshWedge> LODWedges;
	TArray<SkeletalMeshImportData::FMeshFace> LODFaces;
	TArray<SkeletalMeshImportData::FVertInfluence> LODInfluences;
	TArray<int32> LODPointToRawMap;
	SkelMeshImportDataPtr->CopyLODImportData(LODPoints,LODWedges,LODFaces,LODInfluences,LODPointToRawMap);
	FSkeletalMeshModel *ImportedResource = SkeletalMesh->GetImportedModel();
	FSkeletalMeshLODModel& LODModel = ImportedResource->LODModels[ImportLODModelIndex];

	//The imported LOD is always 0 here, the LOD custom import will import the LOD alone(in a temporary skeletalmesh) and add it to the base skeletal mesh later
	check(SkeletalMesh->GetLODInfo(ImportLODModelIndex) != nullptr);
	//Set the build options
	SkeletalMesh->GetLODInfo(ImportLODModelIndex)->BuildSettings = BuildOptions;
	//New MeshDescription build process
	IMeshBuilderModule& MeshBuilderModule = IMeshBuilderModule::GetForRunningPlatform();
	//We must build the LODModel so we can restore properly the mesh, but we do not have to regenerate LODs
	const bool bRegenDepLODs = false;
	FSkeletalMeshBuildParameters SkeletalMeshBuildParameters(SkeletalMesh, GetTargetPlatformManagerRef().GetRunningTargetPlatform(), ImportLODModelIndex, bRegenDepLODs);
	// JIRA UE-250536: Change of API to IMeshBuilderModule::BuildSkeletalMesh now requires a rendering resource
	// Solution: Allocate an uninitialized rendering resource then release it after the call to IMeshBuilderModule::BuildSkeletalMesh
	//			 Although the legacy importer does not support Nanite for skeletal mesh, the call succeeds even if Nanite is manually set to true
	//			 Hopefully, this solution will last until the legacy FBX importer is deprecated.
	SkeletalMesh->AllocateResourceForRendering();
	bool bBuildSuccess = MeshBuilderModule.BuildSkeletalMesh(*SkeletalMesh->GetResourceForRendering(), SkeletalMeshBuildParameters);
	SkeletalMesh->ReleaseResources();

	if( !bBuildSuccess )
	{
		SkeletalMesh->MarkAsGarbage();
		return false;
	}

	if (ExistingSkelMesh)
	{
		//In case of a re-import we update only the type we re-import
		SkeletalMesh->GetAssetImportData()->AddFileName(UFactory::GetCurrentFilename(), 1);
	}
	else
	{
		//When we create a new skeletal mesh asset we create 1 or 3 source files. 1 in case we import all the content, 3 in case we import geo or skinning
		int32 SourceFileIndex = 1;
		//Always add the base sourcefile
		SkeletalMesh->GetAssetImportData()->AddFileName(UFactory::GetCurrentFilename(), 0, NSSkeletalMeshSourceFileLabels::GeoAndSkinningText().ToString());
		if (SourceFileIndex != 0)
		{
			//If the user import geo or skinning, we add both entries to allow reimport of them
			SkeletalMesh->GetAssetImportData()->AddFileName(UFactory::GetCurrentFilename(), 1, NSSkeletalMeshSourceFileLabels::GeometryText().ToString());
			SkeletalMesh->GetAssetImportData()->AddFileName(UFactory::GetCurrentFilename(), 2, NSSkeletalMeshSourceFileLabels::SkinningText().ToString());
		}
	}

	if (ExistSkelMeshDataPtr)
	{
		SkeletalMeshImportUtils::RestoreExistingSkelMeshData(ExistSkelMeshDataPtr, SkeletalMesh, ImportLODModelIndex, true, false, false);
	}

	SkeletalMesh->CalculateInvRefMatrices();

	//We need to have a valid render data to create physic asset
	SkeletalMesh->Build();

	return SkeletalMesh->MarkPackageDirty();
}
