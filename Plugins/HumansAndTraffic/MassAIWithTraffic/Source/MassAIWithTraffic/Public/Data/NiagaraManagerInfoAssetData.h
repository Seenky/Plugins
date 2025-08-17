// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NiagaraCrowdData.h"
#include "NiagaraManagerInfoAssetData.generated.h"

UCLASS()
class MASSAIWITHTRAFFIC_API UNiagaraManagerInfoAssetData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FSoftObjectPath NiagaraAssetPath;
};
