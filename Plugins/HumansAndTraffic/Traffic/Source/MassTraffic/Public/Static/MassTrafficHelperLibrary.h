// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MassTrafficHelperLibrary.generated.h"

UCLASS()
class MASSTRAFFIC_API UMassTrafficHelperLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "MassTrafficHelper")
	static bool IsOccluded(
		UWorld* World,
		const TArray<AActor*>& IgnoredActors,
		const FVector& StartLocation,
		const FVector& EndLocation);
	UFUNCTION(BlueprintCallable, Category = "MassTrafficHelper")
	static bool IsInFOV(
		const APlayerController* Controller,
		const FVector& StartLocation,
		const FVector& EndLocation,
		const FRotator& Rotation);
};
