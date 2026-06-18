// Fill out your copyright notice in the Description page of Project Settings.


#include "Action/FlowItemAction.h"

#include "Action/FlowItemActionTypes.h"
#include "Equipment/FlowEquipmentTypes.h"

UFlowItemAction::UFlowItemAction()
{
	SupportedActions = {
		FInstancedStruct::Make(FItemInteractActionsMapTrait()),
		FInstancedStruct::Make(FItemInteractActionsArrayTrait()),
		FInstancedStruct::Make(FItemEquipActionsTrait()),
		FInstancedStruct::Make(FItemUnequipActionsTrait()),
		FInstancedStruct::Make(FItemDropActionsTrait()),
	};
}

UWorld* UFlowItemAction::GetWorld() const
{
	if (WorldPrivate) return WorldPrivate;
	
	if (const UActorComponent* Comp = GetTypedOuter<UActorComponent>())
	{
		return Comp->GetWorld();
	}
	if (const AActor* Actor = GetTypedOuter<AActor>())
	{
		return Actor->GetWorld();
	}

	return nullptr;
}

void UFlowItemAction::ExecuteAction_Implementation(AActor* Owner, UFlowItemInstance* ItemInstance, const FEquipmentInteractionPayload& Payload)
{
	if (Owner)
	{
		WorldPrivate = Owner->GetWorld();
	}
}
