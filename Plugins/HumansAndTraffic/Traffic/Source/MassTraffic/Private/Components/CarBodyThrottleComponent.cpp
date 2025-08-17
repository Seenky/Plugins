// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/CarBodyThrottleComponent.h"

#include "Kismet/KismetMathLibrary.h"


// Sets default values for this component's properties
UCarBodyThrottleComponent::UCarBodyThrottleComponent()
	:InitialRotation(),
	CurrentTargetRotation()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	MaxBodyPitchThrottle = 2.5;
	MaxBodyRollThrottle = 1;
	bIsAccelerating = false;
	bIsBraking = false;
	TargetRoll = 0.0f;
	BackInterpSpeed = 6.0f;
}


// Called when the game starts
void UCarBodyThrottleComponent::BeginPlay()
{
	Super::BeginPlay();

	if(Body)
	{
		InitialRotation = Body->GetRelativeRotation();
		CurrentTargetRotation = InitialRotation;
	}
}

void UCarBodyThrottleComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	Body = nullptr;
}


// Called every frame
void UCarBodyThrottleComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                              FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
}

void UCarBodyThrottleComponent::RotateCarCorpus(const FMassTrafficVehiclePhysicsFragment* PhysicsFragment) const
{
	if (!Body)
		return;
	const FVector AngularVelocity = PhysicsFragment->AngularVelocity;

	const FQuat CurrentCenterOfRotation = Body->GetRelativeRotation().Quaternion();
	const float DeltaTime = GetWorld()->GetDeltaSeconds();
	
	FQuat IntegratedRotation =  Chaos::FRotation3::IntegrateRotationWithAngularVelocity(CurrentCenterOfRotation, AngularVelocity, DeltaTime);

	if (AngularVelocity.X <= 0.01)
	{
		IntegratedRotation.X = FMath::Lerp(CurrentCenterOfRotation.X, 0, DeltaTime * BackInterpSpeed);
	}

	if (AngularVelocity.Y <= 0.015)
	{
		IntegratedRotation.Y = FMath::Lerp(CurrentCenterOfRotation.Y, 0, DeltaTime * BackInterpSpeed);
	}

	IntegratedRotation = Chaos::FRotation3::Negate(IntegratedRotation);
	FRotator NewRotation = IntegratedRotation.Rotator();
	NewRotation.Pitch = UKismetMathLibrary::ClampAngle(NewRotation.Pitch, -MaxBodyPitchThrottle, MaxBodyPitchThrottle);
	NewRotation.Roll = UKismetMathLibrary::ClampAngle(NewRotation.Roll, -MaxBodyRollThrottle, MaxBodyRollThrottle);
	NewRotation.Yaw = Body->GetRelativeRotation().Yaw;
	
	Body->SetRelativeRotation(NewRotation);
}




