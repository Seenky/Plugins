// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Actors/TrafficHider.h"
#include "Subsystems/WorldSubsystem.h"
#include "MassTrafficOccluderSubsystem.generated.h"

UCLASS()
class MASSTRAFFIC_API UMassTrafficOccluderSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void AddTrafficHider(ATrafficHider* TrafficHider) { TrafficHiders.Add(TrafficHider); }
	TArray<ATrafficHider*> GetTrafficHiders() { return TrafficHiders; }

private:
	UPROPERTY()
	TArray<ATrafficHider*> TrafficHiders;
};
