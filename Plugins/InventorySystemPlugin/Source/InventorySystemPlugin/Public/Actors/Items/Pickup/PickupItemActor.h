// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Actors/Items/PickupItemActorBase.h"
#include "PickupItemActor.generated.h"

UCLASS(Abstract)
class INVENTORYSYSTEMPLUGIN_API APickupItemActor : public APickupItemActorBase
{
	GENERATED_BODY()

public:
	APickupItemActor();
};
