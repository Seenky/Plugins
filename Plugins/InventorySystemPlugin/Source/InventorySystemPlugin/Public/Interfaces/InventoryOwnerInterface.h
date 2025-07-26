// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InventoryOwnerInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UInventoryOwnerInterface : public UInterface
{
	GENERATED_BODY()
};

class INVENTORYSYSTEMPLUGIN_API IInventoryOwnerInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Inventory|Getter")
		bool GetCanUseInventory();
};
