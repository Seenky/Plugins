// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Actors/Items/ItemActorBase.h"
#include "Interfaces/InteractInterface.h"
#include "Interfaces/InteractOutlinerInterface.h"
#include "PickupItemActorBase.generated.h"

UCLASS(Abstract)
class INVENTORYSYSTEMPLUGIN_API APickupItemActorBase : public AItemActorBase, public IInteractInterface, public IInteractOutlinerInterface
{
	GENERATED_BODY()

public:
	APickupItemActorBase();

	virtual  void BeginPlay() override { Super::BeginPlay(); if (HasAuthority() && GetItemObject()->GetPickupItemData().bShouldUseOnDrop) DroppedItemUsage(GetItemObject()->GetPickupItemData().bIsUsing); }

	virtual void OnInteractTargetFocusChanged_Implementation(bool bIsFindInteractTarget, FHitResult HitResult, AActor* QueryFromActor) override {}
	virtual const TArray<UPrimitiveComponent*> GetComponentsForHighlight_Implementation() const override { return TArray<UPrimitiveComponent*>{Mesh}; }
	virtual bool CanInteract_Implementation(FHitResult HitResult, AActor* QueryFromActor) override { return true; }
	virtual void SetPickUpMesh_Implementation(UStaticMesh* StaticMesh) override { Mesh->SetStaticMesh(StaticMesh); }

	virtual bool StartInteract_Implementation(FHitResult HitResult, AActor* QueryFromActor) override;

protected:
	/**Use dropped item if it was using in hand*/
	UFUNCTION(BlueprintImplementableEvent, Category = "Pickup Item Functional")
	bool DroppedItemUsage(const bool bIsUsing);
};
