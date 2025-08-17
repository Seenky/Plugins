// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystem/NiagaraCrowdSubsystem.h"

#include "Subsystem/NiagaraCrowdSubsystemSettings.h"
#include "NiagaraSystem.h"
#include "Kismet/GameplayStatics.h"

UNiagaraCrowdSubsystem::UNiagaraCrowdSubsystem()
{
	if (const auto NiagaraCrowdSettings = GetDefault<UNiagaraCrowdSubsystemSettings>())
	{
		bUseNiagaraParticles = NiagaraCrowdSettings->bUseNiagaraParticlesForCrowd;
		if (const UNiagaraManagerInfoAssetData* Data = NiagaraCrowdSettings->GetManagerSettings())
		{
			PathToAsset = Data->NiagaraAssetPath;
		}
	}
}

UNiagaraCrowdSubsystem::~UNiagaraCrowdSubsystem()
{
}

void UNiagaraCrowdSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UNiagaraCrowdSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
}

void UNiagaraCrowdSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (!bUseNiagaraParticles) return;

	TArray<AActor*> FindActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AHumansHider::StaticClass(), FindActors);
	for (auto Actor : FindActors)
	{
		if (AHumansHider* Hider = Cast<AHumansHider>(Actor))
		{
			HumansHiders.Add(Hider);
		}
	}
	
	NiagaraSystem = LoadObject<UNiagaraSystem>(nullptr, *PathToAsset.ToString());
	const FVector SpawnLocation(0.1f, 0.1f, 0.1f);
	const FRotator SpawnRotation(0.0f, 0.0f, 0.0f);
	const FTransform SpawnTransform(SpawnRotation, SpawnLocation);
	NiagaraManager = InWorld.GetWorld()->SpawnActor<ANiagaraManager>(ANiagaraManager::StaticClass(), SpawnTransform);
	NiagaraManager->SetNiagaraSystem(NiagaraSystem.Get());
	PC = UGameplayStatics::GetPlayerController(InWorld.GetWorld(), 0);
}






