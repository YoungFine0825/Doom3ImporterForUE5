#include "MD5ImportUtil.h"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

namespace MD5ImportUtil
{
	const aiScene* LoadAssimpSceneFromFile(Assimp::Importer* Importer,const FString& Filename)
	{
		const aiScene* Scene = Importer->ReadFile(TCHAR_TO_UTF8(*Filename),
			aiProcess_Triangulate
			|aiProcess_PopulateArmatureData
			|aiProcess_FlipUVs
			|aiProcess_GenSmoothNormals//生成平滑法线
			|aiProcess_GenSmoothNormals//修正反向法线
			//|aiProcess_MakeLeftHanded
			);
		if (Scene == nullptr || !Scene->HasMeshes())
		{
			UE_LOG(LogTemp,Error,TEXT("Failed to read md5meshes %hs"),TCHAR_TO_UTF8(Importer->GetErrorString()));
		}
		return Scene;
	}
	
}
