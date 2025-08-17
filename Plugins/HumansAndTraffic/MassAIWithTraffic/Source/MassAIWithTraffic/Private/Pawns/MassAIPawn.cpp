// Fill out your copyright notice in the Description page of Project Settings.


#include "Pawns/MassAIPawn.h"

#include "Subsystem/NiagaraCrowdSubsystem.h"
#include "Subsystem/NiagaraCrowdSubsystemSettings.h"
#include "Pawns/Movement/NewCharacterMovementComponent.h"
#include "SkeletalMeshComponentBudgeted.h"
#include "Kismet/GameplayStatics.h"

class UNiagaraCrowdSubsystemSettings;
// Sets default values
AMassAIPawn::AMassAIPawn(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer
	.SetDefaultSubobjectClass<UNewCharacterMovementComponent>(ACharacter::CharacterMovementComponentName)
	.SetDefaultSubobjectClass<USkeletalMeshComponentBudgeted>(ACharacter::MeshComponentName))
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetComponentTickEnabled(false);
	GetMesh()->bEnableUpdateRateOptimizations = true;
	GetMesh()->bSkipBoundsUpdateWhenInterpolating = true;
	GetMesh()->bSkipKinematicUpdateWhenInterpolating = true;

	//Parameter names for clothes colors array
	bShouldLerpOpacity = false;
	CurrentOpacity = 1.f;
	SetPawnNiagaraParameters();
}

void AMassAIPawn::SetPawnNiagaraParameters()
{
	if (auto NiagaraCrowdSettings = GetDefault<UNiagaraCrowdSubsystemSettings>())
	{
		bUseNiagaraParticlesForCrowd = NiagaraCrowdSettings->bUseNiagaraParticlesForCrowd;
		if (bUseNiagaraParticlesForCrowd)
		{
			LerpSpeed = NiagaraCrowdSettings->LerpSpeed;
			bUseParticlesOnly = NiagaraCrowdSettings->bUseOnlyNiagaraParticles;
		}
		else
		{
			LerpSpeed = 0.f;
			bUseParticlesOnly = false;
		}
	}
}

void AMassAIPawn::Destroyed()
{
	Super::Destroyed();
}

// Called every frame
void AMassAIPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	CheckMeshVisibility(DeltaTime);
}

// Called when the game starts or when spawned
void AMassAIPawn::BeginPlay()
{
	Super::BeginPlay();
	
	SetActorTickEnabled(false);

	UNiagaraCrowdSubsystem* NiagaraSubsystem = GetWorld()->GetSubsystem<UNiagaraCrowdSubsystem>();
	if (IsValid(NiagaraSubsystem->GetNiagaraManager()))
	{
		NiagaraManager = NiagaraSubsystem->GetNiagaraManager();
		NiagaraManagerPrepared(NiagaraSubsystem->GetNiagaraManager());
	}
}


//Visibility logic
//----------------------------------------------------------------------------------------------------------------------
//Change pawn visibility, call from niagara crowd processor
void AMassAIPawn::SetPawnVisibility_Implementation(const bool bVisible, const float& NewLerpSpeed)
{
	IPawnInteractInterface::SetPawnVisibility_Implementation(bVisible, NewLerpSpeed);

	if (!GetMesh() || !IsReadyForAsyncPostLoad()) return;

	LerpSpeed = NewLerpSpeed;

	if (GetMesh()->GetVisibleFlag() != bVisible)
	{
		if (bVisible)
		{
			UpdateAnimation(GetWorld()->GetDeltaSeconds());
		}
		StartOpacityLerp(bVisible);
	}
}

void AMassAIPawn::UpdatePerformance_Implementation(EMassLOD::Type LOD)
{
	IPawnInteractInterface::UpdatePerformance_Implementation(LOD);

	UpdateClothSettings(LOD);
}

void AMassAIPawn::CheckMeshVisibility(const float& Time)
{
	if (!bShouldLerpOpacity || !GetMesh()) return;
	
	if (OpacityAlphaParameterName.IsNone() || UseOpacityAlphaParameterName.IsNone())
	{
		SetActorTickEnabled(false);
		return;
	}

	if (LerpSpeed > 0.0f)
	{
		CurrentOpacity = FMath::Lerp(CurrentOpacity, TargetOpacity, Time * LerpSpeed);
	}
	else
	{
		CurrentOpacity = TargetOpacity;
	}

	GetMesh()->SetScalarParameterValueOnMaterials(OpacityAlphaParameterName, CurrentOpacity);
	
	if (FMath::IsNearlyEqual(CurrentOpacity, TargetOpacity, 1E-02))
	{
		CurrentOpacity = TargetOpacity;
		GetMesh()->SetScalarParameterValueOnMaterials(OpacityAlphaParameterName, CurrentOpacity);
		bShouldLerpOpacity = false;
		GetMesh()->SetScalarParameterValueOnMaterials(UseOpacityAlphaParameterName, 0);
		GetMesh()->SetVisibility(bIsVisibleNow, true);
		OnChangePawnVisibility.Broadcast(bIsVisibleNow);
		SetActorTickEnabled(false);
		GetMesh()->SetComponentTickEnabled(bIsVisibleNow);
		
		for (const auto Component : GetComponents())
		{
			Component->SetComponentTickEnabled(bIsVisibleNow);
			Component->SetComponentTickInterval(bIsVisibleNow? 0 : BIG_NUMBER);
		}
	}
}
//----------------------------------------------------------------------------------------------------------------------

//Prepare character helper functions
//----------------------------------------------------------------------------------------------------------------------

void AMassAIPawn::UpdateNiagaraPawn(const FPawnMeshData& PawnData)
{
	if (NiagaraManager.IsValid())
	{
		NiagaraManager->SetPawnInstance(PawnData);
		DestroyMeshes();
	}
}

//Destroy meshes if needed
void AMassAIPawn::DestroyMeshes()
{
	if (bUseParticlesOnly && bUseNiagaraParticlesForCrowd)
	{
		TArray<USceneComponent*> SkeletalMeshChildren;
		GetMesh()->GetChildrenComponents(true, SkeletalMeshChildren);
		for (auto Component : SkeletalMeshChildren)
		{
			Component->DestroyComponent();
		}
		GetMesh()->DestroyComponent();

		SetActorTickInterval(BIG_NUMBER);
		for (const auto Component : GetComponents())
		{
			Component->SetComponentTickInterval(BIG_NUMBER);
		}
	}
}
//----------------------------------------------------------------------------------------------------------------------

//Tick update
//----------------------------------------------------------------------------------------------------------------------
void AMassAIPawn::UpdateClothSettings(EMassLOD::Type LOD) const
{
	if (LOD == EMassLOD::High)
	{
		GetMesh()->bDisableClothSimulation = false;
	}
	else 
	{
		GetMesh()->bDisableClothSimulation = true;
	}
}

void AMassAIPawn::StartOpacityLerp(const bool bVisible)
{
	AsyncTask(ENamedThreads::GameThread, [this, bVisible]
	{
		bIsVisibleNow = bVisible;

		if (bIsVisibleNow || OpacityAlphaParameterName.IsNone() || UseOpacityAlphaParameterName.IsNone())
		{
			GetMesh()->SetVisibility(bIsVisibleNow, true);

			for (const auto Component : GetComponents())
			{
				Component->SetComponentTickEnabled(bIsVisibleNow);
				Component->SetComponentTickInterval(bIsVisibleNow? 0 : BIG_NUMBER);
			}
		}
		else
		{
			TArray<USceneComponent*> ChildrenComponents;
			GetMesh()->GetChildrenComponents(true, ChildrenComponents);
			for (auto ChildComp : ChildrenComponents)
			{
				ChildComp->SetVisibility(false);
			}
		}

		if (!OpacityAlphaParameterName.IsNone() && !UseOpacityAlphaParameterName.IsNone())
		{
			GetMesh()->SetScalarParameterValueOnMaterials(UseOpacityAlphaParameterName, 1);
			TargetOpacity = bIsVisibleNow ? 1.f : 0.f;
			bShouldLerpOpacity = true;
			SetActorTickEnabled(true);
		}
	});
}

void AMassAIPawn::UpdateAnimation(const float& DeltaTime) const
{
	TArray<UActorComponent*> SKMComponents = K2_GetComponentsByClass(UActorComponent::StaticClass());
	for (auto SKMComponent : SKMComponents)
	{
		const auto SKMC = Cast<USkeletalMeshComponent>(SKMComponent);
		if (!SKMC) continue;
		if (SKMC->GetAnimInstance()) SKMC->GetAnimInstance()->UpdateAnimation(
										DeltaTime,false,
										UAnimInstance::EUpdateAnimationFlag::ForceParallelUpdate);
	}
}

//----------------------------------------------------------------------------------------------------------------------


