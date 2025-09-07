// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Actors/Items/ItemActorBase.h"
#include "Interfaces/UsableItemsInteract.h"
#include "UsableItemActorBase.generated.h"

USTRUCT(BlueprintType)
struct FUseSettings
{
	GENERATED_BODY()

public:
	/*Do we need to call an action*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
		bool bNeedCallAction;
	/*Enable or disable timer on use item*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bNeedCallAction", EditConditionHides), Category = "Timer")
		bool bStartTimerOnUsing;
	/*Delay on use timer*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bStartTimerOnUsing && bNeedCallAction", EditConditionHides), Category = "Timer")
		float DelayOnUsing;
	/*Use timer once and clear it*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bStartTimerOnUsing && bNeedCallAction", EditConditionHides), Category = "Timer")
		bool bUseOnce;
	/*Shoot instant use and then delayed one*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bStartTimerOnUsing && !bUseOnce && bNeedCallAction", EditConditionHides), Category = "Timer")
		bool bInstantUseBeforeDelay;
	/*Reset timer after canceled use*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bStartTimerOnUsing && bNeedCallAction", EditConditionHides), Category = "Timer")
		bool bResetTimerOnCancelUse;
	/*Reset timer after owner moves*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bStartTimerOnUsing && bNeedCallAction", EditConditionHides), Category = "Timer")
		bool bResetTimerOnMovement;
	/*Velocity threshold when check reset on movement*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bResetTimerOnMovement && bNeedCallAction && bStartTimerOnUsing", EditConditionHides), Category = "Timer")
		float VelocityThreshold;
	/*Set max velocity that item can used with (if 0 it will be used in any velocity)*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0, EditCondition = "bNeedCallAction", EditConditionHides), Category = "Action")
		float MaxVelocityToUse;
	/*Call use item (false) or not*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bNeedCallAction", EditConditionHides), Category = "Action")
		bool bCallCancelUse;

	FUseSettings()
		:bNeedCallAction(true)
		,bStartTimerOnUsing(false)
		,DelayOnUsing(0.0f)
		,bUseOnce(false)
		,bInstantUseBeforeDelay(true)
		,bResetTimerOnCancelUse(true)
		,bResetTimerOnMovement(true)
		,VelocityThreshold(10.0f)
		,MaxVelocityToUse(0.0f)
		,bCallCancelUse(true)
	{}
};

USTRUCT(BlueprintType)
struct FActionSettings
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FUseSettings UseSettings;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FUseSettings VisualUseSettings;
};

USTRUCT(BlueprintType)
struct FInteractActionInfo
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString FunctionName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FUseSettings ActionUseSettings;
};

UCLASS(Abstract)
class INVENTORYSYSTEMPLUGIN_API AUsableItemActorBase : public AItemActorBase, public IUsableItemsInteract
{
	GENERATED_BODY()

public:
	AUsableItemActorBase();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
		USkeletalMeshComponent* SkeletalMesh;

	UFUNCTION(Server, Unreliable, WithValidation, BlueprintCallable, Category = "UsableItemActorBase")
		void Server_UseItemTagged(const bool bUse, const FGameplayTag ActionTag, const FActionSettings UseSettings);
		bool Server_UseItemTagged_Validate(const bool bUse, const FGameplayTag ActionTag, const FActionSettings UseSettings) { return true; }

	UFUNCTION(Server, Unreliable, WithValidation, BlueprintCallable, Category = "UsableItemActorBase")
		void Server_ForceCancelUse();
		bool Server_ForceCancelUse_Validate() { return true; }

protected:
	/*Called on server when we use item*/
	UFUNCTION(BlueprintImplementableEvent, Category = "UsableItemAction", meta = (ForceAsFunction))
		void UseItemTagged(const bool bUse, FGameplayTag ActionTag, const FUseSettings UseSettings);
	/*Called on all clients when we use item*/
	UFUNCTION(BlueprintImplementableEvent, Category = "UsableItemVisual", meta = (ForceAsFunction))
		void UseItemTaggedVisual(const bool bUse, FGameplayTag ActionTag, const FUseSettings UseSettings);

	UFUNCTION(NetMulticast, Unreliable)
	void NetMulticast_UseItemTagged(const bool bUse, FGameplayTag ActionTag, const FUseSettings UseSettings);
	void NetMulticast_UseItemTagged_Implementation(const bool bUse, const FGameplayTag ActionTag, const FUseSettings UseSettings) { UseItemTaggedVisual(bUse, ActionTag, UseSettings); }

	//Getter
	/*Get Use Timer Handle if use activated by timer*/
	UFUNCTION(BlueprintPure, Category = "UsableItemAction")
		FTimerHandle GetUseTimerHandle() const { return UseItemTimer; }

	virtual void InventoryItemInteract_Implementation(FGameplayTagContainer InteractTags, const bool bInteract) override;

	virtual void HandleUseItem(
			const bool bUse,
			TFunction<void(bool)> UseFunction,
			FTimerHandle& TimerHandle,
			FTimerHandle& CheckMovementHandle,
			FUseSettings UseSettings);
	
	/*Interact action tags, used to call any logic form child classes*/
	UPROPERTY(EditDefaultsOnly, Category = "Settings")
		TMap<FGameplayTag, FActionSettings> InteractActionByTag;
	/*Tag that used when item has bIsUsing and selected*/
	UPROPERTY(EditDefaultsOnly, Category = "Settings")
		FGameplayTag DefaultActionTag;


private:
	void CheckMovement(const float VelocityThreshold, FTimerHandle& TimerHandle, FTimerHandle& CheckMovementHandle) const;
	FTimerHandle UseItemTimer;
	FTimerHandle MulticastUseItemTimer;
	FTimerHandle CheckMovementTimer;
	FTimerHandle MulticastCheckMovementTimer;
	
};
