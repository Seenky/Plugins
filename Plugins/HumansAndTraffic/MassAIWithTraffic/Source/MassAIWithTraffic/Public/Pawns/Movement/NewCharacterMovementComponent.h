// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NewCharacterMovementComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MASSAIWITHTRAFFIC_API UNewCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UNewCharacterMovementComponent();

	virtual void UpdateMovementInfo(float Time);

	const FVector GetPreviousLocation() const
	{
		return PreviousLocation;
	}
	
public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;
	
protected:
	virtual void BeginPlay() override;
	
	FVector PreviousLocation;
};
