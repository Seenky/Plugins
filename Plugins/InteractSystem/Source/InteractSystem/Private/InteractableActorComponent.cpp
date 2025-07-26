// Florist Game. All rights reserved.


#include "InteractableActorComponent.h"
#include "Interfaces/InteractableActorInterface.h"
#include "Interfaces/InteractableActorWidgetInfoInterface.h"


UInteractableActorComponent::UInteractableActorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	
}

void UInteractableActorComponent::AddOrRemoveInteractInfo(const FName InteractSceneCompTag,
const FInteractLogicInfo NeInteractLogicInfo, const bool bIsAddInteractInfo)
{
	if(bIsAddInteractInfo) InteractInfo.InteractInfoList.Add(InteractSceneCompTag, NeInteractLogicInfo);
	else InteractInfo.InteractInfoList.Remove(InteractSceneCompTag);
}

void UInteractableActorComponent::BeginPlay()
{
	Super::BeginPlay();
	
}

//--------------------------------------Interact Interface Support Functions------------------------------------------//


const bool UInteractableActorComponent::GetIsShowInteractWidgetLogic_Implementation() const
{
	if(!GetOwner()) return false;

	// Check is show widget from owner interface method
	auto bLocalIsShowWidgetByOwner = true;
	if(GetOwner()->Implements<UInteractableActorWidgetInfoInterface>())
	{
		bLocalIsShowWidgetByOwner = IInteractableActorWidgetInfoInterface::Execute_GetIsShowInteractWidgetFromInteractable(GetOwner(),
			CurrentGlobalInputInfo);
	}
	
	return CurrentGlobalInputInfo.GetIsValidInputInfo() && bLocalIsShowWidgetByOwner;
}

void UInteractableActorComponent::FailedTryInteractLogic_Implementation(FHitResult HitResult)
{
	// Call failed try interact
	if(!CurrentGlobalInputInfo.GetIsValidInputInfo()) return;
	OnFailedTryInteract.Broadcast(CurrentGlobalInputInfo);
}

void UInteractableActorComponent::TraceHitLogic_Implementation(FHitResult HitResult, AActor* QueryFromActor)
{
	// Return Interact Info is empty return false
	if(InteractInfo.InteractInfoList.IsEmpty()) return;
	// Cancel from interact if Hit nothing
	if(!HitResult.bBlockingHit || !HitResult.GetComponent())
	{
		ClearCurrentGlobalInfo();
		return;
	}

	// Save current hit location
	CurrentHitLocation = HitResult.ImpactPoint;
	
	const auto& LocalInteractArray = InteractInfo.InteractInfoList;
	FInteractLogicInfo LocalInteractLogicInfo;
	
	bool bLocalIsFindComponent = false;
	// Try find interact scene component by tag
	for (const auto& LocalInteractInfo : LocalInteractArray)
	{
		// If find scene component with tag - save logic
		if(HitResult.GetComponent()->ComponentHasTag(LocalInteractInfo.Key))
		{
			LocalInteractLogicInfo = LocalInteractInfo.Value;
			const auto LocalNewActionName = LocalInteractLogicInfo.InteractActionName;
			const auto LocalNewSceneCompTag= LocalInteractInfo.Key;
			// Check Is Change Interact Input Info
			const bool bLocalIsDifferentInteract = CheckIsChangeInteractInputInfo(FInteractInputInfo(LocalNewActionName, LocalNewSceneCompTag));
			
			CurrentGlobalInputInfo.InteractActionName = LocalNewActionName;
			CurrentGlobalInputInfo.InteractSceneCompTag = LocalNewSceneCompTag;
			
			if(bLocalIsDifferentInteract) OnInteractInputInfoChange.Broadcast(CurrentGlobalInputInfo, GetCurrentInputProgressInfo());
			
			bLocalIsFindComponent = true;
			return;
		}
	}
	
	// Is doesnt find component - try find interact scene component with empty tag
	if(!bLocalIsFindComponent && LocalInteractArray.Contains(""))
	{
		LocalInteractLogicInfo = LocalInteractArray.FindRef("");
		const auto LocalNewActionName = LocalInteractLogicInfo.InteractActionName;
		const auto LocalNewSceneCompTag = "";
		// Check Is Change Interact Input Info
		const bool bLocalIsDifferentInteract = CheckIsChangeInteractInputInfo(FInteractInputInfo(LocalNewActionName, LocalNewSceneCompTag));
		
		CurrentGlobalInputInfo.InteractActionName = LocalNewActionName;
		CurrentGlobalInputInfo.InteractSceneCompTag = "";

		if(bLocalIsDifferentInteract) OnInteractInputInfoChange.Broadcast(CurrentGlobalInputInfo, GetCurrentInputProgressInfo());
		
		bLocalIsFindComponent = true;
		return;
	}

	// If nothing find - cancel from interact input
	ClearCurrentGlobalInfo();
}

bool UInteractableActorComponent::StartInteractLogic_Implementation(FHitResult HitResult, AActor* InteractByActor)
{
	const EResultStateNow LocalSuccessState = TryInteractInput(HitResult, InteractByActor);
	bool LocalSuccess = false;
	// If no success interact
	switch (LocalSuccessState)
	{
		case EResultStateNow::None:
		case EResultStateNow::During:
			break;
		case EResultStateNow::Failed:
			LocalSuccess = false;
			break;
		case EResultStateNow::Finish:
			LocalSuccess = true;
			break;
	}
	
	return LocalSuccess;
}

bool UInteractableActorComponent::CanInteractLogic_Implementation(FHitResult HitResult, AActor* QueryFromActor)
{
	return GetIsCanInteractNow(CurrentGlobalInputInfo, HitResult, QueryFromActor);
}

void UInteractableActorComponent::CancelCanInteractLogic_Implementation(FHitResult HitResult)
{
	StopAllCurrentInteract();
	ClearCurrentGlobalInfo();
}

void UInteractableActorComponent::StopInteractLogic_Implementation(FHitResult HitResult)
{
	if(!CurrentGlobalInputInfo.GetIsValidInputInfo()) return;

	StopAllCurrentInteract();
	OnStopFromInputInteraction.Broadcast(CurrentGlobalInputInfo);
}

void UInteractableActorComponent::GetInteractWidgetLocationLogic_Implementation(FVector& WidgetLocation) const
{
	// Get Scene Interact Component
	FVector LocalWidgetLoc;
	const bool bLocalSuccessLoc = GetWidgetLocByCurrentInteractInputInfo(LocalWidgetLoc);
	
	// Get Owner Location
	if(GetOwner() && !bLocalSuccessLoc)
	WidgetLocation = GetOwner()->GetActorLocation();

	WidgetLocation = LocalWidgetLoc;
}


//--------------------------------------------------------------------------------------------------------------------//


//--------------------------------------------By Interact Input Type Logic--------------------------------------------//


const EResultStateNow UInteractableActorComponent::TryInteractInput(FHitResult HitResult, AActor* InteractByActor)
{
	// Return Interact Info is empty return false
	if(InteractInfo.InteractInfoList.IsEmpty() || !CurrentGlobalInputInfo.GetIsValidInputInfo()) return EResultStateNow::None;
	
	FInteractLogicInfo LocalInteractLogicInfo;
	// Find Interact Logic Info. if failed - return EResultStateNow::None
	if(!GetInteractLogicInfo(LocalInteractLogicInfo)) return EResultStateNow::None;

	// Is cant interact now (cooldown or other reason) - return none
	if(!GetIsCanInteractNow(CurrentGlobalInputInfo, HitResult, InteractByActor)) return EResultStateNow::None;
	
	// Do logic by interact input type
	const auto& LocalInteractInputInfo = LocalInteractLogicInfo.AllInteractInputInfo;
	
	
	EResultStateNow LocalCurrentInteractSuccess = EResultStateNow::None;
	switch(LocalInteractInputInfo.InteractInputType)
	{
		case EInteractInputType::Simple:
			{
				CurrentSimpleInteractInfo = TPair<FSimpleInteractInfo, bool>(LocalInteractInputInfo.SimpleInteractInfo, true);
				LocalCurrentInteractSuccess = SimpleInteract(CurrentGlobalInputInfo,
				LocalInteractInputInfo.SimpleInteractInfo);
			}
			break;
		case EInteractInputType::Advanced:
			{
				CurrentAdvancedInteractInfo = TPair<FAdvancedInteractInfo, bool>(LocalInteractInputInfo.AdvancedInteractInfo, true);
				LocalCurrentInteractSuccess = AdvancedInteract(CurrentGlobalInputInfo,
				LocalInteractInputInfo.AdvancedInteractInfo, true);
			}
			break;
		case EInteractInputType::None:
		default:
			break;
	}

	return LocalCurrentInteractSuccess;
}

// On Change Interact Input Info
void UInteractableActorComponent::OnChangeInteractInputInfo()
{
	StopAllCurrentInteract();
}

bool UInteractableActorComponent::CheckIsChangeInteractInputInfo(const FInteractInputInfo& OtherInputInfo)
{
	if(OtherInputInfo.InteractActionName == CurrentGlobalInputInfo.InteractActionName &&
		OtherInputInfo.InteractSceneCompTag == CurrentGlobalInputInfo.InteractSceneCompTag) return false;
	
	OnChangeInteractInputInfo();
	return true;
}


//--------------------------------------------------------------------------------------------------------------------//


//----------------------------------------------Input interact logic--------------------------------------------------//


const EResultStateNow UInteractableActorComponent::SimpleInteract(const FInteractInputInfo& InputInfo,
	const FSimpleInteractInfo& SingleTapInputInfo)
{
	OnFinishInput(EResultStateNow::Finish, InputInfo, SingleTapInputInfo.CooldownTime);
    return EResultStateNow::Finish;
}

const EResultStateNow UInteractableActorComponent::AdvancedInteract(const FInteractInputInfo& InputInfo,
	const FAdvancedInteractInfo& AdvancedInteractInfo, const bool bIsHoldNow)
{
	if(!GetWorld() || !InputInfo.GetIsValidInputInfo()) return EResultStateNow::None;
	
	FCurrentInputProgressInfo LocalCurrentHoldInputInfo;
	if(CurrentAdvancedInputInfo.Contains(InputInfo)) LocalCurrentHoldInputInfo = CurrentAdvancedInputInfo.FindRef(InputInfo);

	//----------------------------------------------Calculate Hold Info-----------------------------------------------//
	
	LocalCurrentHoldInputInfo.SetInteractInputInfo(InputInfo); // Set interact input info
	LocalCurrentHoldInputInfo.SuccessProgressValue = AdvancedInteractInfo.SuccessProgressValue;
	const float LocalSuccessValue = LocalCurrentHoldInputInfo.SuccessProgressValue;
	CurrentAdvancedInputInfo.Add(InputInfo, LocalCurrentHoldInputInfo); // Save calculated info
	
	const auto LocalDeltaValueInfo = bIsHoldNow ? AdvancedInteractInfo.InputIncreaseValueInfo :
	AdvancedInteractInfo.InputDecreaseValueInfo; // Increase Delta Value Info
	const float LocalDeltaValue = LocalDeltaValueInfo.ChangeDeltaValue; // Increase Value
	
	// Set on success increase progress delegate
	TFunction<void()> LocalOnSuccessHoldCallback = [this, InputInfo, AdvancedInteractInfo, bIsHoldNow]()
	{
		OnFinishInput(bIsHoldNow ? EResultStateNow::Finish : EResultStateNow::Failed, InputInfo,
			AdvancedInteractInfo.CooldownTime, CurrentAdvancedInputInfo);
	};

	EResultStateNow LocalResultStateChangeProgress = EResultStateNow::None;
	// Set increase timer
	FTimerDelegate AdvancedChangeProgressTimerDelegate;
	AdvancedChangeProgressTimerDelegate.BindLambda([this,&LocalResultStateChangeProgress, InputInfo,bIsHoldNow,LocalDeltaValue,LocalOnSuccessHoldCallback,LocalSuccessValue]()
	{
		// Increase Progress from hold info
		ChangeProgress(LocalResultStateChangeProgress,LocalDeltaValue, bIsHoldNow ? LocalSuccessValue : 0.f, LocalOnSuccessHoldCallback,
			CurrentAdvancedInputInfo.Find(InputInfo), bIsHoldNow);
	});

	SetLogicTimer(
		InputInfo,
		AdvancedChangeProgressTimerDelegate,
		LocalDeltaValueInfo.ChangeValueUpdateTime,
		ChangeValueTimersByInputInfo,
		true
		);
	
	// Dont Call If Set Logic Timer - Execute Method and Success
	
	// Set time to interact if TimeToInteract > 0.0f
	if(LocalResultStateChangeProgress != EResultStateNow::Finish) SetInteractTimeTimer(CurrentGlobalInputInfo,
		AdvancedInteractInfo.TimeToInteract, AdvancedInteractInfo.CooldownTime,CurrentAdvancedInputInfo
		, AdvancedInteractInfo.bIsUpdateInteractTimeOnTap);
	
	// else during input
	return EResultStateNow::During;
}

//--------------------------------------------------------------------------------------------------------------------//


//------------------------------------------------Decrease Functions--------------------------------------------------//

// Change progress for Hold and Multitap
void UInteractableActorComponent::ChangeProgress(EResultStateNow& EResultStateNow, const float ChangeValue, const float SuccessValue,
	TFunction<void()> OnFinishCallback, FCurrentInputProgressInfo* CurrentInputProgressInfo, const bool bIsAddProgress)
{
	if(!CurrentInputProgressInfo->GetInteractInputInfo().GetIsValidInputInfo()) EResultStateNow = EResultStateNow::None;
		
	// Change value from current progress
	CurrentInputProgressInfo->CurrentProgressValue = bIsAddProgress ? CurrentInputProgressInfo->CurrentProgressValue + ChangeValue :
	CurrentInputProgressInfo->CurrentProgressValue - ChangeValue;
	CurrentInputProgressInfo->CalculateProgressPercent(); // Recalculate progress percent
	OnInputProgressChange.Broadcast(*CurrentInputProgressInfo); // Call delegate after calculate logic
	if(CurrentInputProgressInfo->GetInteractInputInfo() == CurrentGlobalInputInfo) // Call delegate if calculated logic is current focused
	{
		OnCurrentInputProgressChange.Broadcast(*CurrentInputProgressInfo);
	}
	
	// Is reach success value by current calculated value
	const auto bLocalIsReachSuccessValue = FMath::IsNearlyEqual(CurrentInputProgressInfo->CurrentProgressValue,
		SuccessValue, 0.001f) || (bIsAddProgress ? CurrentInputProgressInfo->CurrentProgressValue > SuccessValue :
			CurrentInputProgressInfo->CurrentProgressValue < SuccessValue);
	
	// Stop change if progress reach success value
	if(bLocalIsReachSuccessValue && !FMath::IsNearlyZero(ChangeValue,0.00001f))
	{
		EResultStateNow = EResultStateNow::Finish;
		OnFinishCallback();
	}
}

//--------------------------------------------------------------------------------------------------------------------//


//-------------------------------------------------Stop functions-----------------------------------------------------//

void UInteractableActorComponent::StopAdvancedInteract(const FInteractInputInfo& InputInfo)
{
	if(!InputInfo.GetIsValidInputInfo() || !CurrentAdvancedInputInfo.Contains(InputInfo) || !CurrentAdvancedInteractInfo.Value) return;
	const auto LocalCurrentHoldInfo = MoveTemp(CurrentAdvancedInteractInfo.Key);
	// Clear Info after advanced interact
	CurrentAdvancedInteractInfo = TPair<FAdvancedInteractInfo, bool>(FAdvancedInteractInfo(), false);
	AdvancedInteract(InputInfo, LocalCurrentHoldInfo, false);
}

void UInteractableActorComponent::StopSimpleInteract(const FInteractInputInfo& InputInfo)
{
	CurrentSimpleInteractInfo = TPair<FSimpleInteractInfo, bool>(FSimpleInteractInfo(), false);
}

void UInteractableActorComponent::StopAllCurrentInteract(const FInteractInputInfo& InputInfo)
{
	StopAdvancedInteract(InputInfo);
	StopSimpleInteract(InputInfo);
}

//-----------------------------------------------Timer functions------------------------------------------------------//

void UInteractableActorComponent::SetLogicTimer(const FInteractInputInfo& InputInfo, FTimerDelegate TimerDelegate,
	const float Time, TMap<const FInteractInputInfo, FTimerHandle>& MapWithTimers, const bool bIsRestartTimer)
{
	if(!GetWorld()) return;
		
	FTimerHandle LocalChangeTimerHandle;
	// Check is contains timer handle. If timer active - return
	if(MapWithTimers.Contains(InputInfo))
	{
		LocalChangeTimerHandle = MapWithTimers.FindRef(InputInfo);
		if(GetWorld()->GetTimerManager().IsTimerActive(LocalChangeTimerHandle) && !bIsRestartTimer) return;
		// Restart timer if bIsRestartTimer == true
		if(bIsRestartTimer) GetWorld()->GetTimerManager().ClearTimer(LocalChangeTimerHandle);
	}
	
	// Dont set timer if time <= 0.f
	if(Time > 0.f)
	{
		GetWorld()->GetTimerManager().SetTimer(LocalChangeTimerHandle, TimerDelegate, Time, true);
		MapWithTimers.Add(InputInfo, LocalChangeTimerHandle);
	}
	else
	{
		ClearTimerByInputInfo(InputInfo,MapWithTimers);
		TimerDelegate.Execute();
	}
}


//--------------------------------------------------------------------------------------------------------------------//


//--------------------------------------------------Clear Functions---------------------------------------------------//

const bool UInteractableActorComponent::ClearTimerByInputInfo(const FInteractInputInfo& InputInfo,
	TMap<const FInteractInputInfo, FTimerHandle>& ArrayToClearTimer)
{
	if(!GetWorld()) return false;
	FTimerHandle LocalCooldownTimerHandle;
	if(ArrayToClearTimer.RemoveAndCopyValue(InputInfo, LocalCooldownTimerHandle))
	{
		// Clear timer from Map
		GetWorld()->GetTimerManager().ClearTimer(LocalCooldownTimerHandle);
		return true;
	}

	return false;
}

void UInteractableActorComponent::OnFinishInput(const EResultStateNow FinishState, const FInteractInputInfo& InputInfo,
	const float CooldownTime)
{
	switch (FinishState)
	{
	case EResultStateNow::None:
	case EResultStateNow::During:
		break;
	case EResultStateNow::Failed:
		OnFailedFromInputInteraction.Broadcast(InputInfo);
		break;
	case EResultStateNow::Finish:
		OnStartInputInteraction.Broadcast(InputInfo);
		break;
	}
	// Clear all currented interacted info
	CurrentAdvancedInteractInfo = TPair<FAdvancedInteractInfo, bool>(FAdvancedInteractInfo(), false);
	CurrentSimpleInteractInfo = TPair<FSimpleInteractInfo, bool>(FSimpleInteractInfo(), false);
	
	ClearTimerByInputInfo(InputInfo, InteractTimersByInputInfo);
	ClearTimerByInputInfo(InputInfo, ChangeValueTimersByInputInfo);
	
	// Set cooldown and clear interact timer if failed input
	SetCooldownTimer(InputInfo, CooldownTime);
}

// Call if lost actor with THIS component
void UInteractableActorComponent::CancelFromInteractInput()
{
	ClearCurrentGlobalInfo();
	ClearAllTimers(CooldownTimersByInputInfo);
	ClearAllTimers(InteractTimersByInputInfo);
	ClearAllTimers(ChangeValueTimersByInputInfo);
	ClearAllInputInfo();
}

// Clear all input info
void UInteractableActorComponent::ClearAllInputInfo()
{
	CurrentAdvancedInputInfo.Empty();
}

// Clear all timers
void UInteractableActorComponent::ClearAllTimers(TMap<const FInteractInputInfo, FTimerHandle>& ArrayToClearTimer)
{
	// Clear all timers
	for (auto& InputInfoInteract : ArrayToClearTimer)
	{
		ClearTimerByInputInfo(InputInfoInteract.Key, ArrayToClearTimer);
	}
}


//--------------------------------------------------------------------------------------------------------------------//


//------------------------------------------------Cooldown functions--------------------------------------------------//


void UInteractableActorComponent::SetCooldownTimer(const FInteractInputInfo& InputInfo, const float CooldownTime)
{
	if(!GetWorld() || CooldownTime <= 0.f) return;
	
	FTimerHandle LocalCooldownTimerHandle;
	if(CooldownTimersByInputInfo.Contains(InputInfo))
	{
		// If Already cooldown timer handle and its active - return
		LocalCooldownTimerHandle = CooldownTimersByInputInfo.FindRef(InputInfo);
		if(GetWorld()->GetTimerManager().IsTimerActive(LocalCooldownTimerHandle)) return;
	}

	// Start Cooldown timer
	FTimerDelegate CooldownTimerDelegate;
	CooldownTimerDelegate.BindLambda([this,InputInfo]()
	{
		OnCooldownTimeOver(InputInfo);
	});
	
	GetWorld()->GetTimerManager().SetTimer(LocalCooldownTimerHandle, CooldownTimerDelegate, CooldownTime, false);

	// Add timer handle cooldown
	CooldownTimersByInputInfo.Add(InputInfo, LocalCooldownTimerHandle);
}

void UInteractableActorComponent::OnCooldownTimeOver(const FInteractInputInfo& InputInfo)
{
	ClearTimerByInputInfo(InputInfo, CooldownTimersByInputInfo);
}

//--------------------------------------------------------------------------------------------------------------------//


//----------------------------------------------------Getter----------------------------------------------------------//


const bool UInteractableActorComponent::GetIsCanInteractNow(
	const FInteractInputInfo& CurrentInputInfo, FHitResult HitResult, AActor* QueryFromActor) const
{
	if(!GetOwner() || !GetWorld() || InteractInfo.InteractInfoList.IsEmpty() ||
		!CurrentGlobalInputInfo.GetIsValidInputInfo()) return false;

	// Get cooldown info
	// If timer is active - cant interact by cooldown
	const auto bLocalIsNotCooldownNow = CooldownTimersByInputInfo.Contains(CurrentInputInfo) ?
		!GetWorld()->GetTimerManager().IsTimerActive(CooldownTimersByInputInfo.FindRef(CurrentInputInfo)) : true;

	// Check is can interact with object by Owner
	auto bLocalIsActorCanInteract = true;
	if(GetOwner()->Implements<UInteractableActorInterface>())
	{
		bLocalIsActorCanInteract = IInteractableActorInterface::Execute_CanInteract(GetOwner(),
			CurrentGlobalInputInfo, HitResult, QueryFromActor);
	}
		
	return bLocalIsNotCooldownNow && bLocalIsActorCanInteract;
}

const bool UInteractableActorComponent::GetInteractLogicInfo(const FInteractInputInfo& InteractInputInfo,
	FInteractLogicInfo& InteractLogicInfo) const
{
	if(InteractInfo.InteractInfoList.IsEmpty() || !InteractInputInfo.GetIsValidInputInfo()) return false;
	const auto& LocalInteractArray = InteractInfo.InteractInfoList;
	
	// Try find interact scene component by tag
	if(LocalInteractArray.Contains(InteractInputInfo.InteractSceneCompTag))
	{
		InteractLogicInfo = LocalInteractArray.FindRef(InteractInputInfo.InteractSceneCompTag);
		return true;
	}
	// Is doesnt find component - try find interact scene component with empty tag
	if(LocalInteractArray.Contains(""))
	{
		InteractLogicInfo = LocalInteractArray.FindRef("");
		return true;
	}

	return false;
}

const bool UInteractableActorComponent::GetWidgetLocByCurrentInteractInputInfo(FVector& WidgetLoc) const
{
	if(!GetOwner()) return false;
	
	// If use custom widget location - get interact widget location from interface method
	if(GetOwner()->Implements<UInteractableActorWidgetInfoInterface>() && bIsUseCustomWidgetLocation)
	{
		IInteractableActorWidgetInfoInterface::Execute_GetInteractWidgetLocationFromInteractable(GetOwner(), WidgetLoc,
			CurrentGlobalInputInfo);
		return true;
	}
	
	// Return owner comp loc if contains
	if(const auto LocalInteractSceneComp = GetOwner()->FindComponentByTag<USceneComponent>(CurrentGlobalInputInfo.InteractSceneCompTag))
	{
		WidgetLoc = LocalInteractSceneComp->GetComponentLocation();
		return true;
	}
	
	// else return current hit loc
	WidgetLoc = CurrentHitLocation;
	return true;
}

//--------------------------------------------------------------------------------------------------------------------//


//------------------------------------------------Debug Functions-----------------------------------------------------//

void UInteractableActorComponent::ShowDebug()
{
	if(!GetWorld()) return;
	
	FString LocalDebugText = "-------------------------InputInfo-------------------------";
	LocalDebugText += FString("\nInteract Info Length - ") + FString::FromInt(InteractInfo.InteractInfoList.Num());
	LocalDebugText += FString("\nCurrent Interaction Action Name - ") + CurrentGlobalInputInfo.InteractActionName.ToString();
	LocalDebugText += FString("\nCurrent Interaction Scene Comp Tag - ") + CurrentGlobalInputInfo.InteractSceneCompTag.ToString();
	LocalDebugText += FString("\nCurrent Is Can Interact Now - ") + FString(GetIsCanInteractNow() ? "True" : "False");
	LocalDebugText += "\n--------------------------------------------------------------------------------------";
	
	GEngine->AddOnScreenDebugMessage(-1, 0.f, CurrentGlobalInputInfo.GetIsValidInputInfo() ?
		FColor::Green : FColor::Yellow, LocalDebugText);
	
	// Show Interact Timers Info
	ShowInteractTimersInfo(CooldownTimersByInputInfo, "CooldownTimersByInputInfo");
	ShowInteractTimersInfo(InteractTimersByInputInfo, "InteractTimersByInputInfo");
	ShowInteractTimersInfo(ChangeValueTimersByInputInfo, "ChangeValueTimersByInputInfo");
	
	// Show Advanced Input Progress Info
	ShowInputProgressInfo(CurrentAdvancedInputInfo, "AdvancedProgressInfo");

	// Show Current Advanced Input Info
	ShowCurrentAdvancedInputInfo();
}

void UInteractableActorComponent::ShowInteractTimersInfo(const TMap<const FInteractInputInfo, FTimerHandle> InteractTimersInfo, const FString DebugName, const FColor DebugColor) const
{
	if(!GetWorld() || InteractTimersInfo.IsEmpty()) return;
	FString LocalDebugText = "-------------------------" + DebugName + "-------------------------";
	
	for(auto Element : InteractTimersInfo)
	{
		LocalDebugText += FString("\nCurrent Interaction Action Name - ") + Element.Key.InteractActionName.ToString();
		LocalDebugText += FString("\nIs Timer active - ") + (GetWorld()->GetTimerManager().IsTimerActive(Element.Value) ?
		"Active" : "No");
		LocalDebugText += FString("\nRemaining time - ") + FString::SanitizeFloat(GetWorld()->GetTimerManager().GetTimerRemaining(Element.Value));
	}
	LocalDebugText += "\n--------------------------------------------------------------------------------------";
	
	GEngine->AddOnScreenDebugMessage(-1, 0.f, DebugColor, LocalDebugText);
}

void UInteractableActorComponent::ShowInputProgressInfo(const TMap<const FInteractInputInfo, FCurrentInputProgressInfo>& MapToShowDebug, const FString DebugName) const
{
	if(!GetWorld() || MapToShowDebug.IsEmpty()) return;
	FString LocalDebugText = "-------------------------" + DebugName + "-------------------------";

	for (const auto InputProgressInfo : MapToShowDebug)
	{
		LocalDebugText += FString("\nCurrent Interaction Scene Comp Tag - ") + InputProgressInfo.Value.InteractSceneCompTag.ToString();
		LocalDebugText += FString("\nCurrent Interaction Action Name - ") + InputProgressInfo.Value.InteractActionName.ToString();
		LocalDebugText += FString("\nCurrent Progress Value - ") + FString::SanitizeFloat(InputProgressInfo.Value.CurrentProgressValue);
		LocalDebugText += FString("\nCurrent Progress Percent - ") + FString::SanitizeFloat(InputProgressInfo.Value.CurrentProgressPercent);
		LocalDebugText += FString("\nMax Progress Value - ") + FString::SanitizeFloat(InputProgressInfo.Value.SuccessProgressValue);
		LocalDebugText += "\n--------------------------------------------------------------------------------------\n";
	}

	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::White, LocalDebugText);
}

void UInteractableActorComponent::ShowCurrentAdvancedInputInfo() const
{
	if(!GetWorld()) return;
	
	FString LocalDebugText = "-------------------------AdvancedInteractInfo-------------------------";
	LocalDebugText += FString("\nCurrent Advanced Interact Info - ") + FString(CurrentAdvancedInteractInfo.Value ? "True" : "False");
	LocalDebugText += "\n--------------------------------------------------------------------------------------\n";
	
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Purple, LocalDebugText);
}

//--------------------------------------------------------------------------------------------------------------------//
