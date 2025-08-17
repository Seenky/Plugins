// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/CarSplineMovementComponent.h"


UCarSplineMovementComponent::UCarSplineMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	WheelBase = 200.f;  
	MaxSteerAngle = 45.f;  
	MovementSpeed = 500.f; 
	LookAheadDistance = 1000.f; 
}

void UCarSplineMovementComponent::MoveSplineCar(
	USceneComponent* CarBody,
	USceneComponent* FR,
	USceneComponent* FL,
	USceneComponent* BR,
	USceneComponent* BL,
	USplineComponent* Spline)
{
	const FVector SplineDirection = Spline->FindDirectionClosestToWorldLocation(GetOwner()->GetActorLocation(),
																				ESplineCoordinateSpace::World);
	const FRotator SplineRotation = Spline->FindRotationClosestToWorldLocation(GetOwner()->GetActorLocation(),
																				ESplineCoordinateSpace::World);
	
	const FVector NewLocation = GetOwner()->GetActorLocation() + SplineDirection * MovementSpeed;
	const FRotator NewRotation = FMath::RInterpTo(GetOwner()->GetActorRotation(), SplineRotation, GetWorld()->GetDeltaSeconds(), 5.0f);

	GetOwner()->SetActorLocationAndRotation(NewLocation, NewRotation);
}



