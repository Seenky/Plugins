// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "UsableItemsInteract.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, Blueprintable)
class UUsableItemsInteract : public UInterface
{
	GENERATED_BODY()
};

class INVENTORYSYSTEMPLUGIN_API IUsableItemsInteract
{
	GENERATED_BODY()


public:
	/*Call interact with item in hand by gameplay tag*/
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
		void InventoryItemInteract(FGameplayTagContainer InteractTags, const bool bInteract);
};
