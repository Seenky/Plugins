// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventoryData/Inventory/InventoryData.h"
#include "Objects/ReplicatedObjectBase.h"
#include "StructUtils/InstancedStruct.h"
#include "InventoryItem.generated.h"


UCLASS(Blueprintable)
class INVENTORYSYSTEMPLUGIN_API UInventoryItem : public UReplicatedObjectBase
{
	GENERATED_BODY()

public:
	UInventoryItem(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool IsSupportedForNetworking() const override { return true; }
	virtual void PostInitProperties() override { UReplicatedObjectBase::PostInitProperties(); if (GetWorld()) ObjectBeginPlay(); }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item")
		FORCEINLINE FInventoryItemData& GetItemData() { return ItemData; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item")
		FORCEINLINE FInstancedStruct GetDynamicItemData() const { return DynamicItemData; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item")
		FORCEINLINE FPickupItemData GetPickupItemData() const { return PickupItemData; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item")
		FORCEINLINE FGameplayTag GetItemTag() const { return ItemTag; }

	UFUNCTION(Server, Unreliable)
		FORCEINLINE void Server_SetItemData(const FInventoryItemData InItemData);
		FORCEINLINE void Server_SetItemData_Implementation(const FInventoryItemData InItemData) { ItemData = InItemData; }
	UFUNCTION(Server, Unreliable)
		FORCEINLINE void Server_SetDynamicItemData(const FInstancedStruct InDynamicItemData);
		FORCEINLINE void Server_SetDynamicItemData_Implementation(const FInstancedStruct InDynamicItemData) { DynamicItemData = InDynamicItemData; }
	UFUNCTION(Server, Unreliable, BlueprintCallable, Category = "Item")
		FORCEINLINE void Server_SetIsItemUsing(const bool bIsUsing);
		FORCEINLINE void Server_SetIsItemUsing_Implementation(const bool bIsUsing) { PickupItemData.bIsUsing = bIsUsing; }
	UFUNCTION(Server, Unreliable)
		FORCEINLINE void Server_SetPickupItemData(const FPickupItemData InPickupItemData);
		FORCEINLINE void Server_SetPickupItemData_Implementation(const FPickupItemData InPickupItemData) { PickupItemData = InPickupItemData; }
	UFUNCTION(Server, Unreliable)
		FORCEINLINE void Server_SetItemTag(const FGameplayTag InTag);
		FORCEINLINE void Server_SetItemTag_Implementation(const FGameplayTag InTag) { ItemTag = InTag; }

	UFUNCTION(BlueprintPure, Category = "Item")
		FORCEINLINE bool GetIsItemUsing() const { return PickupItemData.bIsUsing; }

protected:
	UPROPERTY(EditAnywhere, Replicated)
		FInventoryItemData ItemData;
	UPROPERTY(EditAnywhere, Replicated)
		FInstancedStruct DynamicItemData;
	UPROPERTY(EditAnywhere, Replicated)
		FPickupItemData PickupItemData;
	UPROPERTY(EditAnywhere, Replicated)
		FGameplayTag ItemTag;
};
