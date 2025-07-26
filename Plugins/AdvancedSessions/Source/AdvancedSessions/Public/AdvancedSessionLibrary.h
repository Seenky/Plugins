// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FindSessionsCallbackProxy.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AdvancedSessionLibrary.generated.h"

UCLASS()
class ADVANCEDSESSIONS_API UAdvancedSessionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Online|Session")
		static FName GetSessionNameAdvanced(FBlueprintSessionResult SearchResult);
};
