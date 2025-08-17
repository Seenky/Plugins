// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassTrafficProcessorBase.h"
#include "Fragments/MassTrafficFragments.h"
#include "MassCommonFragments.h"
#include "MassTrafficInitTrafficVehicleSpeedProcessor.generated.h"


UCLASS()
class MASSTRAFFIC_API UMassTrafficInitTrafficVehicleSpeedProcessor : public UMassTrafficProcessorBase
{
	GENERATED_BODY()

public:
	UMassTrafficInitTrafficVehicleSpeedProcessor();

protected:
	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager& EntitySubSystem, FMassExecutionContext& Context) override;

	FMassEntityQuery EntityQuery;
};
