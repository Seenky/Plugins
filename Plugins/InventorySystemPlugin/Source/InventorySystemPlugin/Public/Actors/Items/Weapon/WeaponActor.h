// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Actors/Items/EquipItemActorBase.h"
#include "WeaponActor.generated.h"

UCLASS(Abstract)
class INVENTORYSYSTEMPLUGIN_API AWeaponActor : public AEquipItemActorBase
{
	GENERATED_BODY()

public:
	AWeaponActor();

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	/*Server Event. Call Weapon Shoot*/
	UFUNCTION(Server, Unreliable, WithValidation, BlueprintCallable, Category = "Server weapon interaction")
		void Server_Shoot(const bool bShoot);
		bool Server_Shoot_Validate(const bool bShoot) { return true; }
	UFUNCTION(NetMulticast, Unreliable)
		void NetMulticast_Shoot(const FVector& InDirection);
		void NetMulticast_Shoot_Implementation(const FVector& InDirection) { ShootVisual(InDirection); }
	/*Multicast Event. Called when weapon make shoot*/
	UFUNCTION(BlueprintImplementableEvent, Category = "Visual")
		bool ShootVisual(const FVector& InDirection);

	/*Server Event. Call Weapon Reload*/
	UFUNCTION(Server, Unreliable, WithValidation, BlueprintCallable, Category = "Server weapon interaction")
		void Server_Reload();
		bool Server_Reload_Validate() { return true; }
	UFUNCTION(NetMulticast, Unreliable)
		void NetMulticast_Reload();
		void NetMulticast_Reload_Implementation() { ReloadVisual(); }
	/*Multicast Event. Called when weapon starts reload*/
	UFUNCTION(BlueprintImplementableEvent, Category = "Visual")
		bool ReloadVisual();

	//Debug
	UFUNCTION(NetMulticast, Reliable)
		void NetMulticast_DebugShoot(const FVector& StartLocation, const FVector& EndLocation, const FColor DebugColor, const float& Thickness);

	//Getter
	/*Get current state of weapon (Idle, Preparing, Reloading, Shooting,)*/
	UFUNCTION(BlueprintPure, Category = "Weapon info")
		FORCEINLINE TEnumAsByte<EWeaponState> GetCurrentWeaponState() const { return CurrentWeaponState; }

	UPROPERTY(Replicated)
	TEnumAsByte<EWeaponState> CurrentWeaponState;

private:
	void ShootLogic(const FVector& InDirection);
	void PrepareWeaponLogic();
	FTimerHandle ShootTimerHandle;
	FTimerHandle ResetShootTimerHandle;
};
