// Fill out your copyright notice in the Description page of Project Settings.


#include "Pawns/Movement/ComplexMovementComponent.h"


// Sets default values for this component's properties
UComplexMovementComponent::UComplexMovementComponent()
{

	PrimaryComponentTick.bCanEverTick = true;
	
	InterpolatedVelocityDirection = FVector::ZeroVector;
}

void UComplexMovementComponent::UpdateMovementInfo(float Time)
{
	Super::UpdateMovementInfo(Time);

	if (!GetOwner()) return;

	FVector CurrentLocation = GetOwner()->GetActorLocation();
	FRotator LocalOwnerRotation = GetOwner()->GetActorRotation();
	FVector InterpolatedVelocity = InterpolatedVelocityDirection;
		
	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this, CurrentLocation, Time, InterpolatedVelocity, LocalOwnerRotation]()
	{
		FVector LocalInterpolatedVelocity;
		float LocalYawValue;
		
		{
			LocalInterpolatedVelocity = FMath::VInterpTo(InterpolatedVelocity, CurrentLocation - PreviousLocation, Time, YawRotateWheelsInterpSpeed);
			LocalYawValue = FRotator::NormalizeAxis((LocalOwnerRotation.Clamp() - LocalInterpolatedVelocity.Rotation().Clamp()).Yaw);
			LocalYawValue = Velocity.Size() < 10.f ? 0.f : LocalYawValue;
		}
		
		AsyncTask(ENamedThreads::GameThread, [this, LocalInterpolatedVelocity, LocalYawValue]()
		{
			// Save Calculated Interpolated Velocity Direction
			InterpolatedVelocityDirection = LocalInterpolatedVelocity;
			// Set Current Yaw Rotation
			CurrentYawRotate = LocalYawValue;
		});
	});
}


// Called when the game starts
void UComplexMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	InterpolatedVelocityDirection = FVector();
	
}


// Called every frame
void UComplexMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                             FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
}

