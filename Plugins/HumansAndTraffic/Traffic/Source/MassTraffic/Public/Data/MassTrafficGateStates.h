// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassTrafficGateStates.generated.h"

UENUM(BlueprintType)
enum class EMassTrafficGateStates : uint8
{
	Opened UMETA(DisplayName = "Opened"),
	Moving UMETA(DisplayName = "Moving"),
	Closed UMETA(DisplayName = "Closed")
};

