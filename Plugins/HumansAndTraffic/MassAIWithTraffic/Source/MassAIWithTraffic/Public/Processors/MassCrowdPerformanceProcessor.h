// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassCrowdPerformanceProcessor.generated.h"

UCLASS()
class MASSAIWITHTRAFFIC_API UMassCrowdPerformanceProcessor: public UMassProcessor
{
	GENERATED_BODY()
	
public:
	UMassCrowdPerformanceProcessor();

protected:
	virtual void ConfigureQueries() override;
	virtual void Initialize(UObject& Owner) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery EntityQuery;
};
