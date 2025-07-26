// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/Items/UsableItems/UsableItemActor.h"


AUsableItemActor::AUsableItemActor()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = false;
}

void AUsableItemActor::Server_AdditionalUseItem_Implementation(const bool bUse)
{
	HandleUseItem(
	   bUse,
	   [this](const bool bFlag) { AdditionalUseItem(bFlag); },
	   [this](const bool bFlag) { NetMulticast_AdditionalUseItem(bFlag); },
	   AdditionalUseItemSettings
   );
}

void AUsableItemActor::Server_UseItem_Implementation(const bool bUse)
{
	HandleUseItem(
	   bUse,
	   [this](const bool bFlag) { UseItem(bFlag); },
	   [this](const bool bFlag) { NetMulticast_UseItem(bFlag); },
	   UseItemSettings
   );
}

void AUsableItemActor::HandleUseItem(
	const bool bUse,
	TFunction<void(bool)>UseFunction,
	TFunction<void(bool)> MulticastFunction,
	const FUseSettings UseSettings)
{
	auto ExecuteUse = [UseFunction, MulticastFunction](const bool bUseFlag) {
		UseFunction(bUseFlag);
		MulticastFunction(bUseFlag);
	};
	
	GetWorldTimerManager().ClearTimer(UseItemTimer);

	if (bUse && UseSettings.bStartTimerOnUsing)
	{
		if (UseSettings.bInstantUseBeforeDelay) ExecuteUse(true);
		
		GetWorldTimerManager().SetTimer(
			UseItemTimer,
			FTimerDelegate::CreateLambda([ExecuteUse] {
				ExecuteUse(true);
			}),
			UseSettings.DelayOnUsing,
			!UseSettings.bUseOnce
		);
	}
	else
	{
		ExecuteUse(bUse);
	}
}



