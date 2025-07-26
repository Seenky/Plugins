// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "WeaponData.generated.h"

USTRUCT(BlueprintType)
struct FWeaponData
{
	GENERATED_BODY()

public:
	/*Max weapon ammo, should not edit at runtime*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeaponData")
		int32 MaxAmmo;

	/*Current weapon ammo*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeaponData")
		int32 CurrentAmmo;

	/*Total amount of ammo, including current ammo*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeaponData")
		int32 TotalAmmo;

	/*Distance weapon can shoot*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeaponData")
		float ShootDistance;

	/*Weapon fire rate*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeaponData")
		float FireRate;

	/*Weapon damage*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeaponData")
		float Damage;

	/*Bullet spread (for shotguns), if pellets count and bullets per shot are 1, it'll make weapon shoot random direction*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeaponData")
		float BulletSpread;

	/*Pellets count (for shotguns), consume one ammo but make a few shots*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeaponData")
		int32 PelletsCount;

	/*Bullets per shot, consume as much ammo as bullets per shot variable*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeaponData")
		int32 BulletsPerShot;

	/*Time to reload weapon*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeaponData")
		float ReloadTime;

	/*Is Weapon can shoot auto or you need to click again to make next shoot*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeaponData")
		bool bAuto;

	FWeaponData()
		:MaxAmmo(20)
		,CurrentAmmo(6)
		,TotalAmmo(60)
		,ShootDistance(1000.0f)
		,FireRate(1.0f)
		,Damage(0.0f)
		,BulletSpread(0.0f)
		,PelletsCount(1)
		,BulletsPerShot(1)
		,ReloadTime(1.0f)
		,bAuto(false)
	{}
};

UENUM(BlueprintType)
enum EWeaponState
{
	/*Idle state, weapon prepared for shoot or reload*/
	Idle,
	/*Weapon is preparing for shoot, cant make another one*/
	Preparing,
	/*Weapon is reloading, cant make shoot*/
	Reloading,
	/*Weapon is shooting*/
	Shooting,
};


