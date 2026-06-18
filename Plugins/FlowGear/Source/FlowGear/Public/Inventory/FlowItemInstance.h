// Fill out your copyright notice in the Description page of Project Settings.

// ReSharper disable CppUE4BlueprintCallableFunctionMayBeStatic
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "UObject/Object.h"
#include "FlowItemInstance.generated.h"

class AFlowPickupActor;
class AFlowEquipItemActor;
struct FGameplayTag;
class UItemDefinitionAsset;
struct FInstancedStruct;
class UInputMappingContext;

/**
 * @class UFlowItemInstance
 * @brief Represents a mutable, dynamic instance of an item in the game world or inventory.
 * @details Wraps a static UItemDefinitionAsset to inherit base configurations while managing 
 * custom runtime traits (e.g., durability, remaining ammo) and structural dynamic states via gameplay tags.
 */
UCLASS()
class FLOWGEAR_API UFlowItemInstance final : public UObject
{
    GENERATED_BODY()
    
public:
    /**
     * @brief Assigns the static definition backing data asset for this item instance.
     * @param InDefinitionAsset Pointer to the configured static data asset.
     */
    UFUNCTION(BlueprintCallable, Category = "Flow Gear|Item Instance")
    void SetDefinitionAsset(UItemDefinitionAsset* InDefinitionAsset);
    
    /** @brief Gets the unique static gameplay tag identifying this item type. */
    UFUNCTION(BlueprintPure, Category = "Flow Gear|Item Instance")
    FGameplayTag GetItemTag() const;
    
    /** @brief Gets the localized user-facing display name of the item. */
    UFUNCTION(BlueprintPure, Category = "Flow Gear|Item Instance")
    FText GetItemName() const;
    
    /** @brief Gets the soft object pointer to the item's UI inventory icon. */
    UFUNCTION(BlueprintPure, Category = "Flow Gear|Item Instance")
    TSoftObjectPtr<UTexture2D> GetItemIcon() const;
    
    /** @brief Gets the soft class pointer of the visual actor spawned when equipping the item. */
    UFUNCTION(BlueprintPure, Category = "Flow Gear|Item Instance")
    TSoftClassPtr<AFlowEquipItemActor> GetItemActorClass() const;
    
    /** @brief Gets the soft class pointer of the visual actor spawned when drop the item. */
    UFUNCTION(BlueprintPure, Category = "Flow Gear|Item Instance")
    TSoftClassPtr<AFlowPickupActor> GetPickupActorClass() const;
    
    /** @brief Gets the soft class pointer of the visual actor spawned when drop the item. */
    UFUNCTION(BlueprintPure, Category = "Flow Gear|Item Instance")
    TSoftObjectPtr<UStaticMesh> GetDefaultPickupMesh() const;
    
    /** @brief Gets the soft object pointer to the Input Mapping Context (IMC) specific to this item type. */
    UFUNCTION(BlueprintPure, Category = "Flow Gear|Item Instance")
    TSoftObjectPtr<UInputMappingContext> GetInputMappingContext() const;
    
    /** @brief Gets the application priority of this item's IMC when registered within the Enhanced Input Subsystem. */
    UFUNCTION(BlueprintPure, Category = "Flow Gear|Item Instance")
    int32 GetInputPriority() const;
    
    /** @brief Gets the InputAction to GameplayTag routing map configured within the item's static definition asset. */
    UFUNCTION(BlueprintPure, Category = "Flow Gear|Item Instance")
    TMap<class UInputAction*, FGameplayTag> GetInputBindings() const;
    
    /** @brief Checks if the item definition allows this item type to stack. */
    UFUNCTION(BlueprintPure, Category = "Flow Gear|Item Instance")
    bool IsStackable() const;
    
    /** @brief Gets the maximum configured stack size constraint for this item type. */
    UFUNCTION(BlueprintPure, Category = "Flow Gear|Item Instance")
    int32 GetStackSize() const;
    
    /**
     * @brief Batch inserts an array of polymorphic traits into this item instance.
     * @param InTraits Array of data structures acting as specialized item components.
     * @param bAppend If true, traits not currently existing will be added to the end of the array.
     * @param bReplace If true, traits with matching struct types will overwrite the previous configurations.
     */
    UFUNCTION(BlueprintCallable, Category = "Flow Gear|Item Instance")
    void AddItemTraits(const TArray<FInstancedStruct>& InTraits, const bool bAppend = true, const bool bReplace = true);
    
    /**
     * @brief Inserts or replaces a single polymorphic trait on this item instance.
     * @param InTrait The localized instanced structure configuration to embed.
     * @param bReplace If true, a matching struct type will overwrite the previous record.
     */
	UFUNCTION(BlueprintCallable, CustomThunk, Category = "Flow Gear|Item Instance", meta = (CustomStructureParam = "InTrait"))
	void AddItemTrait(const FInstancedStruct& InTrait, const bool bReplace = true) {}

	DECLARE_FUNCTION(execAddItemTrait);
    
    /** @brief Returns a constant reference to the full list of embedded dynamic traits. */
    UFUNCTION(BlueprintPure, Category = "Flow Gear|Item Instance")
    const TArray<FInstancedStruct>& GetItemTraits() const { return ItemTraits; }
    
    /**
     * @brief Blueprint-friendly query to extract a trait reference by structural type match.
     * @param OutTrait Output container filled with the located trait data if successful.
     * @return true if a matching structural trait type was found and successfully extracted.
     */
	UFUNCTION(BlueprintPure, CustomThunk, Category = "Flow Gear|Item Instance", meta = (CustomStructureParam = "OutTrait"))
    UPARAM(DisplayName = "Success") bool GetItemTrait(FInstancedStruct& OutTrait) const { return false; }
	
	DECLARE_FUNCTION(execGetItemTrait);
    
    /**
     * @brief Strongly-typed C++ template to find a read-only trait pointer by UScriptStruct type.
     * @tparam T The explicit UScriptStruct type to extract (e.g., FWeaponAmmoTrait).
     * @return Constant pointer to the requested trait structure data, or nullptr if not found.
     */
    template<typename T>
    const T* FindItemTrait() const
    {
       const UScriptStruct* TargetStruct = T::StaticStruct();
       
       if (const uint32* Index = TypeToTrait.Find(TargetStruct))
       {
          if (ItemTraits.IsValidIndex(*Index))
          {
             const FInstancedStruct& Struct = ItemTraits[*Index];
             return Struct.GetPtr<T>();
          }
       }
       return nullptr;
    }

    /**
     * @brief Strongly-typed C++ template to find a mutable trait pointer by UScriptStruct type.
     * @tparam T The explicit UScriptStruct type to modify (e.g., FWeaponAmmoTrait).
     * @return Mutable pointer to the requested trait structure data, or nullptr if not found.
     */
    template<typename T>
    T* FindItemTraitMutable()
    {
       return const_cast<T*>(FindItemTrait<T>());
    }
    
    /**
     * @brief Appends a dynamic runtime gameplay tag onto this specific item instance.
     * @example FlowGear_Equipment_States_Equipped
     * @param DynamicStateTag State modifier tag to apply.
     */
    UFUNCTION(BlueprintCallable, Category = "Flow Gear|Item Instance")
    void AddDynamicState(FGameplayTag DynamicStateTag);
    
    /**
     * @brief Strips a dynamic runtime gameplay tag off this specific item instance.
     * @param DynamicStateTag State modifier tag to detach.
     */
    UFUNCTION(BlueprintCallable, Category = "Flow Gear|Item Instance")
    void RemoveDynamicState(FGameplayTag DynamicStateTag);
    
    /**
     * @brief Checks if this item instance currently carries a specific dynamic status tag.
     * @param DynamicStateTag The state tag query.
     * @return true if the item instance has the specified state tag attached.
     */
    UFUNCTION(BlueprintPure, Category = "Flow Gear|Item Instance")
    bool ContainsDynamicState(FGameplayTag DynamicStateTag) const;
    
    /**
     * @brief Validates whether this item contains any matching tag within the specified container.
     * @param DynamicStateTag Container of query state tags.
     * @return true if there is at least one overlapping match between the item's states and the container.
     */
    UFUNCTION(BlueprintPure, Category = "Flow Gear|Item Instance")
    bool HasAnyDynamicStates(FGameplayTagContainer DynamicStateTag) const;
    
private:
	void AddItemTraitInternal(const FInstancedStruct& InTrait, const bool bReplace = true);
    /** @brief Internal helper to safely resolve a mutable structure memory block pointer. */
    FInstancedStruct* GetTraitPtr(const UScriptStruct* InTrait);
    
    /** @brief Internal helper to safely resolve a constant structure memory block pointer. */
    const FInstancedStruct* GetTraitPtr(const UScriptStruct* InTrait) const;
    
private:
    /** @brief Reference back to the immutable base asset properties configuration. */
    UPROPERTY()
    TObjectPtr<UItemDefinitionAsset> DefinitionAsset;
    
    /** @brief Internal raw collection containing all polymorphically embedded trait records. */
    UPROPERTY()
    TArray<FInstancedStruct> ItemTraits = {};
    
    /** @brief Fast O(1) optimization map cache tracking struct reflections to their index locations inside the flat ItemTraits array. */
    UPROPERTY()
    TMap<const UScriptStruct*, uint32> TypeToTrait;
    
    /** @brief Flat runtime container storing dynamic status contexts (e.g., Equipped, Broken, Stolen). */
    UPROPERTY()
    FGameplayTagContainer DynamicStateTags;
};