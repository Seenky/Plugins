// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Materials/MaterialParameterCollection.h"
#include "TrafficLightsComponent.generated.h"

UCLASS(BlueprintType, Blueprintable)
class MASSTRAFFIC_API UTrafficLightsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	
	// Sets default values for this component's properties
	UTrafficLightsComponent();

	UFUNCTION(BlueprintPure, Category="Light Control")
		UMaterialParameterCollectionInstance* GetLightMPC() const { return GetWorld()->GetParameterCollectionInstance(MPC); }
	UFUNCTION(BlueprintPure, Category="Light Control")
		FName GetLightName() const { return TimeParameterName; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "Settings")
		TObjectPtr<UMaterialParameterCollection> MPC;
	UPROPERTY(EditDefaultsOnly, Category = "Settings")
		FName TimeParameterName = TEXT("ShowVehicleLight");
};
