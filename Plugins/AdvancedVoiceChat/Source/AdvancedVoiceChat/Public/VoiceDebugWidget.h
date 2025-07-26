// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/ProgressBar.h"
#include "VoiceDebugWidget.generated.h"

class UAdvancedVoiceChatComponent;

UCLASS()
class ADVANCEDVOICECHAT_API UVoiceDebugWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetVoiceComp(UAdvancedVoiceChatComponent* Comp) { VoiceComp = Comp; }

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Service")
	UAdvancedVoiceChatComponent* VoiceComp;
};
