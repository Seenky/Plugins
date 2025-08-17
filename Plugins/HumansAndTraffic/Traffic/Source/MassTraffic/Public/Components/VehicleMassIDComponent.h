// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VehicleMassIDComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MASSTRAFFIC_API UVehicleMassIDComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UVehicleMassIDComponent();

	UFUNCTION(BlueprintCallable, Category="VehicleMassID")
		void SetVehicleNeedStop(const bool bStop) { bNeedStop = bStop; }

	UFUNCTION(BlueprintPure, Category="VehicleMassID")
		int GetVehicleNeedStop() const { return bNeedStop; }

private:
	bool bNeedStop;
};
