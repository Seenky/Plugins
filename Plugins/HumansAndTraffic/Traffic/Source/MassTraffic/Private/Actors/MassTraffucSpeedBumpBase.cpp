// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/MassTraffucSpeedBumpBase.h"

#include "Subsystems/MassTrafficGatesSubsystem.h"


AMassTraffucSpeedBumpBase::AMassTraffucSpeedBumpBase()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SpeedBumpMesh = CreateDefaultSubobject<UStaticMeshComponent>("SpeedBumpMesh");
	SpeedBumpMesh->SetupAttachment(RootComponent);
}

void AMassTraffucSpeedBumpBase::BeginPlay()
{
	Super::BeginPlay();
	
	UMassTrafficGatesSubsystem* GatesSubsystem = GetWorld()->GetSubsystem<UMassTrafficGatesSubsystem>();
	GatesSubsystem->AddBumpToList(this);
}

void AMassTraffucSpeedBumpBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

