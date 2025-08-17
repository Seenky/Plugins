// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityQuery.h"
#include "MassTrafficProcessorBase.h"
#include "MassTrafficSpeedBumpsProcessor.generated.h"

UCLASS()
class MASSTRAFFIC_API UMassTrafficSpeedBumpsProcessor : public UMassTrafficProcessorBase
{
	GENERATED_BODY()
	
public:
	UMassTrafficSpeedBumpsProcessor();
	
protected:
	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager& EntitySubSystem, FMassExecutionContext& Context) override;

	FMassEntityQuery EntityQuery;
};
