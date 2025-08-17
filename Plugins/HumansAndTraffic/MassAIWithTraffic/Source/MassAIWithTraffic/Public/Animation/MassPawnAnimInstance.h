// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "SkeletalMeshComponentBudgeted.h"
#include "Interface/PrepareEventsInterface.h"
#include "MassPawnAnimInstance.generated.h"

USTRUCT(BlueprintType)
struct FCharacterStyles
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TSoftObjectPtr<UBlendSpace>> WalkBlendSpaces;
};

UCLASS()
class MASSAIWITHTRAFFIC_API UMassPawnAnimInstance : public UAnimInstance, public IPrepareEventsInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Character")
	float GetSafeSpeed() const;

	UFUNCTION(BlueprintCallable, Category = "Character")
	void SetSafeBlendSpace();

protected:
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

	virtual void NativeInitializeAnimation() override;

	virtual void NativeBeginPlay() override;

	virtual void OnPrepared_Implementation() override;
	virtual void SetupAnimOffset_Implementation(const float& Offset) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animation")
	TMap<TSoftObjectPtr<USkeletalMesh>, FCharacterStyles> CharacterStyles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animation")
	float StartOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Reference")
	TObjectPtr<APawn> CachedCharacter;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Reference")
	TObjectPtr<UPawnMovementComponent> MovementComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Reference")
	TObjectPtr<UBlendSpace> WalkBlendSpace;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement")
	float Speed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement")
	bool bIsWalk;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement")
	float Direction;

private:
	
	TSoftObjectPtr<USkeletalMeshComponent> SkeletalMeshComponent;
	
	mutable FCriticalSection MovementMutex;

	mutable FCriticalSection Animmutex;
};
