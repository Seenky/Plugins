// Copyright Epic Games, Inc. All Rights Reserved.

#include "Traits/MassTrafficIntersectionSimulationTrait.h"
#include "Fragments/MassTrafficFragments.h"

#include "MassEntityTemplateRegistry.h"
#include "MassCommonFragments.h"

void UMassTrafficIntersectionSimulationTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	BuildContext.AddTag<FMassTrafficIntersectionTag>();
	BuildContext.RequireFragment<FMassTrafficIntersectionFragment>();
	BuildContext.RequireFragment<FTransformFragment>();
}
