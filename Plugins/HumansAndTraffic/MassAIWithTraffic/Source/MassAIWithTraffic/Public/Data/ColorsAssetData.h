// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NiagaraCrowdData.h"
#include "ColorsAssetData.generated.h"

UCLASS(BlueprintType, Blueprintable)
class MASSAIWITHTRAFFIC_API UColorsAssetData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<TSoftObjectPtr<USkeletalMesh>, FColorsSet> SavedColors;
};
