// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

struct FDefaultPhysMaterialSettings
{
	float Friction = 0.7f;
	float StaticFriction = 0.0f;
	EFrictionCombineMode::Type FrictionCombineMode = EFrictionCombineMode::Min;
	bool OverrideFrictionCombineMode = true;
	float Restitution = 0.3f;
	EFrictionCombineMode::Type RestitutionCombineMode = EFrictionCombineMode::Average;
	bool OverrideRestitutionCombineMode = false;
	float Density = 1.0f;
	float SleepLinearVelocityThreshold = 1.0f;
	float SleepAngularVelocityThreshold = 0.05f;
	int32 SleepCounterThreshold = 4;
	float RaiseMassToPower = 0.75f;
	float TensileStrength = 2.0f;
	float CompressionStrength = 20.0f;
	float ShearStrength = 6.0f;
	float DamageThresholdMultiplier = 1.0f;
};
