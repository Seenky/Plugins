// Fill out your copyright notice in the Description page of Project Settings.


#include "Processor/MassTrafficSpeedBumpsProcessor.h"

#include "MassRepresentationFragments.h"
#include "Subsystems/MassTrafficGatesSubsystem.h"
#include "Processor/MassTrafficVehicleControlProcessor.h"
#include "MassZoneGraphNavigationFragments.h"
#include "ZoneGraphSubsystem.h"
#include "MassGameplayExternalTraits.h"


UMassTrafficSpeedBumpsProcessor::UMassTrafficSpeedBumpsProcessor()
	:	EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::Client | EProcessorExecutionFlags::Standalone);
	bRequiresGameThreadExecution = false;
	bAutoRegisterWithProcessingPhases = true;
	ExecutionOrder.ExecuteInGroup = UE::MassTraffic::ProcessorGroupNames::VehicleBehavior;
	ExecutionOrder.ExecuteAfter.Add(UE::MassTraffic::ProcessorGroupNames::FrameStart);
	ExecutionOrder.ExecuteAfter.Add(UE::MassTraffic::ProcessorGroupNames::PreVehicleBehavior);
	ExecutionOrder.ExecuteAfter.Add(UE::MassTraffic::ProcessorGroupNames::VehicleSimulationLOD);
	ExecutionOrder.ExecuteAfter.Add(UMassTrafficVehicleControlProcessor::StaticClass()->GetFName());
}

void UMassTrafficSpeedBumpsProcessor::ConfigureQueries()
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassTrafficVehicleControlFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassZoneGraphLaneLocationFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassRepresentationFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddSubsystemRequirement<UMassTrafficGatesSubsystem>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddSubsystemRequirement<UZoneGraphSubsystem>(EMassFragmentAccess::ReadOnly);
}

void UMassTrafficSpeedBumpsProcessor::Execute(FMassEntityManager& EntitySubSystem, FMassExecutionContext& Context)
{
	EntityQuery.ForEachEntityChunk(EntitySubSystem, Context, [&](FMassExecutionContext& ComponentSystemExecutionContext)
	{
		const UZoneGraphSubsystem& ZoneGraphSubsystem = ComponentSystemExecutionContext.GetSubsystemChecked<UZoneGraphSubsystem>();
		UMassTrafficGatesSubsystem& GatesSubsystem = ComponentSystemExecutionContext.GetMutableSubsystemChecked<UMassTrafficGatesSubsystem>();
		
		const TConstArrayView<FTransformFragment> TransformFragments = Context.GetFragmentView<FTransformFragment>();
		const TConstArrayView<FMassZoneGraphLaneLocationFragment> LaneLocationFragments = Context.GetFragmentView<FMassZoneGraphLaneLocationFragment>();
		const TArrayView<FMassTrafficVehicleControlFragment> VehicleControlFragments = Context.GetMutableFragmentView<FMassTrafficVehicleControlFragment>();
		const TArrayView<FMassRepresentationFragment> RepresentationFragments = Context.GetMutableFragmentView<FMassRepresentationFragment>();

		// ReSharper disable once CppTooWideScopeInitStatement
		const TArray<AActor*> SpeedBumps = GatesSubsystem.GetBumpsList();

		if (SpeedBumps.IsEmpty()) return;

		//Iterate over all gate refs to check if car on same lane with gate
		//We should use Context.GetEntity(Index).AsNumber() cause entities can have same Indexes on multiple lanes
		for (const auto SpeedBump : SpeedBumps)
		{
			//Get box component for gate lane check
			UStaticMeshComponent* BumpMeshComponent = SpeedBump->GetComponentByClass<UStaticMeshComponent>();

			if (!BumpMeshComponent) continue;
			if (!BumpMeshComponent->GetStaticMesh()) continue;

			//Find nearest lane with gate box component
			TArray<FZoneGraphLaneHandle> OverlapLanes;
			FBox SpeedBumpBounds = FBox::BuildAABB(BumpMeshComponent->GetComponentLocation(), BumpMeshComponent->Bounds.GetBox().GetExtent() + FVector(0.0f, 0.0f, 90.0f));
			if (BumpMeshComponent)
			{
				ZoneGraphSubsystem.FindOverlappingLanes(
				SpeedBumpBounds,
				FZoneGraphTagFilter(FZoneGraphTagMask::None, FZoneGraphTagMask::None, FZoneGraphTagMask::None),
				OverlapLanes);
			}

			#if WITH_MASSTRAFFIC_DEBUG
			if (GDebugMassTrafficBumps)
			{
				DrawDebugBox(
						GetWorld(),
						SpeedBumpBounds.GetCenter(),
						SpeedBumpBounds.GetExtent(),
						FColor::Green,
						false,
						0.0f,
						1.0f,
						5.0f);
			}
			#endif
			
			if (OverlapLanes.IsEmpty()) continue;

			#if WITH_MASSTRAFFIC_DEBUG
			for (const auto Lane : OverlapLanes)
			{
				if (GDebugMassTrafficBumps)
				{
					FZoneGraphLaneLocation OutLaneLocation;
					float LaneDistance = 0.0f;
					ZoneGraphSubsystem.FindNearestLocationOnLane(Lane, SpeedBumpBounds, OutLaneLocation, LaneDistance);
					
					DrawDebugDirectionalArrow(
						GetWorld(),
						OutLaneLocation.Position,
						OutLaneLocation.Position + OutLaneLocation.Direction * 500,
						120.0f,
						FColor::Emerald,
						false,
						0.0f,
						1.0f,
						10.0f);
				}
			}
			#endif
			
            const int32 NumEntities = Context.GetNumEntities();
            for (int32 Index = 0; Index < NumEntities; ++Index)
            {
            	const FMassRepresentationFragment& RepresentationFragment = RepresentationFragments[Index];
                FMassTrafficVehicleControlFragment& VehicleControlFragment = VehicleControlFragments[Index];
                const FTransformFragment& TransformFragment = TransformFragments[Index];
                const FMassZoneGraphLaneLocationFragment& LaneLocationFragment = LaneLocationFragments[Index];

            	if (RepresentationFragment.CurrentRepresentation == EMassRepresentationType::StaticMeshInstance || RepresentationFragment.CurrentRepresentation == EMassRepresentationType::None)
            	{
            		VehicleControlFragment.bShouldSlowDown = false;
            		return;
            	}

            	for (const auto Lane : OverlapLanes)
            	{
            		//Check if current vehicle lane == gate nearest lan
					if (LaneLocationFragment.LaneHandle.Index == Lane.Index)
					{
						FZoneGraphLaneLocation OutLaneLocation;
						float LaneDistance = 0.0f;
						ZoneGraphSubsystem.FindNearestLocationOnLane(Lane, SpeedBumpBounds, OutLaneLocation, LaneDistance);
						
						FVector ToCarVector = (TransformFragments[Index].GetTransform().GetLocation() - OutLaneLocation.Position).GetSafeNormal();
						const float ToCarDirection = FVector::DotProduct(OutLaneLocation.Direction, ToCarVector) * -1.0f;
						float ToCarDistance = FVector::Distance(BumpMeshComponent->GetComponentLocation(), TransformFragment.GetTransform().GetLocation());
						VehicleControlFragment.DistanceToSpeedBump = ToCarDistance;

						#if WITH_MASSTRAFFIC_DEBUG
						if (GDebugMassTrafficBumps)
						{
							DrawDebugLine(
									GetWorld(),
									OutLaneLocation.Position,
									TransformFragment.GetTransform().GetLocation(),
									FColor::Purple,
									false,
									0.0f,
									1.0f,
									5.0f);

							DrawDebugString(
									GetWorld(),
									OutLaneLocation.Position,
									FString::SanitizeFloat(ToCarDirection),
									nullptr,
									FColor::Purple,
									0.0f,
									0.0f,
									1.0f);
						}
						#endif

						//Add vehicle to gate quarry
						if (ToCarDirection > 0.0f)
						{
							if (ToCarDistance <= MassTrafficSettings->DistanceToSlowDown)
							{
								VehicleControlFragment.bShouldSlowDown = true;
							}
						}
						else if (VehicleControlFragment.bShouldSlowDown)
						{
							if (ToCarDistance >= MassTrafficSettings->DistanceToPassSpeedBump)
							{
								VehicleControlFragment.bShouldSlowDown = false;
							}
						}
					}
            	}
            }
		}
	});
}


