// Copyright Epic Games, Inc. All Rights Reserved.


#include "Processor/MassTrafficVehicleControlProcessor.h"

#include "MassTraffic.h"
#include "Fragments/MassTrafficFragments.h"
#include "Static/MassTrafficInterpolation.h"
#include "Static/MassTrafficLaneChange.h"
#include "Static/MassTrafficMovement.h"
#include "MassExecutionContext.h"
#include "MassClientBubbleHandler.h"
#include "MassLODUtils.h"
#include "MassZoneGraphNavigationFragments.h"
#include "ZoneGraphSubsystem.h"
#include "ZoneGraphTypes.h"
#include "MassGameplayExternalTraits.h"
#include "MassMovementDelegates.h"
#include "Components/BoxComponent.h"
#include "Components/VehicleMassIDComponent.h"

namespace
{
	// (See all READYLANE.)
	void SetIsVehicleReadyToUseNextIntersectionLane(
		const FMassTrafficVehicleControlFragment& VehicleControlFragment,
		const FMassZoneGraphLaneLocationFragment& LaneLocationFragment,
		const FAgentRadiusFragment& RadiusFragment,
		const FMassTrafficRandomFractionFragment& RandomFractionFragment,
		const FVector2D& StoppingDistanceRange,
		const bool bVehicleHasNoRoom)
	{
		if (!VehicleControlFragment.NextLane ||
			!VehicleControlFragment.NextLane->ConstData.bIsIntersectionLane)
		{
			return;
		}

		const float DistanceAlongLaneToStopAt = UE::MassTraffic::GetDistanceAlongLaneToStopAt(
			RadiusFragment.Radius,
			LaneLocationFragment.LaneLength,
			RandomFractionFragment.RandomFraction,
			StoppingDistanceRange);

		if (LaneLocationFragment.DistanceAlongLane < DistanceAlongLaneToStopAt - 150.0f/*1m safety fudge*/)
		{
			return;
		}
		
		VehicleControlFragment.NextLane->bIsVehicleReadyToUseLane = !bVehicleHasNoRoom; // (See all READYLANE.)
	}

	
	// (See all CANTSTOPLANEEXIT.)
	void SetVehicleCantStopAtLaneExit(
		FMassTrafficVehicleControlFragment& VehicleControlFragment,
		const FMassZoneGraphLaneLocationFragment& LaneLocationFragment,
		const FMassTrafficNextVehicleFragment& NextVehicleFragment,
		const FMassEntityManager& EntityManager)
	{
		// Return if -
		//		- This vehicle is already marked as being unable to stop at the lane exit.
		//		- Or, it has no next lane.
		if (VehicleControlFragment.bCantStopAtLaneExit || !VehicleControlFragment.NextLane)			
		{
			return;
		}
		
		
		if (NextVehicleFragment.GetNextVehicle().IsSet())
		{
			const FMassEntityView NextVehicleEntityView(EntityManager, NextVehicleFragment.GetNextVehicle());
			const FMassZoneGraphLaneLocationFragment& NextLaneLocationFragment = NextVehicleEntityView.GetFragmentData<FMassZoneGraphLaneLocationFragment>();
			const FMassTrafficVehicleControlFragment& NextVehicleControlFragment = NextVehicleEntityView.GetFragmentData<FMassTrafficVehicleControlFragment>();
			
			// Return if we're not at the front of the lane, and the next vehicle isn't going to continue through a closed
			// lane - which means we can't either. (It may happen that the vehicle behind checks this before the vehicle
			// ahead has, and then later the vehicle ahead will mark itself as being unable to stop. But on the next frame,
			// the vehicle behind should then be able to see that the vehicle ahead can't stop, if it decides not to stop
			// either.
			const bool bIsVehicleAtFrontOfLane = (LaneLocationFragment.LaneHandle != NextLaneLocationFragment.LaneHandle);
			const bool bNextVehicleCantStop = NextVehicleControlFragment.bCantStopAtLaneExit;
			if (!bIsVehicleAtFrontOfLane && bNextVehicleCantStop)
			{
				return;
			}
		}
		
		VehicleControlFragment.bCantStopAtLaneExit = true; // (See all CANTSTOPLANEEXIT.)		
		++VehicleControlFragment.NextLane->NumReservedVehiclesOnLane;
	}

	// (See all CANTSTOPLANEEXIT.)
	void UnsetVehicleCantStopAtLaneExit(FMassTrafficVehicleControlFragment& VehicleControlFragment)
	{
		// Return if -
		//		- This vehicle is not marked as being unable to stop at the lane exit.
		//		- Or, it has no next lane.
		if (!VehicleControlFragment.bCantStopAtLaneExit || !VehicleControlFragment.NextLane)			
		{
			UE_LOG(LogTemp, Warning, TEXT("UNSET FAIL (cantstop:%d, next:0x%x)"), VehicleControlFragment.bCantStopAtLaneExit, VehicleControlFragment.NextLane);
			return;
		}

		VehicleControlFragment.bCantStopAtLaneExit = false; // (See all CANTSTOPLANEEXIT.)
		--VehicleControlFragment.NextLane->NumReservedVehiclesOnLane;
	}
}


UMassTrafficVehicleControlProcessor::UMassTrafficVehicleControlProcessor()
	: SimpleVehicleControlEntityQuery_Conditional(*this)
	, PIDVehicleControlEntityQuery_Conditional(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ExecutionOrder.ExecuteInGroup = UE::MassTraffic::ProcessorGroupNames::VehicleBehavior;
	ExecutionOrder.ExecuteAfter.Add(UE::MassTraffic::ProcessorGroupNames::FrameStart);
	ExecutionOrder.ExecuteAfter.Add(UE::MassTraffic::ProcessorGroupNames::PreVehicleBehavior);
	ExecutionOrder.ExecuteAfter.Add(UE::MassTraffic::ProcessorGroupNames::VehicleSimulationLOD);
}

void UMassTrafficVehicleControlProcessor::ConfigureQueries()
{
	SimpleVehicleControlEntityQuery_Conditional.AddTagRequirement<FMassTrafficVehicleTag>(EMassFragmentPresence::Any);
	SimpleVehicleControlEntityQuery_Conditional.AddRequirement<FMassTrafficPIDVehicleControlFragment>(EMassFragmentAccess::None, EMassFragmentPresence::None);
	SimpleVehicleControlEntityQuery_Conditional.AddRequirement<FAgentRadiusFragment>(EMassFragmentAccess::ReadOnly);
	SimpleVehicleControlEntityQuery_Conditional.AddRequirement<FMassTrafficRandomFractionFragment>(EMassFragmentAccess::ReadOnly);
	SimpleVehicleControlEntityQuery_Conditional.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	SimpleVehicleControlEntityQuery_Conditional.AddRequirement<FMassTrafficVehicleControlFragment>(EMassFragmentAccess::ReadWrite);
	SimpleVehicleControlEntityQuery_Conditional.AddRequirement<FMassTrafficVehicleLightsFragment>(EMassFragmentAccess::ReadWrite);
	SimpleVehicleControlEntityQuery_Conditional.AddRequirement<FMassZoneGraphLaneLocationFragment>(EMassFragmentAccess::ReadWrite);
	SimpleVehicleControlEntityQuery_Conditional.AddRequirement<FMassTrafficLaneOffsetFragment>(EMassFragmentAccess::ReadWrite);
	SimpleVehicleControlEntityQuery_Conditional.AddRequirement<FMassTrafficObstacleAvoidanceFragment>(EMassFragmentAccess::ReadWrite);
	SimpleVehicleControlEntityQuery_Conditional.AddRequirement<FMassTrafficNextVehicleFragment>(EMassFragmentAccess::ReadWrite);
	SimpleVehicleControlEntityQuery_Conditional.AddRequirement<FMassTrafficDebugFragment>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::Optional);
	SimpleVehicleControlEntityQuery_Conditional.AddRequirement<FMassTrafficVehicleLaneChangeFragment>(EMassFragmentAccess::ReadWrite, EMassFragmentPresence::Optional);
	SimpleVehicleControlEntityQuery_Conditional.AddRequirement<FMassSimulationVariableTickFragment>(EMassFragmentAccess::ReadOnly);
	SimpleVehicleControlEntityQuery_Conditional.AddChunkRequirement<FMassSimulationVariableTickChunkFragment>(EMassFragmentAccess::ReadOnly);
	SimpleVehicleControlEntityQuery_Conditional.SetChunkFilter(FMassSimulationVariableTickChunkFragment::ShouldTickChunkThisFrame);
	SimpleVehicleControlEntityQuery_Conditional.AddSubsystemRequirement<UMassTrafficSubsystem>(EMassFragmentAccess::ReadWrite);
	SimpleVehicleControlEntityQuery_Conditional.AddSubsystemRequirement<UZoneGraphSubsystem>(EMassFragmentAccess::ReadOnly);
	SimpleVehicleControlEntityQuery_Conditional.AddRequirement<FMassActorFragment>(EMassFragmentAccess::ReadWrite);
	SimpleVehicleControlEntityQuery_Conditional.AddRequirement<FMassTrafficGatesFragment>(EMassFragmentAccess::ReadWrite);

	PIDVehicleControlEntityQuery_Conditional.AddTagRequirement<FMassTrafficVehicleTag>(EMassFragmentPresence::Any);
	PIDVehicleControlEntityQuery_Conditional.AddRequirement<FMassTrafficPIDVehicleControlFragment>(EMassFragmentAccess::ReadWrite);
	PIDVehicleControlEntityQuery_Conditional.AddRequirement<FMassTrafficObstacleAvoidanceFragment>(EMassFragmentAccess::ReadOnly);
	PIDVehicleControlEntityQuery_Conditional.AddRequirement<FAgentRadiusFragment>(EMassFragmentAccess::ReadOnly);
	PIDVehicleControlEntityQuery_Conditional.AddRequirement<FMassTrafficRandomFractionFragment>(EMassFragmentAccess::ReadOnly);
	PIDVehicleControlEntityQuery_Conditional.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	PIDVehicleControlEntityQuery_Conditional.AddRequirement<FMassTrafficVehicleControlFragment>(EMassFragmentAccess::ReadWrite);
	PIDVehicleControlEntityQuery_Conditional.AddRequirement<FMassTrafficVehicleLightsFragment>(EMassFragmentAccess::ReadWrite);
	PIDVehicleControlEntityQuery_Conditional.AddRequirement<FMassZoneGraphLaneLocationFragment>(EMassFragmentAccess::ReadWrite);
	PIDVehicleControlEntityQuery_Conditional.AddRequirement<FMassTrafficLaneOffsetFragment>(EMassFragmentAccess::ReadWrite);
	PIDVehicleControlEntityQuery_Conditional.AddRequirement<FMassTrafficPIDControlInterpolationFragment>(EMassFragmentAccess::ReadWrite);
	PIDVehicleControlEntityQuery_Conditional.AddRequirement<FMassTrafficNextVehicleFragment>(EMassFragmentAccess::ReadWrite);
	PIDVehicleControlEntityQuery_Conditional.AddRequirement<FMassTrafficDebugFragment>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::Optional);
	PIDVehicleControlEntityQuery_Conditional.AddRequirement<FMassTrafficVehicleLaneChangeFragment>(EMassFragmentAccess::ReadWrite, EMassFragmentPresence::Optional);
	PIDVehicleControlEntityQuery_Conditional.AddRequirement<FMassSimulationVariableTickFragment>(EMassFragmentAccess::ReadOnly);
	PIDVehicleControlEntityQuery_Conditional.AddChunkRequirement<FMassSimulationVariableTickChunkFragment>(EMassFragmentAccess::ReadOnly);
	PIDVehicleControlEntityQuery_Conditional.SetChunkFilter(FMassSimulationVariableTickChunkFragment::ShouldTickChunkThisFrame);
	PIDVehicleControlEntityQuery_Conditional.AddSubsystemRequirement<UZoneGraphSubsystem>(EMassFragmentAccess::ReadOnly);
	PIDVehicleControlEntityQuery_Conditional.AddRequirement<FMassActorFragment>(EMassFragmentAccess::ReadWrite);
	PIDVehicleControlEntityQuery_Conditional.AddRequirement<FMassTrafficGatesFragment>(EMassFragmentAccess::ReadWrite);
}


void UMassTrafficVehicleControlProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	// Advance simple agents
	SimpleVehicleControlEntityQuery_Conditional.ForEachEntityChunk(EntityManager, Context, [&](FMassExecutionContext& ComponentSystemExecutionContext)
		{
			const UZoneGraphSubsystem& ZoneGraphSubsystem = ComponentSystemExecutionContext.GetSubsystemChecked<UZoneGraphSubsystem>();
		
			UMassTrafficSubsystem& MassTrafficSubsystem = ComponentSystemExecutionContext.GetMutableSubsystemChecked<UMassTrafficSubsystem>();
			const TConstArrayView<FMassSimulationVariableTickFragment> VariableTickFragments = Context.GetFragmentView<FMassSimulationVariableTickFragment>();
			const TConstArrayView<FMassTrafficRandomFractionFragment> RandomFractionFragments = Context.GetFragmentView<FMassTrafficRandomFractionFragment>();
			const TConstArrayView<FTransformFragment> TransformFragments = Context.GetFragmentView<FTransformFragment>();
			const TConstArrayView<FAgentRadiusFragment> RadiusFragments = Context.GetFragmentView<FAgentRadiusFragment>();
			const TArrayView<FMassTrafficVehicleControlFragment> VehicleControlFragments = Context.GetMutableFragmentView<FMassTrafficVehicleControlFragment>();
			const TArrayView<FMassTrafficVehicleLightsFragment> VehicleLightsFragments = Context.GetMutableFragmentView<FMassTrafficVehicleLightsFragment>();
			const TArrayView<FMassZoneGraphLaneLocationFragment> LaneLocationFragments = Context.GetMutableFragmentView<FMassZoneGraphLaneLocationFragment>();
			const TArrayView<FMassTrafficLaneOffsetFragment> LaneOffsetFragments = Context.GetMutableFragmentView<FMassTrafficLaneOffsetFragment>();
			const TArrayView<FMassTrafficObstacleAvoidanceFragment> AvoidanceFragments = Context.GetMutableFragmentView<FMassTrafficObstacleAvoidanceFragment>();
			const TConstArrayView<FMassTrafficDebugFragment> DebugFragments = Context.GetFragmentView<FMassTrafficDebugFragment>();
			const TArrayView<FMassTrafficVehicleLaneChangeFragment> LaneChangeFragments = Context.GetMutableFragmentView<FMassTrafficVehicleLaneChangeFragment>();
			const TConstArrayView<FMassTrafficNextVehicleFragment> NextVehicleFragments = Context.GetMutableFragmentView<FMassTrafficNextVehicleFragment>();
			const TArrayView<FMassTrafficGatesFragment> GatesFragments = Context.GetMutableFragmentView<FMassTrafficGatesFragment>();

			const TArrayView<FMassActorFragment> ActorFragments = Context.GetMutableFragmentView<FMassActorFragment>();

			const int32 NumEntities = Context.GetNumEntities();
			for (int32 Index = 0; Index < NumEntities; ++Index)
			{
				const FMassSimulationVariableTickFragment& VariableTickFragment = VariableTickFragments[Index];
				const FMassTrafficRandomFractionFragment& RandomFractionFragment = RandomFractionFragments[Index];
				const FTransformFragment& TransformFragment = TransformFragments[Index];
				const FAgentRadiusFragment& RadiusFragment = RadiusFragments[Index];
				FMassTrafficVehicleControlFragment& VehicleControlFragment = VehicleControlFragments[Index];
				FMassTrafficVehicleLightsFragment& VehicleLightsFragment = VehicleLightsFragments[Index];
				FMassZoneGraphLaneLocationFragment& LaneLocationFragment = LaneLocationFragments[Index];
				FMassTrafficLaneOffsetFragment& LaneOffsetFragment = LaneOffsetFragments[Index];
				FMassTrafficObstacleAvoidanceFragment& AvoidanceFragment = AvoidanceFragments[Index];
				FMassTrafficVehicleLaneChangeFragment* LaneChangeFragment = !LaneChangeFragments.IsEmpty() ? &LaneChangeFragments[Index] : nullptr;
				const FMassTrafficNextVehicleFragment& NextVehicleFragment = NextVehicleFragments[Index];
				FMassTrafficGatesFragment& GateFragment = GatesFragments[Index];

				const FZoneGraphStorage* ZoneGraphStorage = ZoneGraphSubsystem.GetZoneGraphStorage(LaneLocationFragment.LaneHandle.DataHandle);
				check(ZoneGraphStorage);
				
				// Debug
				const bool bVisLog = DebugFragments.IsEmpty() ? false : DebugFragments[Index].bVisLog > 0;

				FMassActorFragment& ActorFragment = ActorFragments[Index];

				SimpleVehicleControl(
					GateFragment,
					ActorFragment,
					EntityManager,
					MassTrafficSubsystem,
					Context,
					Index,
					*ZoneGraphStorage,
					RadiusFragment,
					RandomFractionFragment,
					TransformFragment,
					VariableTickFragment,
					VehicleControlFragment,
					VehicleLightsFragment,
					LaneLocationFragment,
					LaneOffsetFragment,
					AvoidanceFragment,
					LaneChangeFragment, NextVehicleFragment, bVisLog);
				}
		});

	// Prepare physics inputs for PID vehicles
	PIDVehicleControlEntityQuery_Conditional.ForEachEntityChunk(EntityManager, Context, [&](FMassExecutionContext& ComponentSystemExecutionContext)
		{
			const UZoneGraphSubsystem& ZoneGraphSubsystem = ComponentSystemExecutionContext.GetSubsystemChecked<UZoneGraphSubsystem>();

			const TConstArrayView<FMassSimulationVariableTickFragment> VariableTickFragments = Context.GetFragmentView<FMassSimulationVariableTickFragment>();
			const TConstArrayView<FMassTrafficRandomFractionFragment> RandomFractionFragments = Context.GetFragmentView<FMassTrafficRandomFractionFragment>();
			const TConstArrayView<FMassTrafficObstacleAvoidanceFragment> AvoidanceFragments = Context.GetFragmentView<FMassTrafficObstacleAvoidanceFragment>();
			const TConstArrayView<FAgentRadiusFragment> RadiusFragments = Context.GetFragmentView<FAgentRadiusFragment>();
			const TConstArrayView<FTransformFragment> TransformFragments = Context.GetFragmentView<FTransformFragment>();
			const TArrayView<FMassTrafficVehicleControlFragment> VehicleControlFragments = Context.GetMutableFragmentView<FMassTrafficVehicleControlFragment>();
			const TArrayView<FMassTrafficVehicleLightsFragment> VehicleLightsFragments = Context.GetMutableFragmentView<FMassTrafficVehicleLightsFragment>();
			const TArrayView<FMassZoneGraphLaneLocationFragment> LaneLocationFragments = Context.GetMutableFragmentView<FMassZoneGraphLaneLocationFragment>();
			const TArrayView<FMassTrafficPIDVehicleControlFragment> PIDVehicleControlFragments = Context.GetMutableFragmentView<FMassTrafficPIDVehicleControlFragment>();
			const TArrayView<FMassTrafficPIDControlInterpolationFragment> VehiclePIDMovementInterpolationFragments = Context.GetMutableFragmentView<FMassTrafficPIDControlInterpolationFragment>();
			const TConstArrayView<FMassTrafficDebugFragment> DebugFragments = Context.GetFragmentView<FMassTrafficDebugFragment>();
			const TArrayView<FMassTrafficVehicleLaneChangeFragment> LaneChangeFragments = Context.GetMutableFragmentView<FMassTrafficVehicleLaneChangeFragment>();
			const TConstArrayView<FMassTrafficNextVehicleFragment> NextVehicleFragments = Context.GetFragmentView<FMassTrafficNextVehicleFragment>();
			const TArrayView<FMassTrafficGatesFragment> GatesFragments = Context.GetMutableFragmentView<FMassTrafficGatesFragment>();

			const TArrayView<FMassActorFragment> ActorFragments = Context.GetMutableFragmentView<FMassActorFragment>();

			const int32 NumEntities = Context.GetNumEntities();
			for (int32 Index = 0; Index < NumEntities; ++Index)
			{
				const FMassSimulationVariableTickFragment& VariableTickFragment = VariableTickFragments[Index];
				const FMassTrafficRandomFractionFragment& RandomFractionFragment = RandomFractionFragments[Index];
				const FAgentRadiusFragment& RadiusFragment = RadiusFragments[Index];
				const FMassTrafficObstacleAvoidanceFragment& AvoidanceFragment = AvoidanceFragments[Index];
				const FTransformFragment& TransformFragment = TransformFragments[Index];
				FMassTrafficVehicleControlFragment& VehicleControlFragment = VehicleControlFragments[Index];
				FMassTrafficVehicleLightsFragment& VehicleLightsFragment = VehicleLightsFragments[Index];
				FMassZoneGraphLaneLocationFragment& LaneLocationFragment = LaneLocationFragments[Index];
				FMassTrafficPIDControlInterpolationFragment& VehiclePIDMovementInterpolationFragment = VehiclePIDMovementInterpolationFragments[Index];
				FMassTrafficVehicleLaneChangeFragment* LaneChangeFragment = !LaneChangeFragments.IsEmpty() ? &LaneChangeFragments[Index] : nullptr;
				const FMassTrafficNextVehicleFragment& NextVehicleFragment = NextVehicleFragments[Index];
				FMassTrafficGatesFragment& GateFragment = GatesFragments[Index];

				FMassActorFragment& ActorFragment = ActorFragments[Index];

				const FZoneGraphStorage* ZoneGraphStorage = ZoneGraphSubsystem.GetZoneGraphStorage(LaneLocationFragment.LaneHandle.DataHandle);
				check(ZoneGraphStorage);
				
				// Debug
				const bool bVisLog = DebugFragments.IsEmpty() ? false : DebugFragments[Index].bVisLog > 0;

				PIDVehicleControl(
					GateFragment,
					ActorFragment,
					EntityManager,
					Context,
					Index,
					*ZoneGraphStorage,
					AvoidanceFragment,
					RadiusFragment,
					RandomFractionFragment,
					VariableTickFragment,
					TransformFragment,
					LaneChangeFragment,
					VehicleControlFragment,
					VehicleLightsFragment,
					LaneLocationFragment,
					PIDVehicleControlFragments[Index],
					VehiclePIDMovementInterpolationFragment,
					NextVehicleFragment, bVisLog);
				}
		});
}

void UMassTrafficVehicleControlProcessor::SimpleVehicleControl(
	FMassTrafficGatesFragment& GateFragment,
	FMassActorFragment& ActorFragment,
	FMassEntityManager& EntityManager,
	UMassTrafficSubsystem& MassTrafficSubsystem,
	FMassExecutionContext& Context,
	const int32 EntityIndex,
	const FZoneGraphStorage& ZoneGraphStorage,
	const FAgentRadiusFragment& AgentRadiusFragment,
	const FMassTrafficRandomFractionFragment& RandomFractionFragment,
	const FTransformFragment& TransformFragment,
	const FMassSimulationVariableTickFragment& VariableTickFragment,
	FMassTrafficVehicleControlFragment& VehicleControlFragment,
	FMassTrafficVehicleLightsFragment& VehicleLightsFragment,
	FMassZoneGraphLaneLocationFragment& LaneLocationFragment,
	FMassTrafficLaneOffsetFragment& LaneOffsetFragment,
	FMassTrafficObstacleAvoidanceFragment& AvoidanceFragment,
	FMassTrafficVehicleLaneChangeFragment* LaneChangeFragment,
	const FMassTrafficNextVehicleFragment& NextVehicleFragment, const bool bVisLog
) const
{
	// Compute stable distance based noise
	const float NoiseValue = UE::MassTraffic::CalculateNoiseValue(VehicleControlFragment.NoiseInput, MassTrafficSettings->NoisePeriod);

	// Noise based lateral offset
	LaneOffsetFragment.LateralOffset = NoiseValue * MassTrafficSettings->LateralOffsetMax;

	// Calculate varied speed limit along lane
	const float SpeedLimit = UE::MassTraffic::GetSpeedLimitAlongLane(LaneLocationFragment.LaneLength,
	                                                                 VehicleControlFragment.CurrentLaneConstData.SpeedLimit,
	                                                                 VehicleControlFragment.CurrentLaneConstData.AverageNextLanesSpeedLimit,
	                                                                 LaneLocationFragment.DistanceAlongLane, VehicleControlFragment.Speed, MassTrafficSettings->SpeedLimitBlendTime
	);
	const float VariedSpeedLimit = UE::MassTraffic::VarySpeedLimit(SpeedLimit, MassTrafficSettings->SpeedLimitVariancePct, MassTrafficSettings->SpeedVariancePct, RandomFractionFragment.RandomFraction, NoiseValue);

	const bool bIsOffLOD = (UE::MassLOD::GetLODFromArchetype(Context) == EMassLOD::Off);
	const bool bIsLowLOD = (UE::MassLOD::GetLODFromArchetype(Context) == EMassLOD::Low);
	
	// Should stop?
	bool bRequestDifferentNextLane = false;
	bool bVehicleCantStopAtLaneExit = VehicleControlFragment.bCantStopAtLaneExit; // (See all CANTSTOPLANEEXIT.)
	bool bIsFrontOfVehicleBeyondEndOfLane = false;
	bool bVehicleHasNoNextLane = false;
	bool bVehicleHasNoRoom = false;
	const bool bMustStopAtLaneExit = UE::MassTraffic::ShouldStopAtLaneExit(
		LaneLocationFragment.DistanceAlongLane,
		VehicleControlFragment.Speed,
		AgentRadiusFragment.Radius,
		RandomFractionFragment.RandomFraction,
		LaneLocationFragment.LaneLength,
		VehicleControlFragment.NextLane,
		MassTrafficSettings->MinimumDistanceToNextVehicleRange,
		EntityManager,
		/*out*/bRequestDifferentNextLane,
		/*in/out*/bVehicleCantStopAtLaneExit,
		/*out*/bIsFrontOfVehicleBeyondEndOfLane,
		/*out*/bVehicleHasNoNextLane,
		/*out*/bVehicleHasNoRoom,
		MassTrafficSettings->StandardTrafficPrepareToStopSeconds
		#if WITH_MASSTRAFFIC_DEBUG
			, bVisLog
			, LogOwner
			, &Context.GetFragmentView<FTransformFragment>()[EntityIndex].GetTransform()
		#endif
	);
	
	if (bRequestDifferentNextLane)
	{
		VehicleControlFragment.ChooseNextLanePreference = EMassTrafficChooseNextLanePreference::ChooseDifferentNextLane;
	}

	// Need to always do this, but - EDGE CASE: NOT if it's in off LOD.
	// (See all CANTSTOPLANEEXIT.)
	if (!bIsOffLOD /*EDGE CASE-ish - don't do this in off-LOD*/ && bVehicleCantStopAtLaneExit) 
	{
		// Vehicle can't stop before hitting the red light.
		SetVehicleCantStopAtLaneExit(VehicleControlFragment, LaneLocationFragment, NextVehicleFragment, EntityManager);
	}

	// EDGE CASE. Happens when a vehicle has decided it can't stop at same point in the recent past, but then soon
	// after discovers it must stop after all (has run out of room, or has lost it's next lane.)
	// (See all CANTSTOPLANEEXIT.)
	if (bMustStopAtLaneExit && VehicleControlFragment.bCantStopAtLaneExit) 
	{
		UnsetVehicleCantStopAtLaneExit(VehicleControlFragment);
		bVehicleCantStopAtLaneExit = false;
	}
	
	// EDGE CASE. Happens when a vehicle has decided it can't stop at same point in the recent past, but then soon
	// after discovers that, during it's very brief can't-stop phase, it has slipped into off-LOD territory, where we
	// handle this situation differently. (See off-LOD handling below.)
	// (See all CANTSTOPLANEEXIT.)
	if (bIsOffLOD && VehicleControlFragment.bCantStopAtLaneExit) 
	{
		UnsetVehicleCantStopAtLaneExit(VehicleControlFragment);
		bVehicleCantStopAtLaneExit = false;
	}

	// EDGE CASE. Happens when a vehicle has decided it can't stop at same point in the recent past, but then does in
	// fact somehow stop before the lane exit. (Seems to only happen in off LOD simple vehicles, but let's be safe.)
	// (See all CANTSTOPLANEEXIT.)
	if (VehicleControlFragment.Speed < 0.1f && !bIsFrontOfVehicleBeyondEndOfLane && VehicleControlFragment.bCantStopAtLaneExit)
	{
		UnsetVehicleCantStopAtLaneExit(VehicleControlFragment);
		bVehicleCantStopAtLaneExit = false;		
	}

	
	// Calculate target speed
	float TargetSpeed = UE::MassTraffic::CalculateTargetSpeed(
		LaneLocationFragment.DistanceAlongLane,
		VehicleControlFragment.Speed,
		AvoidanceFragment.DistanceToNext,
		AvoidanceFragment.TimeToCollidingObstacle,
		AvoidanceFragment.DistanceToCollidingObstacle,
		AgentRadiusFragment.Radius,
		RandomFractionFragment.RandomFraction,
		LaneLocationFragment.LaneLength,
		VariedSpeedLimit,
		MassTrafficSettings->IdealTimeToNextVehicleRange,
		MassTrafficSettings->MinimumDistanceToNextVehicleRange,
		/*NextVehicleAvoidanceBrakingPower*/3.0f, // @todo Expose
		MassTrafficSettings->ObstacleAvoidanceBrakingTimeRange,
		MassTrafficSettings->MinimumDistanceToObstacleRange,
		/*ObstacleAvoidanceBrakingPower*/0.5f, // @todo Expose
		MassTrafficSettings->StopSignBrakingTime,
		MassTrafficSettings->StoppingDistanceRange,
		/*StopSignBrakingPower*/0.5f, // @todo Expose
		bMustStopAtLaneExit
		#if WITH_MASSTRAFFIC_DEBUG
			, bVisLog
			, Context.GetSubsystem<UMassTrafficSubsystem>()
			, &Context.GetFragmentView<FTransformFragment>()[EntityIndex].GetTransform()
		#endif
	);


	// (See all READYLANE.)
	SetIsVehicleReadyToUseNextIntersectionLane(VehicleControlFragment, LaneLocationFragment, AgentRadiusFragment, RandomFractionFragment, MassTrafficSettings->StoppingDistanceRange, bVehicleHasNoRoom);

	const float SteeringControlLookAheadDistance = FMath::Max(MassTrafficSettings->SteeringControlMinLookAheadDistance, MassTrafficSettings->SteeringControlLaneLookAheadTime * VehicleControlFragment.Speed);
	
	FMassTrafficLaneSegment SteeringLaneSegment;
	FVector SteeringControlChaseTargetLocation;
	FQuat SteeringControlChaseTargetOrientation;
	UE::MassTraffic::InterpolatePositionAndOrientationAlongContinuousLanes(
		ZoneGraphStorage,
		LaneLocationFragment.LaneHandle.Index,
		LaneLocationFragment.LaneLength,
		VehicleControlFragment.NextLane ? VehicleControlFragment.NextLane->LaneHandle.Index : INDEX_NONE,
		LaneLocationFragment.DistanceAlongLane + SteeringControlLookAheadDistance,
		ETrafficVehicleMovementInterpolationMethod::CubicBezier,
		SteeringLaneSegment,
		SteeringControlChaseTargetLocation,
		SteeringControlChaseTargetOrientation);
	
	// Offset steering chase target by LateralOffset, with noise calculated at SteeringControlChaseTargetDistance ahead
	// to be consistent with simple vehicle lateral offset computed at that location.
	const float SteeringControlChaseTargetNoiseValue = UE::MassTraffic::CalculateNoiseValue(VehicleControlFragment.NoiseInput + SteeringControlLookAheadDistance, MassTrafficSettings->NoisePeriod);
	const float SteeringControlChaseTargetLateralOffset = MassTrafficSettings->LateralOffsetMax * SteeringControlChaseTargetNoiseValue;
	SteeringControlChaseTargetLocation += SteeringControlChaseTargetOrientation.GetRightVector() * SteeringControlChaseTargetLateralOffset;

	// When lane changing, apply lateral offsets to smoothly transition into the target lane
	if (LaneChangeFragment && LaneChangeFragment->IsLaneChangeInProgress())
	{
		FTransform SteeringControlChaseTargetTransform(SteeringControlChaseTargetOrientation, SteeringControlChaseTargetLocation);
		// When adjusting the transform for lane change - also add the steering look-ahead distance to the distance along
		// lane. This makes the chase target be in a more natural position, makes the car follow it easier, and prevents
		// the chase target from becoming deviant. (See all LANECHANGEPHYSICS1.)
		UE::MassTraffic::AdjustVehicleTransformDuringLaneChange(*LaneChangeFragment, LaneLocationFragment.DistanceAlongLane + SteeringControlLookAheadDistance, SteeringControlChaseTargetTransform, MassTrafficSettings->GetWorld());
		SteeringControlChaseTargetLocation = SteeringControlChaseTargetTransform.GetLocation();
		SteeringControlChaseTargetOrientation = SteeringControlChaseTargetTransform.GetRotation();
	}

	const float SpeedControlLookAheadDistance = FMath::Max(MassTrafficSettings->SpeedControlMinLookAheadDistance, MassTrafficSettings->SpeedControlLaneLookAheadTime * VehicleControlFragment.Speed);
	
	FMassTrafficLaneSegment LaneSegment;
	FVector SpeedControlChaseTargetLocation;
	FQuat SpeedControlChaseTargetOrientation;
	UE::MassTraffic::InterpolatePositionAndOrientationAlongContinuousLanes(
		ZoneGraphStorage,
		LaneLocationFragment.LaneHandle.Index,
		LaneLocationFragment.LaneLength,
		VehicleControlFragment.NextLane ? VehicleControlFragment.NextLane->LaneHandle.Index : INDEX_NONE,
		LaneLocationFragment.DistanceAlongLane + SpeedControlLookAheadDistance,
		ETrafficVehicleMovementInterpolationMethod::CubicBezier,
		LaneSegment,
		SpeedControlChaseTargetLocation,
		SpeedControlChaseTargetOrientation);
	
	FMassTrafficLaneSegment AngleLaneSegment;
	FVector AngleControlTargetLocation;
	FQuat AngleControlTargetOrientation;
	UE::MassTraffic::InterpolatePositionAndOrientationAlongContinuousLanes(
		ZoneGraphStorage,
		LaneLocationFragment.LaneHandle.Index,
		LaneLocationFragment.LaneLength,
		VehicleControlFragment.NextLane ? VehicleControlFragment.NextLane->LaneHandle.Index : INDEX_NONE,
		LaneLocationFragment.DistanceAlongLane - 200,
		ETrafficVehicleMovementInterpolationMethod::CubicBezier,
		AngleLaneSegment,
		AngleControlTargetLocation,
		AngleControlTargetOrientation);
	
	const float TurnAngle = TransformFragment.GetTransform().InverseTransformVectorNoScale(SpeedControlChaseTargetOrientation.GetForwardVector()).HeadingAngle();
	const float CurrentTurnAngle = TransformFragment.GetTransform().InverseTransformVectorNoScale(AngleControlTargetOrientation.GetForwardVector()).HeadingAngle();
	const float TurnSpeedFactor = FMath::GetMappedRangeValueClamped<>(TRange<float>(0.0f, HALF_PI), TRange<float>(1.0f, MassTrafficSettings->TurnSpeedScale), FMath::Abs(TurnAngle));
	const float TurnAngleFactor = FMath::GetMappedRangeValueClamped<>(TRange<float>(-HALF_PI, HALF_PI), TRange<float>(1.0f, -1.0f), CurrentTurnAngle);
	TargetSpeed *= TurnSpeedFactor;

	VehicleControlFragment.AngleTurn = TurnAngleFactor;
	
	// Additive distance to next point on zone graph
	const float LocalNextDistanceOnLane = LaneLocationFragment.DistanceAlongLane +
		FMath::GetMappedRangeValueClamped<float>(TRange<float>(0.f, 500.f),TRange<float>(0.f, 1.f),
		VehicleControlFragment.Speed) * 200.f;

	
	//--------------------------------------------Find next loc angle-------------------------------------------------//

	
	FZoneGraphLaneLocation LocalNextLocationOnLine;
	// Find next location on zone graph for find next angle
	UE::MassTraffic::CalculateLocationAlongContinuousLanes(ZoneGraphStorage,
		LaneLocationFragment.LaneHandle.Index,
		LaneLocationFragment.LaneLength,
		VehicleControlFragment.NextLane ? VehicleControlFragment.NextLane->LaneHandle.Index : INDEX_NONE,
		LocalNextDistanceOnLane,
		LocalNextLocationOnLine);
	
	const auto LocalActor = ActorFragment.GetMutable();
	
	if(LocalActor)
	{
		const FRotator LocalAngleFrom = LocalActor->GetActorRotation();
		const FRotator LocalAngleTo = LocalNextLocationOnLine.Direction.Rotation();
		FRotator LocalNextAngle = LocalAngleTo - LocalAngleFrom;
		LocalNextAngle.Normalize();
		// Save Yaw Angle for next loc
		VehicleControlFragment.ForWheelsRotateYawAngle = LocalNextAngle.Yaw;
	}

	//----------------------------------------------------------------------------------------------------------------// 

	if (!VehicleControlFragment.bShouldStop && !VehicleControlFragment.bNeedStop)
	{
		// Accelerate / decelerate Speed to TargetSpeed
		if (!FMath::IsNearlyEqual(VehicleControlFragment.Speed, TargetSpeed, 1.0f)) 
		{
			// Accelerate up to TargetSpeed
			if (TargetSpeed > VehicleControlFragment.Speed)
			{
				const float VariedAcceleration = MassTrafficSettings->Acceleration * (1.0f + MassTrafficSettings->AccelerationVariancePct * (RandomFractionFragment.RandomFraction * 2.0f - 1.0f));
				VehicleControlFragment.Speed = FMath::Min(TargetSpeed, VehicleControlFragment.Speed + VariableTickFragment.DeltaTime * VariedAcceleration);
				VehicleControlFragment.BrakeLightHysteresis = VehicleControlFragment.BrakeLightHysteresis - VariableTickFragment.DeltaTime;
			}
			// Decelerate down to TargetSpeed
			else
			{
				const float VariedDeceleration = MassTrafficSettings->Deceleration * (1.0f + MassTrafficSettings->DecelerationVariancePct * (RandomFractionFragment.RandomFraction * 2.0f - 1.0f));
				if (VehicleControlFragment.Speed - TargetSpeed > MassTrafficSettings->SpeedDeltaBrakingThreshold)
				{
					VehicleControlFragment.BrakeLightHysteresis = 1.0f + RandomFractionFragment.RandomFraction * 0.25;
				}
				VehicleControlFragment.Speed = FMath::Max(TargetSpeed, VehicleControlFragment.Speed - VariableTickFragment.DeltaTime * VariedDeceleration);
			}
		}
	}
	else
	{
		VehicleControlFragment.Speed = FMath::FInterpTo(VehicleControlFragment.Speed, 0, VariableTickFragment.DeltaTime, 10);
		VehicleControlFragment.BrakeLightHysteresis = 1.0f + RandomFractionFragment.RandomFraction * 0.25;
		TargetSpeed = 0;
	}

	// VehicleControlFragment.bBraking is set for visual purposes only. I found that when we aren't applying throttle
	// gives the most natural looking brake light.
	if (VehicleControlFragment.BrakeLightHysteresis > SMALL_NUMBER)
	{
		VehicleLightsFragment.bBrakeLights = true;
	}
	else
	{
		VehicleLightsFragment.bBrakeLights = false;
		// Keep this at zero if we're not braking. Could a very long stretch of acceleration
		// roll over a float? Doubt it, but this is safe.
		VehicleControlFragment.BrakeLightHysteresis = 0.0f;
	}

	const bool bIsVehicleStoppingOverLaneExit = (bMustStopAtLaneExit && bIsFrontOfVehicleBeyondEndOfLane); // (See all CROSSWALKOVERLAP.)

	if (!bIsOffLOD || !bIsVehicleStoppingOverLaneExit) // (See all CROSSWALKOVERLAP.)
	{
		const float MaxDistanceDelta = FMath::Max(AvoidanceFragment.DistanceToNext - MassTrafficSettings->MinimumDistanceToObstacleRange.X, 0.0f); 
		const float DistanceDelta = FMath::Min(VariableTickFragment.DeltaTime * VehicleControlFragment.Speed, MaxDistanceDelta);

		LaneLocationFragment.DistanceAlongLane += DistanceDelta;
	
		// Advance distance based noise
		VehicleControlFragment.NoiseInput += DistanceDelta;

		// If this vehicle were to get re-chunked during this PrePhysics processing phase, then the
		// UpdateDistanceToNearestObstacle processor that runs in the PostPhysics phase will see a different chunk
		// fragment. This means it can get a differing result from FMassTrafficVariableTickChunkFragment::ShouldTickChunkThisFrame
		// and skip updating the DistanceToNext which we effectively just reduced above.
		// 
		// So, here we speculatively close the gap in case UpdateDistanceToNearestObstacle skips us. If & when
		// UpdateDistanceToNearestObstacle does run (usually the case) we'll get the proper accurate DistanceToNext, but
		// this at least provides a conservative approximation if not. Otherwise, lane changing vehicles could think we
		// have a large gap in front of us they can change into and we could also leapfrog the next vehicle itself
		// on the next update, thinking we have a large space to move in ahead.
		//
		// Note: We only need to do this for SimpleVehicleControl as PIDVehicleControl is never run with variable tick
		// rate so UpdateDistanceToNearestObstacle will always be run in-phase.
		AvoidanceFragment.DistanceToNext = FMath::Max(AvoidanceFragment.DistanceToNext - DistanceDelta, 0.0f);
	}

	
	// Overran the lane?
	if (bIsVehicleStoppingOverLaneExit)
	{
		// (See all CROSSWALKOVERLAP.)
		const float MaxDistanceAlongLaneIfStopped = LaneLocationFragment.LaneLength - AgentRadiusFragment.Radius; 

		if (bIsOffLOD ||
			(bIsLowLOD && (LaneLocationFragment.DistanceAlongLane - MaxDistanceAlongLaneIfStopped <= 10.0f))) 
		{
			// (See all CROSSWALKOVERLAP.)
			LaneLocationFragment.DistanceAlongLane = MaxDistanceAlongLaneIfStopped - 1.0f/*cm*/;
		}
		else
		{
			// (See all CROSSWALKOVERLAP.)
			if (VehicleControlFragment.NextLane)
			{
				VehicleControlFragment.NextLane->bIsStoppedVehicleInPreviousLaneOverlappingThisLane = true;			
			}

			// Whilst the above code will try to clamp us to the ideal MaxDistanceAlongLaneIfStopped, it may be that
			// bIsVehicleStoppingOverLaneExit wasn't triggered until we were already past that point (e.g: surprise
			// light change or the post-intersection lane ran out of space) so as a final fail safe, we at least clamp
			// to the lane length here.
			if (LaneLocationFragment.DistanceAlongLane >= LaneLocationFragment.LaneLength)
			{
				LaneLocationFragment.DistanceAlongLane = LaneLocationFragment.LaneLength;
			}
		}
	}
	else if (LaneLocationFragment.DistanceAlongLane >= LaneLocationFragment.LaneLength)
	{
		// Proceed onto next chosen lane
		if (VehicleControlFragment.NextLane)
		{
			const FMassEntityHandle VehicleEntity = Context.GetEntity(EntityIndex); 
			bool bIsVehicleStuck = false; // (See all RECYCLESTUCK.)
			
			UE::MassTraffic::MoveVehicleToNextLane(
				EntityManager,
				MassTrafficSubsystem,
				VehicleEntity,
				AgentRadiusFragment,
				RandomFractionFragment,
				VehicleControlFragment,
				VehicleLightsFragment,
				LaneLocationFragment,
				Context.GetMutableFragmentView<FMassTrafficNextVehicleFragment>()[EntityIndex],
				LaneChangeFragment,
				bIsVehicleStuck/*out*/);
		}
		// No next lane yet, at least clamp to current lane length
		else
		{
			LaneLocationFragment.DistanceAlongLane = LaneLocationFragment.LaneLength;
		}
	}
	
	// Debug speed
	UE::MassTraffic::DrawDebugSpeed(
		GetWorld(),
		Context.GetFragmentView<FTransformFragment>()[EntityIndex].GetTransform().GetLocation(),
		VehicleControlFragment.Speed,
		VehicleLightsFragment.bBrakeLights,
		LaneLocationFragment.DistanceAlongLane,
		LaneLocationFragment.LaneLength,
		UE::MassLOD::GetLODFromArchetype(Context),
		bVisLog, LogOwner);
}

void UMassTrafficVehicleControlProcessor::PIDVehicleControl(
	FMassTrafficGatesFragment& GateFragment,
	FMassActorFragment& ActorFragment,
	const FMassEntityManager& EntityManager,
	const FMassExecutionContext& Context,
	const int32 EntityIndex,
	const FZoneGraphStorage& ZoneGraphStorage,
	const FMassTrafficObstacleAvoidanceFragment& AvoidanceFragment,
	const FAgentRadiusFragment& AgentRadiusFragment,
	const FMassTrafficRandomFractionFragment& RandomFractionFragment,
	const FMassSimulationVariableTickFragment& VariableTickFragment,
	const FTransformFragment& TransformFragment,
	FMassTrafficVehicleLaneChangeFragment* LaneChangeFragment,
	FMassTrafficVehicleControlFragment& VehicleControlFragment,
	FMassTrafficVehicleLightsFragment& VehicleLightsFragment,
	const FMassZoneGraphLaneLocationFragment& LaneLocationFragment,
	FMassTrafficPIDVehicleControlFragment& PIDVehicleControlFragment,
	FMassTrafficPIDControlInterpolationFragment& VehiclePIDMovementInterpolationFragment,
	const FMassTrafficNextVehicleFragment& NextVehicleFragment,
	const bool bVisLog
) const
{
	// Compute stable distance based noise
	const float NoiseValue = UE::MassTraffic::CalculateNoiseValue(VehicleControlFragment.NoiseInput, MassTrafficSettings->NoisePeriod);

	// Look ahead SpeedControlLookAheadDistance to get speed chase target location & orientation
	const float SpeedControlLookAheadDistance = FMath::Max(MassTrafficSettings->SpeedControlMinLookAheadDistance, MassTrafficSettings->SpeedControlLaneLookAheadTime * VehicleControlFragment.Speed);
	
	FVector SpeedControlChaseTargetLocation;
	FQuat SpeedControlChaseTargetOrientation;
	UE::MassTraffic::InterpolatePositionAndOrientationAlongContinuousLanes(
		ZoneGraphStorage,
		LaneLocationFragment.LaneHandle.Index,
		LaneLocationFragment.LaneLength,
		VehicleControlFragment.NextLane ? VehicleControlFragment.NextLane->LaneHandle.Index : INDEX_NONE,
		LaneLocationFragment.DistanceAlongLane + SpeedControlLookAheadDistance,
		ETrafficVehicleMovementInterpolationMethod::CubicBezier,
		VehiclePIDMovementInterpolationFragment.SpeedChaseTargetLaneSegment,
		SpeedControlChaseTargetLocation,
		SpeedControlChaseTargetOrientation);

	// Look ahead SteeringControlLookAheadDistance to get steering chase target location & orientation
	const float SteeringControlLookAheadDistance = FMath::Max(MassTrafficSettings->SteeringControlMinLookAheadDistance, MassTrafficSettings->SteeringControlLaneLookAheadTime * VehicleControlFragment.Speed);
	
	FVector SteeringControlChaseTargetLocation;
	FQuat SteeringControlChaseTargetOrientation;
	UE::MassTraffic::InterpolatePositionAndOrientationAlongContinuousLanes(
		ZoneGraphStorage,
		LaneLocationFragment.LaneHandle.Index,
		LaneLocationFragment.LaneLength,
		VehicleControlFragment.NextLane ? VehicleControlFragment.NextLane->LaneHandle.Index : INDEX_NONE,
		LaneLocationFragment.DistanceAlongLane + SteeringControlLookAheadDistance,
		ETrafficVehicleMovementInterpolationMethod::CubicBezier,
		VehiclePIDMovementInterpolationFragment.TurningChaseTargetLaneSegment,
		SteeringControlChaseTargetLocation,
		SteeringControlChaseTargetOrientation);
	
	// Offset steering chase target by LateralOffset, with noise calculated at SteeringControlChaseTargetDistance ahead
	// to be consistent with simple vehicle lateral offset computed at that location.
	const float SteeringControlChaseTargetNoiseValue = UE::MassTraffic::CalculateNoiseValue(VehicleControlFragment.NoiseInput + SteeringControlLookAheadDistance, MassTrafficSettings->NoisePeriod);
	const float SteeringControlChaseTargetLateralOffset = MassTrafficSettings->LateralOffsetMax * SteeringControlChaseTargetNoiseValue;
	SteeringControlChaseTargetLocation += SteeringControlChaseTargetOrientation.GetRightVector() * SteeringControlChaseTargetLateralOffset;

	// When lane changing, apply lateral offsets to smoothly transition into the target lane
	if (LaneChangeFragment && LaneChangeFragment->IsLaneChangeInProgress())
	{
		FTransform SteeringControlChaseTargetTransform(SteeringControlChaseTargetOrientation, SteeringControlChaseTargetLocation);
		// When adjusting the transform for lane change - also add the steering look-ahead distance to the distance along
		// lane. This makes the chase target be in a more natural position, makes the car follow it easier, and prevents
		// the chase target from becoming deviant. (See all LANECHANGEPHYSICS1.)
		UE::MassTraffic::AdjustVehicleTransformDuringLaneChange(*LaneChangeFragment, LaneLocationFragment.DistanceAlongLane + SteeringControlLookAheadDistance, SteeringControlChaseTargetTransform, MassTrafficSettings->GetWorld());
		SteeringControlChaseTargetLocation = SteeringControlChaseTargetTransform.GetLocation();
		SteeringControlChaseTargetOrientation = SteeringControlChaseTargetTransform.GetRotation();
	}

	// Calculate varied speed limit along lane
	const float SpeedLimit = UE::MassTraffic::GetSpeedLimitAlongLane(LaneLocationFragment.LaneLength,
		VehicleControlFragment.CurrentLaneConstData.SpeedLimit,
		VehicleControlFragment.CurrentLaneConstData.AverageNextLanesSpeedLimit,
		LaneLocationFragment.DistanceAlongLane, VehicleControlFragment.Speed, MassTrafficSettings->SpeedLimitBlendTime
	);
	const float VariedSpeedLimit = UE::MassTraffic::VarySpeedLimit(SpeedLimit, MassTrafficSettings->SpeedLimitVariancePct, MassTrafficSettings->SpeedVariancePct, RandomFractionFragment.RandomFraction, NoiseValue);

	// Should stop?
	bool bRequestDifferentNextLane = false;
	bool bVehicleCantStopAtLaneExit = VehicleControlFragment.bCantStopAtLaneExit; // (See all CANTSTOPLANEEXIT.)
	bool bIsFrontOfVehicleBeyondEndOfLane = false;
	bool bVehicleHasNoNextLane = false;
	bool bVehicleHasNoRoom = false;
	const bool bMustStopAtLaneExit = UE::MassTraffic::ShouldStopAtLaneExit(
		LaneLocationFragment.DistanceAlongLane,
		VehicleControlFragment.Speed,
		AgentRadiusFragment.Radius,
		RandomFractionFragment.RandomFraction,
		LaneLocationFragment.LaneLength,
		VehicleControlFragment.NextLane,
		MassTrafficSettings->MinimumDistanceToNextVehicleRange,
		EntityManager,
		/*out*/bRequestDifferentNextLane,
		/*in/out*/bVehicleCantStopAtLaneExit,
		/*out*/bIsFrontOfVehicleBeyondEndOfLane,
		/*out*/bVehicleHasNoNextLane,
		/*out*/bVehicleHasNoRoom,
		MassTrafficSettings->StandardTrafficPrepareToStopSeconds
		// DEBUG
		#if WITH_MASSTRAFFIC_DEBUG
			, bVisLog
			, LogOwner
			, &Context.GetFragmentView<FTransformFragment>()[EntityIndex].GetTransform()
		#endif
	);
	
	if (bRequestDifferentNextLane)
	{
		VehicleControlFragment.ChooseNextLanePreference = EMassTrafficChooseNextLanePreference::ChooseDifferentNextLane;
	}
	
	if (bVehicleCantStopAtLaneExit) // (See all CANTSTOPLANEEXIT.)
	{
		// Vehicle can't stop before hitting the red light.
		SetVehicleCantStopAtLaneExit(VehicleControlFragment, LaneLocationFragment, NextVehicleFragment, EntityManager);
	}

	// EDGE CASE. Happens when a vehicle has decided it can't stop at same point in the recent past, but then soon
	// after discovers it must stop after all (has run out of room, or has lost it's next lane.)
	// (See all CANTSTOPLANEEXIT.)
	if (bMustStopAtLaneExit && VehicleControlFragment.bCantStopAtLaneExit)
	{
		UnsetVehicleCantStopAtLaneExit(VehicleControlFragment);
		bVehicleCantStopAtLaneExit = false;
	}
	
	// EDGE CASE. Happens when a vehicle has decided it can't stop at same point in the recent past, but then does in
	// fact somehow stop before the lane exit. (Seems to only happen in off LOD simple vehicles, but let's be safe.)
	// (See all CANTSTOPLANEEXIT.)
	if (VehicleControlFragment.Speed < 0.1f && !bIsFrontOfVehicleBeyondEndOfLane && VehicleControlFragment.bCantStopAtLaneExit)
	{
		UnsetVehicleCantStopAtLaneExit(VehicleControlFragment);
		bVehicleCantStopAtLaneExit = false;
	}

	// If vehicle has stopped in a crosswalk, tell the intersection lane.
	// (See all CROSSWALKOVERLAP.)
	if ((bMustStopAtLaneExit && bIsFrontOfVehicleBeyondEndOfLane) &&
		VehicleControlFragment.NextLane)
	{
		VehicleControlFragment.NextLane->bIsStoppedVehicleInPreviousLaneOverlappingThisLane = true;
	}

	// Calculate target speed
	float TargetSpeed = UE::MassTraffic::CalculateTargetSpeed(
		LaneLocationFragment.DistanceAlongLane,
		VehicleControlFragment.Speed,
		AvoidanceFragment.DistanceToNext,
		AvoidanceFragment.TimeToCollidingObstacle,
		AvoidanceFragment.DistanceToCollidingObstacle,
		AgentRadiusFragment.Radius,
		RandomFractionFragment.RandomFraction,
		LaneLocationFragment.LaneLength,
		VariedSpeedLimit,
		MassTrafficSettings->IdealTimeToNextVehicleRange,
		MassTrafficSettings->MinimumDistanceToNextVehicleRange,
		/*NextVehicleAvoidanceBrakingPower*/3.0f, // @todo Expose
		MassTrafficSettings->ObstacleAvoidanceBrakingTimeRange,
		MassTrafficSettings->MinimumDistanceToObstacleRange,
		/*ObstacleAvoidanceBrakingPower*/0.5f, // @todo Expose
		MassTrafficSettings->StopSignBrakingTime,
		MassTrafficSettings->StoppingDistanceRange,
		/*StopSignBrakingPower*/0.5f, // @todo Expose
		bMustStopAtLaneExit
		#if WITH_MASSTRAFFIC_DEBUG
			, bVisLog
			, LogOwner
			, &Context.GetFragmentView<FTransformFragment>()[EntityIndex].GetTransform()
		#endif
	);

	// (See all READYLANE.)
	SetIsVehicleReadyToUseNextIntersectionLane(VehicleControlFragment, LaneLocationFragment, AgentRadiusFragment, RandomFractionFragment, MassTrafficSettings->StoppingDistanceRange, bVehicleHasNoRoom);

	FMassTrafficLaneSegment AngleLaneSegment;
	FVector AngleControlTargetLocation;
	FQuat AngleControlTargetOrientation;
	UE::MassTraffic::InterpolatePositionAndOrientationAlongContinuousLanes(
		ZoneGraphStorage,
		LaneLocationFragment.LaneHandle.Index,
		LaneLocationFragment.LaneLength,
		VehicleControlFragment.NextLane ? VehicleControlFragment.NextLane->LaneHandle.Index : INDEX_NONE,
		LaneLocationFragment.DistanceAlongLane - 200,
		ETrafficVehicleMovementInterpolationMethod::CubicBezier,
		AngleLaneSegment,
		AngleControlTargetLocation,
		AngleControlTargetOrientation);
	
	const float CurrentTurnAngle = TransformFragment.GetTransform().InverseTransformVectorNoScale(AngleControlTargetOrientation.GetForwardVector()).HeadingAngle();
	const float TurnAngleFactor = FMath::GetMappedRangeValueClamped<>(TRange<float>(-HALF_PI, HALF_PI), TRange<float>(1.0f, -1.0f), CurrentTurnAngle);
	VehicleControlFragment.AngleTurn = TurnAngleFactor;
	
	// Additive distance to next point on zone graph
	const float LocalNextDistanceOnLane = LaneLocationFragment.DistanceAlongLane +
		FMath::GetMappedRangeValueClamped<float>(TRange<float>(0.f, 500.f),TRange<float>(0.f, 1.f),
		VehicleControlFragment.Speed) * 200.f;

	// Reduce speed while cornering
	const float TurnAngle = TransformFragment.GetTransform().InverseTransformVectorNoScale(SpeedControlChaseTargetOrientation.GetForwardVector()).HeadingAngle();
	const float TurnSpeedFactor = FMath::GetMappedRangeValueClamped<>(TRange<float>(0.0f, HALF_PI), TRange<float>(1.0f, MassTrafficSettings->TurnSpeedScale), FMath::Abs(TurnAngle));
	TargetSpeed *= TurnSpeedFactor;

	FZoneGraphLaneLocation LocalNextLocationOnLine;
	// Find next location on zone graph for find next angle
	UE::MassTraffic::CalculateLocationAlongContinuousLanes(ZoneGraphStorage,
		LaneLocationFragment.LaneHandle.Index,
		LaneLocationFragment.LaneLength,
		VehicleControlFragment.NextLane ? VehicleControlFragment.NextLane->LaneHandle.Index : INDEX_NONE,
		LocalNextDistanceOnLane,
		LocalNextLocationOnLine);
	
	const auto LocalActor = ActorFragment.GetMutable();
	
	if(LocalActor)
	{
		const FRotator LocalAngleFrom = LocalActor->GetActorRotation();
		const FRotator LocalAngleTo = LocalNextLocationOnLine.Direction.Rotation();
		FRotator LocalNextAngle = LocalAngleTo - LocalAngleFrom;
		LocalNextAngle.Normalize();
		// Save Yaw Angle for next loc
		VehicleControlFragment.ForWheelsRotateYawAngle = LocalNextAngle.Yaw;
	}
	
	if (VehicleControlFragment.bShouldStop)
	{
		if (GateFragment.Gate)
		{
			if (UBoxComponent* GateBoxComponent = GateFragment.Gate->FindComponentByClass<UBoxComponent>())
			{
				float CurrentDistance = FVector::Distance(
												GateBoxComponent->GetComponentLocation(),
												TransformFragment.GetTransform().GetLocation());
			
				if (CurrentDistance <= MassTrafficSettings->MinDistanceToStopBeforeGates)
				{
					TargetSpeed = 0.0f;
				}
			
				const float GateSpeedFactor = FMath::GetMappedRangeValueClamped<>(
						TRange<float>(MassTrafficSettings->MinDistanceToStopBeforeGates, MassTrafficSettings->MaxDistanceToStopBeforeGates),
						TRange<float>(MassTrafficSettings->GateNearSpeedFactor, 1.0f),
							CurrentDistance);
				TargetSpeed *= GateSpeedFactor;
				#if WITH_MASSTRAFFIC_DEBUG
				if (GDebugMassTrafficGates)
				{
					DrawDebugString(GetWorld(), TransformFragment.GetTransform().GetLocation(), FString::SanitizeFloat(TargetSpeed), nullptr, FColor::Emerald, 0);
				}
				#endif
			}
		}
	}

	if (VehicleControlFragment.bShouldSlowDown)
	{
		const float BumpSpeedFactor = FMath::GetMappedRangeValueClamped<>(
				TRange<float>(MassTrafficSettings->DistanceToPassSpeedBump, MassTrafficSettings->DistanceToSlowDown),
				TRange<float>(MassTrafficSettings->SlowDownScale, 1.0f),
					VehicleControlFragment.DistanceToSpeedBump);
		if (TargetSpeed > 400.0f) TargetSpeed *= BumpSpeedFactor;
		#if WITH_MASSTRAFFIC_DEBUG
		if (GDebugMassTrafficBumps)
		{
			DrawDebugString(
				GetWorld(),
				TransformFragment.GetTransform().GetLocation() + FVector(0, 0, 300),
				FString::SanitizeFloat(TargetSpeed),
				nullptr,
				FColor::Emerald,
				0);
		}
		#endif
	}

	if (ActorFragment.Get())
	{
		TObjectPtr<UVehicleMassIDComponent> IDComponent = ActorFragment.Get()->FindComponentByClass<UVehicleMassIDComponent>();
		if (IDComponent && IDComponent->GetVehicleNeedStop())
		{
			VehicleControlFragment.bNeedStop = true;
			TargetSpeed = 0;
		}
		else
		{
			VehicleControlFragment.bNeedStop = false;
		}
	}

	// Tick the throttle and brake control PID. Feed throttle & brake PID controller with current speed delta. If returned 
	// value is positive, it's applied as throttle - negative values are applied as brake.
	float ThrottleOrBrake = PIDVehicleControlFragment.ThrottleAndBrakeController.Tick(TargetSpeed,
		VehicleControlFragment.Speed,
		VariableTickFragment.DeltaTime,
		MassTrafficSettings->SpeedPIDControllerParams);
	ThrottleOrBrake = FMath::Clamp(ThrottleOrBrake, -1.0f, 1.0f);

	// Handbrake shouldn't be on unless we are a parked vehicle.
	PIDVehicleControlFragment.bHandbrake = false;
	// Default to coasting along, no brake, no throttle.
	PIDVehicleControlFragment.Brake = 0.0f;
	PIDVehicleControlFragment.Throttle = 0.0f;

	if (ThrottleOrBrake > MassTrafficSettings->SpeedCoastThreshold)
	{
		PIDVehicleControlFragment.Throttle = ThrottleOrBrake;
	}
	else if (ThrottleOrBrake < -MassTrafficSettings->SpeedCoastThreshold)
	{
		// We are messing with the returned PID value here as we have one PID for the throttle
		// and brake.
		PIDVehicleControlFragment.Brake = FMath::Abs(ThrottleOrBrake) * MassTrafficSettings->SpeedPIDBrakeMultiplier;
	}
	
	// If we're stopped, we should put the brakes on. Tolerance on .Speed is because the vehicles might
	// bounce a bit on LOD transition and we don't want that to flicker the light.
	if (FMath::IsNearlyZero(PIDVehicleControlFragment.Throttle) &&
		FMath::IsNearlyZero(TargetSpeed, 5.0f) &&
		FMath::IsNearlyZero(VehicleControlFragment.Speed, 5.0f))
	{
		PIDVehicleControlFragment.Brake = 1.0f;
	}
	
	// We consider the brakes on if we're braking by more than 0.1 or if were stopped (effectively < 10CMs) and no
	// throttle is being applied.  BrakeLightHysteresis exists so we don't trigger the light on and off rapidly.
	// We show the light for a minimum ~1 second when we decide we want to show it.
	const float BrakeLightDetectionThreshold = SMALL_NUMBER + 0.05f * RandomFractionFragment.RandomFraction;
	if (PIDVehicleControlFragment.Brake > BrakeLightDetectionThreshold)
	{
		// Adding a quarter of a second variation.
		VehicleControlFragment.BrakeLightHysteresis = 1.0f + RandomFractionFragment.RandomFraction * 0.25;
	}
	else if (PIDVehicleControlFragment.Throttle > 0.25f)
	{
		// Turn the brake off fairly fast if we're really pressing the throttle.
		VehicleControlFragment.BrakeLightHysteresis = VehicleControlFragment.BrakeLightHysteresis - VariableTickFragment.DeltaTime * 4.0f; 
	}
	else
	{
		VehicleControlFragment.BrakeLightHysteresis = VehicleControlFragment.BrakeLightHysteresis - VariableTickFragment.DeltaTime;
	}

	// VehicleControlFragment.bBraking is set for visual purposes only. I found that when we aren't applying throttle
	// gives the most natural looking brake light.
	if (VehicleControlFragment.BrakeLightHysteresis > SMALL_NUMBER)
	{
		VehicleLightsFragment.bBrakeLights = true;
	}
	else
	{
		VehicleLightsFragment.bBrakeLights = false;
		// Keep this at zero if we're not braking. Could a very long stretch of acceleration
		// roll over a float? Doubt it, but this is safe.
		VehicleControlFragment.BrakeLightHysteresis = 0.0f;
	}

	// Feed steering PID controller with current angle delta 
	const FVector ToSteeringControlChaseTargetLocal = TransformFragment.GetTransform().InverseTransformPositionNoScale(SteeringControlChaseTargetLocation);
	const float NormalizedDeltaAngle = ToSteeringControlChaseTargetLocal.HeadingAngle() / PIDVehicleControlFragment.MaxSteeringAngle;
	PIDVehicleControlFragment.Steering = FMath::Clamp(PIDVehicleControlFragment.SteeringController.Tick(0.0f, -NormalizedDeltaAngle, VariableTickFragment.DeltaTime, MassTrafficSettings->SteeringPIDControllerParams), -1.0f, 1.0f);

	// Debug speed
	#if WITH_MASSTRAFFIC_DEBUG
		const FVector& DebugLocation = Context.GetFragmentView<FTransformFragment>()[EntityIndex].GetTransform().GetLocation();
		UE::MassTraffic::DrawDebugSpeed(
			GetWorld(),
			DebugLocation,
			VehicleControlFragment.Speed,
			VehicleLightsFragment.bBrakeLights,
			LaneLocationFragment.DistanceAlongLane,
			LaneLocationFragment.LaneLength,
			UE::MassLOD::GetLODFromArchetype(Context),
			bVisLog,
			LogOwner);
		UE::MassTraffic::DrawDebugChaosVehicleControl(GetWorld(), DebugLocation, SpeedControlChaseTargetLocation, SteeringControlChaseTargetLocation, TargetSpeed, PIDVehicleControlFragment.Throttle, PIDVehicleControlFragment.Brake, PIDVehicleControlFragment.Steering, PIDVehicleControlFragment.bHandbrake, bVisLog, LogOwner);
	#endif 
}