// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SpringArmComponent.h"
#include "CustomSpringArmComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MASSTRAFFIC_API UCustomSpringArmComponent : public USpringArmComponent
{
	GENERATED_BODY()

public:
	UCustomSpringArmComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=CameraCollision, meta=(editcondition="bDoCollisionTest"))
	float OffsetFromOrigin;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=CameraCollision, meta=(editcondition="bDoCollisionTest"))
	bool bDrawDebugCollisionTest;

protected:
	virtual void BeginPlay() override;
	
	virtual void UpdateDesiredArmLocation(bool bDoTrace, bool bDoLocationLag, bool bDoRotationLag, float DeltaTime) override;
};
