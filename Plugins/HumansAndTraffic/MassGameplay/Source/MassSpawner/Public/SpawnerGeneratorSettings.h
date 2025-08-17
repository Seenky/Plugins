// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "SpawnerGeneratorSettings.generated.h"

UCLASS(Config=Game, defaultconfig, meta = (DisplayName = "SpawnerGeneratorDeveloperSettings"))
class MASSSPAWNER_API USpawnerGeneratorSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return TEXT("SpawnerGenerator"); }
	virtual FName GetSectionName() const override { return TEXT("SpawnerGeneratorSettings"); }
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Config, Category="SpawnerGeneratorSettings|Main", meta=(AllowPrivateAccess))
	float DistanceToPlayer = 5000.f;
};
