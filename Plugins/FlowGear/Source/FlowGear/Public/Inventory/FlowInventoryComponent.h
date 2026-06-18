// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FlowInventoryTypes.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "FlowInventoryComponent.generated.h"

class AFlowPickupActor;
class UFlowItemInstance;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryLoaded);

// Inventory notification delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAddItem, UFlowItemInstance*, AddedItem);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRemoveItem, UFlowItemInstance*, RemovedItem);

/**
 * @class UFlowInventoryComponent
 * @brief Core component handling item storage and inventory management.
 * @details Governs adding, removing, stacking, and indexing items. Handles asynchronous 
 * item generation on startup based on defined DataTables.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FLOWGEAR_API UFlowInventoryComponent final : public UActorComponent
{
    GENERATED_BODY()
    
public:
    UFlowInventoryComponent();
    
    virtual void BeginPlay() override;
    
private:
    /** @brief Initiates the asynchronous loading and construction of default items defined in StartItems. */
    void ConstructStartItems();
    
    /** 
     * @brief Callback invoked when a specific item definition asset finishes loading asynchronously.
     * @param SoftObjectPath Asset path of the loaded object.
     * @param Object Pointer to the freshly loaded UObject asset.
     */
    void OnAssetDefinitionLoaded(const FSoftObjectPath& SoftObjectPath, UObject* Object);
    
    UFUNCTION()
    void OnItemAddMessage(FGameplayTag Channel, const FInstancedStruct& Payload);

public:
    UFUNCTION(BlueprintCallable, Category = "Flow Gear|Inventory")
    bool TryPickupItem(AFlowPickupActor* PickupActor);
    
    /**
     * @brief Public method to insert a runtime item instance into the inventory.
     * @param ItemInstance The item instance to add.
     */
    UFUNCTION(BlueprintCallable, Category = "Flow Gear|Inventory")
    void AddItem(UFlowItemInstance* ItemInstance);
    
    /**
     * @brief Public method to remove a runtime item instance from the inventory.
     * @param ItemInstance The item instance to strip.
     */
    UFUNCTION(BlueprintCallable, Category = "Flow Gear|Inventory")
    void RemoveItem(UFlowItemInstance* ItemInstance);
    
    /**
     * @brief Highly efficient query to retrieve the first available item matching a specific tag, while filtering by runtime states.
     * @details Typically used by the equipment system to find an item type while ignoring instances already marked as equipped.
     * @param ItemTag The target identity tag of the item type we are looking for.
     * @param ExcludeStateTags Dynamic runtime tags that the target item must NOT contain to be selected.
     * @return Pointer to a valid UFlowItemInstance, or nullptr if no suitable item matches the criteria.
     */
    UFUNCTION(BlueprintPure, Category = "Flow Gear|Inventory")
    UFlowItemInstance* FindFirstItemWithTags(FGameplayTag ItemTag, FGameplayTagContainer ExcludeStateTags);
    
    UFUNCTION(BlueprintPure, Category = "Flow Gear|Inventory")
    bool IsLoaded() const { return bIsLoaded; }
    
public:
    UPROPERTY(BlueprintAssignable, Category="Events")
    FOnInventoryLoaded OnInventoryLoaded;
    /** @brief Event fired immediately after an item is successfully added to the storage. */
    UPROPERTY(BlueprintAssignable, Category="Events")
    FOnAddItem OnAddItem;
    
    /** @brief Event fired immediately after an item is removed from the storage. */
    UPROPERTY(BlueprintAssignable, Category="Events")
    FOnRemoveItem OnRemoveItem;
    
protected:
    /** @brief Reference to the DataTable storing item row definitions (expects FItemRow structure). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (RequiredAssetDataTags = "RowStructure=/Script/FlowGear.ItemRow"))
    TObjectPtr<UDataTable> ItemsTable;
    
    /** @brief Set of items and quantities given to the owner automatically on game start. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (EditCondition = "ItemsTable != nullptr"))
    TArray<FStartItemInfo> StartItems;

private:
    /** @brief Internal low-level handler for adding items (manages stack bounds and caching). */
    bool AddItemInternal(UFlowItemInstance* ItemInstance);
    
    /** @brief Internal low-level handler for removing items (collapses stacks and performs RemoveAtSwap cleanup). */
    bool RemoveItemInternal(UFlowItemInstance* ItemInstance);
    
    /** @brief Searches for a stackable slot index matching the item tag that hasn't reached its maximum stack capacity. */
    int32 FindStackableSlotByTag(const FGameplayTag& ItemTag);
    
    /** @brief O(1) slot index lookup for unstackable unique item instances via the internal hash map. */
    int32 FindSlotForItem(UFlowItemInstance* ItemInstance);
    
private:
    /** @brief Flat array containing all physical slots and stack quantities in the inventory. */
    UPROPERTY()
    TArray<FItem> Items;
    
    /** @brief Fast-lookup O(1) hash map mapping item instance pointers to their respective slot indexes (unstackable items only). */
    TMap<TObjectPtr<UFlowItemInstance>, int32> ItemToSlotMap;
    
    bool bIsLoaded;
    int32 ItemsToLoad;
    int32 LoadedItems;
};