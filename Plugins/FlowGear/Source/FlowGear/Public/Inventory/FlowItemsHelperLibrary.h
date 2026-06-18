// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FlowItemsHelperLibrary.generated.h"


struct FInstancedStruct;
class UItemDefinitionAsset;
class UFlowItemInstance;

UCLASS()
class FLOWGEAR_API UFlowItemsHelperLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "FlowGear|Items Helper")
	static UFlowItemInstance* CreateFlowItemInstanceFromDefinition(UObject* Outer, UItemDefinitionAsset* DefinitionAsset);
};
