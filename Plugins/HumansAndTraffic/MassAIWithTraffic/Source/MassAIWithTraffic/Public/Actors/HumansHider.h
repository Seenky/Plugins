// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "HumansHider.generated.h"

UCLASS()
class MASSAIWITHTRAFFIC_API AHumansHider : public AActor
{
	GENERATED_BODY()

public:
	AHumansHider();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HumansHider")
		UBoxComponent* BoxComponent;

	UFUNCTION(BlueprintPure, Category = "HumansHider")
		bool IsInBox(const FVector& Location) const;

	UFUNCTION(BlueprintCallable, Category = "HumansHider")
		void SetActive(const bool bNewIsActive) { bIsActive = bNewIsActive; }
	UFUNCTION(BlueprintPure, Category = "HumansHider")
		bool GetActive() const { return bIsActive; }

	UFUNCTION(BlueprintPure, Category = "HumansHider")
		float GetLerpSpeed() const { return LerpSpeed; }

protected:
	UPROPERTY(EditAnywhere, Category = "Settings")
		float LerpSpeed = 1.0f;
	
	UPROPERTY(EditAnywhere, Category = "Settings")
	bool bIsActive;
};
