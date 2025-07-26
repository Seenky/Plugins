// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ItemActorBase.h"
#include "Interfaces/UsableItemsInteract.h"
#include "EquipItemActorBase.generated.h"

UCLASS(Abstract)
class INVENTORYSYSTEMPLUGIN_API AEquipItemActorBase : public AItemActorBase, public IUsableItemsInteract
{
	GENERATED_BODY()

public:
	AEquipItemActorBase();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* SkeletalMesh;

	virtual void InventoryItemInteract_Implementation(FGameplayTagContainer InteractTags, const bool bInteract) override;

protected:
	//Select interaction method by tag and name
	UPROPERTY(EditDefaultsOnly, Category = "InteractTags")
	TMap<FGameplayTag, FString> InteractByTag;
};
