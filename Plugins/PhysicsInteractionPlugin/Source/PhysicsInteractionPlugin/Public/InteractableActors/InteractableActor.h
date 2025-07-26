// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "Interfaces/InteractInterface.h"
#include "Interfaces/PhysInteractActorInterface.h"
#include "InteractableActor.generated.h"

UCLASS(ComponentWrapperClass)
class PHYSICSINTERACTIONPLUGIN_API AInteractableActor : public AActor, public IInteractInterface, public IPhysInteractActorInterface
{
	GENERATED_BODY()

public:
	AInteractableActor();

	virtual void OnInteractTargetFocusChanged_Implementation(bool bIsFindInteractTarget, FHitResult HitResult, AActor* QueryFromActor) override {}
	virtual bool CanInteract_Implementation(FHitResult HitResult, AActor* QueryFromActor) override { return true; }
	virtual bool StartInteract_Implementation(FHitResult HitResult, AActor* InteractByActor) override;
	virtual float GetPhysActorMass_Implementation() override { return Mesh->GetMass(); }
	virtual void PushPhysActor_Implementation(const FVector& Direction, const float& Strength) override
		{ Mesh->AddForce(Direction * Strength, NAME_None, false); }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
		UStaticMeshComponent* Mesh;
	
};
