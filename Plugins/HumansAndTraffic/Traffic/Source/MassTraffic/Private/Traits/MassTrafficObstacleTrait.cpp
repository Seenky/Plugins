// Copyright Epic Games, Inc. All Rights Reserved.

#include "Traits/MassTrafficObstacleTrait.h"
#include "Fragments/MassTrafficFragments.h"

#include "MassCommonFragments.h"
#include "MassEntityTemplateRegistry.h"
#include "MassMovementFragments.h"

void UMassTrafficObstacleTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	BuildContext.AddTag<FMassTrafficObstacleTag>();
	BuildContext.RequireFragment<FTransformFragment>();
	BuildContext.RequireFragment<FMassVelocityFragment>();
	BuildContext.AddFragment<FAgentRadiusFragment>();
}
