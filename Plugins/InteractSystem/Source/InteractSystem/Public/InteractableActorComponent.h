// Florist Game. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "InteractActorComponent.h"
#include "InteractInfo.h"
#include "InteractableActorComponent.generated.h"

// Call on Success Interact
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStartInputInteraction, const FInteractInputInfo&, ActionInfo);
// Call On Stop Interact (release button on input)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStopFromInputInteraction, const FInteractInputInfo&, ActionInfo);
// Call On Failed Interact (interaction times up)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFailedFromInputInteraction, const FInteractInputInfo&, ActionInfo);
// On Input Progress Change (call on change any progress)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInputProgressChange, const FCurrentInputProgressInfo&, CurrentInputProgressInfo);
// On Current Input Progress Change (call on change current progress)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCurrentInputProgressChange, const FCurrentInputProgressInfo&, CurrentInputProgressInfo);
// On Input Progress Change
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInteractInputInfoChange, const FInteractInputInfo&, ActionInfo,
	const FCurrentInputProgressInfo&, CurrentInputProgressInfo);
// Call on Failed Try Interact (if can interact equal FALSE)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFailedTryInteract, const FInteractInputInfo&, ActionInfo);

enum class EResultStateNow : uint8
{
	None = 0,
	During = 1 << 0,
	Failed = 1 << 1,
	Finish = 1 << 2
};


UCLASS(Blueprintable, ClassGroup=(InteractSystem), meta=(BlueprintSpawnableComponent))
class INTERACTSYSTEM_API UInteractableActorComponent : public UInteractActorComponent
{
	GENERATED_BODY()

public:
	UInteractableActorComponent();
	
	// Delegates

	UPROPERTY(BlueprintAssignable, Category="InteractableSetup")
		FOnStartInputInteraction OnStartInputInteraction;
	UPROPERTY(BlueprintAssignable, Category="InteractableSetup")
		FOnStopFromInputInteraction OnStopFromInputInteraction;
	UPROPERTY(BlueprintAssignable, Category="InteractableSetup")
		FOnFailedFromInputInteraction OnFailedFromInputInteraction;
	UPROPERTY(BlueprintAssignable, Category="InteractableSetup")
		FOnInputProgressChange OnInputProgressChange;
	UPROPERTY(BlueprintAssignable, Category="InteractableSetup")
		FOnCurrentInputProgressChange OnCurrentInputProgressChange;
	UPROPERTY(BlueprintAssignable, Category="InteractableSetup")
		FOnInteractInputInfoChange OnInteractInputInfoChange;
	UPROPERTY(BlueprintAssignable, Category="InteractableSetup")
		FOnFailedTryInteract OnFailedTryInteract;
	
	// Functions

	// Add new interact info logic
	UFUNCTION(BlueprintCallable, Category="InteractableSetup")
		void AddOrRemoveInteractInfo(const FName InteractSceneCompTag, const FInteractLogicInfo NeInteractLogicInfo,
			const bool bIsAddInteractInfo = true);

	// Getter

	// Get Advanced Progress Info by Interact Info
	UFUNCTION(BlueprintCallable, Category="InteractableSetup")
	const FCurrentInputProgressInfo GetInputProgressInfo(const FInteractInputInfo& ActionInfo) const
	{
		if(CurrentAdvancedInputInfo.Contains(CurrentGlobalInputInfo.GetInteractInputInfo()))
			return CurrentAdvancedInputInfo.FindRef(CurrentGlobalInputInfo.GetInteractInputInfo());

		return FCurrentInputProgressInfo(CurrentGlobalInputInfo.GetInteractInputInfo());
	}
	// Get Current Advanced Progress Info
	UFUNCTION(BlueprintCallable, Category="InteractableSetup")
		const FCurrentInputProgressInfo GetCurrentInputProgressInfo() const
	{
		return GetInputProgressInfo(CurrentGlobalInputInfo.GetInteractInputInfo());
	}
	
protected:
	virtual void BeginPlay() override;

	virtual void ShowDebug() override;
	
	
	// Interact Interface Support Functions

	virtual bool StartInteractLogic_Implementation(FHitResult HitResult, AActor* InteractByActor) override;
	virtual bool CanInteractLogic_Implementation(FHitResult HitResult, AActor* QueryFromActor) override;
	virtual void CancelCanInteractLogic_Implementation(FHitResult HitResult) override;
	virtual void StopInteractLogic_Implementation(FHitResult HitResult) override;
	virtual void GetInteractWidgetLocationLogic_Implementation(FVector& WidgetLocation) const override;
	virtual const bool GetIsShowInteractWidgetLogic_Implementation() const override;
	virtual void FailedTryInteractLogic_Implementation(FHitResult HitResult) override;
	virtual void TraceHitLogic_Implementation(FHitResult HitResult, AActor* QueryFromActor) override;
	
private:
	
	//---------------------------------------------Interact Setup Info------------------------------------------------//

	/*
	 If TMap not contains interact scene component or cant find interact scene component name or equal "None"
	 can interact with all scene components
	 */
	UPROPERTY(EditAnywhere, Category="InteractableSetup", meta=(AllowPrivateAccess))
		FInteractInfo InteractInfo;
	/*
	 * Is use custom widget location
	 */
	UPROPERTY(EditAnywhere, Category="InteractableSetup", meta=(AllowPrivateAccess))
		bool bIsUseCustomWidgetLocation = false;

	//----------------------------------------------------------------------------------------------------------------//
	
	//------------------------------------------------Functions-------------------------------------------------------//
	
	// Main function to interact
	const EResultStateNow TryInteractInput(FHitResult HitResult, AActor* InteractByActor);
	
	// Clear current info now
	FORCEINLINE void ClearCurrentGlobalInfo()
	{
		CheckIsChangeInteractInputInfo(FInteractInputInfo()); // On Change Interact Input Info
		CurrentGlobalInputInfo = FInteractInputInfo();
		
		OnInteractInputInfoChange.Broadcast(CurrentGlobalInputInfo, FCurrentInputProgressInfo());
	}

	// Is Change Interact Input Info
	void OnChangeInteractInputInfo();
	bool CheckIsChangeInteractInputInfo(const FInteractInputInfo& OtherInputInfo);

	
	//------------------------------------------------Templates-------------------------------------------------------//
	/*
	 * Clear interact info by input info
	 * T - map to clear. InputInfo - info to search for find clear interact info
	*/
	template <typename KeyType, typename ValueType, typename SetAllocator, typename KeyFuncs> /* Map of current info */
	void ClearCurrentInteractInfo(const FInteractInputInfo& InputInfo, TMap<KeyType, ValueType, SetAllocator, KeyFuncs>& MapToClear);
	
	/*
	 * If interact was failed or finished
	 * Clear Current Info
	 */
	template <typename KeyType, typename ValueType, typename SetAllocator, typename KeyFuncs> /* Map of current info */
	void OnFinishInput(const EResultStateNow FinishState, const FInteractInputInfo& InputInfo, const float CooldownTime,
		TMap<KeyType, ValueType, SetAllocator, KeyFuncs>& MapToClear);
	
	// Set Interact Time timer by current input info
	template <typename KeyType, typename ValueType, typename SetAllocator, typename KeyFuncs> /* Map of current info */
	void SetInteractTimeTimer(const FCurrentGlobalInputInfo& InputInfo, const float InteractTime, const float CooldownTime,
		TMap<KeyType, ValueType, SetAllocator, KeyFuncs>& MapToClear, const bool bIsRestartTimer = false);

	
	//----------------------------------------------Clear Functions---------------------------------------------------//
	
	// Cancel from input (clear all inputs info, current info, all timers)
	void CancelFromInteractInput();
	// Clear all input info (hold, multitap)
	void ClearAllInputInfo();
	// Clear all interact timers
	void ClearAllTimers(TMap<const FInteractInputInfo, FTimerHandle>& ArrayToClearTimer);
	// Clear timer info
	const bool ClearTimerByInputInfo(const FInteractInputInfo& InputInfo, TMap<const FInteractInputInfo, FTimerHandle>& ArrayToClearTimer);

	
	//---------------------------------------------Interact logic by input--------------------------------------------//
	
	const EResultStateNow SimpleInteract(const FInteractInputInfo& InputInfo, const FSimpleInteractInfo& SimpleInteractInfo);
	const EResultStateNow AdvancedInteract(const FInteractInputInfo& InputInfo, const FAdvancedInteractInfo& AdvancedInteractInfo, const bool bIsHoldNow);

	void OnFinishInput(const EResultStateNow FinishState, const FInteractInputInfo& InputInfo, const float CooldownTime);

	
	//---------------------------------------------Cooldown functions-------------------------------------------------//

	// Set cooldown timer by current input info
	void SetCooldownTimer(const FInteractInputInfo& InputInfo, const float CooldownTime);

	// Reset cooldown timer by current input info
	void OnCooldownTimeOver(const FInteractInputInfo& InputInfo);

	
	//--------------------------------------------Change Progress functions-------------------------------------------//
	
	// Change progress value
	void ChangeProgress(EResultStateNow& EResultStateNow, const float ChangeValue, const float SuccessValue, TFunction<void()> OnFinishCallback
		, FCurrentInputProgressInfo* CurrentInputProgressInfo, const bool bIsAddProgress = false);

	
	//-----------------------------------------------Stop functions---------------------------------------------------//
	
	void StopAdvancedInteract(const FInteractInputInfo& InputInfo);
	void StopAdvancedInteract() { StopAdvancedInteract(CurrentGlobalInputInfo); }

	void StopSimpleInteract(const FInteractInputInfo& InputInfo);
	void StopSimpleInteract() { StopSimpleInteract(CurrentGlobalInputInfo); }

	void StopAllCurrentInteract(const FInteractInputInfo& InputInfo);
	void StopAllCurrentInteract() { StopAllCurrentInteract(CurrentGlobalInputInfo); }

	
	//-----------------------------------------------Timer functions--------------------------------------------------//

	/*
	 * Set Timer by input info
	 * Return true only if TimerDelegate was called without set timer. (It happens if time < 0.f)
	 * 
	*/
	void SetLogicTimer(const FInteractInputInfo& InputInfo, FTimerDelegate TimerDelegate, const float Time, 
		TMap<const FInteractInputInfo, FTimerHandle>& MapWithTimers, const bool bIsRestartTimer = false);

	
	//------------------------------------------------Getter----------------------------------------------------------//

	// Get is can interact now by Interact Input Info
	const bool GetIsCanInteractNow(const FInteractInputInfo& CurrentInputInfo, FHitResult HitResult, AActor* QueryFromActor) const;
	const bool GetIsCanInteractNow() const
	{ return GetIsCanInteractNow(CurrentGlobalInputInfo, FHitResult(), nullptr); }

	// Get Interact Logic Info by Interact Input Info
	const bool GetInteractLogicInfo(const FInteractInputInfo& InteractInputInfo, FInteractLogicInfo& InteractLogicInfo) const; 
	const bool GetInteractLogicInfo(FInteractLogicInfo& InteractLogicInfo)
	{
		return GetInteractLogicInfo(CurrentGlobalInputInfo, InteractLogicInfo);
	}

	// Get widget location from current interact scene comp location
	const bool GetWidgetLocByCurrentInteractInputInfo(FVector& WidgetLoc) const;

	
	//------------------------------------------------Show Debug------------------------------------------------------//
	
	void ShowInteractTimersInfo(const TMap<const FInteractInputInfo, FTimerHandle> InteractTimersInfo, const FString DebugName,
		const FColor DebugColor = FColor::Yellow) const;
	void ShowInputProgressInfo(const TMap<const FInteractInputInfo, FCurrentInputProgressInfo>& MapToShowDebug, const FString DebugName) const;
	void ShowCurrentAdvancedInputInfo() const;
	
	//------------------------------------------------Variables-------------------------------------------------------//
	
	// Service Info

	FVector CurrentHitLocation = FVector::Zero();
	
	// Timers Info
	TMap<const FInteractInputInfo, FTimerHandle> CooldownTimersByInputInfo;
	TMap<const FInteractInputInfo, FTimerHandle> InteractTimersByInputInfo;
	TMap<const FInteractInputInfo, FTimerHandle> ChangeValueTimersByInputInfo;
	
	// Current Input Info
	TMap<const FInteractInputInfo, FCurrentInputProgressInfo> CurrentAdvancedInputInfo;
	
	// Current Info
	FInteractInputInfo CurrentGlobalInputInfo;

	// Current Interacted Info (current hold, multitap and etc) - bool - is set now
	TPair<FAdvancedInteractInfo, bool> CurrentAdvancedInteractInfo = TPair<FAdvancedInteractInfo, bool>(FAdvancedInteractInfo(), false);
	TPair<FSimpleInteractInfo, bool> CurrentSimpleInteractInfo = TPair<FSimpleInteractInfo, bool>(FSimpleInteractInfo(), false);
	
};


//--------------------------------------------------Templates---------------------------------------------------------//


template <typename KeyType, typename ValueType, typename SetAllocator, typename KeyFuncs> /* Map of current info */
void UInteractableActorComponent::ClearCurrentInteractInfo(const FInteractInputInfo& InputInfo,
	TMap<KeyType, ValueType, SetAllocator, KeyFuncs>& MapToClear)
{
	if(MapToClear.IsEmpty() || !MapToClear.Contains(InputInfo)) return;
	MapToClear.Remove(InputInfo);
}

template <typename KeyType, typename ValueType, typename SetAllocator, typename KeyFuncs> /* Map of current info */
void UInteractableActorComponent::OnFinishInput(const EResultStateNow FinishState, const FInteractInputInfo& InputInfo,
	const float CooldownTime, TMap<KeyType, ValueType, SetAllocator, KeyFuncs>& MapToClear)
{
	OnFinishInput(FinishState, InputInfo, CooldownTime);

	ClearCurrentInteractInfo(InputInfo, MapToClear);
}

// Set timer for interact time. If timer is finished - set cooldown timer and clear all info
template <typename KeyType, typename ValueType, typename SetAllocator, typename KeyFuncs> /* Map of current info */
void UInteractableActorComponent::SetInteractTimeTimer(const FCurrentGlobalInputInfo& InputInfo, const float InteractTime
	, const float CooldownTime, TMap<KeyType, ValueType, SetAllocator, KeyFuncs>& MapToClear, const bool bIsRestartTimer)
{
	if(!GetWorld() || InteractTime <= 0.f) return;
	
	FTimerHandle LocalInteractTimerHandle;
	if(InteractTimersByInputInfo.Contains(InputInfo.InteractInputInfo))
	{
		// If Already cooldown timer handle and its active - return
		LocalInteractTimerHandle = InteractTimersByInputInfo.FindRef(InputInfo.InteractInputInfo);
		if(GetWorld()->GetTimerManager().IsTimerActive(LocalInteractTimerHandle) && !bIsRestartTimer) return;
		
		// Clear timer if is bIsRestartTimer == true
		ClearTimerByInputInfo(InputInfo.InteractInputInfo, InteractTimersByInputInfo);
	}
	
	// If cooldown time <= 0.f - doesnt set cooldown timer
	const float LocalCooldownTime = CooldownTime;
	if(LocalCooldownTime <= 0.f) return;

	const auto& LocalInteractInput = InputInfo.InteractInputInfo;
	// Start Cooldown timer if Interact time is over
	FTimerDelegate InteractTimerDelegate;
	InteractTimerDelegate.BindLambda([this, LocalInteractInput, LocalCooldownTime, &MapToClear]()
	{
		OnFinishInput(EResultStateNow::Failed, LocalInteractInput, LocalCooldownTime, MapToClear);
	});
	
	GetWorld()->GetTimerManager().SetTimer(LocalInteractTimerHandle, InteractTimerDelegate, InteractTime, false);

	// Add timer handle interact
	InteractTimersByInputInfo.Add(InputInfo.InteractInputInfo, LocalInteractTimerHandle);
}

//--------------------------------------------------------------------------------------------------------------------//