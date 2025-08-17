// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MassExternalSubsystemTraits.h"
#include "MassTrafficGatesSubsystem.generated.h"


UCLASS(BlueprintType)
class MASSTRAFFIC_API UMassTrafficGatesSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UMassTrafficGatesSubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	void SetGatesList(const TArray<AActor*>& Gates) { GatesList = Gates; }
	void AddGateToList(AActor* Gate) { GatesList.Add(Gate); }
	void AddBumpToList(AActor* Bump) { SpeedBumps.Add(Bump); }
	TArray<AActor*> GetGatesList() { return GatesList; }
	TArray<AActor*> GetBumpsList() { return SpeedBumps; }
	
private:
	UPROPERTY()
		TArray<TObjectPtr<AActor>> GatesList;
	UPROPERTY()
		TArray<TObjectPtr<AActor>> SpeedBumps;
};

template<>
struct TMassExternalSubsystemTraits<UMassTrafficGatesSubsystem> final
{
	enum
	{
		GameThreadOnly = false
	};
};
