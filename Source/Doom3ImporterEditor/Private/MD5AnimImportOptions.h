#pragma once
#include "CoreMinimal.h"
#include "Animation/Skeleton.h"
#include "MD5AnimImportOptions.generated.h"

UCLASS(BlueprintType, config = EditorPerProjectUserSettings, HideCategories=Object, MinimalAPI)
class UMD5AnimImportOptions : public UObject
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, category = Skeleton)
	TObjectPtr<USkeleton> Skeleton;
};
