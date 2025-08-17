// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WheelRotationComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MASSTRAFFIC_API UWheelRotationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWheelRotationComponent();
	
	// Setter
	/*Set wheels components*/
	UFUNCTION(BlueprintCallable)
		FORCEINLINE void SetWheelComponents(TMap<FString, USceneComponent*> Wheels);
	/*Empty array of wheels components*/
	UFUNCTION(BlueprintCallable)
		void ResetWheels(bool bClearWheelArray = false);
	UFUNCTION(BlueprintCallable, Category = "Wheel Rotation Component")
		void SetWheelsRadius(const float& Radius) { WheelRadius = Radius; }
		
	// Getter
	/*Return wheel component by tag*/
	UFUNCTION(BlueprintCallable, BlueprintPure)
		FORCEINLINE void GetWheelComponentsTags(TArray<FString>& Keys) const { WheelComponents.GetKeys(Keys); }
	/*Return wheel by tag*/
	UFUNCTION(BlueprintCallable, BlueprintPure)
		FORCEINLINE USceneComponent* GetWheelByTag(const FString& WheelTag) const { return WheelComponents.Contains(WheelTag) ? WheelComponents[WheelTag] : nullptr; }
	
	// Wheel Logic
	void RotateWheels(float YawAngle);

protected:
	// Wheel Rotate Methods
	/*Main movement wheels rotation*/
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Wheel Rotation Component")
	void SetMovementRotation();
	/*Main turn wheels rotation*/
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Wheel Rotation Component")
	void SetTurnRotation(const float TurnAngle);
	
	/*Max rotation angle for turns*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Setup")
	float MaxTurnRotation;
	/*Names of front wheels to turn*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Setup")
	TArray<FString> TurnRotateWheels;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Setup")
	TArray<FString> NoMovementRotateWheels;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Setup")
	TMap<FString, float> DecreaseRotationWheels;
	/*Inverse movement rotation*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Setup")
	bool bRotateInverse;
	/*Select movement rotation axis*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Setup")
	TEnumAsByte<EAxis::Type> MovementRotationAxis;
	/*Select turn rotation axis*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Setup")
	TEnumAsByte<EAxis::Type> TurnRotationAxis;
	/*Radius of wheels meshes*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Setup", meta=(ClampMin="1.0"))
	float WheelRadius;

private:
	UPROPERTY()
		TMap<USceneComponent*, FRotator> WheelsRotation;
	UPROPERTY()
		TMap<FString, USceneComponent*> WheelComponents;
};
