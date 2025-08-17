// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassLODTypes.h"

#include "PawnInteractInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, Blueprintable)
class UPawnInteractInterface : public UInterface
{
	GENERATED_BODY()
};

class MASSAIWITHTRAFFIC_API IPawnInteractInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PawnInteract")
	void SetPawnVisibility(const bool bVisible, const float& NewLerpSpeed);
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PawnInteract")
	void UpdatePerformance(EMassLOD::Type LOD);
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PawnInteract")
	bool GetIsVisible() const;
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PawnInteract")
	bool GetIsLerpingNow() const;
};
