#pragma once
#include "CoreMinimal.h"
#include "MD5MeshImportOptions.generated.h"

USTRUCT(BlueprintType)
struct FMD5MeshImportEntry
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	FString MeshName;

	UPROPERTY(EditAnywhere)
	bool bShouldImport = true;

	UPROPERTY()
	int Index = 0;
};

UCLASS(BlueprintType, config = EditorPerProjectUserSettings, HideCategories=Object, MinimalAPI)
class UMD5MeshImportOptions : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, category = Meshes)
	TArray<FMD5MeshImportEntry> SubmesheList;

	//虚幻引擎”小白人“身高180厘米，Doom3角色身高约为70~80个单位（Id Units).
	//按照2.5倍放大差不多合适。
	UPROPERTY(EditAnywhere)
	float ImportScale = 2.5f;
};