#pragma once
#include "Factories/Factory.h"
#include "Rendering/SkeletalMeshLODImporterData.h"
#include "EditorReimportHandler.h"
#include "MD5MeshFactory.generated.h"

struct aiNode;
struct aiScene;
class USkeletalMesh;
struct FExistingSkelMeshData;
class UMD5MeshImportOptions;

namespace Assimp
{
	class Importer;
}

struct FBoneImportData
{
	FString		Name;
	FString		ParentName;
	int32 		NumChildren;
	SkeletalMeshImportData::FJointPos	BonePos;
};

namespace Doom3Importer
{
	struct FMD5MeshSectionInfo
	{
		uint32 NumVertices = 0;
		uint32 StartVertexIndex = 0;
		uint32 StartIndex = 0;
		uint32 NumTriangles;
		uint32 MaterialIndex = 0;
		TArray<uint16> BoneIndices;
	};
	
	class FMD5MeshImportData : public FSkeletalMeshImportData
	{
	public:
		FMD5MeshImportData() : FSkeletalMeshImportData()
		{
			
		}

		TArray<FMD5MeshSectionInfo> Sections;
	};

}

UCLASS()
class UMD5MeshFactory : public UFactory,public FReimportHandler
{
	GENERATED_BODY()
public:
	UMD5MeshFactory();

	virtual bool FactoryCanImport(const FString& Filename) override;
	virtual UObject* FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled) override;
	
	//~ Begin FReimportHandler Interface
	virtual bool CanReimport( UObject* Obj, TArray<FString>& OutFilenames ) override;
	virtual void SetReimportPaths( UObject* Obj, const TArray<FString>& NewReimportPaths ) override;
	virtual EReimportResult::Type Reimport( UObject* Obj ) override;
	virtual int32 GetPriority() const override;
	//~ End FReimportHandler Interface
	
protected:
	static  UObject* CreateAssetOfClass(UClass* AssetClass, FString ParentPackageName, FString ObjectName, bool bAllowReplace);
	
	template< class T>
	static T* CreateAsset(FString ParentPackageName, FString ObjectName, bool bAllowReplace = false)
	{
		return (T*)CreateAssetOfClass(T::StaticClass(), ParentPackageName, ObjectName, bAllowReplace);
	}

	UObject* CreateOrOverwriteMD5SkeletonMesh(USkeletalMesh* ExistingSkelMesh,const aiScene* Scene,UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags);
	bool GetMD5MeshImportOptions(UMD5MeshImportOptions* ImportOptions,const aiScene* Scene);
	bool FillImportDataFromAiScene(Doom3Importer::FMD5MeshImportData& ImportData,UMD5MeshImportOptions* ImportOptions,const aiScene* Scene);
	aiNode* FindActualRootBone(aiNode* Node);
	FQuat4f ComputeRootNodeRotationFix(aiNode* Node);
	void ExtractBones(const aiNode* node,TArray<FString>& BonesNames,TArray<FBoneImportData>& BonesData,UMD5MeshImportOptions* ImportOptions);
	bool CreateRenderData(Doom3Importer::FMD5MeshImportData* SkelMeshImportDataPtr,
		USkeletalMesh* SkeletalMesh,
		USkeletalMesh* ExistingSkelMesh,
		TSharedPtr<FExistingSkelMeshData> ExistSkelMeshDataPtr,
		FSkeletalMeshBuildSettings& BuildOptions,
		int ImportLODModelIndex);

protected:
	FQuat4f BoneRootRotationFix;
};
