// Fill out your copyright notice in the Description page of Project Settings.


#include "Processor/MassTrafficTransformCorrectProcessor.h"

#include "MassExecutionContext.h"
#include "MassRepresentationFragments.h"
#include "Processor/MassTrafficFindDeviantTrafficVehiclesProcessor.h"
#include "Static/MassTrafficHelperLibrary.h"
#include "Processor/MassTrafficInterpolationProcessor.h"
#include "Processor/MassTrafficVehicleControlProcessor.h"
#include "Processor/MassTrafficVehiclePhysicsProcessor.h"
#include "Traits/MassTrafficVehicleSimulationTrait.h"
#include "MassZoneGraphNavigationFragments.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "ProfilingDebugging/CookStats.h"
#include "ZoneGraphSubsystem.h"
#include "MassGameplayExternalTraits.h"

UMassTrafficTransformCorrectProcessor::UMassTrafficTransformCorrectProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::Client | EProcessorExecutionFlags::Standalone);
	bRequiresGameThreadExecution = false;
	bAutoRegisterWithProcessingPhases = true;
	bAllowMultipleInstances = true;
	ExecutionOrder.ExecuteInGroup = UE::MassTraffic::ProcessorGroupNames::VehicleBehavior;
	ExecutionOrder.ExecuteAfter.Add(UE::MassTraffic::ProcessorGroupNames::FrameStart);
	ExecutionOrder.ExecuteAfter.Add(UE::MassTraffic::ProcessorGroupNames::PreVehicleBehavior);
	ExecutionOrder.ExecuteAfter.Add(UE::MassTraffic::ProcessorGroupNames::VehicleSimulationLOD);
	ExecutionOrder.ExecuteAfter.Add(UMassTrafficVehicleControlProcessor::StaticClass()->GetFName());
	ExecutionOrder.ExecuteBefore.Add(UMassTrafficFindDeviantTrafficVehiclesProcessor::StaticClass()->GetFName());
	ExecutionOrder.ExecuteBefore.Add(UMassTrafficVehiclePhysicsProcessor::StaticClass()->GetFName());
}

void UMassTrafficTransformCorrectProcessor::ConfigureQueries()
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassActorFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassTrafficVehicleControlFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassSimulationVariableTickFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassZoneGraphLaneLocationFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassRepresentationFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassRepresentationLODFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddConstSharedRequirement<FMassTrafficVehicleSimulationParameters>();
	EntityQuery.AddSubsystemRequirement<UZoneGraphSubsystem>(EMassFragmentAccess::ReadOnly);
}

void UMassTrafficTransformCorrectProcessor::Execute(FMassEntityManager& EntitySubSystem, FMassExecutionContext& Context)
{
	const APlayerController* Controller = UGameplayStatics::GetPlayerController(Context.GetWorld(), 0);

	if (!Controller) return;

	if (CurrentFrames < FramesToStart)
	{
		CurrentFrames++;
		return;
	}
	
	EntityQuery.ForEachEntityChunk(EntitySubSystem, Context, [&, Controller](FMassExecutionContext& ComponentSystemExecutionContext)
	{
		const UZoneGraphSubsystem& ZoneGraphSubsystem = ComponentSystemExecutionContext.GetSubsystemChecked<UZoneGraphSubsystem>();
		
		const TArrayView<FMassActorFragment> ActorFragments = Context.GetMutableFragmentView<FMassActorFragment>();
		const TConstArrayView<FMassSimulationVariableTickFragment> VariableTickFragments = Context.GetFragmentView<FMassSimulationVariableTickFragment>();
		const TConstArrayView<FMassZoneGraphLaneLocationFragment> LaneLocationFragments = Context.GetFragmentView<FMassZoneGraphLaneLocationFragment>();
		const TArrayView<FMassTrafficVehicleControlFragment> VehicleControlFragments = Context.GetMutableFragmentView<FMassTrafficVehicleControlFragment>();
		const FMassTrafficVehicleSimulationParameters& SimulationParams =  Context.GetConstSharedFragment<FMassTrafficVehicleSimulationParameters>();
		const TArrayView<FTransformFragment> TransformFragments = Context.GetMutableFragmentView<FTransformFragment>();
		const TArrayView<FMassRepresentationFragment> VisualizationFragments = Context.GetMutableFragmentView<FMassRepresentationFragment>();
		const TArrayView<FMassRepresentationLODFragment> LodFragments = Context.GetMutableFragmentView<FMassRepresentationLODFragment>();

		const UMassTrafficSettings* MassTrafficSettings = GetDefault<UMassTrafficSettings>();

		if (!MassTrafficSettings->bEnableLocationFix) return;
		
		const int32 NumEntities = Context.GetNumEntities();
		for (int32 EntityIdx = 0; EntityIdx < NumEntities; EntityIdx++)
		{
			TRACE_CPUPROFILER_EVENT_SCOPE_STR("Transform correct processor work");
			
			FMassActorFragment& ActorFragment = ActorFragments[EntityIdx];
			FTransformFragment& TransformFragment = TransformFragments[EntityIdx];
			const FMassSimulationVariableTickFragment& VariableTickFragment = VariableTickFragments[EntityIdx];
			FMassTrafficVehicleControlFragment& VehicleControlFragment = VehicleControlFragments[EntityIdx];
			const FMassZoneGraphLaneLocationFragment& LaneLocationFragment = LaneLocationFragments[EntityIdx];
			FMassRepresentationFragment& RepresentationFragment = VisualizationFragments[EntityIdx];
			FMassRepresentationLODFragment& LodFragment = LodFragments[EntityIdx];

			if (RepresentationFragment.CurrentRepresentation == EMassRepresentationType::StaticMeshInstance ||
				RepresentationFragment.CurrentRepresentation == EMassRepresentationType::None)
			{
				VehicleControlFragment.bUseFixedTransform = false;
				continue;
			}

			if (VehicleControlFragment.Speed <= 10.0f)
			{
				continue;
			}

			AActor* TrafficActor = ActorFragment.GetMutable();
	
			if (!TrafficActor) continue;

			FZoneGraphLaneLocation OutLaneLocation;
			ZoneGraphSubsystem.CalculateLocationAlongLane(LaneLocationFragment.LaneHandle,
														  LaneLocationFragment.DistanceAlongLane,
														  OutLaneLocation);
			
			TArray<AActor*> IgnoredActors;
			if (TrafficActor) IgnoredActors  = {TrafficActor, Controller->GetPawn()};

			FVector PlayerLocation;
			FRotator PlayerRotation;
			Controller->GetPlayerViewPoint(PlayerLocation, PlayerRotation);
			FVector CalculatedEntityLocation = OutLaneLocation.Position +
											   FVector(SimulationParams.HalfLength,
													   SimulationParams.HalfWidth,
													   SimulationParams.HalfWidth);
			
			TArray<FWheelTraceParameters> WheelConfigs = {};
			SetupWheelConfig(WheelConfigs, LodFragment, SimulationParams);

			const bool bIsInFov = UMassTrafficHelperLibrary::IsInFOV(Controller, PlayerLocation,
												CalculatedEntityLocation, PlayerRotation);
			if (!bIsInFov)
			{
				WheelConfigs.Empty();
				SetupWheelConfig(WheelConfigs, LodFragment, SimulationParams, EMassLOD::Low, true);
			}

			if (bIsInFov)
			{
				const bool bIsOccluded = UMassTrafficHelperLibrary::IsOccluded(Context.GetWorld(), IgnoredActors,
													PlayerLocation, CalculatedEntityLocation);
				if (!bIsOccluded)
				{
					WheelConfigs.Empty();
					SetupWheelConfig(WheelConfigs, LodFragment, SimulationParams, EMassLOD::Low, true);
				}
			}
			
			TArray<FVector> FrontPoints{};
			TArray<FVector> RearPoints{};
			TArray<FVector> LeftPoints{};
			TArray<FVector> RightPoints{};
			TArray<FVector> AllHits{};
			FString DebugOutReason;
			bool bSuccessTraces = StartWheelTraces(FrontPoints, RearPoints, LeftPoints,
												   RightPoints, AllHits, WheelConfigs,
												   OutLaneLocation, IgnoredActors, TransformFragment,
												   SimulationParams, DebugOutReason);

			if (!bSuccessTraces)
			{
#if WITH_MASSTRAFFIC_DEBUG
				if (GDebugMassTrafficTransformFix)
					DrawDebugString(GetWorld(), TransformFragment.GetTransform().GetLocation(), DebugOutReason, nullptr, FColor::Red, 0.0f);
#endif
				TransformFragment.GetMutableTransform().SetLocation(OutLaneLocation.Position);
				TransformFragment.GetMutableTransform().SetRotation(OutLaneLocation.Tangent.Rotation().Quaternion());
				VehicleControlFragment.FixedVehicleTransform = FTransform::Identity;
				VehicleControlFragment.bUseFixedTransform = false;
				continue;
			}
			
			if (FrontPoints.Num() > 0 && RearPoints.Num() > 0 && LeftPoints.Num() > 0 &&
				RightPoints.Num() > 0 && AllHits.Num() == WheelConfigs.Num())
			{
				FVector MidPointFront = CreateMidPoint(FrontPoints);
				FVector MidPointRear = CreateMidPoint(RearPoints);
				FVector MidPointLeft = CreateMidPoint(LeftPoints);
				FVector MidPointRight = CreateMidPoint(RightPoints);
				
				FVector MidPoint = (MidPointFront + MidPointRear + MidPointLeft + MidPointRight)  / 4;
				
				const FVector DesiredForward = (MidPointFront - MidPointRear).GetSafeNormal();
				const FVector RightVector = (MidPointRight - MidPointLeft).GetSafeNormal();
				const FRotator NewRotation = FRotationMatrix::MakeFromXY(DesiredForward, RightVector).Rotator();
				
				const FTransform NewTransform(
					NewRotation,
					MidPoint - FVector(0, 0, MassTrafficSettings->RelativeErrorDistance),
					TransformFragment.GetTransform().GetScale3D()
				);

				DrawDebugInfo(MidPoint, MidPointFront, MidPointRear, MidPointLeft, MidPointRight);

				VehicleControlFragment.FixedVehicleTransform = NewTransform;
				
				VehicleControlFragment.AngleForward = NewRotation;

				VehicleControlFragment.bUseFixedTransform = true;
			}
			else
			{
				VehicleControlFragment.FixedVehicleTransform = FTransform::Identity;
				VehicleControlFragment.bUseFixedTransform = false;
			}
		}
	});
}

// ReSharper disable once CppMemberFunctionMayBeStatic
FVector UMassTrafficTransformCorrectProcessor::CreateMidPoint(const TArray<FVector>& Points)
{
	FVector MidPoint= FVector::ZeroVector;
	for (const auto& Point : Points)
	{
		MidPoint += Point;
	}
	MidPoint /= Points.Num();

	return MidPoint;
}

FHitResult UMassTrafficTransformCorrectProcessor::PerformWheelTrace(
	const FVector& BaseLocation,
	const FVector& ForwardVector,
	const FQuat& Rotation,
	const FWheelTraceParameters& Params,
	const float TraceLength,
	const float SphereRadius,
	const FCollisionQueryParams& CollisionParams) const
{
	const FVector TraceStart = BaseLocation 
		+ ForwardVector * Params.AxleOffsetX
		+ Rotation.GetRightVector() * Params.WheelOffsetY
		+ FVector(0, 0, TraceLength); 
    
	const FVector TraceEnd = BaseLocation 
		+ ForwardVector * Params.AxleOffsetX
		+ Rotation.GetRightVector() * Params.WheelOffsetY
		- FVector(0, 0, TraceLength);

	FHitResult HitResult;
	GetWorld()->SweepSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		FQuat::Identity,
		MassTrafficSettings->TransformFixCollisionChannel,
		FCollisionShape::MakeSphere(SphereRadius),
		CollisionParams
	);
	
	if (!HitResult.bBlockingHit)
	{
		HitResult.ImpactPoint = BaseLocation;
	}
	
	return HitResult;
}

void UMassTrafficTransformCorrectProcessor::SetupWheelConfig(
	TArray<FWheelTraceParameters>& WheelConfigs,
	const FMassRepresentationLODFragment LODFragment,
	const FMassTrafficVehicleSimulationParameters& SimulationParams,
	const EMassLOD::Type ForceLOD,
	const bool bForceLod)
{
	FMassRepresentationLODFragment LODFrag;
	if (bForceLod)
		LODFrag.LOD = ForceLOD;
	else
		LODFrag.LOD = LODFragment.LOD;
	
	switch (LODFrag.LOD)
	{
	case EMassLOD::High:
		{
			switch (SimulationParams.VehicleType)
			{
			case EMassTrafficVehicleType::Vehicle:
				{
					WheelConfigs = {
						{SimulationParams.FrontAxleX, -SimulationParams.WheelsOffset},
						{SimulationParams.FrontAxleX, SimulationParams.WheelsOffset},
						{SimulationParams.RearAxleX, -SimulationParams.WheelsOffset},
						{SimulationParams.RearAxleX, SimulationParams.WheelsOffset}
					};
					break;
				}
			case EMassTrafficVehicleType::Bicycle:
				{
					WheelConfigs = {
						{SimulationParams.FrontAxleX, 0.0f},
						{SimulationParams.RearAxleX, 0.0f},
					};
					break;
				}
			default:
				{
					WheelConfigs = {
						{SimulationParams.FrontAxleX, 0.0f},
						{SimulationParams.RearAxleX, 0.0f},
					};
					break;
				}
			}
		}
		break;
	case EMassLOD::Medium:
		{
			WheelConfigs = {
				{SimulationParams.FrontAxleX, 0.0f},
				{SimulationParams.RearAxleX, 0.0f},
			};
		}
		break;
	case EMassLOD::Low:
		{
			WheelConfigs = {
				{0.0f, 0.0f},
			};
		}
		break;
	default:
		break;
	}
}

bool UMassTrafficTransformCorrectProcessor::StartWheelTraces(
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
	FString& OutReason) const
{
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActors(IgnoredActors);
	CollisionParams.bTraceComplex = true;
	
	for (const auto& WheelConfig : WheelConfigs)
	{
		FVector CorrectedLoc = TransformFragment.GetTransform().GetLocation() - LaneLocation.Position;
		CorrectedLoc.Z = LaneLocation.Position.Z;
		FHitResult Hit = PerformWheelTrace(
			LaneLocation.Position + CorrectedLoc,
			TransformFragment.GetTransform().GetRotation().GetForwardVector(),
			TransformFragment.GetTransform().GetRotation(),
			WheelConfig,
			MassTrafficSettings->LocationFixLength,
			SimulationParams.WheelsRadius * MassTrafficSettings->WheelRadiusPercent,
			CollisionParams
		);
			    
		if (Hit.bBlockingHit)
		{
			if (FVector::Distance(LaneLocation.Position, TransformFragment.GetTransform().GetLocation()) > MassTrafficSettings->DeviantLocationFixMax)
			{
				OutReason = "Deviant Location too big";
				return false;
			}

			if (Hit.Location.Z > TransformFragment.GetTransform().GetLocation().Z + MassTrafficSettings->MaxLowerDistance)
			{
				OutReason = "Hit location Z bigger than transform location";
				return false;
			}
					
			if (WheelConfig.AxleOffsetX > 0.0f) FrontPoints.Add(Hit.ImpactPoint);
			else if (WheelConfig.AxleOffsetX < 0.0f) RearPoints.Add(Hit.ImpactPoint);
			else
			{
				FrontPoints.Add(Hit.ImpactPoint);
				RearPoints.Add(Hit.ImpactPoint);
			}

			if (WheelConfig.WheelOffsetY > 0.0f) RightPoints.Add(Hit.ImpactPoint);
			else if (WheelConfig.WheelOffsetY < 0.0f) LeftPoints.Add(Hit.ImpactPoint);
			else
			{
				RightPoints.Add(Hit.ImpactPoint);
				LeftPoints.Add(Hit.ImpactPoint);
			}
					
			AllHits.Add(Hit.ImpactPoint);

#if WITH_MASSTRAFFIC_DEBUG
			if (GDebugMassTrafficTransformFix)
			{
				DrawDebugPoint(
					GetWorld(),
					Hit.ImpactPoint,
					12.0f,
					FColor::Green,
					false,
					0.0f,
					1.0f);
			}
#endif
		}
		else
		{
			OutReason = "No hit blocking";
			return false;
		}
	}

	return true;
}

void UMassTrafficTransformCorrectProcessor::DrawDebugInfo(const FVector& MidPoint, const FVector& MidPointFront,
	const FVector& MidPointRear, const FVector& MidPointLeft, const FVector& MidPointRight) const
{
#if WITH_MASSTRAFFIC_DEBUG
	if (GDebugMassTrafficTransformFix)
	{
		DrawDebugPoint(
			GetWorld(),MidPoint,12.0f,
			FColor::Blue,false,0.0f,1.0f);

		DrawDebugPoint(
			GetWorld(),MidPointFront,12.0f,
			FColor::Purple,false,0.0f,1.0f);

		DrawDebugPoint(
			GetWorld(),MidPointRear,12.0f,
			FColor::Purple,false,0.0f,1.0f);
					

		DrawDebugPoint(
			GetWorld(),MidPointLeft,12.0f,
			FColor::Magenta,false,0.0f,1.0f);

		DrawDebugPoint(
			GetWorld(),MidPointRight,12.0f,
			FColor::Red,false,0.0f,1.0f);
					
		DrawDebugDirectionalArrow(
			GetWorld(),MidPointRear,MidPointFront,
			120.0f,FColor::Emerald,false,0.0f,1.0f,2.0f);

		DrawDebugDirectionalArrow(
			GetWorld(),MidPointLeft,MidPointRight,
			120.0f,FColor::Emerald,false,0.0f,1.0f,2.0f);
	}
#endif
}


