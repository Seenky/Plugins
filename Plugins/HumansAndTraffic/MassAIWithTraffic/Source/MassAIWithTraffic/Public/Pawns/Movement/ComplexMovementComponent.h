// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NewCharacterMovementComponent.h"
#include "SimplePawnMovementComponent.h"
#include "ComplexMovementComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MASSAIWITHTRAFFIC_API UComplexMovementComponent : public USimplePawnMovementComponent
{
	GENERATED_BODY()

public:
	UComplexMovementComponent();

	virtual void UpdateMovementInfo(float Time) override;
	
protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	// Getter

	const FVector GetInterpolatedVelocityDirection() const
	{
		return InterpolatedVelocityDirection;
	}

	UFUNCTION(BlueprintCallable, Category = "Movement")
	const float GetCurrentYawRotate() const
	{
		return CurrentYawRotate;
	}
	
private:

	UPROPERTY(EditAnywhere, Category="Rotate Info", meta=(AllowPrivateAccess))
		float YawRotateWheelsInterpSpeed = 5.f;
	
	FVector InterpolatedVelocityDirection;
	float CurrentYawRotate = 0.f;
};
