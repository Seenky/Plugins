// Fill out your copyright notice in the Description page of Project Settings.


#include "Processors/MassCrowdPerformanceProcessor.h"

#include "MassActorSubsystem.h"
#include "MassCommonTypes.h"
#include "MassCrowdFragments.h"
#include "MassExecutionContext.h"
#include "MassMovementFragments.h"
#include "MassNavigationFragments.h"
#include "MassRepresentationFragments.h"
#include "Interface/PawnInteractInterface.h"
#include "Translators/MassSceneComponentLocationTranslator.h"

UMassCrowdPerformanceProcessor::UMassCrowdPerformanceProcessor()
	: EntityQuery(*this)
{
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::Movement);
	bRequiresGameThreadExecution = false;
}

void UMassCrowdPerformanceProcessor::ConfigureQueries()
{
	EntityQuery.AddTagRequirement<FMassCrowdTag>(EMassFragmentPresence::All);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassActorFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassRepresentationLODFragment>(EMassFragmentAccess::ReadWrite);
}

void UMassCrowdPerformanceProcessor::Initialize(UObject& Owner)
{
	Super::Initialize(Owner);
}

void UMassCrowdPerformanceProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	EntityQuery.ForEachEntityChunk(EntityManager, Context, [this](FMassExecutionContext& Context)
	{
		const TArrayView<FTransformFragment> LocationList = Context.GetMutableFragmentView<FTransformFragment>();
		const TArrayView<FMassActorFragment> ActorList = Context.GetMutableFragmentView<FMassActorFragment>();
		const TConstArrayView<FMassRepresentationLODFragment> RepresentationFragments = Context.GetFragmentView<FMassRepresentationLODFragment>();
		
		const int32 NumEntities = Context.GetNumEntities();
		
		for (int32 i = 0; i < NumEntities; ++i)
		{
			FMassActorFragment& ActorInfo = ActorList[i];
			FTransformFragment& TransformFragment = LocationList[i];
			const FMassRepresentationLODFragment& RepresentationLODFragment = RepresentationFragments[i];

			AActor* MutableActor = ActorInfo.GetMutable();
			if (MutableActor && MutableActor->Implements<UPawnInteractInterface>())
			{
				IPawnInteractInterface::Execute_UpdatePerformance(MutableActor, RepresentationLODFragment.LOD);
			}
		}
	});
}
