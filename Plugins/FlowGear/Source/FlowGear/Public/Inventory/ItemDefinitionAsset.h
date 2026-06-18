// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "InputMappingContext.h"
#include "Engine/DataAsset.h"
#include "StructUtils/InstancedStruct.h"
#include "ItemDefinitionAsset.generated.h"

class AFlowPickupActor;
class AFlowEquipItemActor;

/**
 * @class UItemDefinitionAsset
 * @brief Static configuration data asset defining an item type.
 * @details Contains all read-only, non-changing data for a specific class of items, 
 * such as assets, UI representations, and maximum stack behavior constraints.
 */
UCLASS()
class FLOWGEAR_API UItemDefinitionAsset : public UDataAsset
{
    GENERATED_BODY()
    
public:
    /** @brief The user-facing localized display name of the item. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
    FText FriendlyName = FText::GetEmpty();
    
    /** @brief Soft reference to the 2D texture icon used for UI inventory slots. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
    TSoftObjectPtr<UTexture2D> Icon = nullptr;
    
    /** @brief Unique gameplay tag identifying this item type (e.g., Item.Weapon.Rifle). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
    FGameplayTag Tag = FGameplayTag::EmptyTag;
    
    /** @brief Soft class reference to the specialized equipment actor spawned when this item is held. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
    TSoftClassPtr<AFlowEquipItemActor> EquipItemActorClass = nullptr;
    
    /** @brief Soft class reference to the specialized pickup actor spawned when this item is dropped. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
    TSoftClassPtr<AFlowPickupActor> PickupItemActorClass = nullptr;
    
    /** @brief Soft reference to the 3D static mesh used when the item is dropped in the world as a pickup. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
    TSoftObjectPtr<UStaticMesh> DefaultPickupMesh = nullptr;
    
    /** @brief Optional Enhanced Input Mapping Context unique to this item type (e.g., specific triggers/actions for weapons). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    TSoftObjectPtr<UInputMappingContext> ItemInputMappingContext = nullptr;

    /** @brief Priority level at which this item's input mapping context will be injected into the local player system. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (EditCondition = "ItemInputMappingContext != nullptr"))
    int32 InputContextPriority = 1;
    
    /** @brief Maps specific Input Actions used in this item's IMC to their runtime interaction gameplay tags. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    TMap<UInputAction*, FGameplayTag> InputBindings;
    
    /** @brief Dictates whether multiple copies of this item can occupy a single inventory slot stack. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stacking")
    bool bIsStackable = false;
    
    /** * @brief The maximum allowed quantity per inventory slot stack.
     * @note Only evaluated and configurable in the editor if bIsStackable is set to true.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stacking", meta = (EditCondition = "bIsStackable"))
    int32 MaxStackSize = 1;
    
    /** @brief Array of extensible structural components (Traits) injecting custom functional modules (e.g., ammo tracker, durability values). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FInstancedStruct> ItemTraits = {};
};