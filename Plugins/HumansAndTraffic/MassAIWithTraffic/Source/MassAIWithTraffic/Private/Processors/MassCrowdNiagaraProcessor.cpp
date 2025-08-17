// Fill out your copyright notice in the Description page of Project Settings.


#include "Processors/MassCrowdNiagaraProcessor.h"

#include "Debug/CrowdDebug.h"
#include "MassActorSubsystem.h"
#include "MassCommonFragments.h"
#include "MassCommonTypes.h"
#include "MassCrowdFragments.h"
#include "MassEntityView.h"
#include "MassExecutionContext.h"
#include "MassMovementFragments.h"
#include "MassNavigationFragments.h"
#include "MassRepresentationTypes.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Interface/PawnInteractInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Subsystem/NiagaraCrowdSubsystem.h"
#include "Subsystem/NiagaraCrowdSubsystemSettings.h"
#include "Translators/MassSceneComponentLocationTranslator.h"

int32 GDebugMassCrowdForwardVisibility = 0;
FAutoConsoleVariableRef CVarDebugMassCrowdForwardVisibility(
	TEXT("MassCrowd.DebugForwardVisibility"),
	GDebugMassCrowdForwardVisibility,
	TEXT("MassCrowd debug forward visibility.\n")
	TEXT("0 = Off (default.)\n")
	TEXT("1 = On"),
	ECVF_Cheat
	);

int32 GDebugMassCrowdLocationFix = 0;
FAutoConsoleVariableRef CVarDebugMassCrowdLocationFix(
	TEXT("MassCrowd.DebugLocationFix"),
	GDebugMassCrowdLocationFix,
	TEXT("MassCrowd debug location fix.\n")
	TEXT("0 = Off (default.)\n")
	TEXT("1 = On"),
	ECVF_Cheat
	);



UMassCrowdNiagaraProcessor::UMassCrowdNiagaraProcessor()
	: EntityQuery(*this)
{
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::Movement);
	bRequiresGameThreadExecution = false;
}

void UMassCrowdNiagaraProcessor::ConfigureQueries()
{
	EntityQuery.AddTagRequirement<FMassCrowdTag>(EMassFragmentPresence::All);
	EntityQuery.AddRequirement<FMassSceneComponentWrapperFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassVelocityFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassShouldUseAvoidanceFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FCurrentRepresentationFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassVisibilityFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassActorFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FTranslatorUsageFragment>(EMassFragmentAccess::ReadWrite);
}

void UMassCrowdNiagaraProcessor::Initialize(UObject& Owner)
{
	Super::Initialize(Owner);
}

void UMassCrowdNiagaraProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	const UNiagaraCrowdSubsystemSettings* NiagaraCrowdSettings =  GetDefault<UNiagaraCrowdSubsystemSettings>();

	APlayerController* Controller = UGameplayStatics::GetPlayerController(Context.GetWorld(), 0);
	
	const UNiagaraCrowdSubsystem* NiagaraCrowdSubsystem = Context.GetWorld()->GetSubsystem<UNiagaraCrowdSubsystem>();
	check(NiagaraCrowdSubsystem)
	
	ANiagaraManager* NiagaraManager = NiagaraCrowdSubsystem->GetNiagaraManager();
	TArray<FVector> LocationsArray;
	TArray<FVector> VelocityArray;
	TArray<FVector> ForwardVectorsArray;
	TArray<int32> VisibilityArray;
	TArray<float> LerpArray;
	TArray<AActor*> ActorsToIgnore;

	if (!NiagaraCrowdSettings || !NiagaraManager || !Controller) return;
	if (!NiagaraCrowdSettings->bUseNiagaraParticlesForCrowd) return;
	
	EntityQuery.ForEachEntityChunk(EntityManager, Context, [this, NiagaraCrowdSubsystem, Controller,
						&LocationsArray, &VelocityArray, &LerpArray, &ForwardVectorsArray, &VisibilityArray,
						&ActorsToIgnore, NiagaraCrowdSettings](FMassExecutionContext& Context)
	{
		const TConstArrayView<FMassSceneComponentWrapperFragment> ComponentList = Context.GetFragmentView<FMassSceneComponentWrapperFragment>();
		const TArrayView<FTransformFragment> LocationList = Context.GetMutableFragmentView<FTransformFragment>();
		const TArrayView<FMassVelocityFragment> VelocityList = Context.GetMutableFragmentView<FMassVelocityFragment>();
		TArrayView<FMassShouldUseAvoidanceFragment> ShouldUseAvoidanceFragments = Context.GetMutableFragmentView<FMassShouldUseAvoidanceFragment>();
		TArrayView<FCurrentRepresentationFragment> CurrentRepresentationFragments = Context.GetMutableFragmentView<FCurrentRepresentationFragment>();
		TArrayView<FMassVisibilityFragment> VisibilityFragments = Context.GetMutableFragmentView<FMassVisibilityFragment>();
		const TArrayView<FMassActorFragment> ActorList = Context.GetMutableFragmentView<FMassActorFragment>();
		const TArrayView<FTranslatorUsageFragment> TranslatorUsageFragments = Context.GetMutableFragmentView<FTranslatorUsageFragment>();
		
		for (auto Actor : ActorList)
		{
			ActorsToIgnore.AddUnique(Actor.GetMutable());
		}

		FVector PlayerLocation;
		FRotator PlayerRotation;
		Controller->GetPlayerViewPoint(PlayerLocation, PlayerRotation);
		
		const int32 NumEntities = Context.GetNumEntities();
		for (int32 i = 0; i < NumEntities; ++i)
		{
			FMassActorFragment& ActorInfo = ActorList[i];
			FMassShouldUseAvoidanceFragment& ShouldUseAvoidanceFragment = ShouldUseAvoidanceFragments[i];
			FTranslatorUsageFragment& TranslatorUsageFragment = TranslatorUsageFragments[i];
			FCurrentRepresentationFragment& CurrentRepresentationFragment = CurrentRepresentationFragments[i];
			FMassVisibilityFragment& VisibilityFragment = VisibilityFragments[i];
			
			if (const USceneComponent* AsComponent = ComponentList[i].Component.Get())
			{
				FVector CurrentEntityLocation = LocationList[i].GetTransform().GetLocation() +
												FVector(0.f, 0.f, AsComponent->Bounds.BoxExtent.Z);
				
				const float DistToPlayer = FVector::Dist(PlayerLocation, CurrentEntityLocation);
				
				int32 VisibilityIndex;
				float LerpSpeed = NiagaraCrowdSettings->LerpSpeed;

				//Calculate current visibility and representation type
				CalculateVisibilityAndRepresentation(NiagaraCrowdSettings, Controller, CurrentEntityLocation,
													 PlayerLocation, PlayerRotation, ActorInfo,
													 VisibilityFragment, CurrentRepresentationFragment,LerpSpeed);

				//Promote special cases like humans hider
				PromoteSpecialCases(NiagaraCrowdSubsystem, NiagaraCrowdSettings, LocationList[i],
									CurrentRepresentationFragment, LerpSpeed,
									CurrentEntityLocation, PlayerRotation);

				//Hit traces to snap entities to ground
				SnapEntitiesToGround(LocationList[i], CurrentRepresentationFragment, ActorInfo,
								     NiagaraCrowdSettings, CurrentEntityLocation, DistToPlayer, AsComponent);

				//Draw debug for visibility
				DrawDebugInfo(CurrentRepresentationFragment, LocationList[i], CurrentEntityLocation, AsComponent);

				CheckVisibilityRepresentation(CurrentRepresentationFragment, VisibilityIndex);

				//Check if actors should use translation and avoidance by visibility
				ChangeAvoidanceFragments(ShouldUseAvoidanceFragment, TranslatorUsageFragment,
										 CurrentRepresentationFragment, ActorInfo);

				//Add data for niagara particles
				LocationsArray.Add(CurrentEntityLocation);
				VelocityArray.Add(VelocityList[i].Value);
				ForwardVectorsArray.Add(LocationList[i].GetTransform().GetRotation().GetForwardVector());
				VisibilityArray.Add(VisibilityIndex);
				LerpArray.Add(LerpSpeed);

				//Update actors visibility
				bool bVisibleActor = CurrentRepresentationFragment.CurrentRepresentationType == ECurrentRepresentationType::Actor;
				if (ActorInfo.IsValid() && !NiagaraCrowdSettings->bUseOnlyNiagaraParticles)
				{
					//We need to set visibility on next tick to prevent teleport flickering
					FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([this, ActorInfo, bVisibleActor, LerpSpeed] (float DeltaTime) mutable
					{
						AActor* MutableActor = ActorInfo.GetMutable();
						if (!MutableActor) return false;
						if (MutableActor->Implements<UPawnInteractInterface>())
								IPawnInteractInterface::Execute_SetPawnVisibility(MutableActor, bVisibleActor, LerpSpeed);
						return false;
					}));
				}
			}
		}
	});

	if (NiagaraManager)
	{
		NiagaraManager->SetTranslatedLocation(LocationsArray);
		NiagaraManager->SetTranslatedVelocity(VelocityArray);
		NiagaraManager->SetTranslatedForwardVectors(ForwardVectorsArray);
		NiagaraManager->SetTranslatedVisibility(VisibilityArray);
		NiagaraManager->SetTranslatedLerpSpeed(LerpArray);
	}
}

void UMassCrowdNiagaraProcessor::CalculateVisibilityAndRepresentation(
	const UNiagaraCrowdSubsystemSettings* NiagaraSettings,
	const APlayerController* Controller,
	const FVector& EntityLocation,
	const FVector& PlayerLocation,
	const FRotator& PlayerRotation,
	FMassActorFragment& ActorInfo,
	FMassVisibilityFragment& VisibilityFragment,
	FCurrentRepresentationFragment& Representation,
	float& LerpSpeed) const
{
	const float DistToPlayer = FVector::Dist(PlayerLocation, EntityLocation);
	
	const TArray<AActor*> ActorsToIgnore = { Controller->GetPawn(), ActorInfo.GetMutable() };

	if (!NiagaraSettings->bUseOnlyNiagaraParticles)
	{
		const bool bInDistToSwap = DistToPlayer < NiagaraSettings->DistanceToSwapNiagaraParticle;
		const bool bInDistToHighLOD = DistToPlayer > NiagaraSettings->HighLODVisibilityDistance;
		
		VisibilityFragment.bIsInFov = IsInFOV(Controller, EntityLocation, PlayerLocation, PlayerRotation);
					
		if (!VisibilityFragment.bIsInFov)
		{
			VisibilityFragment.bIsOccluded = true;
		}
		else if (bInDistToSwap)
		{
			const bool bOccluded = IsOccluded(ActorsToIgnore, EntityLocation, PlayerLocation);
			if (bOccluded != VisibilityFragment.bIsOccluded)
			{
				LerpSpeed = 0;
			}
			VisibilityFragment.bIsOccluded = bOccluded;
		}

		if (!VisibilityFragment.bIsOccluded)
		{
			if (bInDistToSwap && bInDistToHighLOD)
				Representation.CurrentRepresentationType = ECurrentRepresentationType::Actor;
			else if (!bInDistToSwap)
				Representation.CurrentRepresentationType = ECurrentRepresentationType::Niagara;
			else
				Representation.CurrentRepresentationType = ECurrentRepresentationType::None;
		}
		else
		{
			if (!bInDistToSwap && VisibilityFragment.bIsInFov)
				Representation.CurrentRepresentationType = ECurrentRepresentationType::Niagara;
			else
				Representation.CurrentRepresentationType = ECurrentRepresentationType::None;
		}
	}
	else
	{
		Representation.CurrentRepresentationType = DistToPlayer < NiagaraSettings->DistanceToSwapNiagaraParticle ?
				ECurrentRepresentationType::None : ECurrentRepresentationType::Niagara;
	}

	VisibilityFragment.bIsVisible = Representation.CurrentRepresentationType != ECurrentRepresentationType::None;
}

void UMassCrowdNiagaraProcessor::SnapEntitiesToGround(
	FTransformFragment& TransformFragment,
	const FCurrentRepresentationFragment& Representation,
	const FMassActorFragment& ActorInfo,
	const UNiagaraCrowdSubsystemSettings* NiagaraSettings,
	FVector& EntityLocation,
	const float& DistToPlayer,
	const USceneComponent* AsComponent) const
{
	if (!NiagaraSettings->bEnableLocationFix) return;
	
	bool bHit = false;
	FHitResult OutHit;
	
	if (DistToPlayer < NiagaraSettings->DistanceToOptimizeTraces && Representation.CurrentRepresentationType == ECurrentRepresentationType::Actor)
	{
		FVector StartTrace = TransformFragment.GetTransform().GetLocation() + FVector(0, 0, NiagaraSettings->LocationFixUpDistance);
		FVector EndTrace = TransformFragment.GetTransform().GetLocation() - FVector(0, 0, NiagaraSettings->LocationFixUpDistance);
		FCollisionQueryParams Params;
		if (ActorInfo.Get())
			Params.AddIgnoredActor(ActorInfo.Get());
		Params.bTraceComplex = true;
		bHit = GetWorld()->SweepSingleByChannel(OutHit, StartTrace,
												EndTrace, FQuat::Identity, ECC_Visibility,
												FCollisionShape::MakeSphere(10.0f), Params);
	}
				
	if (bHit)
	{
		FVector NewLocation = FVector(TransformFragment.GetTransform().GetLocation().X,
									  TransformFragment.GetTransform().GetLocation().Y,
									  OutHit.Location.Z);

		if (GDebugMassCrowdLocationFix)
			DrawDebugPoint(GetWorld(), NewLocation, 5.0f, FColor::Green, false, 0.0f);

		const FVector ApplyLocation = FMath::VInterpTo(TransformFragment.GetMutableTransform().GetLocation(),
			FVector(EntityLocation.X, EntityLocation.Y, NewLocation.Z), GetWorld()->GetDeltaSeconds(), NiagaraSettings->LocationFixInterpSpeed);
		TransformFragment.GetMutableTransform().SetLocation(ApplyLocation);
		EntityLocation = NewLocation + FVector(0.f, 0.f, AsComponent->Bounds.BoxExtent.Z);
	}
}

void UMassCrowdNiagaraProcessor::PromoteSpecialCases(
	const UNiagaraCrowdSubsystem* NiagaraSubsystem,
	const UNiagaraCrowdSubsystemSettings* NiagaraSettings,
	const FTransformFragment& TransformFragment,
	FCurrentRepresentationFragment& RepresentationFragment,
	float& LerpSpeed,
	const FVector& EntityLocation,
	const FRotator& PlayerRotation) const
{
	for (const auto Hider : NiagaraSubsystem->GetAllHiders())
	{
		if (Hider->IsInBox(EntityLocation) && Hider->GetActive())
		{
			RepresentationFragment.CurrentRepresentationType = ECurrentRepresentationType::None;
			LerpSpeed = Hider->GetLerpSpeed();
		}
	}

	if (NiagaraSettings->bHideForwardPeople)
	{
		const float DotToHuman = FVector::DotProduct(PlayerRotation.Vector(), TransformFragment.GetTransform().GetRotation().GetForwardVector());
		if (GDebugMassCrowdForwardVisibility)
			DrawDebugString(GetWorld(), TransformFragment.GetTransform().GetLocation(),
							FString::SanitizeFloat(DotToHuman), nullptr, FColor::Emerald, 0.0f);
		if (DotToHuman < -0.5f)
		{
			RepresentationFragment.CurrentRepresentationType = ECurrentRepresentationType::None;
			LerpSpeed = 0.0f;
		}
	}
}

void UMassCrowdNiagaraProcessor::CheckVisibilityRepresentation(const FCurrentRepresentationFragment& Representation, int32& VisibilityIndex)
{
	switch (Representation.CurrentRepresentationType)
	{
	case ECurrentRepresentationType::Actor:
		{
			VisibilityIndex = 1;
			break;
		}
	case ECurrentRepresentationType::Niagara:
		{
			VisibilityIndex = 0;
			break;
		}
	case ECurrentRepresentationType::None:
		{
			VisibilityIndex = 1;
			break;
		}
	}
}

void UMassCrowdNiagaraProcessor::ChangeAvoidanceFragments(
	FMassShouldUseAvoidanceFragment& ShouldUseAvoidanceFragment,
	FTranslatorUsageFragment& TranslatorUsageFragment,
	const FCurrentRepresentationFragment& CurrentRepresentationFragment,
	const FMassActorFragment& ActorFragment)
{
	switch (CurrentRepresentationFragment.CurrentRepresentationType)
	{
		case ECurrentRepresentationType::Actor:
			{
				ShouldUseAvoidanceFragment.bShouldUseAvoidance = true;
				TranslatorUsageFragment.bShouldUseTranslation = true;
				break;
			}
		case ECurrentRepresentationType::Niagara:
			{
				ShouldUseAvoidanceFragment.bShouldUseAvoidance = false;
				TranslatorUsageFragment.bShouldUseTranslation = false;
				break;
			}
		case ECurrentRepresentationType::None:
			{
				if (ActorFragment.Get() && ActorFragment.Get()->Implements<UPawnInteractInterface>())
				{
					if (!IPawnInteractInterface::Execute_GetIsVisible(ActorFragment.Get()))
					{
						ShouldUseAvoidanceFragment.bShouldUseAvoidance = false;
						TranslatorUsageFragment.bShouldUseTranslation = false;
					}
					else
					{
						ShouldUseAvoidanceFragment.bShouldUseAvoidance = true;
						TranslatorUsageFragment.bShouldUseTranslation = true;
					}
				}
				else
				{
					ShouldUseAvoidanceFragment.bShouldUseAvoidance = false;
					TranslatorUsageFragment.bShouldUseTranslation = false;
				}
				break;
			}
		default:
			break;
	}
}

void UMassCrowdNiagaraProcessor::DrawDebugInfo(
	const FCurrentRepresentationFragment& Representation,
	const FTransformFragment& TransformFragment,
	const FVector& EntityLocation,
	const USceneComponent* AsComponent) const
{
#if !UE_BUILD_SHIPPING
	const FVector DebugLocation = TransformFragment.GetTransform().GetLocation() + FVector(0.0f, 0.0f, 250.0f);
	if (GDebugNiagaraCrowdVisibility)
	{
		switch (Representation.CurrentRepresentationType)
		{
			case ECurrentRepresentationType::Actor:
				{
					DrawDebugString(GetWorld(), DebugLocation, "VISIBLE", nullptr, FColor::Green, 0, false, 1);
					DrawDebugBox(GetWorld(), EntityLocation,  AsComponent->Bounds.BoxExtent, FColor::Green, false, 0);
					break;
				}
			case ECurrentRepresentationType::Niagara:
				{
					DrawDebugString(GetWorld(),DebugLocation, "NIAGARA", nullptr, FColor::Blue, 0, false, 1);
					DrawDebugBox(GetWorld(), EntityLocation,  AsComponent->Bounds.BoxExtent, FColor::Blue, false, 0);
					break;
				}
			case ECurrentRepresentationType::None:
				{
					DrawDebugString(GetWorld(),DebugLocation, "INVISIBLE", nullptr, FColor::Red, 0, false, 1);
					DrawDebugBox(GetWorld(), EntityLocation,  AsComponent->Bounds.BoxExtent, FColor::Red, false, 0);
					break;
				}
			default:
				break;
		}
	}
#endif
}

bool UMassCrowdNiagaraProcessor::IsOccluded(
	const TArray<AActor*>& ActorsToIgnore,
	const FVector& EntityLocation,
	const FVector& PlayerLocation) const
{
	const UNiagaraCrowdSubsystemSettings* NiagaraCrowdSettings = GetDefault<UNiagaraCrowdSubsystemSettings>();

	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActors(ActorsToIgnore);
	FCollisionResponseParams ResponseParams;

	FHitResult HitResult;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(
	HitResult,
	PlayerLocation,
	EntityLocation,
	ECC_Visibility,
	CollisionParams
	);

	if (HitResult.GetActor() && NiagaraCrowdSettings)
	{
		if (HitResult.GetActor()->ActorHasTag(NiagaraCrowdSettings->TagToIgnoreVisibilityCheck)) return true;
	}
		
	return bHit;
}

// ReSharper disable once CppMemberFunctionMayBeStatic
bool UMassCrowdNiagaraProcessor::IsInFOV(
	const APlayerController* Controller,
	const FVector& EntityLocation,
	const FVector& PlayerLocation,
	const FRotator& PlayerRotation) const
{
	bool bInFOV;
	if (const APlayerCameraManager* CameraManager = Controller->PlayerCameraManager)
	{
		const float FOVAngle = CameraManager->GetFOVAngle();
		const FVector ToEntityDir = (EntityLocation - PlayerLocation).GetSafeNormal();
		const FVector ViewDir = PlayerRotation.Vector().GetSafeNormal();
                    
		const float DotProduct = FVector::DotProduct(ViewDir, ToEntityDir);
		const float CosHalfFOV = FMath::Cos(FMath::DegreesToRadians(FOVAngle));
		bInFOV = (DotProduct >= CosHalfFOV);
	}
	else
	{
		bInFOV = true;
	}

	return bInFOV;
}

