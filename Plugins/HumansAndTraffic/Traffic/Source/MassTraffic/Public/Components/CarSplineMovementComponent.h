// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/SplineComponent.h"
#include "CarSplineMovementComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MASSTRAFFIC_API UCarSplineMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCarSplineMovementComponent();

protected:
	UFUNCTION(BlueprintCallable, Category="Car Spline Movement")
		void MoveSplineCar(
			USceneComponent* CarBody,
			USceneComponent* FR,
			USceneComponent* FL,
			USceneComponent* BR,
			USceneComponent* BL,
			USplineComponent* Spline);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Car Spline Movement")
		float WheelBase;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Car Spline Movement")
		float MaxSteerAngle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Car Spline Movement")
		float MovementSpeed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Car Spline Movement")
		float LookAheadDistance;

private:
};
