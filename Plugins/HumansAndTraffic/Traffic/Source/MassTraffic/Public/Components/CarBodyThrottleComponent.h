// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Static/MassTrafficPhysics.h"
#include "Components/ActorComponent.h"
#include "CarBodyThrottleComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MASSTRAFFIC_API UCarBodyThrottleComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UCarBodyThrottleComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	void RotateCarCorpus(const FMassTrafficVehiclePhysicsFragment* PhysicsFragment) const;

public:
	//Settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	float MaxBodyPitchThrottle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	float MaxBodyRollThrottle;
	UPROPERTY(EditAnywhere, Category="Settings")
	float BackInterpSpeed;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	TObjectPtr<USceneComponent> Body;

private:
	bool bIsAccelerating;
	bool bIsBraking;
	float TargetRoll;
	FRotator InitialRotation;
	FRotator CurrentTargetRotation;
};
