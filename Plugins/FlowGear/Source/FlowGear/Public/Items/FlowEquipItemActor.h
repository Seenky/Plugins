// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Equipment/FlowEquipmentTypes.h"
#include "Items/FlowItemActorBase.h"
#include "FlowEquipItemActor.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemInteraction, UFlowItemInstance*, ItemInstance, const FEquipmentInteractionPayload&, ContextData);

/**
 * @class AFlowEquipItemActor
 * @brief Base physical actor class for weapons, tools, and visual armor components.
 * @details Inherits core data management from AFlowItemActorBase and integrates 
 * IFlowEquipmentInteractionInterface to respond directly to combat and interaction triggers.
 */
UCLASS(Abstract)
class FLOWGEAR_API AFlowEquipItemActor : public AFlowItemActorBase
{
    GENERATED_BODY()

public:
    AFlowEquipItemActor();
    
    /**
     * @brief Extends baseline lifecycle construction to pass reference data into nested static or skeletal meshes.
     * @param InItemInstance The dynamic item transaction data mapping to this physical layout.
     */
    virtual void ConstructItem(UFlowItemInstance* InItemInstance) override;
    
    UFUNCTION(BlueprintNativeEvent, Category = "Events", meta = (ForceAsFunction))
    void TriggerInteract(const FGameplayTag& InteractTag, const FInputActionValue& Value, const float TriggeredTime, const float ElapsedTime);
    virtual void TriggerInteract_Implementation(const FGameplayTag& InteractTag, const FInputActionValue& Value, const float TriggeredTime, const float ElapsedTime) {}
    
    UFUNCTION(BlueprintNativeEvent, Category = "Events", meta = (ForceAsFunction))
    void CompleteInteract(const FGameplayTag& InteractTag);
    virtual void CompleteInteract_Implementation(const FGameplayTag& InteractTag) {}
    
    UPROPERTY(BlueprintCallable, BlueprintAssignable, Category = "Events")
    FOnItemInteraction OnItemInteraction;
    
protected:
    virtual void BeginPlay() override;
    
    /** @brief Root coordinate transform anchor scene component. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USceneComponent> Scene;
    
    /** @brief Primitive static mesh component layer for rigid mechanical objects (e.g., knives, pistols). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> StaticMesh;
    
    /** @brief Deformable skeletal hierarchy component managing bones and local weapon animations (e.g., assault rifles, bows). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USkeletalMeshComponent> SkeletalMesh;
};