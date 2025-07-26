// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/Items/UsableItems/UsableItemActorBase.h"

#include "Misc/OutputDeviceNull.h"


AUsableItemActorBase::AUsableItemActorBase()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = false;

	SkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>("SkeletalMesh");
	SkeletalMesh->SetupAttachment(DefaultRoot);
	SkeletalMesh->SetIsReplicated(true);
}

void AUsableItemActorBase::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && GetItemObject() && !GetItemObject()->GetPickupItemData().bShouldUseOnDrop) return;
		
	if (const FActionSettings* ActionInfo = InteractActionByTag.Find(DefaultActionTag))
	{
		Server_UseItemTagged(GetItemObject()->GetPickupItemData().bIsUsing, DefaultActionTag, *ActionInfo);
	}
}

void AUsableItemActorBase::InventoryItemInteract_Implementation(FGameplayTagContainer InteractTags, const bool bInteract)
{
	for (FGameplayTag Tag : InteractTags)
	{
		if (const FActionSettings* ActionInfo = InteractActionByTag.Find(Tag))
		{
			Server_UseItemTagged(bInteract, Tag, *ActionInfo);
		}
	}
}

void AUsableItemActorBase::Server_UseItemTagged_Implementation(const bool bUse, const FGameplayTag ActionTag, const FActionSettings UseSettings)
{
	if (UseSettings.UseSettings.bNeedCallAction)
		HandleUseItem(
		   bUse,
		   [this, ActionTag, UseSettings](const bool bFlag) { UseItemTagged(bFlag, ActionTag, UseSettings.UseSettings); },
		   UseItemTimer,
		   CheckMovementTimer,
		   UseSettings.UseSettings);

	if (UseSettings.VisualUseSettings.bNeedCallAction)
		HandleUseItem(
		   bUse,
		   [this, ActionTag, UseSettings](const bool bFlag) { NetMulticast_UseItemTagged(bFlag, ActionTag, UseSettings.VisualUseSettings); },
		   MulticastUseItemTimer,
		   MulticastCheckMovementTimer,
		   UseSettings.VisualUseSettings);
}

void AUsableItemActorBase::Server_ForceCancelUse_Implementation()
{
	GetWorldTimerManager().ClearTimer(UseItemTimer);
	GetWorldTimerManager().ClearTimer(MulticastUseItemTimer);
	GetWorldTimerManager().ClearTimer(CheckMovementTimer);
	GetWorldTimerManager().ClearTimer(MulticastCheckMovementTimer);
}

//Бля я надеюсь это никогда не крашнет но лучше я не придумал
void AUsableItemActorBase::HandleUseItem(
	const bool bUse,
	TFunction<void(bool)>UseFunction,
	FTimerHandle UseTimerHandle,
	FTimerHandle CheckMovementHandle,
	const FUseSettings UseSettings)
{
	if (!UseSettings.bCallCancelUse && !bUse) return;
	
	auto ExecuteUse = [this, UseFunction, CheckMovementHandle](const bool bUseFlag) mutable {
		GetWorldTimerManager().ClearTimer(CheckMovementHandle);
		UseFunction(bUseFlag);
	};

	if (UseSettings.MaxVelocityToUse > 0.0f)
	{
		if (const AActor* ItemOwner = GetItemObject()->GetOwner())
		{
			if (ItemOwner->GetVelocity().Length() > UseSettings.MaxVelocityToUse)
			{
				GetWorldTimerManager().ClearTimer(UseTimerHandle);
				GetWorldTimerManager().ClearTimer(CheckMovementHandle);
				return;
			}
		}
	}

	GetWorldTimerManager().ClearTimer(CheckMovementHandle);

	if (!bUse && UseSettings.bResetTimerOnCancelUse)
		GetWorldTimerManager().ClearTimer(UseTimerHandle);
	else if (bUse)
		GetWorldTimerManager().ClearTimer(UseTimerHandle);

	if (bUse && UseSettings.bStartTimerOnUsing)
	{
		if (UseSettings.bInstantUseBeforeDelay) ExecuteUse(true);
		
		GetWorldTimerManager().SetTimer(
			UseTimerHandle,
			FTimerDelegate::CreateLambda([ExecuteUse]() mutable  {
				ExecuteUse(true);
			}),
			UseSettings.DelayOnUsing,
			!UseSettings.bUseOnce
		);

		if (UseSettings.bResetTimerOnMovement)
		{
			const FTimerDelegate CheckMovementDelegate = FTimerDelegate::CreateUObject(this, &AUsableItemActorBase::CheckMovement,
														 UseSettings.VelocityThreshold, UseTimerHandle, CheckMovementHandle);
			GetWorldTimerManager().SetTimer(CheckMovementHandle, CheckMovementDelegate, 0.1f, true);
		}
	}
	else
	{
		ExecuteUse(bUse);
	}
}

void AUsableItemActorBase::CheckMovement(const float VelocityThreshold, FTimerHandle TimerHandle, FTimerHandle CheckMovementHandle) const
{
	const AActor* ItemOwner = GetItemObject()->GetOwner();
	if (!ItemOwner) return;

	if (ItemOwner->GetVelocity().Length() > VelocityThreshold)
	{
		GetWorldTimerManager().ClearTimer(TimerHandle);
		GetWorldTimerManager().ClearTimer(CheckMovementHandle);
	}
}



