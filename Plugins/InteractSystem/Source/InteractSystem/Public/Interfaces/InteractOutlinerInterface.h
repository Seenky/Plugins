// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractOutlinerInterface.generated.h"

UINTERFACE()
class UInteractOutlinerInterface : public UInterface
{
	GENERATED_BODY()
};

class INTERACTSYSTEM_API IInteractOutlinerInterface
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Interact Interface Logic")
		const TArray<UPrimitiveComponent*> GetComponentsForHighlight() const;
};
