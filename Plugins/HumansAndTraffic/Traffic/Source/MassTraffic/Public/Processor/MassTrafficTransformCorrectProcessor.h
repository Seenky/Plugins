// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassCommonFragments.h"
#include "MassRepresentationFragments.h"
#include "MassTrafficProcessorBase.h"
#include "HAL/Runnable.h"
#include "Traits/MassTrafficVehicleSimulationTrait.h"
#include "MassTrafficTransformCorrectProcessor.generated.h"

struct FWheelTraceParameters
{
	float AxleOffsetX;
	float WheelOffsetY;
};

UCLASS()
class MASSTRAFFIC_API UMassTrafficTransformCorrectProcessor : public UMassTrafficProcessorBase
{
	GENERATED_BODY()
	
public:
	UMassTrafficTransformCorrectProcessor();

protected:
	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager& EntitySubSystem, FMassExecutionContext& Context) override;

	FMassEntityQuery EntityQuery;

private:
	int32 FramesToStart = 256;
	int32 CurrentFrames = 0;
	TMap<AActor*, bool> VisibleStates;
	
	FVector CreateMidPoint(const TArray<FVector>& Points);
	FHitResult PerformWheelTrace(
		const FVector& BaseLocation,
		const FVector& ForwardVector,
		const FQuat& Rotation,
		const FWheelTraceParameters& Params,
		float TraceLength,
		float SphereRadius,
		const FCollisionQueryParams& CollisionParams) const;
	void SetupWheelConfig(
		TArray<FWheelTraceParameters>& WheelConfig,
		FMassRepresentationLODFragment LODFragment,
		const FMassTrafficVehicleSimulationParameters& SimulationParams,
		EMassLOD::Type ForceLOD = EMassLOD::Low,
		bool bForceLod = false);
	bool StartWheelTraces(
		TArray<FVector>& FrontPoints,
		TArray<FVector>& RearPoints,
		TArray<FVector>& LeftPoints,
		TArray<FVector>& RightPoints,
		TArray<FVector>& AllHits,
		TArray<FWheelTraceParameters> WheelConfigs,
		FZoneGraphLaneLocation LaneLocation,
		TArray<AActor*> IgnoredActors,
		FTransformFragment TransformFragment,
		FMassTrafficVehicleSimulationParameters SimulationParams,
		FString& OutReason) const;
	void DrawDebugInfo(
		const FVector& MidPoint,
		const FVector& MidPointFront,
		const FVector& MidPointRear,
		const FVector& MidPointLeft,
		const FVector& MidPointRight) const;
};
