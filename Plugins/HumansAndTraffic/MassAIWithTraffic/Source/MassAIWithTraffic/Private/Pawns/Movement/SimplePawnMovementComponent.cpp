// Fill out your copyright notice in the Description page of Project Settings.


#include "Pawns/Movement/SimplePawnMovementComponent.h"


USimplePawnMovementComponent::USimplePawnMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	PreviousLocation = FVector::ZeroVector;
}

void USimplePawnMovementComponent::UpdateMovementInfo(float Time)
{
	FVector CurrentLocation = GetOwner()->GetActorLocation();
	
	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this, CurrentLocation, Time]()
	{
		FVector CalculatedVelocity;
		
		{
			CalculatedVelocity = (CurrentLocation - PreviousLocation) / Time;
		}
		
		AsyncTask(ENamedThreads::GameThread, [this, CalculatedVelocity, CurrentLocation]()
		{
			// Save Calculated Interpolated Velocity Direction
			Velocity = CalculatedVelocity;
			PreviousLocation = CurrentLocation;
		});
	});
}

void USimplePawnMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	PreviousLocation = GetOwner()->GetActorLocation();
}

void USimplePawnMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                                 FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateMovementInfo(DeltaTime);
}

