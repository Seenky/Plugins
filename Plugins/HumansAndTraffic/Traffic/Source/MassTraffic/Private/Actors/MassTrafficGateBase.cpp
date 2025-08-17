// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/MassTrafficGateBase.h"

#include "Subsystems/MassTrafficGatesSubsystem.h"
#include "Static/MassTrafficSettings.h"


class UMassTrafficSettings;
class UMassTrafficGatesSubsystem;

AMassTrafficGateBase::AMassTrafficGateBase()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneComponent = CreateDefaultSubobject<USceneComponent>(FName("SceneComponent"));
	SetRootComponent(SceneComponent);
	GateLaneLocationBoxComponent = CreateDefaultSubobject<UBoxComponent>(FName("GateLaneLocationBoxComponent"));
	GateLaneLocationBoxComponent->SetComponentTickEnabled(false);
	GateLaneLocationBoxComponent->SetupAttachment(RootComponent);
	GateDirectionArrowComponent = CreateDefaultSubobject<UArrowComponent>(FName("GateDirectionArrowComponent"));
	GateDirectionArrowComponent->SetComponentTickEnabled(false);
	GateDirectionArrowComponent->SetupAttachment(RootComponent);
	HandStaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(FName("HandStaticMeshComponent"));
	HandStaticMeshComponent->SetComponentTickEnabled(false);
	HandStaticMeshComponent->SetupAttachment(RootComponent);
}

void AMassTrafficGateBase::BeginPlay()
{
	Super::BeginPlay();

	const UMassTrafficSettings* MassTrafficSettings = GetDefault<UMassTrafficSettings>();
	if (!MassTrafficSettings) return;
	
	UMassTrafficGatesSubsystem* GatesSubsystem = GetWorld()->GetSubsystem<UMassTrafficGatesSubsystem>();
	if (GatesSubsystem && ActorHasTag(MassTrafficSettings->GateTag)) GatesSubsystem->AddGateToList(this);
}

void AMassTrafficGateBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void AMassTrafficGateBase::OpenGates_Implementation(bool bOpen)
{
	AsyncTask(ENamedThreads::GameThread, [this, bOpen]()
	{
		OnOpenGate.Broadcast(bOpen);
	});
}



