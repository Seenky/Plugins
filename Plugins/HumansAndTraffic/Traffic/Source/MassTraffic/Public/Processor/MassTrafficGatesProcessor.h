// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Fragments/MassTrafficFragments.h"
#include "MassActorSubsystem.h"
#include "MassTrafficProcessorBase.h"
#include "MassTrafficGatesProcessor.generated.h"

UCLASS()
class MASSTRAFFIC_API UMassTrafficGatesProcessor : public UMassTrafficProcessorBase
{
	GENERATED_BODY()

public:
	UMassTrafficGatesProcessor();

protected:
	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager& EntitySubSystem, FMassExecutionContext& Context) override;

	FMassEntityQuery GatesEntityQuery;
};
