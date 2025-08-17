// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/ZoneGraphActorCopy.h"

// Sets default values
AZoneGraphActorCopy::AZoneGraphActorCopy(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	ZoneShapeComponent = CreateDefaultSubobject<UZoneShapeComponent>(TEXT("ShapeComp"));

#if WITH_EDITORONLY_DATA
	bIsSpatiallyLoaded = false;
#endif
}

// Called when the game starts or when spawned
void AZoneGraphActorCopy::BeginPlay()
{
	Super::BeginPlay();
	
}

void AZoneGraphActorCopy::CopySplinePoints(USplineComponent* SplineComponent)
{
	if (!SplineComponent) return;

	TArray<FZoneShapePoint> LanePoints;
	
	int32 NumberOfSplinePoints = SplineComponent->GetNumberOfSplinePoints();
	for (int32 i = 0; i < NumberOfSplinePoints; ++i)
	{
		FVector SplinePointLocation = SplineComponent->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
		
		FVector InTangent = SplineComponent->GetTangentAtSplinePoint(i, ESplineCoordinateSpace::World);
		FVector OutTangent = SplineComponent->GetTangentAtSplinePoint(i, ESplineCoordinateSpace::World);
		
		FZoneShapePoint LanePoint;
		LanePoint.Position = SplinePointLocation;
		LanePoint.Type = FZoneShapePointType::AutoBezier;

		LanePoints.Add(LanePoint);
	}
	
	ZoneShapeComponent->GetMutablePoints().Empty();
	ZoneShapeComponent->GetMutablePoints().Append(LanePoints);
	
	ZoneShapeComponent->UpdateShape();
	ZoneShapeComponent->UpdateShapeConnectors();
	ZoneShapeComponent->UpdateConnectedShapes();
	
	FVector SplineLocation = SplineComponent->GetOwner()->GetActorLocation();
	SetActorLocation(SplineLocation);
}

// Called every frame
void AZoneGraphActorCopy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

