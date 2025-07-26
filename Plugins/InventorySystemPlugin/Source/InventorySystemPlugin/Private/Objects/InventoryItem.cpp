// Fill out your copyright notice in the Description page of Project Settings.


#include "Objects/InventoryItem.h"

#include "Net/UnrealNetwork.h"


void UInventoryItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	UReplicatedObjectBase::GetLifetimeReplicatedProps(OutLifetimeProps);
	

	DOREPLIFETIME(UInventoryItem, ItemData);
	DOREPLIFETIME(UInventoryItem, DynamicItemData);
	DOREPLIFETIME(UInventoryItem, PickupItemData);
	DOREPLIFETIME(UInventoryItem, ItemTag);
}

