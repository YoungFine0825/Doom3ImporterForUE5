#include "Doom3ImporterFactory.h"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

//UE5
#include "Engine/SkeletalMesh.h"
#include "Rendering/SkeletalMeshLODModel.h"
#include "Rendering/SkeletalMeshLODImporterData.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "ImportUtils/SkeletalMeshImportUtils.h"

struct BoneImportData
{
	FString		Name;
	FString		ParentName;
	int32 		NumChildren;
	SkeletalMeshImportData::FJointPos	BonePos;
};

FVector3f aiVec3ToFVec3(aiVector3D& aiVec)
{
	return FVector3f{ aiVec.x, aiVec.y, aiVec.z };
}

UDoom3ImporterFactory::UDoom3ImporterFactory()
{
	bEditorImport = true;
	bText = false;// 虽然 MD5 是文本，但由于我们用 Assimp 统一处理，设为 false 即可
	SupportedClass = USkeletalMesh::StaticClass();
	Formats.Add(TEXT("md5mesh;Doom 3 Mesh"));
	// 设置优先级
	// 如果有多个工厂支持同一个后缀，值越大优先级越高。通常设为高一点防止被通用导入器拦截。
	ImportPriority = DefaultImportPriority + 10;
}

UObject* UDoom3ImporterFactory::FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled)
{
	Assimp::Importer* Importer = new Assimp::Importer();
	const aiScene* Scene = Importer->ReadFile(TCHAR_TO_UTF8(*Filename),
		aiProcess_Triangulate
		|aiProcess_PopulateArmatureData
		//|aiProcess_FlipUVs
		|aiProcess_GenSmoothNormals//生成平滑法线
		|aiProcess_GenSmoothNormals//修正反向法线
		);

	if(!Scene || !Scene->HasMeshes())
	{
		UE_LOG(LogTemp,Error,TEXT("Failed to read md5meshes %s"),UTF8_TO_TCHAR(Importer->GetErrorString()));
		return nullptr;
	}
	//创建SkeletalMesh实例
	USkeletalMesh* UEMesh = NewObject<USkeletalMesh>(InParent,InClass,InName,Flags);
	//导入中间数据结构
	FSkeletalMeshImportData ImportData;
	ImportData.bHasNormals = true;
	ImportData.bHasTangents = true;
	//过滤无用的Mesh
	TArray<aiMesh*> ValidMeshes;
	for(unsigned int i = 0; i < Scene->mNumMeshes; ++i)
	{
		FString MeshName = Scene->mMeshes[i]->mName.C_Str();
		if(!MeshName.Equals("textures/common/shadow.msh"))//我们不需要用于渲染阴影的Mesh
		{
			ValidMeshes.Add(Scene->mMeshes[i]);
		}
	}
	int32 MeshCount = ValidMeshes.Num();
	//收集骨骼
	TArray<FString> BonesNames;
	TArray<BoneImportData> Bones;
	for(int32 i = 0; i < MeshCount; ++i)
	{
		aiMesh* Mesh = ValidMeshes[i];
		for(unsigned int b = 0; b < Mesh->mNumBones; ++b)
		{
			aiBone* Bone = Mesh->mBones[b];
			if(b == 0 && Bone->mNode != nullptr)
			{
				TArray<aiNode*> ParentNodes;
				aiNode* Parent = Bone->mNode->mParent;
				while(Parent != nullptr)
				{
					ParentNodes.Add(Parent);
					Parent = Parent->mParent;
				}
				for(int32 p = ParentNodes.Num() - 1; p >= 0; --p)
				{
					Parent = ParentNodes[p];
					FString ParentBoneName = Parent->mName.C_Str();
					if(!BonesNames.Contains(ParentBoneName))
					{
						BonesNames.Add(ParentBoneName);
						int32 BoneIndex = BonesNames.Num() - 1;
						UE_LOG(LogTemp,Log,TEXT("Insert Bone = %s Index = %d"),*ParentBoneName,BoneIndex);
						BoneImportData BoneData;
						BoneData.Name = ParentBoneName;
						if(Parent->mParent != nullptr)
						{
							BoneData.ParentName = Parent->mParent->mName.C_Str();
						}
						aiVector3D Pos,Scale;
						aiQuaternion Rotation;
						Parent->mTransformation.Decompose(Scale, Rotation, Pos);
						BoneData.BonePos.Transform.SetLocation(FVector3f(Pos.x, Pos.y, Pos.z));
						BoneData.BonePos.Transform.SetRotation(FQuat4f(Rotation.x, Rotation.y, Rotation.z, Rotation.w));
						Bones.Add(BoneData);
					}
				}
			}
			FString BoneName = Bone->mName.C_Str();
			if(!BonesNames.Contains(BoneName))
			{
				BonesNames.Add(BoneName);
				int32 BoneIndex = BonesNames.Num() - 1;
				//
				UE_LOG(LogTemp,Log,TEXT("Insert Bone = %s Index = %d"),*BoneName,BoneIndex);
				//
				BoneImportData BoneData;
				BoneData.Name = BoneName;
				//
				if(Bone->mNode != nullptr && Bone->mNode->mParent != nullptr)
				{
					BoneData.ParentName = Bone->mNode->mParent->mName.C_Str();
					BoneData.NumChildren = Bone->mNode->mNumChildren;
				}
				//
				aiVector3D Pos,Scale;
				aiQuaternion Rotation;
				Bone->mOffsetMatrix.Decompose(Scale, Rotation, Pos);
				BoneData.BonePos.Transform.SetLocation(FVector3f(Pos.x, Pos.y, Pos.z));
				BoneData.BonePos.Transform.SetRotation(FQuat4f(Rotation.x, Rotation.y, Rotation.z, Rotation.w));
				Bones.Add(BoneData);
			}
		}
	}
	//保存骨骼数据并判断父骨骼的索引
	for(int32 b = 0; b < Bones.Num(); ++b)
	{
		SkeletalMeshImportData::FBone Bone;
		Bone.Name = Bones[b].Name;
		Bone.NumChildren = Bones[b].NumChildren;
		Bone.BonePos = Bones[b].BonePos;
		Bone.ParentIndex = INDEX_NONE;
		if(!Bones[b].ParentName.IsEmpty())
		{
			Bone.ParentIndex = BonesNames.IndexOfByKey(Bones[b].ParentName);
		}
		else
		{
			Bone.ParentIndex = INDEX_NONE;
		}
		ImportData.RefBonesBinary.Add(Bone);
	}
	//读取Mesh数据
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
		//顶点数据
		for(unsigned int v = 0;v < Mesh->mNumVertices;++v)
		{
			aiVector3d Vertex = Mesh->mVertices[v];
			ImportData.Points.Add(FVector3f(Vertex.x,Vertex.y,Vertex.z));
			//
			SkeletalMeshImportData::FVertex VertexData;
			VertexData.VertexIndex = VertexOffset + v;
			if(Mesh->HasTextureCoords(0))
			{
				VertexData.UVs[0] = FVector2f(Mesh->mTextureCoords[0][v].x, Mesh->mTextureCoords[0][v].y);
			}
			ImportData.Wedges.Add(VertexData);
		}
		//三角面数据
		for(unsigned int f = 0;f < Mesh->mNumFaces;++f)
		{
			aiFace Face = Mesh->mFaces[f];
			SkeletalMeshImportData::FTriangle FaceData;
			for(int32 index = 0; index < 3; index++)
			{
				uint32 VertexIndex = Face.mIndices[index] + VertexOffset;
				FaceData.WedgeIndex[index] = VertexIndex;
				if(Mesh->HasNormals())
				{
					FaceData.TangentX[index] = aiVec3ToFVec3(Mesh->mNormals[VertexIndex]);
				}
				if(Mesh->HasTangentsAndBitangents())
				{
					FaceData.TangentY[index] = aiVec3ToFVec3(Mesh->mTangents[VertexIndex]);
					FaceData.TangentZ[index] = aiVec3ToFVec3(Mesh->mBitangents[VertexIndex]);
				}
			}
			ImportData.Faces.Add(FaceData);
		}
		//处理权重 (Influences) ---
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
		}
	}
	//
	SkeletalMeshImportData::FMaterial NewMat;
	NewMat.MaterialImportName = TEXT("M_MD5_Default");
	ImportData.Materials.Add(NewMat);
	//
	// 创建骨架资产并关联
	USkeleton* SkeletonAsset = NewObject<USkeleton>(InParent, *FString(InName.ToString() + "_Skeleton"), Flags);
	SkeletonAsset->MergeAllBonesToBoneTree(UEMesh);
	UEMesh->SetSkeleton(SkeletonAsset);
	//
	int32 SkeletonDepth = 0;
	if(SkeletalMeshImportUtils::ProcessImportMeshSkeleton(SkeletonAsset,UEMesh->GetRefSkeleton(),SkeletonDepth,ImportData))
	{
		UEMesh->PostEditChange();
		SkeletonAsset->PostEditChange();
        
		FAssetRegistryModule::AssetCreated(UEMesh);
		FAssetRegistryModule::AssetCreated(SkeletonAsset);
	}
	else
	{
		UE_LOG(LogTemp,Error,TEXT("SkeletalMeshImportUtils.ProcessImportMeshSkeleton failed!"));
	}
	return UEMesh;
}
