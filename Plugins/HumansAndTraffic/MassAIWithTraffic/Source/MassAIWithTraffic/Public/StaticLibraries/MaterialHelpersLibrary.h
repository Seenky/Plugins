// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Managers/NiagaraManager.h"

#include "MaterialHelpersLibrary.generated.h"

UCLASS()
class MASSAIWITHTRAFFIC_API UMaterialHelpersLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Material Tools")
	static TArray<UMaterialInterface*>  GetSkeletalMeshMaterialsFromLOD(const int32 LODIndex, const USkeletalMesh* SkeletalMesh);
};
