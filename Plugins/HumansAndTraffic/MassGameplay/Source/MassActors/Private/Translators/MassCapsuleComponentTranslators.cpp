// Copyright Epic Games, Inc. All Rights Reserved.

#include "Translators/MassCapsuleComponentTranslators.h"
#include "MassCommonTypes.h"
#include "MassEntityManager.h"
#include "MassExecutionContext.h"
#include "Components/CapsuleComponent.h"


//----------------------------------------------------------------------//
// UMassCapsuleTransformToMassTranslator
//----------------------------------------------------------------------//
UMassCapsuleTransformToMassTranslator::UMassCapsuleTransformToMassTranslator()
	: EntityQuery(*this)
{
	ExecutionFlags = (int32)EProcessorExecutionFlags::AllNetModes;
	RequiredTags.Add<FMassCapsuleTransformCopyToMassTag>();
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::SyncWorldToMass;
}

void UMassCapsuleTransformToMassTranslator::ConfigureQueries()
{
	AddRequiredTagsToQuery(EntityQuery);
	EntityQuery.AddRequirement<FCapsuleComponentWrapperFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FTranslatorUsageFragment>(EMassFragmentAccess::ReadOnly);
}

void UMassCapsuleTransformToMassTranslator::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	EntityQuery.ForEachEntityChunk(EntityManager, Context, [this](FMassExecutionContext& Context)
		{
			const TConstArrayView<FCapsuleComponentWrapperFragment> CapsuleComponentList = Context.GetFragmentView<FCapsuleComponentWrapperFragment>();
			const TArrayView<FTransformFragment> LocationList = Context.GetMutableFragmentView<FTransformFragment>();
			const TConstArrayView<FTranslatorUsageFragment> TranslatorUsageFragments = Context.GetFragmentView<FTranslatorUsageFragment>();
			for (int i = 0; i < CapsuleComponentList.Num(); ++i)
			{
				if (const UCapsuleComponent* CapsuleComp = CapsuleComponentList[i].Component.Get())
				{
					if (!TranslatorUsageFragments[i].bShouldUseTranslation) continue;
					LocationList[i].GetMutableTransform() = CapsuleComp->GetComponentTransform();
				}
			}
		});
}

//----------------------------------------------------------------------//
// UMassTransformToActorCapsuleTranslator
//----------------------------------------------------------------------//
UMassTransformToActorCapsuleTranslator::UMassTransformToActorCapsuleTranslator()
	: EntityQuery(*this)
{
	ExecutionFlags = (int32)EProcessorExecutionFlags::AllNetModes;
	RequiredTags.Add<FMassCapsuleTransformCopyToActorTag>();
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::UpdateWorldFromMass;
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::Movement);
	bRequiresGameThreadExecution = true;
}

void UMassTransformToActorCapsuleTranslator::ConfigureQueries()
{
	AddRequiredTagsToQuery(EntityQuery);
	EntityQuery.AddRequirement<FCapsuleComponentWrapperFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FTranslatorUsageFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.RequireMutatingWorldAccess(); // due to mutating World by setting component transform
}

void UMassTransformToActorCapsuleTranslator::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	EntityQuery.ForEachEntityChunk(EntityManager, Context, [this](FMassExecutionContext& Context)
		{
			const TArrayView<FCapsuleComponentWrapperFragment> CapsuleComponentList = Context.GetMutableFragmentView<FCapsuleComponentWrapperFragment>();
			const TConstArrayView<FTransformFragment> LocationList = Context.GetFragmentView<FTransformFragment>();
		    const TConstArrayView<FTranslatorUsageFragment> TranslatorUsageFragments = Context.GetFragmentView<FTranslatorUsageFragment>();
			for (int i = 0; i < CapsuleComponentList.Num(); ++i)
			{
				if (UCapsuleComponent* CapsuleComp = CapsuleComponentList[i].Component.Get())
				{
					if (!TranslatorUsageFragments[i].bShouldUseTranslation) continue;
					CapsuleComp->SetWorldTransform(LocationList[i].GetTransform(), /*bSweep=*/false);
				}
			}
		});
}
