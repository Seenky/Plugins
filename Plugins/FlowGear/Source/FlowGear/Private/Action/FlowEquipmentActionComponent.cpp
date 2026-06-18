// Fill out your copyright notice in the Description page of Project Settings.

// ReSharper disable CppMemberFunctionMayBeStatic
// ReSharper disable CppMemberFunctionMayBeConst
#include "FlowEquipmentActionComponent.h"

#include "Action/FlowItemAction.h"
#include "Action/FlowItemActionTypes.h"
#include "Equipment/FlowEquipmentComponent.h"
#include "Inventory/FlowItemInstance.h"
#include "Items/FlowEquipItemActor.h"

template<typename T>
void ExecuteTraitActions(AActor* Owner, UFlowItemInstance* ItemInstance)
{
    static_assert(std::is_base_of_v<FItemActionsTraitBase, T>, "T must derive from FItemActionsTraitBase");
    
    if (!Owner || !ItemInstance) return;
    
    if (const T* ActionsTrait = ItemInstance->FindItemTrait<T>())
    {
       for (const auto& Action : ActionsTrait->Actions)
       {
          if (!Action) continue;
          
          Action->ExecuteAction(Owner, ItemInstance, FEquipmentInteractionPayload());
       }
    }
}

UFlowEquipmentActionComponent::UFlowEquipmentActionComponent()
{
    PrimaryComponentTick.bCanEverTick = false; 
}

void UFlowEquipmentActionComponent::BeginPlay()
{
    Super::BeginPlay();
    
    EquipmentComponent = GetOwner()->FindComponentByClass<UFlowEquipmentComponent>();
    if (!EquipmentComponent)
    {
       DestroyComponent();
       return;
    }
	
    EquipmentComponent->OnEquipmentInteraction.AddDynamic(this, &UFlowEquipmentActionComponent::OnEquipmentInteraction);
    EquipmentComponent->OnEquipItem.AddDynamic(this, &UFlowEquipmentActionComponent::OnEquipItem);
    EquipmentComponent->OnUnequipItem.AddDynamic(this, &UFlowEquipmentActionComponent::OnUnequipItem);
    EquipmentComponent->OnDropItem.AddDynamic(this, &UFlowEquipmentActionComponent::OnDropItem);
}

void UFlowEquipmentActionComponent::OnEquipmentInteraction(UFlowItemInstance* ItemInstance, const FEquipmentInteractionPayload& ContextData)
{
    if (!GetOwner() || !ItemInstance) return;
	
	if (const FItemInteractActionsMapTrait* MapTrait = ItemInstance->FindItemTrait<FItemInteractActionsMapTrait>())
	{
		const FGameplayTag& QueryTag = ContextData.InteractionTag;

		if (const FFlowActionEntry* FoundEntry = MapTrait->ActionEntries.FindByKey(QueryTag))
		{
			for (const auto& Action : FoundEntry->Actions)
			{
				if (!Action) continue;
             
				Action->ExecuteAction(GetOwner(), ItemInstance, ContextData);
			}
		}
	}
	
    if (const FItemInteractActionsArrayTrait* ActionsArrayTrait = ItemInstance->FindItemTrait<FItemInteractActionsArrayTrait>())
    {
       for (const auto& Action : ActionsArrayTrait->Actions)
       {
          if (!Action) continue;
             
          Action->ExecuteAction(GetOwner(), ItemInstance, ContextData);
       }
    }
}

void UFlowEquipmentActionComponent::ForceCancelAllActions(UFlowItemInstance* ItemInstance)
{
    if (!ItemInstance) return;
	
	if (const FItemInteractActionsMapTrait* MapTrait = ItemInstance->FindItemTrait<FItemInteractActionsMapTrait>())
	{
		for (const FFlowActionEntry& Entry : MapTrait->ActionEntries)
		{
			for (UFlowItemAction* Action : Entry.Actions)
			{
				if (Action)
				{
					Action->CancelAction();
				}
			}
		}
	}
}

void UFlowEquipmentActionComponent::OnEquipItem(UFlowItemInstance* ItemInstance)
{
    ExecuteTraitActions<FItemEquipActionsTrait>(GetOwner(), ItemInstance);
}

void UFlowEquipmentActionComponent::OnUnequipItem(UFlowItemInstance* ItemInstance)
{
    ForceCancelAllActions(ItemInstance);
    ExecuteTraitActions<FItemUnequipActionsTrait>(GetOwner(), ItemInstance);
}

void UFlowEquipmentActionComponent::OnDropItem(UFlowItemInstance* ItemInstance)
{
    ForceCancelAllActions(ItemInstance);
    ExecuteTraitActions<FItemDropActionsTrait>(GetOwner(), ItemInstance);
}