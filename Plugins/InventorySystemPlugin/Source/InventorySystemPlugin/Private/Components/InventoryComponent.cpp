// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/InventoryComponent.h"

#include "Engine/ActorChannel.h"
#include "Interfaces/InventoryInterface.h"
#include "Interfaces/InventoryOwnerInterface.h"
#include "InventoryData/Weapon/WeaponData.h"
#include "Net/UnrealNetwork.h"


UInventoryComponent::UInventoryComponent()
	: bIsInventoryLocked(false)
{
	SetIsReplicatedByDefault(true);
	bReplicateUsingRegisteredSubObjectList = true;
	PrimaryComponentTick.bCanEverTick = true;
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UInventoryComponent, InventorySlots, COND_OwnerOnly);
	DOREPLIFETIME(UInventoryComponent, CurrentSelectedItem);
	DOREPLIFETIME(UInventoryComponent, CurrentSelectedItemActor);
	DOREPLIFETIME(UInventoryComponent, CurrentSocketName);
	DOREPLIFETIME_CONDITION(UInventoryComponent, ItemTypes, COND_OwnerOnly);
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	FirstInitComponent();
}

void UInventoryComponent::FirstInitComponent()
{
	if (!GetOwner()->HasAuthority()) return;
	
	InventorySlots.Empty();
	
	TMap<FGameplayTag, FItemTypeData> TypeMap;
	for (const FItemTypeData& Type : ItemTypes)
	{
		TypeMap.Add(Type.ItemTag, Type);
	}
	
	for (const auto& Pair : TypeMap)
	{
		const FItemTypeData& Type = Pair.Value;
		for (int32 i = 0; i < Type.MaxCount; ++i)
		{
			InventorySlots.Add(FInventorySlot(Type.ItemTag, Type.MaxCount));
		}
	}

	InitStartItems();

	GEngine->AddOnScreenDebugMessage(
		-1,
		5.f,
		FColor::Green,
		FString::Printf(TEXT("INIT INVENTORY COMPONENT")));
}

void UInventoryComponent::InitStartItems()
{
	if (StartItems.IsEmpty() || !ItemsData) return;
	for (const auto Item : StartItems)
	{
		if (const FItems* FindItem = ItemsData->FindRow<FItems>(Item.RowName, "ItemRow", true))
		{
			UInventoryItem* NewItem = NewObject<UInventoryItem>();
			NewItem->Server_SetItemData(FindItem->ItemData);
			NewItem->Server_SetDynamicItemData(FindItem->DynamicItemData);
			NewItem->Server_SetItemTag(FindItem->ItemTag);
			NewItem->Server_SetPickupItemData(FindItem->PickupItemData);
			AddReplicatedSubObject(NewItem);
			Server_AddItem(NewItem);
		}
	}
}

//Server functions to manage inventory
//----------------------------------------------------------------------------------------------------------------------
//Try to add item to inventory
void UInventoryComponent::Server_AddItem_Implementation(UInventoryItem* Item)
{
	if (!Item) return;
	
	if (!GetOwner() || !GetOwner()->Implements<UInventoryOwnerInterface>() ||
		!IInventoryOwnerInterface::Execute_GetCanUseInventory(GetOwner()))
	{
		return;
	}
	
	const FGameplayTag ItemTag = Item->GetItemTag();
	// ReSharper disable once CppTooWideScopeInitStatement
	const int32 SuitableSlot = FindSuitableSlot(ItemTag);

	if (SuitableSlot != INDEX_NONE)
	{
		InventorySlots[SuitableSlot].Items.Add(HandleNewItem(Item, GetOwner()));
		
		CleanupOldItem(Item, Item->GetOwner());

		OnInventoryChanged.Broadcast();
	}
}

//Try select item from inventory by item slot
void UInventoryComponent::Server_SelectItem_Implementation(const FGameplayTag ItemTag, const int32 ItemIndex)
{
	if (InventorySlots.IsEmpty() || !GetOwner() || !ItemTag.IsValid()) return;

	if (!GetOwner()->Implements<UInventoryOwnerInterface>() ||
		!IInventoryOwnerInterface::Execute_GetCanUseInventory(GetOwner()))
	{
		return;
	}
	
	if (!GetOwner()->FindComponentByClass<USkeletalMeshComponent>()) return;
	
	const int32 ClampedIndex =  FMath::Max(ItemIndex, 0);
	
	UInventoryItem* ItemToSelect = GetItemByTagAndIndex(ItemTag, ClampedIndex);
	if (!ItemToSelect) return;

	if (CurrentSelectedItem ==  ItemToSelect) return;

	//We need to ensure that last selected item was destroyed before select another one
	if (CurrentSelectedItem && CurrentSelectedItemActor) CleanupCurrentItem();
	
	CurrentSelectedItem = ItemToSelect;
	
	if (const TSubclassOf<AActor> CurrentItemClass = CurrentSelectedItem->GetItemData().ItemUseClass.LoadSynchronous())
	{
		//Spawn and attach actor to owner
		const FTransform SpawnTransform = FTransform::Identity;
		CurrentSelectedItemActor = GetWorld()->SpawnActorDeferred<AActor>(
			CurrentItemClass,
			SpawnTransform,
			GetOwner(),
			nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn,
			ESpawnActorScaleMethod::SelectDefaultAtRuntime);

		if (!CurrentSelectedItemActor->Implements<UInventoryInterface>())
		{
			CleanupCurrentItem();
			return;
		}

		IInventoryInterface::Execute_EnableSimulatePhysics(CurrentSelectedItemActor, false);

		//Promote item data to selected actor
		IInventoryInterface::Execute_SetInventoryItemObject(CurrentSelectedItemActor, CurrentSelectedItem);

		CurrentSelectedItemActor->FinishSpawning(SpawnTransform);

		const FItemTypeData ItemType = GetItemTypeByTag(CurrentSelectedItem->GetItemTag());
		CurrentSocketName = ItemType.AttachSocketName;
		
		if (GetOwner()->HasAuthority())
			OnRep_AttachItem();
		
		OnInventoryChanged.Broadcast();
	}
}

void UInventoryComponent::OnRep_AttachItem()
{
	if (!CurrentSelectedItemActor || !GetOwner()) return;

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn) return;
	
	const bool bIsLocallyControlled = OwnerPawn->IsLocallyControlled();
	USkeletalMeshComponent* TargetMesh = nullptr;
	
	if (bIsLocallyControlled)
	{
		TargetMesh = OwnerPawn->FindComponentByClass<USkeletalMeshComponent>();
	}
	else
	{
		TArray<UActorComponent*> ThirdPersonMeshes = OwnerPawn->GetComponentsByTag(
			USkeletalMeshComponent::StaticClass(), 
			ThirdPersonMeshTag
		);
        
		if (ThirdPersonMeshes.Num() > 0)
		{
			TargetMesh = Cast<USkeletalMeshComponent>(ThirdPersonMeshes[0]);
		}
		else
		{
			TargetMesh = Cast<USkeletalMeshComponent>(OwnerPawn->GetRootComponent());
		}
	}
	
	if (TargetMesh)
	{
		CurrentSelectedItemActor->AttachToComponent(
			TargetMesh, 
			AttachItemRules, 
			CurrentSocketName
		);

		IInventoryInterface::Execute_SetAttachParent(CurrentSelectedItemActor, TargetMesh);
	}
}

//Try to drop selected item on ground
void UInventoryComponent::Server_DropSelectedItem_Implementation()
{
	if (CurrentSelectedItemActor && CurrentSelectedItem)
	{
		if (!GetItemTypeByTag(CurrentSelectedItem->GetItemTag()).bCanBeDropped ||
			!CurrentSelectedItemActor->Implements<UInventoryInterface>()) return;
		
		if (const TSubclassOf<AActor> CurrentItemClass = CurrentSelectedItem->GetItemData().ItemPickupClass.LoadSynchronous())
		{
			const FTransform SpawnTransform = CurrentSelectedItemActor->GetActorTransform();
			AActor* DroppedItemActor = GetWorld()->SpawnActorDeferred<AActor>(
				CurrentItemClass,
				SpawnTransform,
				nullptr,
				nullptr,
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn,
				ESpawnActorScaleMethod::SelectDefaultAtRuntime);

			if (!DroppedItemActor->Implements<UInventoryInterface>())
			{
				DroppedItemActor->Destroy();
				return;
			}

			UInventoryItem* DroppedItem = HandleNewItem(CurrentSelectedItem, DroppedItemActor);
			IInventoryInterface::Execute_SetInventoryItemObject(DroppedItemActor, DroppedItem);
			
			if (!RemoveItem(CurrentSelectedItem))
			{
				CleanupOldItem(DroppedItem, DroppedItemActor);
				return;
			}

			CleanupOldItem(CurrentSelectedItem, CurrentSelectedItemActor);

			IInventoryInterface::Execute_EnableSimulatePhysics(DroppedItemActor, true);

			DroppedItemActor->FinishSpawning(SpawnTransform);

			OnInventoryChanged.Broadcast();
		}
	}
}

void UInventoryComponent::Server_DeselectSelectedItem_Implementation()
{
	if (CurrentSelectedItemActor && CurrentSelectedItem)
	{
		CleanupCurrentItem();
	}
}

//Getters
TArray<UInventoryItem*> UInventoryComponent::GetAllInventoryItems()
{
	TArray<UInventoryItem*> CurrentItems;
	for (auto Slot : InventorySlots)
	{
		for (int i = 0; i < Slot.Items.Num(); i++)
		{
			CurrentItems.Add(Slot.Items[i]);
		}
	}
	
	return CurrentItems;
}

//----------------------------------------------------------------------------------------------------------------------

//Helper functions for inventory management
//----------------------------------------------------------------------------------------------------------------------
FItemTypeData UInventoryComponent::GetItemTypeByTag(const FGameplayTag ItemTag)
{
	const FItemTypeData EmptyType;

	for (const auto Type : ItemTypes)
	{
		if (Type.ItemTag == ItemTag) return Type;
	}
	
	return EmptyType;
}

int32 UInventoryComponent::FindSuitableSlot(const FGameplayTag& ItemTag)
{
	for (int32 i = 0; i < InventorySlots.Num(); ++i)
	{
		FInventorySlot& Slot = InventorySlots[i];
		if (Slot.SlotTag == ItemTag && !Slot.IsFull())
		{
			return i;
		}
	}
	return INDEX_NONE;
}

int32 UInventoryComponent::FindSlotByTag(const FGameplayTag ItemTag)
{
	for (int32 i = 0; i < InventorySlots.Num(); ++i)
	{
		FInventorySlot& Slot = InventorySlots[i];
		if (Slot.SlotTag == ItemTag)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

UInventoryItem* UInventoryComponent::GetItemByTagAndIndex(const FGameplayTag ItemTag, const int32 ItemIndex)
{
	const int32 ItemSlot = FindSlotByTag(ItemTag);

	if (ItemSlot != INDEX_NONE)
	{
		FInventorySlot& Slot = InventorySlots[ItemSlot];
		if (Slot.Items.IsValidIndex(ItemIndex))
		{
			return Slot.Items[ItemIndex];
		}
	}

	return nullptr;
}

bool UInventoryComponent::RemoveItem(UInventoryItem* Item)
{
	const int32 ItemSlot = FindSlotByTag(Item->GetItemTag());
	if (ItemSlot != INDEX_NONE)
	{
		if (InventorySlots.IsValidIndex(ItemSlot))
		{
			if (InventorySlots[ItemSlot].Items.IsEmpty()) return false;
			InventorySlots[ItemSlot].Items.RemoveSingleSwap(Item, EAllowShrinking::No);
			return true;
		}
	}

	return false;
}

void UInventoryComponent::CleanupOldItem(UInventoryItem* Item, AActor* Owner)
{
	RemoveReplicatedSubObject(Item);
	Item->ConditionalBeginDestroy();
	if (Owner) Owner->Destroy();
	CurrentSocketName = NAME_None;
}

UInventoryItem* UInventoryComponent::HandleNewItem(const UInventoryItem* Item, AActor* NewOwner)
{
	if (!NewOwner) return nullptr;
	
	UInventoryItem* NewItem = DuplicateObject(Item, this);
	NewItem->Server_SetOwner(NewOwner);
	AddReplicatedSubObject(NewItem);

	return NewItem;
}

void UInventoryComponent::CleanupCurrentItem()
{
	CurrentSelectedItemActor->Destroy();
	CurrentSelectedItemActor = nullptr;
	CurrentSelectedItem = nullptr;
	CurrentSocketName = NAME_None;
}

//----------------------------------------------------------------------------------------------------------------------

