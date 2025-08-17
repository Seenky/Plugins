// Copyright Epic Games, Inc. All Rights Reserved.


#include "Actors/MassTrafficFieldActor.h"

AMassTrafficFieldActor::AMassTrafficFieldActor()
{
 	FieldComponent = CreateDefaultSubobject<UMassTrafficFieldComponent>(TEXT("FieldComponent"));
	RootComponent = FieldComponent;
}
