#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "InteractInfo.generated.h"

//--------------------------------------------------Enumerations-----------------------------------------------------//

UENUM(BlueprintType)
enum class EInteractInputType : uint8
{
	None = 0 UMETA(Hidden), 
	Simple = 1 << 0,
	Advanced = 1 << 1
};

//---------------------------------------------------Structures------------------------------------------------------//

// Single Input Info

USTRUCT(BlueprintType)
struct FSimpleInteractInfo
{
	GENERATED_BODY()

	// Cooldown after interact was failed
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InteractInputSetup", meta=(ClampMin="0.0"))
		float CooldownTime = 0.f;
	
	FSimpleInteractInfo() {}

	FSimpleInteractInfo(const float NewCooldownTime) : CooldownTime(NewCooldownTime) {}
	
};

USTRUCT(BlueprintType)
struct FInputChangeValueInfo
{
	GENERATED_BODY()

	// Time Info for another input types
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InteractInputSetup", meta=(ClampMin="0.0"))
		float ChangeValueUpdateTime = 0.1f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InteractInputSetup", meta=(ClampMin="0.0"))
		float ChangeDeltaValue = 0.1f;
	
	FInputChangeValueInfo() {}
	
};

USTRUCT(BlueprintType)
struct FAdvancedInteractInfo : public FSimpleInteractInfo
{
	GENERATED_BODY()

	// Hold Info

	// Time to interact. If times up interact is failed
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InteractInputSetup", meta=(ClampMin="0.0"))
		float TimeToInteract = 0.f;
	// Is Need Update Interact Time on Tap
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InteractInputSetup")
		bool bIsUpdateInteractTimeOnTap = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InteractInputSetup")
		FInputChangeValueInfo InputIncreaseValueInfo;
	// Success progress value
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InteractInputSetup", meta=(ClampMin="0.1"))
		float SuccessProgressValue = 3.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InteractInputSetup")
		FInputChangeValueInfo InputDecreaseValueInfo;
	
	FAdvancedInteractInfo() {}
	
};

USTRUCT(BlueprintType)
struct FAllInputInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InteractInputSetup")
		EInteractInputType InteractInputType = EInteractInputType::Simple;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InteractInputSetup", meta=(EditCondition="InteractInputType==EInteractInputType::Simple", EditConditionHides))
		FSimpleInteractInfo SimpleInteractInfo;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InteractInputSetup", meta=(EditCondition="InteractInputType==EInteractInputType::Advanced", EditConditionHides))
		FAdvancedInteractInfo AdvancedInteractInfo;

	FAllInputInfo() {}

	FSimpleInteractInfo GetSimpleInputInfoByInputType() const
	{
		switch(InteractInputType)
		{
			case EInteractInputType::Simple:
				return SimpleInteractInfo;
			case EInteractInputType::Advanced:
				return AdvancedInteractInfo;
			case EInteractInputType::None:
			default:
				break;
		}
		return FSimpleInteractInfo();
	}
	
};

USTRUCT(BlueprintType)
struct FInteractLogicInfo
{
	GENERATED_BODY()

	// Interact Action Name
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InteractInputSetup")
		FGameplayTag InteractActionName;
	// Location to attach Interact Widget
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InteractInputSetup")
		FString InteractActionWidgetComponent = "None";
	// Interact Input Info
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InteractInputSetup")
		FAllInputInfo AllInteractInputInfo;

	FInteractLogicInfo() {}
	
};

USTRUCT(BlueprintType)
struct FInteractInfo
{
	GENERATED_BODY()

	/* Interact Info List.
	 * Interact Scene Component Tag - Interact Logic Info.
	 * Make SaveGame flag for Save System
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InteractInputSetup")
		TMap<FName, FInteractLogicInfo> InteractInfoList;

	FInteractInfo() {}
	
};


//-------------------------------------------------CurrentInputInfo--------------------------------------------------//

USTRUCT(BlueprintType)
struct FInteractInputInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InputInfo")
		FGameplayTag InteractActionName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InputInfo")
		FName InteractSceneCompTag = "";

	bool operator==(const FInteractInputInfo& OtherInteractInputInfo) const
	{
		return OtherInteractInputInfo.InteractActionName == InteractActionName &&
			OtherInteractInputInfo.InteractSceneCompTag == InteractSceneCompTag;
	}

	FORCEINLINE bool GetIsValidInputInfo() const
	{ return InteractActionName.IsValid(); }

	FORCEINLINE void SetInteractInputInfo(const FInteractInputInfo& NewInteractInputInfo)
	{
		InteractActionName = NewInteractInputInfo.InteractActionName;
		InteractSceneCompTag = NewInteractInputInfo.InteractSceneCompTag;
	}

	// Get this structure
	FORCEINLINE const FInteractInputInfo& GetInteractInputInfo() const { return *this; }
	
	FInteractInputInfo() {}
	
	FInteractInputInfo(const FGameplayTag NewInteractActionName, const FName NewInteractSceneCompTag) :
		InteractActionName(NewInteractActionName), InteractSceneCompTag(NewInteractSceneCompTag) {}
	
	FInteractInputInfo(const FInteractInputInfo& NewInteractInputInfo) :
	InteractActionName(NewInteractInputInfo.InteractActionName), InteractSceneCompTag(NewInteractInputInfo.InteractSceneCompTag) {}
	
};

FORCEINLINE uint32 GetTypeHash(const FInteractInputInfo& OtherInteractInputInfo)
{
	return HashCombine(GetTypeHash(OtherInteractInputInfo.InteractActionName), GetTypeHash(OtherInteractInputInfo.InteractSceneCompTag));
}

USTRUCT(BlueprintType)
struct FCurrentInputProgressInfo : public FInteractInputInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InputProgressInfo", meta=(ClampMin="0.0"))
		float CurrentProgressValue = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InputProgressInfo", meta=(ClampMin="0.0"))
		float SuccessProgressValue = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InputProgressInfo", meta=(ClampMin="0.0"))
		float CurrentProgressPercent = 0.f;

	const void CalculateProgressPercent()
	{
		CurrentProgressPercent = CurrentProgressValue / SuccessProgressValue;
		// Clamp Progress Percent between 0 and 1
		CurrentProgressPercent = CurrentProgressPercent > 1.f ? 1.f : CurrentProgressPercent < 0.f ? 0.f : CurrentProgressPercent; 
	}
	
	FCurrentInputProgressInfo(const FInteractInputInfo& NewInteractInputInfo = FInteractInputInfo()) : FInteractInputInfo(NewInteractInputInfo) {}
	
};

// Global Info about interact input
struct FCurrentGlobalInputInfo
{
	FInteractInputInfo InteractInputInfo = FInteractInputInfo();

	FCurrentGlobalInputInfo() {}
	FCurrentGlobalInputInfo(FInteractInputInfo NewInteractInpuInfo) : InteractInputInfo(NewInteractInpuInfo) {}
};