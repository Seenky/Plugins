// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/InventoryComponent.h"

#include "Actors/Items/FastenItems/FastenItemActor.h"
#include "Engine/ActorChannel.h"
#include "Engine/AssetManager.h"
#include "Engine/SCS_Node.h"
#include "Engine/StaticMeshActor.h"
#include "Interfaces/InventoryInterface.h"
#include "Interfaces/InventoryOwnerInterface.h"
#include "InventoryData/Weapon/WeaponData.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

TAutoConsoleVariable<int32> CVarEnableInventoryComponentDebug(
	TEXT("Plugin.Inventory.EnableDebug"),    
	0,                               
	TEXT("Enable debug for Inventory Component\n") 
	" 0: Disable debug\n"
	" 1: Enable debug"); 


UInventoryComponent::UInventoryComponent()
	: bIsInventoryLocked(false)
	, bIsDebugEnabled(false)
{
	SetIsReplicatedByDefault(true);
	bReplicateUsingRegisteredSubObjectList = true;
	PrimaryComponentTick.bCanEverTick = true;
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UInventoryComponent, InventorySlots);
	DOREPLIFETIME(UInventoryComponent, CurrentSelectedItem);
	DOREPLIFETIME_CONDITION_NOTIFY(UInventoryComponent, CurrentSelectedItemActor, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME(UInventoryComponent, CurrentSocketName);
	DOREPLIFETIME(UInventoryComponent, ItemTypes);
	DOREPLIFETIME_CONDITION_NOTIFY(UInventoryComponent, CurrentFastenItemActors, COND_None, REPNOTIFY_Always);
}

void UInventoryComponent::TickComponent(float DeltaTime, enum ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsDebugEnabled)
	{
		DrawFastenItemsDebug();
	}
}

#if WITH_EDITOR
void UInventoryComponent::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FProperty* ChangedProperty = PropertyChangedEvent.Property;
	const FName ItemTypesPropertyName = GET_MEMBER_NAME_CHECKED(FItemTypeData, FastenSocketNames);
	if (ItemTypesPropertyName == ChangedProperty->NamePrivate )
	{
		for (auto& Type : ItemTypes)
		{
			if (Type.FastenSocketNames.Num() > Type.MaxCount)
			{
				Type.FastenSocketNames.RemoveAt(Type.FastenSocketNames.Num() - 1, EAllowShrinking::No);
			}
		}
	}
}
#endif

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	FirstInitComponent();

#if !UE_BUILD_SHIPPING
	CVarEnableInventoryComponentDebug->SetOnChangedCallback(
			FConsoleVariableDelegate::CreateUObject(this, &UInventoryComponent::FetchCVars)
		);
#endif
}

void UInventoryComponent::FirstInitComponent_Implementation()
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

void UInventoryComponent::InitStartItems_Implementation()
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

		AddNewFastenItem(ItemTag, InventorySlots[SuitableSlot].Items.Num() - 1);

		if (GetOwner()->HasAuthority())
			OnRep_UpdateFastenItems();
		
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
	if (CurrentSelectedItem && CurrentSelectedItemActor.Get()) CleanupCurrentItem();
	
	CurrentSelectedItem = ItemToSelect;
	
	if (const TSubclassOf<AActor> CurrentItemClass = CurrentSelectedItem->GetItemData().ItemUseClass)
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

		CurrentSelectedItemActor->SetActorHiddenInGame(true);

		if (!CurrentSelectedItemActor->Implements<UInventoryInterface>())
		{
			CleanupCurrentItem();
			return;
		}

		IInventoryInterface::Execute_EnableSimulatePhysics(CurrentSelectedItemActor.Get(), false);

		//Promote item data to selected actor
		IInventoryInterface::Execute_SetInventoryItemObject(CurrentSelectedItemActor.Get(), CurrentSelectedItem);

		CurrentSelectedItemActor->FinishSpawning(SpawnTransform);

		const FItemTypeData ItemType = GetItemTypeByTag(CurrentSelectedItem->GetItemTag());
		CurrentSocketName = ItemType.AttachSocketName;
		
		if (GetOwner()->HasAuthority())
			OnRep_AttachItem();

		UpdateFastenItemsVisibility();

		NetMulticast_PlayEquipSound();
		
		OnInventoryChanged.Broadcast();
	}
}

void UInventoryComponent::OnRep_AttachItem() const
{
	if (!CurrentSelectedItemActor.IsValid()) return;

	USkeletalMeshComponent* TargetMesh =  GetAttachMesh();
	if (!TargetMesh) return;
	
	if (TargetMesh)
	{
		CurrentSelectedItemActor->AttachToComponent(
			TargetMesh, 
			AttachItemRules, 
			CurrentSocketName
		);

		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("ATTACH TO MESH %s, WITH SOCKET %s"), *GetNameSafe(TargetMesh), *CurrentSocketName.ToString()));

		IInventoryInterface::Execute_SetAttachParent(CurrentSelectedItemActor.Get(), TargetMesh);

		CurrentSelectedItemActor->SetActorHiddenInGame(false);
	}
}

//Try to drop selected item on ground
void UInventoryComponent::Server_DropSelectedItem_Implementation()
{
	if (CurrentSelectedItemActor.IsValid() && CurrentSelectedItem)
	{
		if (!GetItemTypeByTag(CurrentSelectedItem->GetItemTag()).bCanBeDropped ||
			!CurrentSelectedItemActor->Implements<UInventoryInterface>()) return;
		
		if (const TSubclassOf<AActor> CurrentItemClass = CurrentSelectedItem->GetItemData().ItemPickupClass)
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

			RemoveFastenItem();

			UInventoryItem* DroppedItem = HandleNewItem(CurrentSelectedItem, DroppedItemActor);
			IInventoryInterface::Execute_SetInventoryItemObject(DroppedItemActor, DroppedItem);
			
			if (!RemoveItem(CurrentSelectedItem))
			{
				CleanupOldItem(DroppedItem, DroppedItemActor);
				return;
			}

			CleanupOldItem(CurrentSelectedItem, CurrentSelectedItemActor.Get());

			IInventoryInterface::Execute_EnableSimulatePhysics(DroppedItemActor, true);
			
			IInventoryInterface::Execute_SetPickUpMesh(DroppedItemActor, CurrentSelectedItem->GetItemData().LightweightMesh);
			
			DroppedItemActor->FinishSpawning(SpawnTransform);

			OnInventoryChanged.Broadcast();
		}
	}
}

void UInventoryComponent::Server_DeselectSelectedItem_Implementation()
{
	if (CurrentSelectedItemActor.IsValid() && CurrentSelectedItem)
	{
		CleanupCurrentItem();
		UpdateFastenItemsVisibility();
	}
}

void UInventoryComponent::Server_ClearInventory_Implementation()
{
	Server_DeselectSelectedItem();

	for (auto Slot : InventorySlots)
	{
		for (const auto Item : Slot.Items)
			RemoveItem(Item);
	}

	InventorySlots.Empty();
}

//Getters
TArray<UInventoryItem*> UInventoryComponent::GetAllInventoryItems()
{
	TArray<UInventoryItem*> CurrentItems;
	CurrentItems.SetNumZeroed(GetValidInventorySize());
	int32 Iterator = 0;
	for (auto Slot : InventorySlots)
	{
		for (int i = 0; i < Slot.Items.Num(); i++)
		{
			CurrentItems.EmplaceAt(Iterator, Slot.Items[i]);
			Iterator++;
		}
	}
	
	return CurrentItems;
}

//----------------------------------------------------------------------------------------------------------------------


//Helper functions for inventory management
//----------------------------------------------------------------------------------------------------------------------

#if !UE_BUILD_SHIPPING
void UInventoryComponent::FetchCVars(IConsoleVariable* Var)
{
	const bool bEnabled = Var->GetBool();
	bIsDebugEnabled = bEnabled;
	if (bEnabled)
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, "Enable Inventory Component Debug");
	else
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, "Disable Inventory Component Debug");
}
#endif

void UInventoryComponent::DrawFastenItemsDebug()
{
#if !UE_BUILD_SHIPPING
	for (const auto FastenItem : CurrentFastenItemActors)
	{
		DrawDebugSphere(GetWorld(), FastenItem.FastenItemActor->GetActorLocation(), 20.0f, 12, FColor::Green, false, 0.0f, 1.0f, 2.0f);
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, TEXT("Enabled Inventory Component Debug"));
	}
#endif
}

void UInventoryComponent::AddNewFastenItem(const FGameplayTag& ItemTag, const int32& ItemIndex)
{
	if (GetFastenItemDataByTagAndIndex(ItemTag, ItemIndex))
	{
		return;
	}
	
	const FItemTypeData ItemType = GetItemTypeByTag(ItemTag);
	
	if (ItemType.FastenSocketNames.IsValidIndex(ItemIndex) && !ItemType.FastenSocketNames[ItemIndex].IsNone())
	{
		UInventoryItem* Item = GetItemByTagAndIndex(ItemTag, ItemIndex);
		if (!Item) return;
		
		if (!Item->GetItemData().LightweightMesh) return;
			
		AFastenItemActor* CurrentSpawnActor = GetWorld()->SpawnActorDeferred<AFastenItemActor>(
												  AFastenItemActor::StaticClass(),
												  FTransform::Identity,
												  nullptr,
												  nullptr,
												  ESpawnActorCollisionHandlingMethod::AlwaysSpawn,
												  ESpawnActorScaleMethod::SelectDefaultAtRuntime);
		
		const FFastenItemsData NewFastenItem(ItemTag, ItemIndex, CurrentSpawnActor, ItemType.FastenSocketNames[ItemIndex]);
		CurrentFastenItemActors.Add(NewFastenItem);
		
		CurrentSpawnActor->GetStaticMeshComponent()->SetVisibility(false);

		CurrentSpawnActor->FinishSpawning(FTransform::Identity);
	}
}

void UInventoryComponent::HideFastenItem(const FGameplayTag& ItemTag, const int32& ItemIndex)
{
	const FFastenItemsData* FastenItemData = GetFastenItemDataByTagAndIndex(ItemTag, ItemIndex);
	if (!FastenItemData || !FastenItemData->FastenItemActor)
	{
		return;
	}
    
	UStaticMeshComponent* MeshComponent = FastenItemData->FastenItemActor->GetStaticMeshComponent();
	if (MeshComponent)
	{
		MeshComponent->SetVisibility(false);
	}
}

void UInventoryComponent::ShowFastenItem(const FGameplayTag& ItemTag, const int32& ItemIndex)
{
	const FFastenItemsData* FastenItemData = GetFastenItemDataByTagAndIndex(ItemTag, ItemIndex);
	if (!FastenItemData || !FastenItemData->FastenItemActor)
	{
		return;
	}
    
	UStaticMeshComponent* MeshComponent = FastenItemData->FastenItemActor->GetStaticMeshComponent();
	if (!MeshComponent)
	{
		return;
	}
    
	MeshComponent->SetVisibility(true);
}

void UInventoryComponent::RemoveFastenItem(const FGameplayTag& ItemTag, const int32& ItemIndex)
{
	int32 FastenItemIndex = INDEX_NONE;
	for (int32 i = 0; i < CurrentFastenItemActors.Num(); i++)
	{
		const FFastenItemsData& FastenData = CurrentFastenItemActors[i];
		if (FastenData.ItemTag == ItemTag && FastenData.ItemIndex == ItemIndex)
		{
			FastenItemIndex = i;
			break;
		}
	}

	if (FastenItemIndex == INDEX_NONE)
	{
		return;
	}
	
	if (CurrentFastenItemActors[FastenItemIndex].FastenItemActor)
	{
		CurrentFastenItemActors[FastenItemIndex].FastenItemActor->Destroy();
	}
	
	CurrentFastenItemActors.RemoveAt(FastenItemIndex);
}

void UInventoryComponent::RemoveFastenItem()
{
	if (!CurrentSelectedItem)
	{
		return;
	}

	const int32 ItemIndex = FindItemIndex(CurrentSelectedItem);
	if (ItemIndex == INDEX_NONE)
	{
		return;
	}

	const FGameplayTag ItemTag = CurrentSelectedItem->GetItemTag();
	
	int32 FastenItemIndex = INDEX_NONE;
	for (int32 i = 0; i < CurrentFastenItemActors.Num(); i++)
	{
		const FFastenItemsData& FastenData = CurrentFastenItemActors[i];
		if (FastenData.ItemTag == ItemTag && FastenData.ItemIndex == ItemIndex)
		{
			FastenItemIndex = i;
			break;
		}
	}

	if (FastenItemIndex == INDEX_NONE)
	{
		return;
	}
	
	if (CurrentFastenItemActors[FastenItemIndex].FastenItemActor)
	{
		CurrentFastenItemActors[FastenItemIndex].FastenItemActor->Destroy();
	}
	
	CurrentFastenItemActors.RemoveAt(FastenItemIndex);
}

void UInventoryComponent::UpdateFastenItemsVisibility()
{
	for (int32 i = 0; i < CurrentFastenItemActors.Num(); i++)
	{
		const FFastenItemsData& FastenItemData = CurrentFastenItemActors[i];
		UInventoryItem* FindItem = GetItemByTagAndIndex(FastenItemData.ItemTag, FastenItemData.ItemIndex);

		if (FindItem == CurrentSelectedItem && FastenItemData.FastenItemActor->GetStaticMeshComponent()->IsVisible())
		{
			HideFastenItem(FastenItemData.ItemTag, FastenItemData.ItemIndex);
		}
		else if (!FastenItemData.FastenItemActor->GetStaticMeshComponent()->IsVisible())
		{
			ShowFastenItem(FastenItemData.ItemTag, FastenItemData.ItemIndex);
		}
	}
}

void UInventoryComponent::OnRep_UpdateFastenItems()
{
	if (CurrentFastenItemActors.IsEmpty())
	{
		return;
	}

	USkeletalMeshComponent* TargetMesh = GetAttachMesh();
	if (!TargetMesh)
	{
		return;
	}
	
	for (int32 i = 0; i < CurrentFastenItemActors.Num(); i++)
	{
	    FFastenItemsData& FastenItemData = CurrentFastenItemActors[i];
		UInventoryItem* FindItem = GetItemByTagAndIndex(FastenItemData.ItemTag, FastenItemData.ItemIndex);

		if (FindItem == CurrentSelectedItem)
		{
			HideFastenItem(FastenItemData.ItemTag, FastenItemData.ItemIndex);
		}
		else
		{
			if (FastenItemData.bIsFasten || !FastenItemData.FastenItemActor) 
			{
				continue;
			}
			
			FastenItemData.FastenItemActor->AttachToComponent(
				TargetMesh, 
				AttachItemRules, 
				FastenItemData.FastenSocketName
			);

			FastenItemData.bIsFasten = true;

			IInventoryInterface::Execute_SetAttachParent(FastenItemData.FastenItemActor, TargetMesh);

			if (FindItem)
			{
				FastenItemData.FastenItemActor->SetStaticMesh(FindItem->GetItemData().LightweightMesh);
				FastenItemData.FastenItemActor->GetStaticMeshComponent()->SetVisibility(true);
			}
		}
	}
}

TArray<FName> UInventoryComponent::GetAvailableMeshNames() const
{
	TArray<FName> ComponentNames;
	
	if (const AActor* Owner = GetOwner())
	{
		TArray<USkeletalMeshComponent*> SkeletalComps;
		Owner->GetComponents(SkeletalComps);

		for (const auto* Comp : SkeletalComps)
		{
			if (Comp)
			{
				ComponentNames.Add(Comp->GetFName());
			}
		}
	}

#if WITH_EDITOR
	if (ComponentNames.Num() == 0)
	{
		if (const UBlueprintGeneratedClass* BPClass = Cast<UBlueprintGeneratedClass>(GetOuter()))
		{
			if (const USimpleConstructionScript* SCS = BPClass->SimpleConstructionScript)
			{
				for (const USCS_Node* Node : SCS->GetAllNodes())
				{
					if (Node && Node->ComponentClass->IsChildOf(USkeletalMeshComponent::StaticClass()))
					{
						ComponentNames.Add(Node->GetVariableName());
					}
				}
			}
			
			if (const AActor* CDO = Cast<AActor>(BPClass->GetDefaultObject()))
			{
				TArray<USkeletalMeshComponent*> SkeletalComps;
				CDO->GetComponents(SkeletalComps);

				for (const auto* Comp : SkeletalComps)
				{
					if (Comp)
					{
						ComponentNames.Add(Comp->GetFName());
					}
				}
			}
		}
	}
#endif

	return ComponentNames;
}

void UInventoryComponent::NetMulticast_PlayEquipSound_Implementation()
{
	if (!GetWorld() || !GetOwner() || !CurrentSelectedItemActor.IsValid()) return;

	USoundBase* EquipSound = nullptr;
	if (EquipItemSound.IsEmpty())
	{
		EquipSound = DefaultEquipSound;
	}
	else
	{
		EquipSound = EquipItemSound.FindRef(GetCurrentItemTag());
		if (!EquipSound)
			EquipSound = DefaultEquipSound;
	}

	if (!EquipSound) return;

	UGameplayStatics::PlaySoundAtLocation(GetWorld(),
		EquipSound,
		CurrentSelectedItemActor->GetActorLocation(),
		FRotator::ZeroRotator
	);
}

FGameplayTag UInventoryComponent::GetCurrentItemTag() const
{
	if (!CurrentSelectedItem) return FGameplayTag::EmptyTag;

	return CurrentSelectedItem->GetItemTag();
}

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

int32 UInventoryComponent::FindItemIndex(const UInventoryItem* InItem)
{
	for (int32 i = 0; i < InventorySlots.Num(); ++i)
	{
		FInventorySlot& Slot = InventorySlots[i];
		for (int32 j = 0; j < Slot.Items.Num(); j++)
		{
			if (Slot.Items[j] == InItem)
				return j;
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
			RemoveReplicatedSubObject(Item);
			Item->ConditionalBeginDestroy();
			InventorySlots[ItemSlot].Items.RemoveSingleSwap(Item, EAllowShrinking::No);
			return true;
		}
	}

	return false;
}

int32 UInventoryComponent::GetFullInventorySize()
{
	int32 Size = 0;
	for (const auto Slot : InventorySlots)
	{
		Size += Slot.MaxCount;
	}

	return Size;
}

int32 UInventoryComponent::GetValidInventorySize()
{
	int32 Size = 0;
	for (const auto Slot : InventorySlots)
	{
		Size += Slot.Items.Num();
	}

	return Size;
}

FFastenItemsData* UInventoryComponent::GetFastenItemDataByTagAndIndex(const FGameplayTag& Tag, const int32& Index)
{
	for (int32 i = 0; i < CurrentFastenItemActors.Num(); i++)
	{
		FFastenItemsData* FastenData = &CurrentFastenItemActors[i];
		if (!FastenData) continue;
		if (FastenData->ItemTag == Tag && FastenData->ItemIndex == Index)
			return FastenData;
	}

	return nullptr;
}

void UInventoryComponent::CleanupOldItem(UInventoryItem* Item, AActor* Owner)
{
	if (Item)
	{
		RemoveReplicatedSubObject(Item);
		Item->ConditionalBeginDestroy();
	}
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
	if (CurrentSelectedItemActor.IsValid())
		CurrentSelectedItemActor->Destroy();
	CurrentSelectedItemActor = nullptr;
	CurrentSelectedItem = nullptr;
	CurrentSocketName = NAME_None;
}

USkeletalMeshComponent* UInventoryComponent::GetAttachMesh() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn) return nullptr;
	
	const bool bIsLocallyControlled = OwnerPawn->IsLocallyControlled();
	USkeletalMeshComponent* TargetMesh =  OwnerPawn->FindComponentByClass<USkeletalMeshComponent>();

	TArray<UActorComponent*> Meshes  = OwnerPawn->K2_GetComponentsByClass(USkeletalMeshComponent::StaticClass());
	for (const auto Mesh : Meshes)
	{
		if (bIsLocallyControlled)
		{
			if (Mesh->GetFName() == FirstPersonMeshName)
				TargetMesh = Cast<USkeletalMeshComponent>(Mesh);
		}
		else
		{
			if (Mesh->GetFName() == ThirdPersonMeshName)
				TargetMesh = Cast<USkeletalMeshComponent>(Mesh);
		}
	}

	return TargetMesh;
}

//----------------------------------------------------------------------------------------------------------------------

