// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "VoiceDebugWidget.h"
#include "Net/VoiceConfig.h"
#include "AdvancedVoiceChatComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(AdvancedVoiceChat, Log, All);

UENUM(BlueprintType)
enum EVoiceMode : uint8
{
	NoVoiceMode UMETA(DisplayName = "None"),
	PushToTalk,
	Always,
};

UENUM(BlueprintType)
enum ECurrentVoiceState : uint8
{
	NoVoiceState UMETA(DisplayName = "None"),
	Positional,
	Radio,
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), HideCategories=("Voice"))
class ADVANCEDVOICECHAT_API UAdvancedVoiceChatComponent : public UVOIPTalker
{
	GENERATED_BODY()

public:
	UAdvancedVoiceChatComponent(const FObjectInitializer& ObjectInitializer);

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(NetMulticast, Unreliable, BlueprintCallable, Category = "Advanced Voice Chat Component")
		void Multicast_InitVoiceComponent();

	UFUNCTION(Server, Unreliable, BlueprintCallable, Category = "Advanced Voice Chat Component")
		void Server_SetVoiceState(const ECurrentVoiceState NewVoiceState);
		void Server_SetVoiceState_Implementation(const ECurrentVoiceState NewVoiceState) { VoiceState = NewVoiceState; }

	UFUNCTION(Client, Unreliable, BlueprintCallable, Category = "Advanced Voice Chat Component")
		void Client_CloseVoiceLine();

	UFUNCTION(Server, Unreliable)
		void Server_SetVoiceLevel(float NewLevel);
		void Server_SetVoiceLevel_Implementation(const float NewLevel) { VoiceLevel = NewLevel; }

	UFUNCTION(BlueprintPure, Category = "Advanced Voice Chat Component")
		float GetReplicatedVoiceLevel() const { return VoiceLevel; }

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(Server, Unreliable, BlueprintCallable, Category = "Advanced Voice Chat Component")
		void StartVoiceSpeach(const ECurrentVoiceState SpeachVoiceState);
	UFUNCTION(Server, Unreliable, BlueprintCallable, Category = "Advanced Voice Chat Component")
		void EndVoiceSpeach();

	UPROPERTY(EditDefaultsOnly, Category = "Voice Settings")
		TEnumAsByte<EVoiceMode> VoiceMode;

	UPROPERTY(EditDefaultsOnly, Category = "Voice Settings")
		TObjectPtr<USoundAttenuation> Attenuation;
	UPROPERTY(EditDefaultsOnly, Category = "Voice Settings")
		TObjectPtr<USoundEffectSourcePresetChain> SourceEffectChain;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Voice Settings")
		float MicThreshold = 3;

private:
	UFUNCTION()
		void OnRep_VoiceState() { ChangeVoiceStateSettings(); }
	
	void ChangeVoiceStateSettings();

#if !UE_BUILD_SHIPPING
	void BindCVars();
	void FetchCVars(IConsoleVariable* Var);
#endif

	UPROPERTY(ReplicatedUsing = OnRep_VoiceState)
		TEnumAsByte<ECurrentVoiceState> VoiceState;

	UPROPERTY(Replicated)
		ACharacter* Player;

	UPROPERTY(Replicated)
		float VoiceLevel;
	
	UVoiceDebugWidget* VoiceDebugWidget;
};
