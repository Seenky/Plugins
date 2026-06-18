// Fill out your copyright notice in the Description page of Project Settings.

// ReSharper disable CppMemberFunctionMayBeConst
#include "Equipment/FlowEquipmentComponent.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Inventory/FlowInventoryComponent.h"
#include "Inventory/FlowItemInstance.h"
#include "Items/FlowEquipItemActor.h"
#include "Items/FlowItemActorBase.h"
#include "Items/FlowPickupActor.h"

UFlowEquipmentComponent::UFlowEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UFlowEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();

	InventoryComponent = GetOwner()->FindComponentByClass<UFlowInventoryComponent>();
	if (!InventoryComponent)
	{
		DestroyComponent();
	}
}

void UFlowEquipmentComponent::SetAttachComponent(USceneComponent* InComponent)
{
	AttachComponent = InComponent;
}

void UFlowEquipmentComponent::Equip(const FEquipRequest Request)
{
	if (!InventoryComponent || Request.SlotName.IsNone() || !AttachComponent)
	{
		return;
	}

	UFlowItemInstance* TargetInstance = Request.ItemInstance;

	if (!TargetInstance)
	{
		if (!Request.ItemTag.IsValid())
		{
			return;
		}

		FGameplayTagContainer Exclude;
		Exclude.AddTag(FlowGear_Equipment_States_Equipped);

		TargetInstance = InventoryComponent->FindFirstItemWithTags(Request.ItemTag, Exclude);
	}

	if (!TargetInstance)
	{
		return;
	}

	FActiveEquipmentSlot* Slot = FindActiveSlot(Request.SlotName);
	if (Slot && TargetInstance == Slot->ItemInstance)
	{
		return;
	}

	AFlowEquipItemActor* NewVisualActor = CreateSlotRepresentation(TargetInstance);
	if (!NewVisualActor)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("FlowEquipment: Failed to create visual representation for item %s! Equip aborted."),
			*TargetInstance->GetItemTag().ToString());
		return;
	}

	NewVisualActor->OnItemInteraction.AddDynamic(this, &UFlowEquipmentComponent::OnItemInteraction);

	if (Slot)
	{
		if (Slot->SpawnedVisualActor)
		{
			Slot->SpawnedVisualActor->OnItemInteraction.RemoveDynamic(
				this,
				&UFlowEquipmentComponent::OnItemInteraction);
			Slot->SpawnedVisualActor->Destroy();
		}
		if (Slot->ItemInstance)
		{
			RemoveMappingContext(*Slot);
			Slot->ItemInstance->RemoveDynamicState(FlowGear_Equipment_States_Equipped);
		}

		Slot->ItemTag = TargetInstance->GetItemTag();
		Slot->ItemInstance = TargetInstance;
		Slot->SpawnedVisualActor = NewVisualActor;
	}
	else
	{
		FActiveEquipmentSlot NewSlot;
		NewSlot.ItemTag = TargetInstance->GetItemTag();
		NewSlot.SlotName = Request.SlotName;
		NewSlot.ItemInstance = TargetInstance;
		NewSlot.SpawnedVisualActor = NewVisualActor;

		Slot = &ActiveSlots.Add_GetRef(NewSlot);
	}

	static const FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, false);
	Slot->SpawnedVisualActor->AttachToComponent(AttachComponent, AttachmentRules, Slot->SlotName);
	
	Slot->SpawnedVisualActor->SetOwner(GetOwner());

	TargetInstance->AddDynamicState(FlowGear_Equipment_States_Equipped);

	AddMappingContext(TargetInstance, *Slot);

	OnEquipItem.Broadcast(TargetInstance);
}

void UFlowEquipmentComponent::Unequip(const FEquipRequest Request)
{
	if (ActiveSlots.IsEmpty())
	{
		return;
	}
	if (!Request.ItemInstance && Request.SlotName.IsNone() && !Request.ItemTag.IsValid())
	{
		return;
	}

	int32 FoundIndex = INDEX_NONE;

	for (int32 i = 0; i < ActiveSlots.Num(); ++i)
	{
		if (Request.ItemInstance && ActiveSlots[i].ItemInstance == Request.ItemInstance)
		{
			FoundIndex = i;
			break;
		}
		if (!Request.SlotName.IsNone() && ActiveSlots[i].SlotName == Request.SlotName)
		{
			FoundIndex = i;
			break;
		}
		if (Request.ItemTag.IsValid() && ActiveSlots[i].ItemTag == Request.ItemTag)
		{
			FoundIndex = i;
			break;
		}
	}

	if (FoundIndex == INDEX_NONE)
	{
		return;
	}

	FActiveEquipmentSlot& SlotToRemove = ActiveSlots[FoundIndex];

	UFlowItemInstance* ItemInstance = SlotToRemove.ItemInstance;

	FTransform LastTransform = FTransform::Identity;
	if (SlotToRemove.SpawnedVisualActor)
	{
		SlotToRemove.SpawnedVisualActor->OnItemInteraction.RemoveDynamic(
			this,
			&UFlowEquipmentComponent::OnItemInteraction);
		LastTransform = SlotToRemove.SpawnedVisualActor->GetTransform();
		SlotToRemove.SpawnedVisualActor->Destroy();
	}

	if (SlotToRemove.ItemInstance)
	{
		SlotToRemove.ItemInstance->RemoveDynamicState(FlowGear_Equipment_States_Equipped);
	}

	RemoveMappingContext(SlotToRemove);

	ActiveSlots.RemoveAtSwap(FoundIndex, 1, EAllowShrinking::No);

	OnUnequipItem.Broadcast(ItemInstance);
}

void UFlowEquipmentComponent::Drop(const FEquipRequest Request)
{
	if (!InventoryComponent || ActiveSlots.IsEmpty())
	{
		return;
	}
	if (!Request.ItemInstance && Request.SlotName.IsNone() && !Request.ItemTag.IsValid())
	{
		return;
	}

	int32 FoundIndex = INDEX_NONE;
	for (int32 i = 0; i < ActiveSlots.Num(); ++i)
	{
		if (Request.ItemInstance && ActiveSlots[i].ItemInstance == Request.ItemInstance)
		{
			FoundIndex = i;
			break;
		}
		if (!Request.SlotName.IsNone() && ActiveSlots[i].SlotName == Request.SlotName)
		{
			FoundIndex = i;
			break;
		}
		if (Request.ItemTag.IsValid() && ActiveSlots[i].ItemTag == Request.ItemTag)
		{
			FoundIndex = i;
			break;
		}
	}

	if (FoundIndex == INDEX_NONE)
	{
		return;
	}

	FActiveEquipmentSlot& SlotToDrop = ActiveSlots[FoundIndex];

	UFlowItemInstance* ItemToDrop = SlotToDrop.ItemInstance;

	FTransform SpawnTransform = FTransform::Identity;
	if (SlotToDrop.SpawnedVisualActor)
	{
		SlotToDrop.SpawnedVisualActor->OnItemInteraction.RemoveDynamic(
			this,
			&UFlowEquipmentComponent::OnItemInteraction);
		SpawnTransform = SlotToDrop.SpawnedVisualActor->GetActorTransform();
		SlotToDrop.SpawnedVisualActor->Destroy();
	}

	if (ItemToDrop)
	{
		ItemToDrop->RemoveDynamicState(FlowGear_Equipment_States_Equipped);
	}

	RemoveMappingContext(SlotToDrop);

	ActiveSlots.RemoveAtSwap(FoundIndex, 1, EAllowShrinking::No);

	AFlowPickupActor* DroppedActor = nullptr;
	if (ItemToDrop)
	{
		DroppedActor = CreatePickupRepresentation(ItemToDrop, SpawnTransform);
		InventoryComponent->RemoveItem(ItemToDrop);
	}
	
	if (DroppedActor)
		OnDropItem.Broadcast(ItemToDrop);
}

void UFlowEquipmentComponent::TriggerInteract(const FEquipRequest Request, const FGameplayTag& InteractTag, const FInputActionValue& Value, const float TriggeredTime, const float ElapsedTime)
{
	if (ActiveSlots.IsEmpty() || !InteractTag.IsValid())
	{
		return;
	}

	FActiveEquipmentSlot* TargetSlot = nullptr;
	for (auto& Slot : ActiveSlots)
	{
		if (Request.ItemInstance && Slot.ItemInstance == Request.ItemInstance)
		{
			TargetSlot = &Slot;
			break;
		}
		if (!Request.SlotName.IsNone() && Slot.SlotName == Request.SlotName)
		{
			TargetSlot = &Slot;
			break;
		}
		if (Request.ItemTag.IsValid() && Slot.ItemTag == Request.ItemTag)
		{
			TargetSlot = &Slot;
			break;
		}
	}

	if (!TargetSlot || !TargetSlot->SpawnedVisualActor)
	{
		return;
	}

	TargetSlot->SpawnedVisualActor->TriggerInteract(InteractTag, Value, TriggeredTime, ElapsedTime);
}

void UFlowEquipmentComponent::TriggerInteractFast(const FEquipRequest Request, const FGameplayTag& InteractTag)
{
	TriggerInteract(Request, InteractTag, FInputActionValue(), 0.0f, 0.0f);
}

void UFlowEquipmentComponent::CompleteInteract(const FEquipRequest Request, const FGameplayTag& InteractTag)
{
	if (ActiveSlots.IsEmpty() || !InteractTag.IsValid())
	{
		return;
	}

	FActiveEquipmentSlot* TargetSlot = nullptr;
	for (auto& Slot : ActiveSlots)
	{
		if (Request.ItemInstance && Slot.ItemInstance == Request.ItemInstance)
		{
			TargetSlot = &Slot;
			break;
		}
		if (!Request.SlotName.IsNone() && Slot.SlotName == Request.SlotName)
		{
			TargetSlot = &Slot;
			break;
		}
		if (Request.ItemTag.IsValid() && Slot.ItemTag == Request.ItemTag)
		{
			TargetSlot = &Slot;
			break;
		}
	}

	if (!TargetSlot || !TargetSlot->SpawnedVisualActor)
	{
		return;
	}
	
	TargetSlot->SpawnedVisualActor->CompleteInteract(InteractTag);
}

FActiveEquipmentSlot* UFlowEquipmentComponent::FindActiveSlotForItemInstance(UFlowItemInstance* ItemInstance)
{
	for (auto& Slot : ActiveSlots)
	{
		if (Slot.ItemInstance == ItemInstance)
		{
			return &Slot;
		}
	}
	return nullptr;
}

FActiveEquipmentSlot* UFlowEquipmentComponent::FindActiveSlot(const FName SlotName)
{
	for (auto& Slot : ActiveSlots)
	{
		if (Slot.SlotName == SlotName)
		{
			return &Slot;
		}
	}
	return nullptr;
}

bool UFlowEquipmentComponent::IsSlotBusy(const FName SlotName)
{
	return FindActiveSlot(SlotName) != nullptr;
}

AFlowEquipItemActor* UFlowEquipmentComponent::CreateSlotRepresentation(UFlowItemInstance* ItemInstance) const
{
	if (!ItemInstance)
	{
		return nullptr;
	}

	TSoftClassPtr<AFlowEquipItemActor> ItemActorClass = ItemInstance->GetItemActorClass();
	if (ItemActorClass.IsNull())
	{
		UE_LOG(LogTemp, Error, TEXT("%hs Invalid item actor class! %s"), __FUNCTION__, *ItemActorClass.ToString());
		return nullptr;
	}

	UClass* LoadedClass = ItemActorClass.LoadSynchronous();
	if (!LoadedClass)
	{
		UE_LOG(LogTemp, Error, TEXT("%hs Invalid loaded item actor class!"), __FUNCTION__);
		return nullptr;
	}

	AFlowEquipItemActor* NewRepresentation = GetWorld()->SpawnActorDeferred<AFlowEquipItemActor>(LoadedClass, FTransform::Identity);
	if (!NewRepresentation)
	{
		UE_LOG(LogTemp, Error, TEXT("%hs Actor wasn't spawned!"), __FUNCTION__);
		return nullptr;
	}

	NewRepresentation->ConstructItem(ItemInstance);
	NewRepresentation->FinishSpawning(FTransform::Identity);

	return NewRepresentation;
}

AFlowPickupActor* UFlowEquipmentComponent::CreatePickupRepresentation(UFlowItemInstance* ItemInstance,
                                                                      const FTransform& Transform) const
{
	if (!ItemInstance)
	{
		return nullptr;
	}

	TSoftClassPtr<AFlowPickupActor> ItemActorClass = ItemInstance->GetPickupActorClass();
	if (ItemActorClass.IsNull())
	{
		UE_LOG(LogTemp, Error, TEXT("%hs Invalid item actor class!"), __FUNCTION__);
		return nullptr;
	}

	UClass* LoadedClass = ItemActorClass.LoadSynchronous();
	if (!LoadedClass)
	{
		UE_LOG(LogTemp, Error, TEXT("%hs Invalid loaded item actor class!"), __FUNCTION__);
		return nullptr;
	}

	AFlowPickupActor* NewRepresentation = GetWorld()->SpawnActorDeferred<AFlowPickupActor>(LoadedClass, Transform);
	if (!NewRepresentation)
	{
		UE_LOG(LogTemp, Error, TEXT("%hs Actor wasn't spawned!"), __FUNCTION__);
		return nullptr;
	}

	NewRepresentation->ConstructItem(ItemInstance);
	NewRepresentation->FinishSpawning(Transform);

	return NewRepresentation;
}

void UFlowEquipmentComponent::OnItemInteraction(UFlowItemInstance* ItemInstance, const FEquipmentInteractionPayload& ContextData)
{
	OnEquipmentInteraction.Broadcast(ItemInstance, ContextData);
}

UEnhancedInputLocalPlayerSubsystem* UFlowEquipmentComponent::GetOwnerEnhancedInputSubsystem() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		return nullptr;
	}

	const APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController());
	if (!PC)
	{
		return nullptr;
	}

	return ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer());
}

void UFlowEquipmentComponent::AddMappingContext(const UFlowItemInstance* ItemInstance, FActiveEquipmentSlot& Slot)
{
	if (!ItemInstance || ItemInstance->GetInputMappingContext().IsNull())
	{
		return;
	}

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(OwnerPawn->Controller);
	if (!PC || !PC->InputComponent)
	{
		return;
	}

	UEnhancedInputComponent* EInputComp = Cast<UEnhancedInputComponent>(PC->InputComponent);
	UEnhancedInputLocalPlayerSubsystem* EISubsystem = GetOwnerEnhancedInputSubsystem();
	if (!EInputComp || !EISubsystem)
	{
		return;
	}

	if (const UInputMappingContext* LoadedIMC = ItemInstance->GetInputMappingContext().LoadSynchronous())
	{
		EISubsystem->AddMappingContext(LoadedIMC, ItemInstance->GetInputPriority());

		TMap<UInputAction*, FGameplayTag> Bindings = ItemInstance->GetInputBindings();

		for (const auto& KVP : Bindings)
		{
			UInputAction* Action = KVP.Key;
			FGameplayTag ActionTag = KVP.Value;

			if (!Action || !ActionTag.IsValid())
			{
				continue;
			}

			CurrentInputRoutes.Add(Action, ActionTag);

			FInputBindingHandle TriggeredHandle = static_cast<FInputBindingHandle>(
				EInputComp->BindAction(
					Action,
					ETriggerEvent::Triggered,
					this,
					&UFlowEquipmentComponent::Native_OnActionTriggered)
			);

			FInputBindingHandle CompletedHandle = static_cast<FInputBindingHandle>(
				EInputComp->BindAction(
					Action,
					ETriggerEvent::Completed,
					this,
					&UFlowEquipmentComponent::Native_OnActionCompleted)
			);

			Slot.BindingHandles.Add(TriggeredHandle);
			Slot.BindingHandles.Add(CompletedHandle);
		}
	}
}

void UFlowEquipmentComponent::RemoveMappingContext(FActiveEquipmentSlot& Slot)
{
	if (!Slot.ItemInstance)
	{
		return;
	}

	if (!Slot.ItemInstance->GetInputMappingContext().IsNull())
	{
		if (UEnhancedInputLocalPlayerSubsystem* EISubsystem = GetOwnerEnhancedInputSubsystem())
		{
			if (UInputMappingContext* OldIMC = Slot.ItemInstance->GetInputMappingContext().LoadSynchronous())
			{
				EISubsystem->RemoveMappingContext(OldIMC);
			}
		}
	}

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn && OwnerPawn->Controller && OwnerPawn->Controller->InputComponent)
	{
		if (UEnhancedInputComponent* EInputComp = Cast<UEnhancedInputComponent>(OwnerPawn->Controller->InputComponent))
		{
			for (const FInputBindingHandle& Handle : Slot.BindingHandles)
			{
				EInputComp->RemoveBinding(Handle);
			}
		}
	}
	Slot.BindingHandles.Empty();

	TMap<UInputAction*, FGameplayTag> Bindings = Slot.ItemInstance->GetInputBindings();
	for (const auto& KVP : Bindings)
	{
		CurrentInputRoutes.Remove(KVP.Key);
	}
}

void UFlowEquipmentComponent::Native_OnActionTriggered(const FInputActionInstance& Instance)
{
	const UInputAction* TriggeredAction = Instance.GetSourceAction();
	if (!TriggeredAction)
	{
		return;
	}

	if (const FGameplayTag* FoundTag = CurrentInputRoutes.Find(TriggeredAction))
	{
		FEquipRequest Request;

		for (const auto& Slot : ActiveSlots)
		{
			if (Slot.ItemInstance && Slot.ItemInstance->GetInputBindings().Contains(TriggeredAction))
			{
				Request.ItemInstance = Slot.ItemInstance;
				TriggerInteract(Request, *FoundTag, Instance.GetValue(), Instance.GetTriggeredTime(), Instance.GetElapsedTime());
			}
		}
	}
}

void UFlowEquipmentComponent::Native_OnActionCompleted(const FInputActionInstance& Instance)
{
	const UInputAction* TriggeredAction = Instance.GetSourceAction();
	if (!TriggeredAction)
	{
		return;
	}

	if (const FGameplayTag* FoundTag = CurrentInputRoutes.Find(TriggeredAction))
	{
		FEquipRequest Request;

		for (const auto& Slot : ActiveSlots)
		{
			if (Slot.ItemInstance && Slot.ItemInstance->GetInputBindings().Contains(TriggeredAction))
			{
				Request.ItemInstance = Slot.ItemInstance;
				CompleteInteract(Request, *FoundTag);
			}
		}
	}
}
