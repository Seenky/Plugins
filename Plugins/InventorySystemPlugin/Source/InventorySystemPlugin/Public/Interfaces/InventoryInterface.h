// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Objects/InventoryItem.h"
#include "UObject/Interface.h"
#include "InventoryInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, Blueprintable)
class UInventoryInterface : public UInterface
{
	GENERATED_BODY()
};

class INVENTORYSYSTEMPLUGIN_API IInventoryInterface
{
	GENERATED_BODY()

public:
	/*Set object for inventory item, dont call it manually*/
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
		void SetInventoryItemObject(UInventoryItem* Item);
	/*Get Item Object from Item Actor*/
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
		UInventoryItem* GetInventoryItemObject();
	/*Enable or disable collision and physics on item actor*/
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
		void EnableSimulatePhysics(const bool bEnable);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
		void SetAttachParent(USceneComponent* AttachParent);
};
