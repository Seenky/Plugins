// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/Items/PickupItems/PickupItemActorBase.h"

#include "Components/InventoryComponent.h"

APickupItemActorBase::APickupItemActorBase()
{
	PrimaryActorTick.bCanEverTick = true;
}

bool APickupItemActorBase::StartInteract_Implementation(FHitResult HitResult, AActor* QueryFromActor)
{
	if (!QueryFromActor || !GetItemObject()) return false;

	// ReSharper disable once CppTooWideScopeInitStatement
	UInventoryComponent* InventoryComp = QueryFromActor->FindComponentByClass<UInventoryComponent>();
	if (!InventoryComp) return false;
	
	InventoryComp->Server_AddItem(GetItemObject());
	
	return true;
}


