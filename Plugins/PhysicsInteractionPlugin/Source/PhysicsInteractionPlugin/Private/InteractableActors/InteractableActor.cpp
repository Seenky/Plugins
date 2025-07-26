// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractableActors/InteractableActor.h"

#include "GrabComponent.h"
#include "Interfaces/PhysInteractInterface.h"


// Sets default values
AInteractableActor::AInteractableActor()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
}

bool AInteractableActor::StartInteract_Implementation(FHitResult HitResult, AActor* QueryFromActor)
{
	if (!QueryFromActor || !Mesh) return false;

	// ReSharper disable once CppTooWideScopeInitStatement
	UGrabComponent* PickupComponent = QueryFromActor->FindComponentByClass<UGrabComponent>();
	if (!PickupComponent) return false;
 
	if (!QueryFromActor->Implements<UPhysInteractInterface>()) return false;
	
	PickupComponent->Server_TryGrabObject(Mesh);
	
	return true;
}


