// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "PhysInteractActorInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UPhysInteractActorInterface : public UInterface
{
	GENERATED_BODY()
};

class PHYSICSINTERACTIONPLUGIN_API IPhysInteractActorInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable,BlueprintNativeEvent, Category="Phys Interact Actor Interface")
		float GetPhysActorMass();
	UFUNCTION(BlueprintCallable,BlueprintNativeEvent, Category="Phys Interact Actor Interface")
		void PushPhysActor(const FVector& Direction, const float& Strength);
};
