// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AdvancedVoiceChatLibrary.generated.h"

UCLASS()
class ADVANCEDVOICECHAT_API UAdvancedVoiceChatLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Advanced Voice Chat", meta = (WorldContext = "WorldContextObject"))
	static void ClearVoicePackets(UObject* WorldContextObject);
};
