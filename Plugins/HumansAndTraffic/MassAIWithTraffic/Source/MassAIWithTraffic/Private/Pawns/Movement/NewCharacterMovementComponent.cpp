// Fill out your copyright notice in the Description page of Project Settings.


#include "Pawns/Movement/NewCharacterMovementComponent.h"


// Sets default values for this component's properties
UNewCharacterMovementComponent::UNewCharacterMovementComponent()
{

	PrimaryComponentTick.bCanEverTick = true;

	PreviousLocation = FVector::ZeroVector;
}

void UNewCharacterMovementComponent::UpdateMovementInfo(float Time)
{
	const FVector CurrentLocation = GetOwner()->GetActorLocation();

	const FVector CalculatedVelocity = (CurrentLocation - PreviousLocation) / Time;
		
	Velocity = CalculatedVelocity;
	PreviousLocation = CurrentLocation;
}


void UNewCharacterMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	
	PreviousLocation = GetOwner()->GetActorLocation();
}

void UNewCharacterMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                                   FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateMovementInfo(DeltaTime);
}

