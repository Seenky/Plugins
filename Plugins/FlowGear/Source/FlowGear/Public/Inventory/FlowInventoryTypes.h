// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"
#include "FlowInventoryTypes.generated.h"

class AFlowPickupActor;
struct FInstancedStruct;
class UItemDefinitionAsset;
class UFlowItemInstance;

UE_DECLARE_GAMEPLAY_TAG_EXTERN(FlowGear_Items_Pickup);

/**
 * @struct FItemRow
 * @brief Data structure representing an item entry within an inventory DataTable.
 * @details Couples a static asset configuration with dynamic, polymorphic runtime traits.
 */
USTRUCT(BlueprintType)
struct FItemRow : public FTableRowBase
{
    GENERATED_BODY()
    
    /** @brief Soft reference to the item's static definition asset (handles UI configurations, weight, actor classes). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSoftObjectPtr<UItemDefinitionAsset> ItemDefinitionAsset = nullptr;
};

/**
 * @struct FItem
 * @brief Represents a concrete inventory storage slot.
 */
USTRUCT(BlueprintType)
struct FItem
{
    GENERATED_BODY()
    
    FItem() = default;

    explicit FItem(UFlowItemInstance* InItemInstance)
    {
       ItemInstance = InItemInstance;
       ItemsNum = 1;
    }
    
    /** @brief Reference to the item instance containing its dynamic runtime logic and status. */
    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<UFlowItemInstance> ItemInstance = nullptr;
    
    /** @brief Current amount of units stored in this slot (used for stack evaluation). */
    UPROPERTY(BlueprintReadOnly)
    int32 ItemsNum = 0;
};

USTRUCT(BlueprintType)
struct FItemRowHandle : public FDataTableRowHandle
{
    GENERATED_BODY()
};

/**
 * @struct FStartItemInfo
 * @brief Configuration blueprint defining items given to a character when initialized.
 */
USTRUCT(BlueprintType)
struct FStartItemInfo
{
    GENERATED_BODY()
    
    /** @brief Row name matching an entry within the primary inventory DataTable. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (DataTable = "ItemsTable"))
    FItemRowHandle RowHandle;
    
    /** @brief Total quantity of this item row to generate upon spawn. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0, UIMin = 0))
    int32 ItemsNum = 0;
};

USTRUCT(BlueprintType)
struct FItemPickupMessage
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<AActor> Instigator = nullptr; 

    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<UFlowItemInstance> ItemInstance = nullptr; 

    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<AFlowPickupActor> PickupActor = nullptr; 
};