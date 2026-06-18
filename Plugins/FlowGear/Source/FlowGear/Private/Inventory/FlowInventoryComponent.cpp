// Fill out your copyright notice in the Description page of Project Settings.


// ReSharper disable CppMemberFunctionMayBeStatic
// ReSharper disable CppUseStructuredBinding
// ReSharper disable CppExpressionWithoutSideEffects

#include "Inventory/FlowInventoryComponent.h"

#include "GameplayMessageSubsystem.h"
#include "Inventory/FlowItemInstance.h"
#include "Inventory/FlowItemsHelperLibrary.h"
#include "Inventory/ItemDefinitionAsset.h"
#include "Items/FlowPickupActor.h"


UFlowInventoryComponent::UFlowInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	
	bIsLoaded = false;
	ItemsToLoad = 0;
	LoadedItems = 0;
}

void UFlowInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	
	ConstructStartItems();
	
	if (UGameplayMessageSubsystem* MessageSubsystem = GetWorld()->GetSubsystem<UGameplayMessageSubsystem>())
	{
		FGameplayMessageListenerSignature AddItemListenerSignature;
		AddItemListenerSignature.BindDynamic(this, &UFlowInventoryComponent::OnItemAddMessage);
		MessageSubsystem->RegisterListener(FlowGear_Items_Pickup, AddItemListenerSignature);
	}
}

void UFlowInventoryComponent::ConstructStartItems()
{
	if (!ItemsTable) return;
	
	ItemsToLoad = StartItems.Num();
	
	if (ItemsToLoad == LoadedItems)
	{
		OnInventoryLoaded.Broadcast();
	}
	
	for (const auto& StartItem : StartItems)
	{
		const FItemRow* ItemRow = ItemsTable->FindRow<FItemRow>(StartItem.RowHandle.RowName, nullptr);
		if (!ItemRow) continue;
		
		FLoadSoftObjectPathAsyncDelegate LoadSoftObjectPathDelegate;
		LoadSoftObjectPathDelegate.BindUObject(this, &UFlowInventoryComponent::OnAssetDefinitionLoaded);
		ItemRow->ItemDefinitionAsset.LoadAsync(LoadSoftObjectPathDelegate);
	}
}

void UFlowInventoryComponent::OnAssetDefinitionLoaded(const FSoftObjectPath& SoftObjectPath, UObject* Object)
{
	UItemDefinitionAsset* LoadedDefinitionAsset = Cast<UItemDefinitionAsset>(Object);
	if (!LoadedDefinitionAsset)
	{
		return;
	}
	
	UFlowItemInstance* NewItemInstance = UFlowItemsHelperLibrary::CreateFlowItemInstanceFromDefinition(this, LoadedDefinitionAsset);
	
	AddItemInternal(NewItemInstance);
	
	LoadedItems++;
	
	if (ItemsToLoad == LoadedItems)
	{
		OnInventoryLoaded.Broadcast();
	}
}

void UFlowInventoryComponent::OnItemAddMessage(const FGameplayTag Channel, const FInstancedStruct& Payload)
{
	if (Channel != FlowGear_Items_Pickup) return;
	
	const FItemPickupMessage* Message = Payload.GetPtr<FItemPickupMessage>();
	if (!Message) return;
	
	if (Message->Instigator != GetOwner()) return;
	
	TryPickupItem(Message->PickupActor);
}

bool UFlowInventoryComponent::TryPickupItem(AFlowPickupActor* PickupActor)
{
	if (!PickupActor) return false;

	UFlowItemInstance* InstanceFromWorld = PickupActor->GetItemInstance(); 
	if (!InstanceFromWorld) return false;
	
	if (AddItemInternal(InstanceFromWorld))
	{
		OnAddItem.Broadcast(InstanceFromWorld);
		
		PickupActor->Destroy();
		return true;
	}

	return false;
}

void UFlowInventoryComponent::AddItem(UFlowItemInstance* ItemInstance)
{
	if (AddItemInternal(ItemInstance))
	{
		OnAddItem.Broadcast(ItemInstance);
	}
}

void UFlowInventoryComponent::RemoveItem(UFlowItemInstance* ItemInstance)
{
	if (RemoveItemInternal(ItemInstance))
	{
		OnRemoveItem.Broadcast(ItemInstance);
	}
}

UFlowItemInstance* UFlowInventoryComponent::FindFirstItemWithTags(const FGameplayTag ItemTag, const FGameplayTagContainer ExcludeStateTags)
{
	const FItem* FoundSlot = Items.FindByPredicate([&](const FItem& Slot)
	{
		if (!Slot.ItemInstance) 
		{
			return false;
		}
		
		bool bMatchesTag = Slot.ItemInstance->GetItemTag().MatchesTagExact(ItemTag);
		
		bool bHasExcludedStates = Slot.ItemInstance->HasAnyDynamicStates(ExcludeStateTags);

		return bMatchesTag && !bHasExcludedStates;
	});

	return FoundSlot ? FoundSlot->ItemInstance : nullptr;
}

bool UFlowInventoryComponent::AddItemInternal(UFlowItemInstance* ItemInstance)
{
	if (!ItemInstance) return false;
	
	if (ItemInstance->IsStackable())
	{
		const int32 FoundSlot = FindStackableSlotByTag(ItemInstance->GetItemTag());
		
		if (FoundSlot != INDEX_NONE)
		{
			Items[FoundSlot].ItemsNum++;
			return true; 
		}
	}
	else
	{
		if (FindSlotForItem(ItemInstance) != INDEX_NONE)
		{
			return false;
		}
	}
	
	const int32 NewIndex = Items.Add(FItem(ItemInstance));
	
	if (!ItemInstance->IsStackable())
	{
		ItemToSlotMap.Add(ItemInstance, NewIndex);
	}
    
	ItemInstance->Rename(nullptr, this);
	
	return true;
}

bool UFlowInventoryComponent::RemoveItemInternal(UFlowItemInstance* ItemInstance)
{
	if (!ItemInstance) return false;
    
	const bool bIsStackable = ItemInstance->IsStackable();
	
	const int32 FoundSlot = bIsStackable ? FindStackableSlotByTag(ItemInstance->GetItemTag()) 
										 : FindSlotForItem(ItemInstance);
    
	if (FoundSlot == INDEX_NONE || !Items.IsValidIndex(FoundSlot)) return false;
	
	if (bIsStackable && Items[FoundSlot].ItemsNum > 1)
	{
		Items[FoundSlot].ItemsNum--;
		return true;
	}
	
	const int32 LastIndex = Items.Num() - 1;
	
	if (!bIsStackable)
	{
		ItemToSlotMap.Remove(ItemInstance);
	}
    
	Items.RemoveAtSwap(FoundSlot, 1, EAllowShrinking::No); 
	
	if (FoundSlot != LastIndex && Items.IsValidIndex(FoundSlot))
	{
		UFlowItemInstance* MovedItem = Items[FoundSlot].ItemInstance;
		if (MovedItem && !MovedItem->IsStackable())
		{
			ItemToSlotMap.Add(MovedItem, FoundSlot);
		}
	}
    
	return true;
}

int32 UFlowInventoryComponent::FindStackableSlotByTag(const FGameplayTag& ItemTag)
{
	if (!ItemTag.IsValid()) return INDEX_NONE;
    
	for (int32 i = 0; i < Items.Num(); ++i)
	{
		if (Items[i].ItemInstance && Items[i].ItemInstance->GetItemTag() == ItemTag)
		{
			if (Items[i].ItemsNum < Items[i].ItemInstance->GetStackSize())
			{
				return i;
			}
		}
	}
	return INDEX_NONE;
}

int32 UFlowInventoryComponent::FindSlotForItem(UFlowItemInstance* ItemInstance)
{
	if (!ItemInstance) return INDEX_NONE;
	
	if (const int32* IndexPtr = ItemToSlotMap.Find(ItemInstance))
	{
		return *IndexPtr;
	}
	return INDEX_NONE;
}




