// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/TrafficHider.h"

#include "Subsystems/MassTrafficOccluderSubsystem.h"


// Sets default values
ATrafficHider::ATrafficHider()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bCanEverTick = true;
	BoxComponent = CreateDefaultSubobject<UBoxComponent>(FName("BoxComponent"));
	RootComponent = BoxComponent;
	BoxComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ATrafficHider::BeginPlay()
{
	Super::BeginPlay();

	UMassTrafficOccluderSubsystem* Subsystem = GetWorld()->GetSubsystem<UMassTrafficOccluderSubsystem>();
	if (Subsystem) Subsystem->AddTrafficHider(this);
}

bool ATrafficHider::IsInBox(const FVector& Location) const
{
	const FVector LocalPoint = BoxComponent->GetComponentTransform().InverseTransformPosition(Location);
	
	return FMath::Abs(LocalPoint.X) <= BoxComponent->GetScaledBoxExtent().X &&
		   FMath::Abs(LocalPoint.Y) <= BoxComponent->GetScaledBoxExtent().Y &&
		   FMath::Abs(LocalPoint.Z) <= BoxComponent->GetScaledBoxExtent().Z;
}


