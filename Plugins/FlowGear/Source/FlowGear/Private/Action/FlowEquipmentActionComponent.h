// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Equipment/FlowEquipmentTypes.h"
#include "FlowEquipmentActionComponent.generated.h"


class AFlowPickupActor;
class AFlowEquipItemActor;
class UFlowItemInstance;
class UFlowEquipmentComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FLOWGEAR_API UFlowEquipmentActionComponent final : public UActorComponent
{
	GENERATED_BODY()

public:
	UFlowEquipmentActionComponent();
	
	virtual void BeginPlay() override;
	
protected:
	UFUNCTION()
	void OnEquipmentInteraction(UFlowItemInstance* ItemInstance, const FEquipmentInteractionPayload& ContextData);
	
	UFUNCTION()
	void ForceCancelAllActions(UFlowItemInstance* ItemInstance);

	UFUNCTION()
	void OnEquipItem(UFlowItemInstance* ItemInstance);
	
	UFUNCTION()
	void OnUnequipItem(UFlowItemInstance* ItemInstance);
	
	UFUNCTION()
	void OnDropItem(UFlowItemInstance* ItemInstance);
	
private:
	
	UPROPERTY()
	TObjectPtr<UFlowEquipmentComponent> EquipmentComponent;
};
