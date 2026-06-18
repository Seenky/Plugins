// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnhancedInputComponent.h"
#include "GameplayTagContainer.h"
#include "InputTriggers.h" 
#include "NativeGameplayTags.h"
#include "StructUtils/InstancedStruct.h"
#include "FlowEquipmentTypes.generated.h"

class AFlowEquipItemActor;
class AFlowItemActorBase;
class UFlowItemInstance;

/**
 * @brief Global state tag indicating that an item is currently equipped.
 * @note Used to filter items in the inventory component to prevent duplicating equipped items.
 */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(FlowGear_Equipment_States_Equipped);

/**
 * @struct FEquipRequest
 * @brief Represents a request to equip or unequip an item.
 * @details Supports both explicit object passing (UI/Inventory clicks) and implicit tag-based evaluation (hotkeys).
 */
USTRUCT(BlueprintType)
struct FEquipRequest
{
    GENERATED_BODY()
    
    /** 
     * @brief Direct pointer to a specific runtime item instance.
     * @note If valid, the system bypasses ItemTag and equips this exact instance.
     */
    UPROPERTY(BlueprintReadWrite, Category = "Equipment")
    TObjectPtr<UFlowItemInstance> ItemInstance = nullptr;
    
    /** 
     * @brief Gameplay Tag representing the type of item (e.g., Item.Weapon.Pistol).
     * @note Used to automatically find an available matching item in the inventory if ItemInstance is nullptr.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
    FGameplayTag ItemTag = FGameplayTag::EmptyTag;
    
    /** 
     * @brief Name of the target equipment slot (must match the socket name on the character's Skeletal Mesh).
     * @example Weapon_Right, Weapon_Left
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
    FName SlotName;
};

/**
 * @struct FActiveEquipmentSlot
 * @brief Data structure holding information about an active (occupied) equipment slot.
 * @details Couples the logical item instance from the inventory with its physical 3D actor spawned in the world.
 */
USTRUCT(BlueprintType)
struct FActiveEquipmentSlot
{
    GENERATED_BODY()

    /** @brief The Gameplay Tag of the item currently occupying this slot. */
    UPROPERTY()
    FGameplayTag ItemTag;
    
    /** @brief Name of the slot (socket) where the visual actor is attached. */
    UPROPERTY()
    FName SlotName;
    
    TArray<FInputBindingHandle> BindingHandles;

    /** @brief The logical instance of the equipped item. */
    UPROPERTY()
    TObjectPtr<UFlowItemInstance> ItemInstance = nullptr;

    /** @brief Reference to the 3D visual representation (Actor) spawned in the world. */
    UPROPERTY()
    TObjectPtr<AFlowEquipItemActor> SpawnedVisualActor = nullptr;
};

UENUM(BlueprintType)
enum class EFlowInputStatus : uint8
{
    Triggered,
    Completed
};

USTRUCT(BlueprintType)
struct FEquipmentInteractionPayload
{
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTag InteractionTag;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FInstancedStruct Payload;
};
