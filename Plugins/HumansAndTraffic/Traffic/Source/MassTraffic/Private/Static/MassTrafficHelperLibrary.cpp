// Fill out your copyright notice in the Description page of Project Settings.


#include "Static/MassTrafficHelperLibrary.h"

bool UMassTrafficHelperLibrary::IsOccluded(
	UWorld* World,
	const TArray<AActor*>& IgnoredActors,
	const FVector& StartLocation,
	const FVector& EndLocation)
{
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActors(IgnoredActors);
	FCollisionResponseParams ResponseParams;

	FHitResult HitResult;
	const bool bHit = World->LineTraceSingleByChannel(
	HitResult,
	StartLocation,
	EndLocation,
	ECC_Visibility,
	CollisionParams
	);

	if (HitResult.GetActor())
	{
		return true;
	}
		
	return !bHit;
}

bool UMassTrafficHelperLibrary::IsInFOV(
	const APlayerController* Controller,
	const FVector& StartLocation,
	const FVector& EndLocation,
	const FRotator& Rotation)
{
	bool bInFOV;
	if (const APlayerCameraManager* CameraManager = Controller->PlayerCameraManager)
	{
		const float FOVAngle = CameraManager->GetFOVAngle();
		const FVector ToEntityDir = (EndLocation - StartLocation).GetSafeNormal();
		const FVector ViewDir = Rotation.Vector().GetSafeNormal();
                    
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
