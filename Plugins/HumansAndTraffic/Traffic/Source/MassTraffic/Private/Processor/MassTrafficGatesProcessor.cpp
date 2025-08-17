// Fill out your copyright notice in the Description page of Project Settings.


#include "Processor/MassTrafficGatesProcessor.h"

#include "MassExecutionContext.h"
#include "Processor//MassTrafficChooseNextLaneProcessor.h"
#include "Interfaces/MassTrafficGatesInterface.h"
#include "Subsystems/MassTrafficGatesSubsystem.h"
#include "Processor/MassTrafficLaneChangingProcessor.h"
#include "MassZoneGraphNavigationFragments.h"
#include "Data/MassTrafficGateStates.h"
#include "Static/MassTrafficHelperLibrary.h"
#include "Processor/MassTrafficVehicleControlProcessor.h"
#include "Processor/MassTrafficVehicleVisualizationProcessor.h"
#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "SubmixEffects/AudioMixerSubmixEffectDynamicsProcessor.h"
#include "ZoneGraphSubsystem.h"
#include "MassGameplayExternalTraits.h"


UMassTrafficGatesProcessor::UMassTrafficGatesProcessor()
: GatesEntityQuery(*this)
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

void UMassTrafficGatesProcessor::ConfigureQueries()
{
	GatesEntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	GatesEntityQuery.AddRequirement<FMassTrafficVehicleControlFragment>(EMassFragmentAccess::ReadWrite);
	GatesEntityQuery.AddRequirement<FMassTrafficGatesFragment>(EMassFragmentAccess::ReadWrite);
	GatesEntityQuery.AddRequirement<FMassZoneGraphLaneLocationFragment>(EMassFragmentAccess::ReadWrite);
	GatesEntityQuery.AddRequirement<FMassRepresentationFragment>(EMassFragmentAccess::ReadWrite);
	GatesEntityQuery.AddSubsystemRequirement<UMassTrafficGatesSubsystem>(EMassFragmentAccess::ReadWrite);
	GatesEntityQuery.AddSubsystemRequirement<UZoneGraphSubsystem>(EMassFragmentAccess::ReadOnly);
}

void UMassTrafficGatesProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	const APlayerController* Controller = UGameplayStatics::GetPlayerController(Context.GetWorld(), 0);

	if (!Controller) return;
	
	GatesEntityQuery.ForEachEntityChunk(EntityManager, Context, [&, Controller](FMassExecutionContext& ComponentSystemExecutionContext)
	{
		const UZoneGraphSubsystem& ZoneGraphSubsystem = ComponentSystemExecutionContext.GetSubsystemChecked<UZoneGraphSubsystem>();
		UMassTrafficGatesSubsystem& GatesSubsystem = ComponentSystemExecutionContext.GetMutableSubsystemChecked<UMassTrafficGatesSubsystem>();
		
		const TConstArrayView<FTransformFragment> TransformFragments = Context.GetFragmentView<FTransformFragment>();
		const TConstArrayView<FMassZoneGraphLaneLocationFragment> LaneLocationFragments = Context.GetFragmentView<FMassZoneGraphLaneLocationFragment>();
		const TArrayView<FMassTrafficVehicleControlFragment> VehicleControlFragments = Context.GetMutableFragmentView<FMassTrafficVehicleControlFragment>();
		const TArrayView<FMassTrafficGatesFragment> GatesFragments = Context.GetMutableFragmentView<FMassTrafficGatesFragment>();
		const TArrayView<FMassRepresentationFragment> RepresentationFragments = Context.GetMutableFragmentView<FMassRepresentationFragment>();

		// ReSharper disable once CppTooWideScopeInitStatement
		const TArray<AActor*> Gates = GatesSubsystem.GetGatesList();

		//Iterate over all gate refs to check if car on same lane with gate
		//We should use Context.GetEntity(Index).AsNumber() cause entities can have same Indexes on multiple lanes
		for (const auto Gate : Gates)
		{
			FVector PlayerLocation;
			FRotator PlayerRotation;
			Controller->GetPlayerViewPoint(PlayerLocation, PlayerRotation);
			TArray<AActor*> IgnoredActors = {Controller->GetPawn(), Gate};

			if (FVector::Distance(PlayerLocation, Gate->GetActorLocation()) > MassTrafficSettings->DistanceToGatesWork) continue;
			
			const bool bIsInFov = UMassTrafficHelperLibrary::IsInFOV(Controller, PlayerLocation, Gate->GetActorLocation(), PlayerRotation);
			
			//Get box component for gate lane check
			UBoxComponent* GateBoxComponent = Gate->GetComponentByClass<UBoxComponent>();
			//Get arrow component to check gate direction
			UArrowComponent* GateArrowComponent = Gate->GetComponentByClass<UArrowComponent>();

			if (!GateBoxComponent || !GateArrowComponent ) continue;

			//Find nearest lane with gate box component
			TArray<FZoneGraphLinkedLane> LinkedLanes;
			FZoneGraphLaneLocation FindGateLaneLocation;
			FBox LocalBounds = FBox::BuildAABB(GateBoxComponent->GetComponentTransform().GetLocation(), GateBoxComponent->GetScaledBoxExtent());
			if (GateBoxComponent)
			{
				float FindGateDistance;
				ZoneGraphSubsystem.FindNearestLane(LocalBounds, FZoneGraphTagFilter(), FindGateLaneLocation, FindGateDistance);
				ZoneGraphSubsystem.GetLinkedLanes(
					FindGateLaneLocation.LaneHandle,
					EZoneLaneLinkType::All,
					EZoneLaneLinkFlags::None,
					EZoneLaneLinkFlags::None,
					LinkedLanes);
			}
			
			if (!FindGateLaneLocation.IsValid()) continue;

			#if WITH_MASSTRAFFIC_DEBUG
			if (GDebugMassTrafficGates)
			{
				DrawDebugDirectionalArrow(
					GetWorld(),
					GateBoxComponent->GetComponentLocation(),
					GateBoxComponent->GetComponentLocation() + FindGateLaneLocation.Direction * 500,
					40.0f,
					FColor::Emerald,
					false,
					0.0f,
					20.0f,
					5.0f);
			}
			#endif
			
            const int32 NumEntities = Context.GetNumEntities();
            for (int32 Index = 0; Index < NumEntities; ++Index)
            {
            	const FMassRepresentationFragment& RepresentationFragment = RepresentationFragments[Index];
                FMassTrafficVehicleControlFragment& VehicleControlFragment = VehicleControlFragments[Index];
                const FTransformFragment& TransformFragment = TransformFragments[Index];
                const FMassZoneGraphLaneLocationFragment& LaneLocationFragment = LaneLocationFragments[Index];
            	FMassTrafficGatesFragment& GateFragment = GatesFragments[Index];

            	if (RepresentationFragment.CurrentRepresentation == EMassRepresentationType::StaticMeshInstance || RepresentationFragment.CurrentRepresentation == EMassRepresentationType::None)
            	{
            		VehicleControlFragment.bShouldStop = false;
            		continue;
            	}

            	bool bOnSameLane = false;
            	for (const auto Lane : LinkedLanes)
            	{
            		if (Lane.DestLane.Index == LaneLocationFragment.LaneHandle.Index) bOnSameLane = true;
            	}

            	//Check if current vehicle lane == gate nearest lan
                if (LaneLocationFragment.LaneHandle.Index == FindGateLaneLocation.LaneHandle.Index || bOnSameLane)
                {
                	FVector ToCarVector = (TransformFragments[Index].GetTransform().GetLocation() - GateArrowComponent->GetComponentLocation()).GetSafeNormal();
                	const float Direction = FVector::DotProduct(GateArrowComponent->GetForwardVector(), ToCarVector);

                	//Add vehicle to gate quarry
	                if (FVector::Distance(GateBoxComponent->GetComponentLocation(), TransformFragment.GetTransform().GetLocation()) <= MassTrafficSettings->MaxDistanceToStopBeforeGates && Direction > 0.0f && bIsInFov)
                    {
	                	GateFragment.Gate = Gate;
	                	IMassTrafficGatesInterface::Execute_AddEntityUniqueIDToList(Gate, Context.GetEntity(Index).AsNumber());
                    }
	                else if (Direction < 0.0f || !bIsInFov)
	                {
		                VehicleControlFragment.bShouldStop = false;
	                }
                }
            }

            if (!Gate->Implements<UMassTrafficGatesInterface>()) continue;
			
			//Get current gate state and quarry list
			EMassTrafficGateStates CurrentGateState = IMassTrafficGatesInterface::Execute_GetGateState(Gate);
			TArray<int64> CurrentUniqueIDList = IMassTrafficGatesInterface::Execute_GetEntityUniqueIDList(Gate);

			#if WITH_MASSTRAFFIC_DEBUG
			if (GDebugMassTrafficGates)
			{
				DrawDebugString(
					GetWorld(),
					GateBoxComponent->GetComponentLocation(),
					UEnum::GetValueAsString(CurrentGateState),
					nullptr,
					FColor::Emerald,
					false,
					0.0f);
			}
			#endif

			if (CurrentUniqueIDList.IsEmpty()) continue;

			//Check if gates are pending to close(must close before other car can continue moving)
			if (!IMassTrafficGatesInterface::Execute_GetPendingToClose(Gate))
			{
				float MinDistance = FLT_MAX;
				for (int32 Index = 0; Index < NumEntities; ++Index)
				{
					//Can crash without this validation
					if(!TransformFragments.IsValidIndex(Index)) continue;
					
					const FTransformFragment& TransformFragment = TransformFragments[Index];
					
					//Check if current list are the same as entity indexes on checking lane
					const int64 CurrentUniqueID = Context.GetEntity(Index).AsNumber();
					if (!CurrentUniqueIDList.Contains(CurrentUniqueID)) continue;

					if (CurrentGateState == EMassTrafficGateStates::Closed)
					{
						const float Distance = FVector::Distance(GateBoxComponent->GetComponentLocation(), TransformFragment.GetTransform().GetLocation());
						if (Distance < MinDistance)
						{
							MinDistance = Distance;
							IMassTrafficGatesInterface::Execute_SetNearestEntity(Gate, Context.GetEntity(Index).AsNumber());
						}
					}
				}
				
				for (int32 Index = 0; Index < NumEntities; ++Index)
				{
					if(!TransformFragments.IsValidIndex(Index)) continue;
					
					int64 CurrentUniqueID = Context.GetEntity(Index).AsNumber();
					if (!CurrentUniqueIDList.Contains(CurrentUniqueID))
					{
						continue;
					}

					//We should check if current id is first vehicle, it should move when gates opened, all other vehicles should wait
					FMassTrafficVehicleControlFragment& VehicleControlFragment = VehicleControlFragments[Index];
					if (CurrentUniqueID == IMassTrafficGatesInterface::Execute_GetNearestEntity(Gate))
					{
						const FTransformFragment& TransformFragment = TransformFragments[Index];

						//Need to check gate states to ensure that vehicle wont move while gates closed or moving
						switch (CurrentGateState)
						{
						case EMassTrafficGateStates::Closed:
							VehicleControlFragment.bShouldStop = true;
							break;
						case EMassTrafficGateStates::Moving:
							VehicleControlFragment.bShouldStop = true;
							break;
						case EMassTrafficGateStates::Opened:
							VehicleControlFragment.bShouldStop = false;
							break;
						default:
							break;
						}

						if (IMassTrafficGatesInterface::Execute_GetNearestEntity(Gate) != INDEX_NONE &&
							CurrentGateState == EMassTrafficGateStates::Closed &&
							FVector::Distance(GateBoxComponent->GetComponentLocation(), TransformFragment.GetTransform().GetLocation()) <= MassTrafficSettings->MinDistanceToStopBeforeGates + 100.0f)
						{
							IMassTrafficGatesInterface::Execute_OpenGates(Gate, true);
						}
							
						FVector ToCarVector = (TransformFragments[Index].GetTransform().GetLocation() - GateArrowComponent->GetComponentLocation()).GetSafeNormal();
						float Direction = FVector::DotProduct(GateArrowComponent->GetForwardVector(), ToCarVector);
						if (FVector::Distance(GateBoxComponent->GetComponentLocation(), TransformFragment.GetTransform().GetLocation()) > MassTrafficSettings->DistancePassAfterGates && Direction < 0)
						{
							VehicleControlFragment.bPassedGate = true;
							VehicleControlFragment.bShouldStop = false;
							IMassTrafficGatesInterface::Execute_SetNearestEntity(Gate, INDEX_NONE);
							IMassTrafficGatesInterface::Execute_SendPendingToClose(Gate);
							IMassTrafficGatesInterface::Execute_RemoveEntityFromQueueList(Gate, Index);
							IMassTrafficGatesInterface::Execute_RemoveEntityUniqueIDFromList(Gate, Context.GetEntity(Index).AsNumber());
						}
					}
					else
					{
						VehicleControlFragment.bShouldStop = true;
					}
				}
			}
        }
	});
}


