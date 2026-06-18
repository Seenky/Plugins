// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FlowItemActorBase.generated.h"

class UFlowItemInstance;

UCLASS(Abstract)
class FLOWGEAR_API AFlowItemActorBase : public AActor
{
	GENERATED_BODY()

public:
	AFlowItemActorBase();
	
	virtual void ConstructItem(UFlowItemInstance* InItemInstance) {}
	
	UFUNCTION(BlueprintPure, Category = "Getter")
	UFlowItemInstance* GetItemInstance() const { return ItemInstance;}
	
protected:
	UPROPERTY()
	TObjectPtr<UFlowItemInstance> ItemInstance;
};
