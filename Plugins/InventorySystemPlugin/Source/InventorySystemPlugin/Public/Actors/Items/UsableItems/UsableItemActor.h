// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Actors/Items/EquipItemActorBase.h"
#include "UsableItemActor.generated.h"

USTRUCT(BlueprintType)
struct FUseSettings
{
	GENERATED_BODY()

public:
	/*Enable or disable timer on use item*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool bStartTimerOnUsing;
	/*Delay on use timer*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float DelayOnUsing;
	/*Use timer once and clear it*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool bUseOnce;
	/*Shoot instant use and then delayed one*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool bInstantUseBeforeDelay;

	FUseSettings()
		:bStartTimerOnUsing(false)
		,DelayOnUsing(0.0f)
		,bUseOnce(false)
		,bInstantUseBeforeDelay(true)
	{}
};

UCLASS(Abstract)
class INVENTORYSYSTEMPLUGIN_API AUsableItemActor : public AEquipItemActorBase
{
	GENERATED_BODY()

public:
	AUsableItemActor();
	virtual  void BeginPlay() override { Super::BeginPlay(); if (HasAuthority() && GetItemObject()->GetPickupItemData().bShouldUseOnDrop) Server_UseItem(GetItemObject()->GetPickupItemData().bIsUsing); }

	/*Call use item on server(non interface method)*/
	UFUNCTION(Server, Unreliable, WithValidation, BlueprintCallable, Category = "UsableItemActorBase")
		void Server_UseItem(const bool bUse);
		bool Server_UseItem_Validate(const bool bUse) { return true; }
	UFUNCTION(NetMulticast, Unreliable)
		void NetMulticast_UseItem(bool bUse);
		void NetMulticast_UseItem_Implementation(const bool bUse) { UseItemVisual(bUse); }

	/*Call additional use item on server(non interface method)*/
	UFUNCTION(Server, Unreliable, WithValidation, BlueprintCallable, Category = "UsableItemActorBase")
	void Server_AdditionalUseItem(const bool bUse);
	bool Server_AdditionalUseItem_Validate(const bool bUse) { return true; }
	UFUNCTION(NetMulticast, Unreliable)
	void NetMulticast_AdditionalUseItem(bool bUse);
	void NetMulticast_AdditionalUseItem_Implementation(const bool bUse) { AdditionalUseItemVisual(bUse); }

protected:
	/*Server Event. Called when item was used by *Server_UseItem**/
	UFUNCTION(BlueprintImplementableEvent, Category = "UsableItemAction")
		bool UseItem(const bool bUse);
	/*Multicast Event. Called when item was used by *Server_UseItem**/
	UFUNCTION(BlueprintImplementableEvent, Category = "UsableItemVisual")
		bool UseItemVisual(const bool bUse);
	/*Server Event. Called when item was used by *Server_AdditionalUseItem**/
	UFUNCTION(BlueprintImplementableEvent, Category = "UsableItemAction")
		bool AdditionalUseItem(const bool bUse);
	/*Multicast Event. Called when item was used by *Server_AdditionalUseItem**/
	UFUNCTION(BlueprintImplementableEvent, Category = "UsableItemVisual")
		bool AdditionalUseItemVisual(const bool bUse);

	//Getter
	/*Get Use Timer Handle if use activated by timer*/
	UFUNCTION(BlueprintPure, Category = "UsableItemAction")
		FTimerHandle GetUseTimerHandle() const { return UseItemTimer; }

	/*Settings for use item event*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings|Use")
		FUseSettings UseItemSettings;
	/*Settings for additional use item event*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings|Use")
		FUseSettings AdditionalUseItemSettings;


private:
	void HandleUseItem(
		const bool bUse,
		TFunction<void(bool)> UseFunction,
		TFunction<void(bool)> MulticastFunction,
		FUseSettings UseSettings);
	FTimerHandle UseItemTimer;
};
