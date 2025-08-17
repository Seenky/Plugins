// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "PrepareEventsInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, Blueprintable)
class UPrepareEventsInterface : public UInterface
{
	GENERATED_BODY()
};

class MASSAIWITHTRAFFIC_API IPrepareEventsInterface
{
	GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Events")
	void OnPrepared();
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Events")
	void SetupAnimOffset(const float& Offset);
};
