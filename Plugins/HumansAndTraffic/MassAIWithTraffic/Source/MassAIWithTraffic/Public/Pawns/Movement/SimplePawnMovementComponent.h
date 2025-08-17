// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "SimplePawnMovementComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MASSAIWITHTRAFFIC_API USimplePawnMovementComponent : public UPawnMovementComponent
{
	GENERATED_BODY()

public:
	USimplePawnMovementComponent();

	virtual void UpdateMovementInfo(float Time);

	const FVector GetPreviousLocation() const
	{
		return PreviousLocation;
	}

protected:
	virtual void BeginPlay() override;

	FVector PreviousLocation;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
};
