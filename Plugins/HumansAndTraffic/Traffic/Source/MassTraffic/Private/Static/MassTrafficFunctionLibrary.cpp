// Copyright Epic Games, Inc. All Rights Reserved.

#include "Static/MassTrafficFunctionLibrary.h"
#include "Subsystems/MassTrafficSubsystem.h"

#include "MassAgentComponent.h"
#include "MassMovementFragments.h"
#include "ZoneShapeComponent.h"

bool UMassTrafficFunctionLibrary::GetPackedTrafficVehicleInstanceCustomData(const UPrimitiveComponent* PrimitiveComponent, FMassTrafficPackedVehicleInstanceCustomData& OutPackedCustomData, int32 DataIndex)
{
	const TArray<float>& CustomPrimitiveData = PrimitiveComponent->GetCustomPrimitiveData().Data;
	if (CustomPrimitiveData.IsValidIndex(DataIndex))
	{
		OutPackedCustomData = FMassTrafficPackedVehicleInstanceCustomData(CustomPrimitiveData[DataIndex],
																		  CustomPrimitiveData[DataIndex + 1],
																		  CustomPrimitiveData[DataIndex + 2],
																		  CustomPrimitiveData[DataIndex + 3],
																		  CustomPrimitiveData[DataIndex + 4]);
		return true;
	}
	else
	{
		OutPackedCustomData = FMassTrafficPackedVehicleInstanceCustomData();
		return false;
	}
}

void UMassTrafficFunctionLibrary::RemoveVehiclesOverlappingPlayers(UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (UMassTrafficSubsystem* MassTrafficSubsystem = UWorld::GetSubsystem<UMassTrafficSubsystem>(World))
	{
		MassTrafficSubsystem->RemoveVehiclesOverlappingPlayers();
	}
}

void UMassTrafficFunctionLibrary::DespawnNonPlayerDrivenParkedVehicles(AMassSpawner* ParkedVehiclesMassSpawner)
{
	if ((ParkedVehiclesMassSpawner))
	{
		UWorld* World = GEngine->GetWorldFromContextObject(ParkedVehiclesMassSpawner, EGetWorldErrorMode::ReturnNull);
		if (UMassTrafficSubsystem* MassTrafficSubsystem = UWorld::GetSubsystem<UMassTrafficSubsystem>(World))
		{
			TArray<FMassEntityHandle> PlayerVehicleAgentsToPersist;
			MassTrafficSubsystem->GetPlayerVehicleAgents(PlayerVehicleAgentsToPersist);

			ParkedVehiclesMassSpawner->DoDespawning(/*EntitiesToIgnore*/PlayerVehicleAgentsToPersist);
		}
	}
}
