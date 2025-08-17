// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Data/NiagaraManagerInfoAssetData.h"
#include "NiagaraCrowdSubsystemSettings.generated.h"


extern int32 GDebugMassCrowdForwardVisibility;
extern int32 GDebugMassCrowdLocationFix;

UCLASS(Config=Game, defaultconfig, meta = (DisplayName = "Niagara Crowd Subsystem Settings"))
class MASSAIWITHTRAFFIC_API UNiagaraCrowdSubsystemSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return TEXT("CustomSettings"); }
	virtual FName GetSectionName() const override { return TEXT("NiagaraCrowdSubsystem"); }

	//Enable or disable niagara manager to use optimized particles for crowd
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Config, Category="NiagaraCrowdSubsystemSettings|Main", meta=(AllowPrivateAccess))
	bool bUseNiagaraParticlesForCrowd = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Config, Category="NiagaraCrowdSubsystemSettings|Main", meta=(AllowPrivateAccess, EditCondition="bUseNiagaraParticlesForCrowd"))
	FSoftObjectPath ManagerSettings;
	//Use particles without swap to characters on distance
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Config, Category="NiagaraCrowdSubsystemSettings|Main", meta=(AllowPrivateAccess, EditCondition="bUseNiagaraParticlesForCrowd"))
	bool bUseOnlyNiagaraParticles = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Config, Category="NiagaraCrowdSubsystemSettings|Main", meta=(AllowPrivateAccess, EditCondition="bUseNiagaraParticlesForCrowd"))
	bool bHideForwardPeople = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Config, Category="NiagaraCrowdSubsystemSettings|Main", meta=(AllowPrivateAccess, EditCondition="bUseNiagaraParticlesForCrowd"))
	float DistanceToOptimizeTraces = 10000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Config, Category="NiagaraCrowdSubsystemSettings|Main", meta=(AllowPrivateAccess, EditCondition="bUseNiagaraParticlesForCrowd"))
	bool bEnableLocationFix = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Config, Category="NiagaraCrowdSubsystemSettings|Main", meta=(AllowPrivateAccess, EditCondition="bUseNiagaraParticlesForCrowd && bEnableLocationFix"))
	TEnumAsByte<ECollisionChannel> LocationFixChannel = ECC_Visibility;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Config, Category="NiagaraCrowdSubsystemSettings|Main", meta=(AllowPrivateAccess, EditCondition="bUseNiagaraParticlesForCrowd && bEnableLocationFix"))
	float LocationFixUpDistance = 90.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Config, Category="NiagaraCrowdSubsystemSettings|Main", meta=(AllowPrivateAccess, EditCondition="bUseNiagaraParticlesForCrowd && bEnableLocationFix"))
	float LocationFixInterpSpeed = 4.0f;
	//Distance to swap niagara particle to character
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Config, Category="NiagaraCrowdSubsystemSettings|Main", meta=(AllowPrivateAccess, EditCondition="bUseNiagaraParticlesForCrowd"))
	float DistanceToSwapNiagaraParticle = 5000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Config, Category="NiagaraCrowdSubsystemSettings|Main", meta=(AllowPrivateAccess, EditCondition="bUseNiagaraParticlesForCrowd"))
	float HighLODVisibilityDistance = 2000.0f;
	//Lerp speed to linear change opacity while swap meshes
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Config, Category="NiagaraCrowdSubsystemSettings|Main", meta=(AllowPrivateAccess, EditCondition="bUseNiagaraParticlesForCrowd"))
	float LerpSpeed = 10.f;
	//Number of mesh renderers in niagara
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Config, Category="NiagaraCrowdSubsystemSettings|Main", meta=(ClampMin = 1, AllowPrivateAccess, EditCondition="bUseOnlyNiagaraParticles && bUseNiagaraParticlesForCrowd"))
	int32 NiagaraMeshesCount = 1;
	//Number of frames to update pawns visibility
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Config, Category="NiagaraCrowdSubsystemSettings|Main", meta=(AllowPrivateAccess, EditCondition="!bUseOnlyNiagaraParticles && bUseNiagaraParticlesForCrowd"))
	int32 VisibilityFramesCheck = 30;
	//Tag to ignore pawn visibility trace check (for objects like invisible collisions)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Config, Category="NiagaraCrowdSubsystemSettings|Main", meta=(AllowPrivateAccess, EditCondition="bUseNiagaraParticlesForCrowd"))
	FName TagToIgnoreVisibilityCheck = "IgnoreTrace";

	//Niagara parameter names
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Config, Category="NiagaraCrowdSubsystemSettings|Parameter names", meta=(AllowPrivateAccess, EditCondition="bUseNiagaraParticlesForCrowd"))
	FName NiagaraMeshIndexesParameterName = "User.MeshIndexes";
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Config, Category="NiagaraCrowdSubsystemSettings|Parameter names", meta=(AllowPrivateAccess, EditCondition="bUseNiagaraParticlesForCrowd"))
	FName NiagaraLocationArrayParameterName = "User.LocationArray";
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Config, Category="NiagaraCrowdSubsystemSettings|Parameter names", meta=(AllowPrivateAccess, EditCondition="bUseNiagaraParticlesForCrowd"))
	FName NiagaraForwardVectorArrayParameterName = "User.ForwardVectorsArray";
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Config, Category="NiagaraCrowdSubsystemSettings|Parameter names", meta=(AllowPrivateAccess, EditCondition="bUseNiagaraParticlesForCrowd"))
    FName NiagaraVelocityArrayParameterName = "User.VelocityArray";
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Config, Category="NiagaraCrowdSubsystemSettings|Parameter names", meta=(AllowPrivateAccess, EditCondition="bUseNiagaraParticlesForCrowd"))
	FName NiagaraVisibilityArrayParameterName = "User.VisibilityArray";
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Config, Category="NiagaraCrowdSubsystemSettings|Parameter names", meta=(AllowPrivateAccess, EditCondition="bUseNiagaraParticlesForCrowd"))
	FName NiagaraColorsArrayParameterName = "User.ColorArray";
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Config, Category="NiagaraCrowdSubsystemSettings|Parameter names", meta=(AllowPrivateAccess, EditCondition="bUseNiagaraParticlesForCrowd"))
	FName NiagaraOffsetArrayParameterName = "User.OffsetArray";
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Config, Category="NiagaraCrowdSubsystemSettings|Parameter names", meta=(AllowPrivateAccess, EditCondition="bUseNiagaraParticlesForCrowd"))
	FName LerpSpeedParameterName = "LerpSpeed";

	//Debug settings
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Config, Category="NiagaraCrowdSubsystemSettings|Debug", meta=(AllowPrivateAccess, EditCondition="bUseNiagaraParticlesForCrowd"))
	bool bEnableDebug = false;

	UNiagaraManagerInfoAssetData* GetManagerSettings() const
	{
		if (!ManagerSettings.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("ManagerSettingsPath is invalid"));
			return nullptr;
		}

		return Cast<UNiagaraManagerInfoAssetData>(ManagerSettings.TryLoad());
	}

protected:
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override
	{
		Super::PostEditChangeProperty(PropertyChangedEvent);

		const FName PropertyName = PropertyChangedEvent.GetMemberPropertyName();
	}
#endif
};
