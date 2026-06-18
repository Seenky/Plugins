// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FlowItemActorBase.h"
#include "Interfaces/InteractInterface.h"
#include "Interfaces/InteractOutlinerInterface.h"
#include "Inventory/FlowInventoryTypes.h"
#include "FlowPickupActor.generated.h"

USTRUCT(BlueprintType)
struct FItemActorPayload
{
    GENERATED_BODY()
    
    FItemActorPayload() = default;

    explicit FItemActorPayload(class AFlowPickupActor* InItemActor)
    {
        ItemActor = InItemActor;
    }
    
    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<class AFlowPickupActor> ItemActor = nullptr;
};

/**
 * @class AFlowPickupActor
 * @brief World representation of an item that can be picked up by players.
 * @details Manages physical simulation behavior on impact and handles asynchronous 
 * item generation from data assets or runtime item instances.
 */
UCLASS()
class FLOWGEAR_API AFlowPickupActor : public AFlowItemActorBase, public IInteractInterface, public IInteractOutlinerInterface
{
    GENERATED_BODY()

public:
    AFlowPickupActor();
    
    virtual void ConstructItem(UFlowItemInstance* InItemInstance) override;
    
    /*~IInteractInterface*/
    virtual void OnInteractTargetFocusChanged_Implementation(AActor* QueryFromActor, const FHitResult& HitResult, bool bIsFindInteractTarget) override {}
    
    virtual bool CanInteract_Implementation(AActor* QueryFromActor, const FHitResult& HitResult) override { return true; }

    virtual bool StartInteract_Implementation(AActor* QueryFromActor, const FHitResult& HitResult, const FGameplayTag& InteractTypeTag, FInteractCallbackInfo& InteractCallbackInfo) override;
    /*~IInteractInterface*/
    
    /*~IInteractOutlinerInterface*/
    virtual const TArray<UPrimitiveComponent*> GetComponentsForHighlight_Implementation() const override { return TArray<UPrimitiveComponent*>{Mesh}; }
    /*~IInteractOutlinerInterface*/
    
protected:
    virtual void BeginPlay() override;
    
    /** @brief Data table used to initialize default item states if spawned directly into a level. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (RequiredAssetDataTags = "RowStructure=/Script/FlowGear.ItemRow"))
    TObjectPtr<UDataTable> ItemsTable;

    /** @brief Row and count configurations used for baseline initialization. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (EditCondition = "ItemsTable != nullptr"))
    FStartItemInfo ItemInfo;
    
    /** @brief Visual static mesh representation which simulates physics in the scene. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> Mesh;
    
private:
    /** @brief Callback triggered once the item configuration asset has asynchronously loaded. */
    void OnAssetDefinitionLoaded(const FSoftObjectPath& SoftObjectPath, UObject* Object);
    
    /** @brief Bound physics impact callback to stabilize and sleep item physics after dropping. */
    UFUNCTION()
    void OnComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
    
    /** @brief Safely deactivates physical simulation on the static mesh component. */
    void DisableMeshPhysics() const;
    
private:
    /** @brief Handle tracking the physics deactivation schedule. */
    FTimerHandle DeactivatePhysicsHandle;
};