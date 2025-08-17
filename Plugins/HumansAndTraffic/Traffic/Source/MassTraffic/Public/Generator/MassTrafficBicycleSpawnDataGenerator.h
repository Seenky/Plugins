// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassTrafficVehicleSpawnDataGenerator.h"
#include "MassTrafficBicycleSpawnDataGenerator.generated.h"

UCLASS()
class MASSTRAFFIC_API UMassTrafficBicycleSpawnDataGenerator : public UMassEntitySpawnDataGeneratorBase
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(TitleProperty="Name"))
	TArray<FMassTrafficVehicleSpacing> VehicleTypeSpacings;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float DefaultSpace = 500.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 RandomSeed = 0;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float MinGapBetweenSpaces = 100.0f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float MaxGapBetweenSpaces = 300.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float ObstacleExclusionRadius = 5000.0f;
	
	virtual void Generate(UObject& QueryOwner, TConstArrayView<FMassSpawnedEntityType> EntityTypes, int32 Count, FFinishedGeneratingSpawnDataSignature& FinishedGeneratingSpawnPointsDelegate) const override;

	static bool FindNonOverlappingLanePoints(
		const FZoneGraphStorage& ZoneGraphStorage,
		const FZoneGraphTagFilter& LaneFilter,
		const TArray<FMassTrafficLaneDensity>& LaneDensities,
		const FRandomStream& RandomStream,
		const TArray<FMassTrafficVehicleSpacing>& Spacings,
		float MinGapBetweenSpaces,
		float MaxGapBetweenSpaces,
		TArray<TArray<FZoneGraphLaneLocation>>& OutSpawnPointsPerSpacing,
		bool bShufflePoints = true,
		TFunction<bool(const FZoneGraphStorage&, int32 LaneIndex)> LaneFilterFunction = nullptr,
		TFunction<bool(const FZoneGraphLaneLocation& LaneLocation)> LaneLocationFilterFunction = nullptr);
};
