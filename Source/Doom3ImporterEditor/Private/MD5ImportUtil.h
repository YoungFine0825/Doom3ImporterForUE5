#pragma once
#include "CoreMinimal.h"
#include "assimp/quaternion.h"
#include "assimp/vector3.h"

struct aiScene;
namespace Assimp { class Importer; }

namespace MD5ImportUtil
{
	FORCEINLINE FVector3f aiVec3ToFVec3(aiVector3D& aiVec)
	{
		return FVector3f{ aiVec.z, aiVec.x, aiVec.y };
	}

	FORCEINLINE FQuat4f aiQuatToFQuat(aiQuaternion quat)
	{
		return FQuat4f(quat.z, quat.x, quat.y, quat.w);
	}

	const aiScene* LoadAssimpSceneFromFile(Assimp::Importer* Importer,const FString& Filename);
}