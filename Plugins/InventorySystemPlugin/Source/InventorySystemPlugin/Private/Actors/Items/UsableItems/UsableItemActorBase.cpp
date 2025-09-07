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

void AUsableItemActorBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	Server_ForceCancelUse();
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
	if (GetWorldTimerManager().IsTimerActive(UseItemTimer))
		GetWorld()->GetTimerManager().ClearTimer(UseItemTimer);
	if (GetWorldTimerManager().IsTimerActive(MulticastUseItemTimer))
		GetWorld()->GetTimerManager().ClearTimer(MulticastUseItemTimer);
	if (GetWorldTimerManager().IsTimerActive(CheckMovementTimer))
		GetWorld()->GetTimerManager().ClearTimer(CheckMovementTimer);
	if (GetWorldTimerManager().IsTimerActive(MulticastCheckMovementTimer))
		GetWorld()->GetTimerManager().ClearTimer(MulticastCheckMovementTimer);
}

//Бля я надеюсь это никогда не крашнет но лучше я не придумал
void AUsableItemActorBase::HandleUseItem(
	const bool bUse,
	TFunction<void(bool)>UseFunction,
	FTimerHandle& UseTimerHandle,
	FTimerHandle& CheckMovementHandle,
	const FUseSettings UseSettings)
{
	UE_LOG(LogTemp, Warning, TEXT("HandleUseItem called: bUse=%s, bCallCancelUse=%s, bResetTimerOnCancelUse=%s"), 
        bUse ? TEXT("true") : TEXT("false"),
        UseSettings.bCallCancelUse ? TEXT("true") : TEXT("false"),
        UseSettings.bResetTimerOnCancelUse ? TEXT("true") : TEXT("false"));
    
    if (!UseSettings.bCallCancelUse && !bUse)
    {
        UE_LOG(LogTemp, Warning, TEXT("Skipping cancel - bCallCancelUse is false and bUse is false"));
        return;
    }
    
    auto ExecuteUse = [this, &CheckMovementHandle, UseFunction](const bool bUseFlag) mutable { 
        UE_LOG(LogTemp, Warning, TEXT("ExecuteUse called: bUseFlag=%s"), bUseFlag ? TEXT("true") : TEXT("false"));
        GetWorldTimerManager().ClearTimer(CheckMovementHandle);
        UseFunction(bUseFlag);
    };

    if (UseSettings.MaxVelocityToUse > 0.0f)
    {
        if (const AActor* ItemOwner = GetItemObject()->GetOwner())
        {
            const float CurrentVelocity = ItemOwner->GetVelocity().Length();
            UE_LOG(LogTemp, Warning, TEXT("Velocity check: Current=%f, Threshold=%f"), CurrentVelocity, UseSettings.MaxVelocityToUse);
            
            if (CurrentVelocity > UseSettings.MaxVelocityToUse)
            {
                UE_LOG(LogTemp, Warning, TEXT("Velocity too high - clearing timers"));
                GetWorldTimerManager().ClearTimer(UseTimerHandle);
                GetWorldTimerManager().ClearTimer(CheckMovementHandle);
                
                // Немедленно вызываем отмену при высокой скорости
                if (!bUse && UseSettings.bCallCancelUse)
                {
                    UE_LOG(LogTemp, Warning, TEXT("Calling cancel due to high velocity"));
                    ExecuteUse(false);
                }
                return;
            }
        }
    }

    GetWorldTimerManager().ClearTimer(CheckMovementHandle);

    if (!bUse && UseSettings.bResetTimerOnCancelUse)
    {
        UE_LOG(LogTemp, Warning, TEXT("Resetting timer on cancel"));
        const bool WasTimerActive = GetWorldTimerManager().IsTimerActive(UseTimerHandle);
        GetWorldTimerManager().ClearTimer(UseTimerHandle);
        
        // Немедленно вызываем отмену при очистке таймера
        if (UseSettings.bCallCancelUse && WasTimerActive)
        {
            UE_LOG(LogTemp, Warning, TEXT("Calling cancel due to timer reset"));
            ExecuteUse(false);
        }
    }
    else if (bUse)
    {
        UE_LOG(LogTemp, Warning, TEXT("Clearing use timer for new use"));
        GetWorldTimerManager().ClearTimer(UseTimerHandle);
    }

    if (bUse && UseSettings.bStartTimerOnUsing)
    {
        UE_LOG(LogTemp, Warning, TEXT("Starting use timer"));
        if (UseSettings.bInstantUseBeforeDelay) 
        {
            UE_LOG(LogTemp, Warning, TEXT("Calling instant use before delay"));
            ExecuteUse(true);
        }
        
        GetWorldTimerManager().SetTimer(
            UseTimerHandle,
            FTimerDelegate::CreateLambda([ExecuteUse]() mutable {
                UE_LOG(LogTemp, Warning, TEXT("Timer delegate executed"));
                ExecuteUse(true);
            }),
            UseSettings.DelayOnUsing,
            !UseSettings.bUseOnce
        );

        if (UseSettings.bResetTimerOnMovement)
        {
            UE_LOG(LogTemp, Warning, TEXT("Starting movement check timer"));
            GetWorldTimerManager().SetTimer(
                CheckMovementHandle,
                FTimerDelegate::CreateLambda([this, VelocityThreshold = UseSettings.VelocityThreshold, &UseTimerHandle, &CheckMovementHandle]() {
                    const AActor* ItemOwner = GetItemObject()->GetOwner();
                    if (!ItemOwner) return;

                    const float CurrentVelocity = ItemOwner->GetVelocity().Length();
                    if (CurrentVelocity > VelocityThreshold)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("Movement detected - clearing timers"));
                        GetWorldTimerManager().ClearTimer(UseTimerHandle);
                        GetWorldTimerManager().ClearTimer(CheckMovementHandle);
                    }
                }),
                0.1f,
                true
            );
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Calling ExecuteUse directly"));
        ExecuteUse(bUse);
    }
}

void AUsableItemActorBase::CheckMovement(const float VelocityThreshold, FTimerHandle& TimerHandle, FTimerHandle& CheckMovementHandle) const
{
	const AActor* ItemOwner = GetItemObject()->GetOwner();
	if (!ItemOwner) return;

	if (ItemOwner->GetVelocity().Length() > VelocityThreshold)
	{
		GetWorldTimerManager().ClearTimer(TimerHandle);
		GetWorldTimerManager().ClearTimer(CheckMovementHandle);
	}
}



