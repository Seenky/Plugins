// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FlowEquipmentTypes.h"
#include "Components/ActorComponent.h"
#include "FlowEquipmentComponent.generated.h"

class UInputAction;
struct FInputActionInstance;
class UEnhancedInputLocalPlayerSubsystem;
class AFlowPickupActor;
class AFlowEquipItemActor;
class UFlowItemInstance;
class UFlowInventoryComponent;

/** @brief Multicast delegate triggered whenever an equipped item notifies the manager component of an interaction milestone. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEquipmentInteraction, UFlowItemInstance*, ItemInstance, const FEquipmentInteractionPayload&, ContextData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEquipItem, UFlowItemInstance*, ItemInstance);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnequipItem, UFlowItemInstance*, ItemInstance);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDropItem, UFlowItemInstance*, ItemInstance);

/**
 * @class UFlowEquipmentComponent
 * @brief Component responsible for managing the character's equipped items.
 * @details Maps logical item instances from the inventory component to specified character slots, 
 * handles the spawning of their 3D visual actors, manages attachment to character sockets, 
 * and automatically injects item-specific Input Mapping Contexts (IMC) while routing input actions.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FLOWGEAR_API UFlowEquipmentComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UFlowEquipmentComponent();
    
    virtual void BeginPlay() override;
    
    /**
     * @brief Sets the base scene component used for attaching visual actors.
     * @param InComponent The scene component (typically the Character's Mesh) to which item actors will attach.
     */
    UFUNCTION(BlueprintCallable, Category = "Flow Gear|Equipment")
    void SetAttachComponent(USceneComponent* InComponent);
    
    /**
     * @brief Equips an item onto the character based on the provided request parameters.
     * @details Implements transaction-like safety: if the visual representation fails to spawn, 
     * the process aborts immediately, leaving the slot's original state and old weapon untouched.
     * If the slot is already occupied, the existing item will be automatically unequipped first.
     * @param Request Structure detailing which item or tag to equip and into which slot.
     */
    UFUNCTION(BlueprintCallable, Category = "Flow Gear|Equipment")
    void Equip(const FEquipRequest Request);
    
    /**
     * @brief Unequips an item, destroying its visual representation in the world.
     * @details Automatically removes the FlowGear_Equipment_States_Equipped tag from the item instance.
     * The target slot is searched sequentially using the priority: Item Instance -> Slot Name -> Item Tag.
     * @param Request Structure specifying the criteria for finding the slot to unequip.
     */
    UFUNCTION(BlueprintCallable, Category = "Flow Gear|Equipment")
    void Unequip(const FEquipRequest Request);
    
    /**
     * @brief Unequips an item, spawns its physical pickup representation in the world, and deletes it from the inventory.
     * @details Resolves the item slot via the request criteria, fetches its current world transform to 
     * use as a drop location, spawns an AFlowPickupActor, and completely deletes the item instance from the companion inventory.
     * @param Request Structure specifying the criteria for finding the slot to drop.
     */
    UFUNCTION(BlueprintCallable, Category = "Flow Gear|Equipment")
    void Drop(const FEquipRequest Request);
    
    /**
     * @brief Forwards a startup interaction trigger alongside custom payload context to the targeted equipped item actor.
     * @details Evaluates the active slots based on the provided request parameters and routes the payload to the underlying interface.
     * @param Request Structure specifying the criteria to find the active equipment slot.
     * @param InteractTag Interaction tag broadcasted to item actor
     * @param Value Input value (bool, float or FVector2D)
     * @param TriggeredTime Time since input triggered
     * @param ElapsedTime Time since input started
     */
    UFUNCTION(BlueprintCallable, Category = "Flow Gear|Equipment", meta = (AutoCreateRefTerm = "Value"))
    void TriggerInteract(const FEquipRequest Request, const FGameplayTag& InteractTag, const FInputActionValue& Value, const float TriggeredTime, const float ElapsedTime);
    
    /**
     * @brief Forwards a startup interaction trigger alongside custom payload context to the targeted equipped item actor.
     * @details Evaluates the active slots based on the provided request parameters and routes the payload to the underlying interface.
     * @param Request Structure specifying the criteria to find the active equipment slot.
     * @param InteractTag Interaction tag broadcasted to item actor
     */
    UFUNCTION(BlueprintCallable, Category = "Flow Gear|Equipment", meta = (AutoCreateRefTerm = "Value"))
    void TriggerInteractFast(const FEquipRequest Request, const FGameplayTag& InteractTag);
    
    /**
     * @brief Forwards a termination interaction trigger alongside custom payload context to the targeted equipped item actor.
     * @details Evaluates the active slots based on the provided request parameters and routes the payload to the underlying interface.
     * @param Request Structure specifying the criteria to find the active equipment slot.
     * @param InteractTag Interaction tag broadcasted to item actor
     */
    UFUNCTION(BlueprintCallable, Category = "Flow Gear|Equipment")
    void CompleteInteract(const FEquipRequest Request, const FGameplayTag& InteractTag);
    
    /** @brief Main broadcast pipeline forwarding child item interaction signals up to character or UI layers. */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnEquipmentInteraction OnEquipmentInteraction;
    
    /** @brief Main broadcast pipeline forwarding item is equipped. */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnEquipItem OnEquipItem;
    
    /** @brief Main broadcast pipeline forwarding item is unequipped. */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnUnequipItem OnUnequipItem;
    
    /** @brief Main broadcast pipeline forwarding item is dropped. */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnDropItem OnDropItem;
    
private:
    /**
    * @brief Searches for an active equipment slot by item instance.
    * @param ItemInstance Item instance to look up.
    * @return Pointer to the active slot structure, or nullptr if the slot is currently empty.
    */
    FActiveEquipmentSlot* FindActiveSlotForItemInstance(UFlowItemInstance* ItemInstance);
    
    /**
     * @brief Searches for an active equipment slot by its registered name.
     * @param SlotName Name of the slot (socket) to look up.
     * @return Pointer to the active slot structure, or nullptr if the slot is currently empty.
     */
    FActiveEquipmentSlot* FindActiveSlot(FName SlotName);
    
    /**
     * @brief Checks whether the specified slot is currently occupied.
     * @param SlotName Name of the slot to verify.
     * @return true if an item is already equipped in this slot.
     */
    bool IsSlotBusy(FName SlotName);
    
    /**
     * @brief Spawns the physical 3D visual representation actor for an item instance.
     * @details Utilizes deferred spawning (SpawnActorDeferred) to guarantee data initialization 
     * on the actor before its BeginPlay lifecycle method executes.
     * @param ItemInstance The item instance for which the visual actor is being created.
     * @return Pointer to the spawned base item actor, or nullptr if spawning failed (e.g., class asset not configured).
     * @warning Performs a synchronous asset load if the actor class has not been preloaded.
     */
    AFlowEquipItemActor* CreateSlotRepresentation(UFlowItemInstance* ItemInstance) const;
    
    /**
     * @brief Spawns a physical pickup object container in the game world.
     * @details Instantiates a generic drop container (AFlowPickupActor) using a deferred transaction 
     * and passes the item instance data into it before completing final replication and physics initialization.
     * @param ItemInstance The item instance being dropped onto the ground.
     * @param Transform The world position, rotation, and scale where the pickup should appear.
     * @return Pointer to the newly generated pickup actor base class, or nullptr if spawning failed.
     */
    AFlowPickupActor* CreatePickupRepresentation(UFlowItemInstance* ItemInstance, const FTransform& Transform) const;
    
    /** @brief Dynamic callback internally bound to intercept raw events from actively held visual item actors. */
    UFUNCTION()
    void OnItemInteraction(UFlowItemInstance* ItemInstance, const FEquipmentInteractionPayload& ContextData);
    
    /**
     * @brief Helper function to retrieve the Enhanced Input Subsystem from the owning Actor's Player Controller.
     * @return Pointer to the local player's enhanced input subsystem, or nullptr if the owning actor is not controlled by a human player.
     */
    UEnhancedInputLocalPlayerSubsystem* GetOwnerEnhancedInputSubsystem() const;

    /**
     * @brief Activates the item's Input Mapping Context (IMC) and dynamically binds its InputActions to component methods.
     * @param ItemInstance The item instance containing the input configuration.
     * @param Slot Reference to the active slot structure where binding handles will be stored for future cleanup.
     */
    void AddMappingContext(const UFlowItemInstance* ItemInstance, FActiveEquipmentSlot& Slot);

    /**
     * @brief Removes the item's Input Mapping Context (IMC) and fully tears down all associated dynamic input bindings.
     * @param Slot Reference to the active slot being cleared.
     */
    void RemoveMappingContext(FActiveEquipmentSlot& Slot);
    
    /**
     * @brief Native handler for continuous input action execution (ETriggerEvent::Triggered).
     * @details Invoked by the Enhanced Input system, determines the matching interaction tag, and forwards the call to TriggerInteract.
     * @param Instance Contextual runtime data of the triggered Enhanced Input action.
     */
    void Native_OnActionTriggered(const FInputActionInstance& Instance);

    /**
     * @brief Native handler for input action completion or release (ETriggerEvent::Completed).
     * @details Invoked by the Enhanced Input system, determines the matching interaction tag, and forwards the call to CompleteInteract.
     * @param Instance Contextual runtime data of the completed Enhanced Input action.
     */
    void Native_OnActionCompleted(const FInputActionInstance& Instance);
    
    /** @brief The scene component (usually the Character's Skeletal Mesh) used as the attachment parent. */
    UPROPERTY()
    TObjectPtr<USceneComponent> AttachComponent;
    
    /** @brief Reference to the inventory component residing on the same owner actor. */
    UPROPERTY()
    TObjectPtr<UFlowInventoryComponent> InventoryComponent;
    
    /** @brief Flat array holding data for all currently occupied equipment slots. */
    UPROPERTY()
    TArray<FActiveEquipmentSlot> ActiveSlots;
    
    /** @brief Global runtime routing map associating specific InputActions with their respective interaction GameplayTags. */
    UPROPERTY()
    TMap<TObjectPtr<UInputAction>, FGameplayTag> CurrentInputRoutes;
};