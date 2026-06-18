// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Equipment/FlowEquipmentTypes.h"
#include "UObject/Object.h"
#include "FlowItemAction.generated.h"


class AFlowItemActorBase;
class UFlowItemInstance;

UCLASS(Blueprintable, BlueprintType, Abstract, DefaultToInstanced, EditInlineNew)
class FLOWGEAR_API UFlowItemAction : public UObject
{
	GENERATED_BODY()
	
public:
	UFlowItemAction();
#if WITH_EDITOR
	virtual bool ImplementsGetWorld() const override { return true; }
#endif
	
	virtual UWorld* GetWorld() const override;
	
	UFUNCTION(BlueprintNativeEvent, Category = "Action", meta = (ForceAsFunction))
	void ExecuteAction(AActor* Owner, UFlowItemInstance* ItemInstance, const FEquipmentInteractionPayload& Payload);
	virtual void ExecuteAction_Implementation(AActor* Owner, UFlowItemInstance* ItemInstance, const FEquipmentInteractionPayload& Payload);
	
	UFUNCTION(BlueprintNativeEvent, Category = "Action", meta = (ForceAsFunction))
	void CancelAction();
	virtual void CancelAction_Implementation() {}
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actions", meta = (BaseStruct = "/Script/FlowGear.ItemActionsTraitBase"))
	TArray<FInstancedStruct> SupportedActions;
	
protected:
	UPROPERTY()
	mutable TObjectPtr<UWorld> WorldPrivate = nullptr;
};
