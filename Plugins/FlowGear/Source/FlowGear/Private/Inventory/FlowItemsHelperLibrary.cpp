// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/FlowItemsHelperLibrary.h"
#include "Inventory/FlowItemInstance.h"
#include "Inventory/ItemDefinitionAsset.h"

UFlowItemInstance* UFlowItemsHelperLibrary::CreateFlowItemInstanceFromDefinition(UObject* Outer, UItemDefinitionAsset* DefinitionAsset)
{
	if (!DefinitionAsset) return nullptr;
	
	UFlowItemInstance* NewItemInstance = NewObject<UFlowItemInstance>(Outer);
	NewItemInstance->SetDefinitionAsset(DefinitionAsset);
	
	return NewItemInstance;
}
