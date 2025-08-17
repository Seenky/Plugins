// Copyright Epic Games, Inc. All Rights Reserved.

#include "Translators/MassSceneComponentLocationTranslator.h"
#include "MassCommonTypes.h"
#include "Components/SceneComponent.h"
#include "MassExecutionContext.h"

//----------------------------------------------------------------------//
//  UMassSceneComponentLocationToMassTranslator
//----------------------------------------------------------------------//
UMassSceneComponentLocationToMassTranslator::UMassSceneComponentLocationToMassTranslator()
	: EntityQuery(*this)
{
	ExecutionFlags = (int32)EProcessorExecutionFlags::AllNetModes;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::SyncWorldToMass;
	RequiredTags.Add<FMassSceneComponentLocationCopyToMassTag>();
}

void UMassSceneComponentLocationToMassTranslator::ConfigureQueries()
{
	AddRequiredTagsToQuery(EntityQuery);
	EntityQuery.AddRequirement<FMassSceneComponentWrapperFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FTranslatorUsageFragment>(EMassFragmentAccess::ReadOnly);
}

void UMassSceneComponentLocationToMassTranslator::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	EntityQuery.ForEachEntityChunk(EntityManager, Context, [this](FMassExecutionContext& Context)
	{
		const TConstArrayView<FMassSceneComponentWrapperFragment> ComponentList = Context.GetFragmentView<FMassSceneComponentWrapperFragment>();
		const TArrayView<FTransformFragment> LocationList = Context.GetMutableFragmentView<FTransformFragment>();
		const TConstArrayView<FTranslatorUsageFragment> TranslatorUsageFragments = Context.GetFragmentView<FTranslatorUsageFragment>();

		const int32 NumEntities = Context.GetNumEntities();
		for (int32 i = 0; i < NumEntities; ++i)
		{
			if (const USceneComponent* AsComponent = ComponentList[i].Component.Get())
			{
				if (!TranslatorUsageFragments[i].bShouldUseTranslation) continue;
				LocationList[i].GetMutableTransform().SetLocation(AsComponent->GetComponentTransform().GetLocation() - FVector(0.f, 0.f, AsComponent->Bounds.BoxExtent.Z));
			}
		}
	});
}

//----------------------------------------------------------------------//
//  UMassSceneComponentLocationToActorTranslator
//----------------------------------------------------------------------//
UMassSceneComponentLocationToActorTranslator::UMassSceneComponentLocationToActorTranslator()
	: EntityQuery(*this)
{
	ExecutionFlags = (int32)EProcessorExecutionFlags::AllNetModes;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::UpdateWorldFromMass;
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::Movement);
	RequiredTags.Add<FMassSceneComponentLocationCopyToActorTag>();
	bRequiresGameThreadExecution = true;
}

void UMassSceneComponentLocationToActorTranslator::ConfigureQueries()
{
	AddRequiredTagsToQuery(EntityQuery);
	EntityQuery.AddRequirement<FMassSceneComponentWrapperFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FTranslatorUsageFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.RequireMutatingWorldAccess(); // due to mutating World by setting actor's location
}

void UMassSceneComponentLocationToActorTranslator::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	EntityQuery.ForEachEntityChunk(EntityManager, Context, [this](FMassExecutionContext& Context)
		{
			const TConstArrayView<FMassSceneComponentWrapperFragment> ComponentList = Context.GetFragmentView<FMassSceneComponentWrapperFragment>();
			const TArrayView<FTransformFragment> LocationList = Context.GetMutableFragmentView<FTransformFragment>();
			const TConstArrayView<FTranslatorUsageFragment> TranslatorUsageFragments = Context.GetFragmentView<FTranslatorUsageFragment>();

			const int32 NumEntities = Context.GetNumEntities();
			for (int32 i = 0; i < NumEntities; ++i)
			{
				if (USceneComponent* AsComponent = ComponentList[i].Component.Get())
				{
					if (!TranslatorUsageFragments[i].bShouldUseTranslation) continue;
					AsComponent->SetWorldLocation(LocationList[i].GetTransform().GetLocation() + FVector(0.f, 0.f, AsComponent->Bounds.BoxExtent.Z));
				}
			}
		});
}
