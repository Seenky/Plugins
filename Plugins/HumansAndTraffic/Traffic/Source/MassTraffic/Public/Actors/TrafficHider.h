// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Actor.h"
#include "TrafficHider.generated.h"

UCLASS()
class MASSTRAFFIC_API ATrafficHider : public AActor
{
	GENERATED_BODY()

public:
	ATrafficHider();
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HumansHider")
	UBoxComponent* BoxComponent;

	UFUNCTION(BlueprintPure, Category = "HumansHider")
	bool IsInBox(const FVector& Location) const;

	UFUNCTION(BlueprintCallable, Category = "HumansHider")
	void SetActive(const bool bNewIsActive) { bIsActive = bNewIsActive; }
	UFUNCTION(BlueprintPure, Category = "HumansHider")
	bool GetActive() const { return bIsActive; }

protected:
	UPROPERTY(EditAnywhere, Category = "Settings")
	bool bIsActive;
};
