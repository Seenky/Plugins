// Copyright Epic Games, Inc. All Rights Reserved.

#include "Processor/MassTrafficVehicleVisualizationProcessor.h"
#include "Components/MassTrafficVehicleComponent.h"
#include "Subsystems/MassTrafficSubsystem.h"
#include "Processor/MassTrafficDamageRepairProcessor.h"
#include "Processor/MassTrafficParkedVehicleVisualizationProcessor.h"
#include "Components/CarBodyThrottleComponent.h"
#include "Components/WheelRotationComponent.h"
#include "MassRepresentationSubsystem.h"
#include "MassEntityManager.h"
#include "MassExecutionContext.h"
#include "MassActorSubsystem.h"
#include "Static/MassTrafficInterpolation.h"
#include "Processor/MassTrafficTransformCorrectProcessor.h"
#include "MassUpdateISMProcessor.h"
#include "Components/HiderComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/TrafficLightsComponent.h"
#include "Components/VehicleMassIDComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Math/PackedVector.h"
#include "Subsystems/MassTrafficOccluderSubsystem.h"
#include "VisualLogger/VisualLogger.h"

//----------------------------------------------------------------------//
// FMassTrafficVehicleInstanceCustomData 
//----------------------------------------------------------------------//
FMassTrafficVehicleInstanceCustomData::FMassTrafficVehicleInstanceCustomData(const FMassTrafficPackedVehicleInstanceCustomData& PackedCustomData)
{
	const uint32& PackedParam1AsUint32 = reinterpret_cast<const uint32&>(PackedCustomData.PackedParam1);
	const uint32& PackedParam2AsUint32 = reinterpret_cast<const uint32&>(PackedCustomData.PackedParamColorRG);
	const uint32& PackedParam3AsUint32 = reinterpret_cast<const uint32&>(PackedCustomData.PackedParamColorB);
	const uint32& PackedParam4AsUint32 = reinterpret_cast<const uint32&>(PackedCustomData.PackedParamRightVectorXY);
	const uint32& PackedParam5AsUint32 = reinterpret_cast<const uint32&>(PackedCustomData.PackedParamRightVectorZ);

	// Unpack half precision random fraction 
	FFloat16 HalfPrecisionRandomFraction;
	HalfPrecisionRandomFraction.Encoded = PackedParam1AsUint32;
	RandomFraction = HalfPrecisionRandomFraction;

	// Get light state bits
	bFrontLeftRunningLights = PackedParam1AsUint32 & 1UL << (16 + 0);
	bFrontRightRunningLights = PackedParam1AsUint32 & 1UL << (16 + 1);
	bRearLeftRunningLights = PackedParam1AsUint32 & 1UL << (16 + 2);
	bRearRightRunningLights = PackedParam1AsUint32 & 1UL << (16 + 3);
	bLeftBrakeLights = PackedParam1AsUint32 & 1UL << (16 + 4);
	bRightBrakeLights = PackedParam1AsUint32 & 1UL << (16 + 5);
	bLeftTurnSignalLights = PackedParam1AsUint32 & 1UL << (16 + 6);
	bRightTurnSignalLights = PackedParam1AsUint32 & 1UL << (16 + 7);
	bLeftHeadlight = PackedParam1AsUint32 & 1UL << (16 + 8);
	bRightHeadlight = PackedParam1AsUint32 & 1UL << (16 + 9);
	bReversingLights = PackedParam1AsUint32 & 1UL << (16 + 10);
	bIsInvisible = PackedParam1AsUint32 & 1UL << (16 + 11);
	bIsCarActive = PackedParam1AsUint32 & 1UL << (16 + 12);
	bUseDither = PackedParam1AsUint32 & 1UL << (16 + 13);
	bIsDay = PackedParam1AsUint32 & 1UL << (16 + 14);

	FFloat16 HalfPrecisionColorR;
	HalfPrecisionColorR.Encoded = PackedParam2AsUint32 & 0xFFFF; 
	Color.R = HalfPrecisionColorR.GetFloat(); 
	
	FFloat16 HalfPrecisionColorG;
	HalfPrecisionColorG.Encoded = (PackedParam2AsUint32 >> 16) & 0xFFFF; 
	Color.G = HalfPrecisionColorG.GetFloat();

	FFloat16 HalfPrecisionColorB;
	HalfPrecisionColorB.Encoded = PackedParam3AsUint32;
	Color.B = HalfPrecisionColorB;

	FFloat16 HalfPrecisionVectorX;
	HalfPrecisionVectorX.Encoded = PackedParam4AsUint32; 
	RightVector.X = HalfPrecisionVectorX.GetFloat(); 
	
	FFloat16 HalfPrecisionVectorY;
	HalfPrecisionVectorY.Encoded = (PackedParam4AsUint32 >> 16); 
	RightVector.Y = HalfPrecisionVectorY.GetFloat();

	FFloat16 HalfPrecisionVectorZ;
	HalfPrecisionVectorZ.Encoded = PackedParam5AsUint32; 
	RightVector.Z = HalfPrecisionVectorZ.GetFloat(); 
}

FMassTrafficVehicleInstanceCustomData FMassTrafficVehicleInstanceCustomData::MakeTrafficVehicleCustomData(
	const FMassTrafficVehicleLightsFragment& VehicleStateFragment,
	const FMassTrafficRandomFractionFragment& RandomFractionFragment,
	const bool bIsInvisible, const FLinearColor Color, const FVector& RightVector, const bool bIsDay)
{
	// Random fraction, running lights on with dynamic brake lights & turn signals  
	FMassTrafficVehicleInstanceCustomData CustomData;
	CustomData.RandomFraction = RandomFractionFragment.RandomFraction;
	CustomData.bFrontLeftRunningLights = true;
	CustomData.bFrontRightRunningLights = true;
	CustomData.bRearLeftRunningLights = true;
	CustomData.bRearRightRunningLights = true;
	CustomData.bLeftBrakeLights = VehicleStateFragment.bBrakeLights;
	CustomData.bRightBrakeLights = VehicleStateFragment.bBrakeLights;
	CustomData.bLeftTurnSignalLights = VehicleStateFragment.bLeftTurnSignalLights;
	CustomData.bRightTurnSignalLights = VehicleStateFragment.bRightTurnSignalLights;
	CustomData.bLeftHeadlight = true;
	CustomData.bRightHeadlight = true;
	CustomData.bReversingLights = false;
	CustomData.bIsInvisible = bIsInvisible;
	CustomData.Color = Color;
	CustomData.RightVector = RightVector;
	CustomData.bIsCarActive = true;
	CustomData.bUseDither = true;
	CustomData.bIsDay = bIsDay;
	
	return MoveTemp(CustomData);
}

FMassTrafficVehicleInstanceCustomData FMassTrafficVehicleInstanceCustomData::MakeParkedVehicleCustomData(
	const FMassTrafficRandomFractionFragment& RandomFractionFragment)
{
	// Random fraction, all lights off
	FMassTrafficVehicleInstanceCustomData CustomData;
	CustomData.RandomFraction = RandomFractionFragment.RandomFraction;
	
	return MoveTemp(CustomData);
}

FMassTrafficVehicleInstanceCustomData FMassTrafficVehicleInstanceCustomData::MakeTrafficVehicleTrailerCustomData(
	const FMassTrafficRandomFractionFragment& RandomFractionFragment)
{
	// Random fraction, running lights on
	FMassTrafficVehicleInstanceCustomData CustomData;
	CustomData.RandomFraction = RandomFractionFragment.RandomFraction;
	
	CustomData.bFrontLeftRunningLights = true;
	CustomData.bFrontRightRunningLights = true;
	CustomData.bRearLeftRunningLights = true;
	CustomData.bRearRightRunningLights = true;
	
	return MoveTemp(CustomData);
}

FMassTrafficPackedVehicleInstanceCustomData::FMassTrafficPackedVehicleInstanceCustomData(const FMassTrafficVehicleInstanceCustomData& UnpackedCustomData)
{
	uint32& PackedParam1AsUint32 = reinterpret_cast<uint32&>(PackedParam1);
	uint32& PackedParam2AsUint32 = reinterpret_cast<uint32&>(PackedParamColorRG);
	uint32& PackedParam3AsUint32 = reinterpret_cast<uint32&>(PackedParamColorB);
	uint32& PackedParam4AsUint32 = reinterpret_cast<uint32&>(PackedParamRightVectorXY);
	uint32& PackedParam5AsUint32 = reinterpret_cast<uint32&>(PackedParamRightVectorZ);
	
	// Encode RandomFraction as 16-bit float in 16 least significant bits
	const FFloat16 HalfPrecisionRandomFraction = UnpackedCustomData.RandomFraction;
	PackedParam1AsUint32 = static_cast<uint32>(HalfPrecisionRandomFraction.Encoded);
	
	// Set light state bits
	if (UnpackedCustomData.bFrontLeftRunningLights)
	{
		PackedParam1AsUint32 |= 1UL << (16 + 0);
	}
	if (UnpackedCustomData.bFrontRightRunningLights)
	{
		PackedParam1AsUint32 |= 1UL << (16 + 1);
	}
	if (UnpackedCustomData.bRearLeftRunningLights)
	{
		PackedParam1AsUint32 |= 1UL << (16 + 2);
	}
	if (UnpackedCustomData.bRearRightRunningLights)
	{
		PackedParam1AsUint32 |= 1UL << (16 + 3);
	}
	if (UnpackedCustomData.bLeftBrakeLights)
	{
		PackedParam1AsUint32 |= 1UL << (16 + 4);
	}
	if (UnpackedCustomData.bRightBrakeLights)
	{
		PackedParam1AsUint32 |= 1UL << (16 + 5);
	}
	if (UnpackedCustomData.bLeftTurnSignalLights)
	{
		PackedParam1AsUint32 |= 1UL << (16 + 6);
	}
	if (UnpackedCustomData.bRightTurnSignalLights)
	{
		PackedParam1AsUint32 |= 1UL << (16 + 7);
	}
	if (UnpackedCustomData.bLeftHeadlight)
	{
		PackedParam1AsUint32 |= 1UL << (16 + 8);
	}
	if (UnpackedCustomData.bRightHeadlight)
	{
		PackedParam1AsUint32 |= 1UL << (16 + 9);
	}
	if (UnpackedCustomData.bReversingLights)
	{
		PackedParam1AsUint32 |= 1UL << (16 + 10);
	}
	if (UnpackedCustomData.bIsInvisible)
	{
		PackedParam1AsUint32 |= 1UL << (16 + 11); 
	}
	if (UnpackedCustomData.bIsCarActive)
	{
		PackedParam1AsUint32 |= 1UL << (16 + 12); 
	}
	if (UnpackedCustomData.bUseDither)
	{
		PackedParam1AsUint32 |= 1UL << (16 + 13); 
	}
	if (UnpackedCustomData.bIsDay)
	{
		PackedParam1AsUint32 |= 1UL << (16 + 14); 
	}
	
	const FFloat16 HalfPrecisionColorR = UnpackedCustomData.Color.R;
	const FFloat16 HalfPrecisionColorG = UnpackedCustomData.Color.G;
	const FFloat16 HalfPrecisionColorB = UnpackedCustomData.Color.B;
	
	PackedParam2AsUint32 = (static_cast<uint32>(HalfPrecisionColorG.Encoded) << 16) | 
							static_cast<uint32>(HalfPrecisionColorR.Encoded);
	PackedParam3AsUint32 = static_cast<uint32>(HalfPrecisionColorB.Encoded);

	const FFloat16 HalfPrecisionVectorX = UnpackedCustomData.RightVector.X;
	const FFloat16 HalfPrecisionVectorY = UnpackedCustomData.RightVector.Y;
	const FFloat16 HalfPrecisionVectorZ = UnpackedCustomData.RightVector.Z;
	
	PackedParam4AsUint32 = (static_cast<uint32>(HalfPrecisionVectorY.Encoded) << 16) | 
							static_cast<uint32>(HalfPrecisionVectorX.Encoded);
	PackedParam5AsUint32 = static_cast<uint32>(HalfPrecisionVectorZ.Encoded);
}

//----------------------------------------------------------------------//
// UMassTrafficVehicleVisualizationProcessor 
//----------------------------------------------------------------------//
UMassTrafficVehicleVisualizationProcessor::UMassTrafficVehicleVisualizationProcessor()
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::Client | EProcessorExecutionFlags::Standalone);
	bRequiresGameThreadExecution = true;
	bAutoRegisterWithProcessingPhases = true;
	ExecutionOrder.ExecuteInGroup = UE::MassTraffic::ProcessorGroupNames::VehicleVisualization;
	ExecutionOrder.ExecuteAfter.Add(UE::MassTraffic::ProcessorGroupNames::VehicleVisualizationLOD);
	ExecutionOrder.ExecuteAfter.Add(UE::MassTraffic::ProcessorGroupNames::VehicleBehavior);
	ExecutionOrder.ExecuteAfter.Add(UE::MassTraffic::ProcessorGroupNames::TrafficIntersectionVisualization);
	ExecutionOrder.ExecuteAfter.Add(UMassTrafficParkedVehicleVisualizationProcessor::StaticClass()->GetFName());
	ExecutionOrder.ExecuteAfter.Add(UMassTrafficDamageRepairProcessor::StaticClass()->GetFName());
	ExecutionOrder.ExecuteAfter.Add(UMassTrafficTransformCorrectProcessor::StaticClass()->GetFName());
}

void UMassTrafficVehicleVisualizationProcessor::ConfigureQueries()
{
	Super::ConfigureQueries();

	EntityQuery.AddTagRequirement<FMassTrafficVehicleTag>(EMassFragmentPresence::All);
}

//----------------------------------------------------------------------//
// UMassTrafficVehicleUpdateCustomVisualizationProcessor 
//----------------------------------------------------------------------//
UMassTrafficVehicleUpdateCustomVisualizationProcessor::UMassTrafficVehicleUpdateCustomVisualizationProcessor()
	: EntityQuery(*this)
#if WITH_MASSTRAFFIC_DEBUG
	, DebugEntityQuery(*this)
#endif // WITH_MASSTRAFFIC_DEBUG
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::Client | EProcessorExecutionFlags::Standalone);
	bRequiresGameThreadExecution = true;
	bAutoRegisterWithProcessingPhases = true;
	ExecutionOrder.ExecuteInGroup = UE::MassTraffic::ProcessorGroupNames::VehicleVisualization;
	ExecutionOrder.ExecuteAfter.Add(UE::MassTraffic::ProcessorGroupNames::VehicleVisualizationLOD);
	ExecutionOrder.ExecuteAfter.Add(UE::MassTraffic::ProcessorGroupNames::VehicleBehavior);
	ExecutionOrder.ExecuteAfter.Add(UE::MassTraffic::ProcessorGroupNames::TrafficIntersectionVisualization);
	ExecutionOrder.ExecuteAfter.Add(UMassTrafficVehicleVisualizationProcessor::StaticClass()->GetFName());
}

void UMassTrafficVehicleUpdateCustomVisualizationProcessor::Initialize(UObject& Owner)
{
	Super::Initialize(Owner);
#if WITH_MASSTRAFFIC_DEBUG
	LogOwner = UWorld::GetSubsystem<UMassTrafficSubsystem>(Owner.GetWorld());
#endif // WITH_MASSTRAFFIC_DEBUG
}

void UMassTrafficVehicleUpdateCustomVisualizationProcessor::ConfigureQueries()
{
	EntityQuery.AddTagRequirement<FMassTrafficVehicleTag>(EMassFragmentPresence::All);
	
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassRepresentationFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassRepresentationLODFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassActorFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassTrafficVehicleControlFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddChunkRequirement<FMassVisualizationChunkFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddSharedRequirement<FMassRepresentationSubsystemSharedFragment>(EMassFragmentAccess::ReadWrite);

	EntityQuery.AddRequirement<FMassTrafficRandomFractionFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassTrafficVehicleLightsFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassTrafficVehicleColorsFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassTrafficVehiclePhysicsFragment>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::Optional);

	// Custom Requirement
	EntityQuery.AddRequirement<FMassTrafficMovementInterpSpeed>(EMassFragmentAccess::ReadWrite);

#if WITH_MASSTRAFFIC_DEBUG
	DebugEntityQuery = EntityQuery;
	DebugEntityQuery.AddRequirement<FMassTrafficDebugFragment>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::Optional);
#endif // WITH_MASSTRAFFIC_DEBUG

	EntityQuery.SetChunkFilter(&FMassVisualizationChunkFragment::AreAnyEntitiesVisibleInChunk);
}

void UMassTrafficVehicleUpdateCustomVisualizationProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	APlayerController* Controller = UGameplayStatics::GetPlayerController(Context.GetWorld(), 0);
	if (!Controller) return;
	UMassTrafficOccluderSubsystem* OccluderSubsystem = GetWorld()->GetSubsystem<UMassTrafficOccluderSubsystem>();
	check(OccluderSubsystem)
	
	EntityQuery.ForEachEntityChunk(EntityManager, Context, [this, Controller, OccluderSubsystem](FMassExecutionContext& Context)
	{
		// Get mutable ISMInfos to append instances & custom data to
		UMassRepresentationSubsystem* RepresentationSubsystem = Context.GetMutableSharedFragment<FMassRepresentationSubsystemSharedFragment>().RepresentationSubsystem;
		check(RepresentationSubsystem);
		const FMassInstancedStaticMeshInfoArrayView ISMInfo = RepresentationSubsystem->GetMutableInstancedStaticMeshInfos();

		const TConstArrayView<FMassTrafficRandomFractionFragment> RandomFractionFragments = Context.GetFragmentView<FMassTrafficRandomFractionFragment>();
		const TConstArrayView<FMassTrafficVehiclePhysicsFragment> SimpleVehiclePhysicsFragments = Context.GetFragmentView<FMassTrafficVehiclePhysicsFragment>();
		const TConstArrayView<FMassTrafficVehicleLightsFragment> VehicleStateFragments = Context.GetFragmentView<FMassTrafficVehicleLightsFragment>();
		const TArrayView<FMassTrafficVehicleColorsFragment> VehicleColorsFragments = Context.GetMutableFragmentView<FMassTrafficVehicleColorsFragment>();
		const TConstArrayView<FTransformFragment> TransformFragments = Context.GetFragmentView<FTransformFragment>();
		const TConstArrayView<FMassRepresentationLODFragment> RepresentationLODFragments = Context.GetFragmentView<FMassRepresentationLODFragment>();
		const TArrayView<FMassActorFragment> ActorFragments = Context.GetMutableFragmentView<FMassActorFragment>();
		const TArrayView<FMassRepresentationFragment> VisualizationFragments = Context.GetMutableFragmentView<FMassRepresentationFragment>();

		const TArrayView<FMassTrafficVehicleControlFragment> VehicleControlFragment = Context.GetMutableFragmentView<FMassTrafficVehicleControlFragment>();

		const int32 NumEntities = Context.GetNumEntities();
		for (int32 EntityIdx = 0; EntityIdx < NumEntities; EntityIdx++)
		{
			const FMassEntityHandle Entity = Context.GetEntity(EntityIdx);

			const FMassTrafficRandomFractionFragment& RandomFractionFragment = RandomFractionFragments[EntityIdx];
			const FMassTrafficVehicleLightsFragment& VehicleStateFragment = VehicleStateFragments[EntityIdx];
			FMassTrafficVehicleColorsFragment& VehicleColorsFragment = VehicleColorsFragments[EntityIdx];
			const FTransformFragment& TransformFragment = TransformFragments[EntityIdx];
			const FMassRepresentationLODFragment& RepresentationLODFragment = RepresentationLODFragments[EntityIdx];
			FMassActorFragment& ActorFragment = ActorFragments[EntityIdx];
			FMassRepresentationFragment& RepresentationFragment = VisualizationFragments[EntityIdx];
			FMassTrafficVehicleControlFragment& ControlFragment = VehicleControlFragment[EntityIdx];

			AActor* Actor = ActorFragment.GetMutable();

			FTransform FixedTransform = TransformFragment.GetTransform();

			if (!MassTrafficSettings->CarsColors.IsEmpty())
			{
				if (VehicleColorsFragment.SelectedColor == FLinearColor::Black)
					VehicleColorsFragment.SelectedColor = MassTrafficSettings->CarsColors[FMath::RandRange(0, MassTrafficSettings->CarsColors.Num() - 1)];
			}
			
			// Update active representation
			{
				// Update current representation
				switch (RepresentationFragment.CurrentRepresentation)
				{
					case EMassRepresentationType::StaticMeshInstance:
					{
							bool bInvisible = false;
							for (const auto Hider : OccluderSubsystem->GetTrafficHiders())
							{
								if (Hider->IsInBox(TransformFragment.GetTransform().GetLocation()) && Hider->GetActive())
								{
									bInvisible = true;
								}
							}
						// Add ISMC instance with custom data
							if (RepresentationFragment.StaticMeshDescHandle.IsValid())
							{
								UMassUpdateISMProcessor::UpdateISMTransform(
										Context.GetEntity(EntityIdx),
										ISMInfo[RepresentationFragment.StaticMeshDescHandle.ToIndex()],
										FixedTransform, RepresentationFragment.PrevTransform,
										RepresentationLODFragment.LODSignificance,
										RepresentationFragment.PrevLODSignificance);
								
								const FMassTrafficPackedVehicleInstanceCustomData PackedCustomData =
									FMassTrafficVehicleInstanceCustomData::MakeTrafficVehicleCustomData(
										VehicleStateFragment, RandomFractionFragment, bInvisible,
										VehicleColorsFragment.SelectedColor,
										FixedTransform.GetRotation().GetRightVector(), true);
									
									TArray<float> DataFloats = {PackedCustomData.PackedParam1};
									if (VehicleColorsFragment.SelectedColor != FLinearColor::Transparent)
									{
										DataFloats.Add(PackedCustomData.PackedParamColorRG);
										DataFloats.Add(PackedCustomData.PackedParamColorB);
									}
								
									DataFloats.Add(PackedCustomData.PackedParamRightVectorXY);
									DataFloats.Add(PackedCustomData.PackedParamRightVectorZ);
								
									ISMInfo[RepresentationFragment.StaticMeshDescHandle.ToIndex()].
									AddBatchedCustomDataFloats(DataFloats, RepresentationLODFragment.LODSignificance);
							}
							break;
					}
					case EMassRepresentationType::LowResSpawnedActor:
					{
						// We should always have a PersistentActor if CurrentRepresentation is LowResSpawnedActor
						ensureMsgf(Actor, TEXT("Traffic actor deleted outside of Mass"));

						if (Actor)
						{
							// Teleport actor to simulated position
							const FTransform NewActorTransform = FixedTransform;
							const auto World = GetWorld();

							bool bInvisible = false;
							for (const auto Hider : OccluderSubsystem->GetTrafficHiders())
							{
								if (Hider->IsInBox(TransformFragment.GetTransform().GetLocation()) && Hider->GetActive())
								{
									bInvisible = true;
								}
							}
							
							Context.Defer().PushCommand<FMassDeferredSetCommand>([Actor, NewActorTransform, World, Entity, RepresentationFragment](FMassEntityManager& System)
							{
								Actor->SetActorTransform(NewActorTransform);
							});

							TObjectPtr<UCarBodyThrottleComponent> CarBodyThrottleComponent = Actor->FindComponentByClass<UCarBodyThrottleComponent>();
							TObjectPtr<UWheelRotationComponent> WheelRotationComponent = Actor->FindComponentByClass<UWheelRotationComponent>();
							TObjectPtr<UHiderComponent> HiderComponent = Actor->FindComponentByClass<UHiderComponent>();
							TObjectPtr<UTrafficLightsComponent> LightComponent = Actor->FindComponentByClass<UTrafficLightsComponent>();

							bool bIsDay = true;
							if (LightComponent)
							{
								if (UMaterialParameterCollectionInstance* LightMPC = LightComponent->GetLightMPC())
								{
									float NewLightValue;
									LightMPC->GetScalarParameterValue(LightComponent->GetLightName(), NewLightValue);
									bIsDay = NewLightValue < 0.9;
								}
							}

							Context.Defer().PushCommand<FMassDeferredSetCommand>([bInvisible, VehicleStateFragment, CarBodyThrottleComponent, WheelRotationComponent, HiderComponent, LightComponent, Entity](FMassEntityManager& CallbackEntitySubsystem)
							{
								const FMassTrafficVehicleControlFragment* VehicleControlFragment = CallbackEntitySubsystem.GetFragmentDataPtr<FMassTrafficVehicleControlFragment>(Entity);
								const FMassTrafficVehiclePhysicsFragment* PhysicsFragment = CallbackEntitySubsystem.GetFragmentDataPtr<FMassTrafficVehiclePhysicsFragment>(Entity);
								if (VehicleControlFragment)
								{
									if(CarBodyThrottleComponent && PhysicsFragment) CarBodyThrottleComponent->RotateCarCorpus(PhysicsFragment);
									if(WheelRotationComponent) WheelRotationComponent->RotateWheels(VehicleControlFragment->ForWheelsRotateYawAngle);
									
									if (HiderComponent && bInvisible != HiderComponent->GetIsHidden())
									{
										HiderComponent->SetHidden(bInvisible);
									}
								}
							});
						
							// Has simple vehicle physics?
							if (!SimpleVehiclePhysicsFragments.IsEmpty())
							{
								// Has a UMassTrafficVehicleComponent with wheel mesh references? 
								UMassTrafficVehicleComponent* MassTrafficVehicleComponent = Actor->FindComponentByClass<UMassTrafficVehicleComponent>();
								if (MassTrafficVehicleComponent)
								{
									// Update wheel component transforms from simple vehicle physics sim
									Context.Defer().PushCommand<FMassDeferredSetCommand>([MassTrafficVehicleComponent, Entity](FMassEntityManager& CallbackEntitySubsystem)
									{
										if (CallbackEntitySubsystem.IsEntityValid(Entity))
										{
											// If the simulation LOD changed this frame, removal of the
											// FDataFragment_SimpleVehiclePhysics would have been queued and executed 
											// before this deferred command, thus actually removing the fragment we
											// thought we had via the check above. So we safely check again here for
											// FDataFragment_SimpleVehiclePhysics using an FMassEntityView

											const FMassTrafficVehiclePhysicsFragment* SimpleVehiclePhysicsFragment = CallbackEntitySubsystem.GetFragmentDataPtr<FMassTrafficVehiclePhysicsFragment>(Entity);
											if (SimpleVehiclePhysicsFragment)
											{
												// Init offsets?
												if (MassTrafficVehicleComponent->WheelOffsets.IsEmpty())
												{
													MassTrafficVehicleComponent->InitWheelAttachmentOffsets(SimpleVehiclePhysicsFragment->VehicleSim);
												}
								
												// Update
												MassTrafficVehicleComponent->UpdateWheelComponents(SimpleVehiclePhysicsFragment->VehicleSim);
											}
										}
									});
								}
							}
							
							// Update primitive component custom data
							const FMassTrafficPackedVehicleInstanceCustomData PackedCustomData =
										FMassTrafficVehicleInstanceCustomData::MakeTrafficVehicleCustomData(
											VehicleStateFragment, RandomFractionFragment, bInvisible,
											VehicleColorsFragment.SelectedColor,
											FixedTransform.GetRotation().GetRightVector(), bIsDay);
									
							TArray<float> DataFloats = {PackedCustomData.PackedParam1};
							if (VehicleColorsFragment.SelectedColor != FLinearColor::Transparent)
							{
								DataFloats.Add(PackedCustomData.PackedParamColorRG);
								DataFloats.Add(PackedCustomData.PackedParamColorB);
							}
						
							DataFloats.Add(PackedCustomData.PackedParamRightVectorXY);
							DataFloats.Add(PackedCustomData.PackedParamRightVectorZ);
							
							Actor->ForEachComponent<UPrimitiveComponent>(true, [&PackedCustomData, DataFloats](UPrimitiveComponent* PrimitiveComponent)
							{
								for (int32 i = 0; i < DataFloats.Num(); i++)
								{
									PrimitiveComponent->SetCustomPrimitiveDataFloat(i + 1, DataFloats[i]);
								}
								
							});
						}

						break;
					}
					case EMassRepresentationType::HighResSpawnedActor:
					{
						ensureMsgf(Actor, TEXT("Traffic actor deleted outside of Mass"));

						if (Actor)
						{
							// Teleport actor to simulated position
							const FTransform NewActorTransform = FixedTransform;
							const auto World = GetWorld();

							bool bInvisible = false;
							for (const auto Hider : OccluderSubsystem->GetTrafficHiders())
							{
								if (Hider->IsInBox(TransformFragment.GetTransform().GetLocation()) && Hider->GetActive())
								{
									bInvisible = true;
								}
							}
							
							Context.Defer().PushCommand<FMassDeferredSetCommand>([Actor, NewActorTransform, World, Entity, RepresentationFragment](FMassEntityManager& System)
							{
								Actor->SetActorTransform(NewActorTransform);
							});

							TObjectPtr<UCarBodyThrottleComponent> CarBodyThrottleComponent = Actor->FindComponentByClass<UCarBodyThrottleComponent>();
							TObjectPtr<UWheelRotationComponent> WheelRotationComponent = Actor->FindComponentByClass<UWheelRotationComponent>();
							TObjectPtr<UHiderComponent> HiderComponent = Actor->FindComponentByClass<UHiderComponent>();
							TObjectPtr<UTrafficLightsComponent> LightComponent = Actor->FindComponentByClass<UTrafficLightsComponent>();

							bool bIsDay = true;
							if (LightComponent)
							{
								if (UMaterialParameterCollectionInstance* LightMPC = LightComponent->GetLightMPC())
								{
									float NewLightValue;
									LightMPC->GetScalarParameterValue(LightComponent->GetLightName(), NewLightValue);
									bIsDay = NewLightValue < 0.9;
								}
							}

							Context.Defer().PushCommand<FMassDeferredSetCommand>([bInvisible, VehicleStateFragment, CarBodyThrottleComponent, WheelRotationComponent, HiderComponent, Entity](FMassEntityManager& CallbackEntitySubsystem)
							{
								const FMassTrafficVehicleControlFragment* VehicleControlFragment = CallbackEntitySubsystem.GetFragmentDataPtr<FMassTrafficVehicleControlFragment>(Entity);
								const FMassTrafficVehiclePhysicsFragment* PhysicsFragment = CallbackEntitySubsystem.GetFragmentDataPtr<FMassTrafficVehiclePhysicsFragment>(Entity);
								if (VehicleControlFragment)
								{
									if(CarBodyThrottleComponent && PhysicsFragment) CarBodyThrottleComponent->RotateCarCorpus(PhysicsFragment);
									if(WheelRotationComponent) WheelRotationComponent->RotateWheels(VehicleControlFragment->ForWheelsRotateYawAngle);
									
									if (HiderComponent && bInvisible != HiderComponent->GetIsHidden())
									{
										HiderComponent->SetHidden(bInvisible);
									}
								}
							});
						
							// Has simple vehicle physics?
							if (!SimpleVehiclePhysicsFragments.IsEmpty())
							{
								// Has a UMassTrafficVehicleComponent with wheel mesh references? 
								UMassTrafficVehicleComponent* MassTrafficVehicleComponent = Actor->FindComponentByClass<UMassTrafficVehicleComponent>();
								if (MassTrafficVehicleComponent)
								{
									// Update wheel component transforms from simple vehicle physics sim
									Context.Defer().PushCommand<FMassDeferredSetCommand>([MassTrafficVehicleComponent, Entity](FMassEntityManager& CallbackEntitySubsystem)
									{
										if (CallbackEntitySubsystem.IsEntityValid(Entity))
										{
											// If the simulation LOD changed this frame, removal of the
											// FDataFragment_SimpleVehiclePhysics would have been queued and executed 
											// before this deferred command, thus actually removing the fragment we
											// thought we had via the check above. So we safely check again here for
											// FDataFragment_SimpleVehiclePhysics using an FMassEntityView

											const FMassTrafficVehiclePhysicsFragment* SimpleVehiclePhysicsFragment = CallbackEntitySubsystem.GetFragmentDataPtr<FMassTrafficVehiclePhysicsFragment>(Entity);
											if (SimpleVehiclePhysicsFragment)
											{
												// Init offsets?
												if (MassTrafficVehicleComponent->WheelOffsets.IsEmpty())
												{
													MassTrafficVehicleComponent->InitWheelAttachmentOffsets(SimpleVehiclePhysicsFragment->VehicleSim);
												}
								
												// Update
												MassTrafficVehicleComponent->UpdateWheelComponents(SimpleVehiclePhysicsFragment->VehicleSim);
											}
										}
									});
								}
							}
							
							// Update primitive component custom data
							const FMassTrafficPackedVehicleInstanceCustomData PackedCustomData =
										FMassTrafficVehicleInstanceCustomData::MakeTrafficVehicleCustomData(
											VehicleStateFragment, RandomFractionFragment, bInvisible,
											VehicleColorsFragment.SelectedColor,
											FixedTransform.GetRotation().GetRightVector(), bIsDay);
									
							TArray<float> DataFloats = {PackedCustomData.PackedParam1};
							if (VehicleColorsFragment.SelectedColor != FLinearColor::Transparent)
							{
								DataFloats.Add(PackedCustomData.PackedParamColorRG);
								DataFloats.Add(PackedCustomData.PackedParamColorB);
							}
						
							DataFloats.Add(PackedCustomData.PackedParamRightVectorXY);
							DataFloats.Add(PackedCustomData.PackedParamRightVectorZ);
							
							Actor->ForEachComponent<UPrimitiveComponent>(true, [&PackedCustomData, DataFloats](UPrimitiveComponent* PrimitiveComponent)
							{
								for (int32 i = 0; i < DataFloats.Num(); i++)
								{
									PrimitiveComponent->SetCustomPrimitiveDataFloat(i + 1, DataFloats[i]);
								}
								
							});
						}

						break;
					}
					case EMassRepresentationType::None:
						break;
					default:
						checkf(false, TEXT("Unsupported visual type"));
						break;
				}
			}

			RepresentationFragment.PrevTransform = FixedTransform;
		}
	});

#if WITH_MASSTRAFFIC_DEBUG
	// Debug draw current visualization
	if (GMassTrafficDebugVisualization && LogOwner.IsValid())
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(TEXT("DebugDisplayVisualization")) 

		UWorld* World = EntityManager.GetWorld();
		const UObject* LogOwnerPtr = LogOwner.Get();

		DebugEntityQuery.ForEachEntityChunk(EntityManager, Context, [World, LogOwnerPtr](FMassExecutionContext& Context)
			{
				const int32 NumEntities = Context.GetNumEntities();
				const TConstArrayView<FTransformFragment> TransformList = Context.GetFragmentView<FTransformFragment>();
				const TConstArrayView<FMassTrafficDebugFragment> TrafficDebugFragments = Context.GetFragmentView<FMassTrafficDebugFragment>();
				const TArrayView<FMassRepresentationFragment> VisualizationList = Context.GetMutableFragmentView<FMassRepresentationFragment>();

				for (int EntityIdx = 0; EntityIdx < NumEntities; EntityIdx++)
				{
					const FTransformFragment& TransformFragment = TransformList[EntityIdx];
					FMassRepresentationFragment& Visualization = VisualizationList[EntityIdx];
					const int32 CurrentVisualIdx = static_cast<int32>(Visualization.CurrentRepresentation);
					DrawDebugPoint(World, TransformFragment.GetTransform().GetLocation() + FVector(50.0f, 0.0f, 200.0f), 10.0f, UE::MassLOD::LODColors[CurrentVisualIdx]);

					const bool bVisLogEvenIfOff = TrafficDebugFragments.Num() > 0 && TrafficDebugFragments[EntityIdx].bVisLog;
					if (((Visualization.CurrentRepresentation != EMassRepresentationType::None || bVisLogEvenIfOff) && GMassTrafficDebugVisualization >= 2) || GMassTrafficDebugVisualization >= 3)
					{
						UE_VLOG_LOCATION(LogOwnerPtr, TEXT("MassTraffic Vis"), Log, TransformFragment.GetTransform().GetLocation() + FVector(50.0f, 0.0f, 200.0f), /*Radius*/ 10.0f, UE::MassLOD::LODColors[CurrentVisualIdx], TEXT("%d %d"), CurrentVisualIdx, Context.GetEntity(EntityIdx).Index);
					}
				}
			});
	}
#endif // WITH_MASSTRAFFIC_DEBUG
}

