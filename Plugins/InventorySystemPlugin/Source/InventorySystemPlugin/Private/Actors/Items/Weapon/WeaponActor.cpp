// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/Items/Weapon/WeaponActor.h"

#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"


AWeaponActor::AWeaponActor()
{
	PrimaryActorTick.bCanEverTick = true;

	CurrentWeaponState = EWeaponState::Idle;
}

void AWeaponActor::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AWeaponActor, CurrentWeaponState, COND_OwnerOnly);
}

//Main server shoot logic
void AWeaponActor::Server_Shoot_Implementation(const bool bShoot)
{
	if (CurrentWeaponState == EWeaponState::Reloading || !GetOwner()) return;
	if (!GetItemObject()->GetDynamicItemData().GetPtr<FWeaponData>()) return;

	const UCameraComponent* CameraComponent = GetOwner()->FindComponentByClass<UCameraComponent>();
	if (!CameraComponent) return;

	//Auto reload logic
	/*if (GetDynamicData().Get<FWeaponData>().CurrentAmmo == 0)
	{
		Server_Reload();
		return;
	}*/

	if (bShoot)
	{
		if (CurrentWeaponState == EWeaponState::Preparing || CurrentWeaponState == EWeaponState::Shooting) return;

		FVector InDirection = CameraComponent->GetForwardVector();
		
		ShootLogic(InDirection);
		NetMulticast_Shoot(InDirection);
		CurrentWeaponState = EWeaponState::Shooting;
		if (GetItemObject()->GetDynamicItemData().Get<FWeaponData>().bAuto)
		{
			GetWorldTimerManager().SetTimer(ShootTimerHandle, FTimerDelegate::CreateLambda([this, &InDirection, CameraComponent]
				{
					InDirection = CameraComponent->GetForwardVector();
					ShootLogic(InDirection);
					NetMulticast_Shoot(InDirection);
				}), GetItemObject()->GetDynamicItemData().Get<FWeaponData>().FireRate, true);
		}
		else
		{
			PrepareWeaponLogic();
		}
	}
	else
	{
		PrepareWeaponLogic();
	}
}

//Reload weapon dependent on weapon reload time and max bullets
void AWeaponActor::Server_Reload_Implementation()
{
	if (!GetItemObject()->GetDynamicItemData().GetPtr<FWeaponData>()) return;
	
	// ReSharper disable once CppTooWideScopeInitStatement
	const FWeaponData CurrentWeaponData = GetItemObject()->GetDynamicItemData().GetMutable<FWeaponData>();

	if (CurrentWeaponData.CurrentAmmo == CurrentWeaponData.MaxAmmo) return;
	
	if (CurrentWeaponState != EWeaponState::Reloading)
	{
		if (GetWorldTimerManager().IsTimerActive(ShootTimerHandle)) GetWorldTimerManager().ClearTimer(ShootTimerHandle);
		if (GetWorldTimerManager().IsTimerActive(ResetShootTimerHandle)) GetWorldTimerManager().ClearTimer(ResetShootTimerHandle);
		
		CurrentWeaponState = EWeaponState::Reloading;
		FTimerHandle ReloadTimerHandle;
		GetWorldTimerManager().SetTimer(ReloadTimerHandle, FTimerDelegate::CreateLambda([this]
		{
			FWeaponData NewWeaponData = GetItemObject()->GetDynamicItemData().GetMutable<FWeaponData>();
			
			const int32 AmmoToAdd = FMath::Min(
			GetItemObject()->GetDynamicItemData().Get<FWeaponData>().MaxAmmo - GetItemObject()->GetDynamicItemData().Get<FWeaponData>().CurrentAmmo, 
			GetItemObject()->GetDynamicItemData().Get<FWeaponData>().TotalAmmo                
			);
			
			NewWeaponData.CurrentAmmo = GetItemObject()->GetDynamicItemData().Get<FWeaponData>().CurrentAmmo + AmmoToAdd;
			NewWeaponData.TotalAmmo = GetItemObject()->GetDynamicItemData().Get<FWeaponData>().TotalAmmo - AmmoToAdd;
			
			GetItemObject()->Server_SetDynamicItemData(FInstancedStruct::Make(NewWeaponData));
			CurrentWeaponState = EWeaponState::Idle;
		}), GetItemObject()->GetDynamicItemData().Get<FWeaponData>().ReloadTime, false);
	}

	NetMulticast_Reload();
}

//Private logic
//Make shoot dependent on bullets per shot and pellets per bullet
void AWeaponActor::ShootLogic(const FVector& InDirection)
{
	if (!GetItemObject()->GetDynamicItemData().GetPtr<FWeaponData>()) return;
	
	FWeaponData CurrentWeaponData = GetItemObject()->GetDynamicItemData().GetMutable<FWeaponData>();

	const FVector StartLocation = GetActorLocation();

	for (int i = 1; i <= CurrentWeaponData.BulletsPerShot; i++)
	{
		//Clear timer and stop shooting if we don't have bullets
		const int32 NewAmmo = CurrentWeaponData.CurrentAmmo - 1;
		if (!(CurrentWeaponData.MaxAmmo > NewAmmo && NewAmmo >= 0))
		{
			GetWorldTimerManager().ClearTimer(ShootTimerHandle);
			CurrentWeaponState = EWeaponState::Idle;
			return;
		}

		CurrentWeaponData.CurrentAmmo = NewAmmo;

		//We need to set this data to update weapon and uobject info
		GetItemObject()->Server_SetDynamicItemData(FInstancedStruct::Make(CurrentWeaponData));
		
		for (int j = 1; j <= CurrentWeaponData.PelletsCount; j++)
		{
			FVector TargetLocation = StartLocation + UKismetMathLibrary::RandomUnitVectorInConeInDegrees(InDirection, CurrentWeaponData.BulletSpread) * CurrentWeaponData.ShootDistance;

			FCollisionQueryParams ShootTraceParams = FCollisionQueryParams(FName(TEXT("ShootTrace")), false, this);
			ShootTraceParams.bReturnPhysicalMaterial = false;
			ShootTraceParams.AddIgnoredActor(GetOwner());
			
			FHitResult ShootHit(ForceInit);
			
			GetWorld()->LineTraceSingleByChannel(
				ShootHit,	
				StartLocation,	
				TargetLocation,
				ECC_Visibility,
				ShootTraceParams
			);

			NetMulticast_DebugShoot(StartLocation, TargetLocation, FColor::Purple, 1);

			if (ShootHit.bBlockingHit)
			{
				NetMulticast_DebugShoot(StartLocation, TargetLocation, FColor::Red, 3);
				if (!ShootHit.GetActor()) continue;
				UGameplayStatics::ApplyDamage(ShootHit.GetActor(), CurrentWeaponData.Damage, GetOwner()->GetInstigatorController(), this, nullptr);
			}
		}
	}
}

//We need to prepare weapon between shots dependent on fire rate of weapon
void AWeaponActor::PrepareWeaponLogic()
{
	if (!GetItemObject()->GetDynamicItemData().GetPtr<FWeaponData>()) return;
	
	if (CurrentWeaponState == EWeaponState::Shooting)
	{
		CurrentWeaponState = EWeaponState::Preparing;
	
		float ResetTime;
		if (GetItemObject()->GetDynamicItemData().Get<FWeaponData>().bAuto && GetWorldTimerManager().IsTimerActive(ShootTimerHandle))
		{
			ResetTime = GetWorldTimerManager().GetTimerRemaining(ShootTimerHandle);
		}
		else
		{
			ResetTime = GetItemObject()->GetDynamicItemData().Get<FWeaponData>().FireRate;
		}
		
		if (GetWorldTimerManager().IsTimerActive(ShootTimerHandle)) GetWorldTimerManager().ClearTimer(ShootTimerHandle);
		
		GetWorldTimerManager().SetTimer(ResetShootTimerHandle, FTimerDelegate::CreateLambda([this]
			{
				CurrentWeaponState = EWeaponState::Idle;
			}), ResetTime, false);
	}
}

//Debug visualization
void AWeaponActor::NetMulticast_DebugShoot_Implementation(const FVector& StartLocation, const FVector& EndLocation, const FColor DebugColor, const float& Thickness)
{
	DrawDebugLine(GetWorld(), StartLocation, EndLocation, DebugColor, false, 2, 0, Thickness);
}





