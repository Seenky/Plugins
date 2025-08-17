// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ArrowComponent.h"
#include "GameFramework/Actor.h"
#include "MassTraffucSpeedBumpBase.generated.h"

UCLASS()
class MASSTRAFFIC_API AMassTraffucSpeedBumpBase : public AActor
{
	GENERATED_BODY()

public:
	AMassTraffucSpeedBumpBase();

protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(VisibleAnywhere, Category = "Components")
		UStaticMeshComponent* SpeedBumpMesh;

public:
	virtual void Tick(float DeltaTime) override;
};
