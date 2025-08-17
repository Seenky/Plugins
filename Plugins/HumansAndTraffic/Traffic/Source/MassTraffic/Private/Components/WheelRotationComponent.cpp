// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/WheelRotationComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "StaticMathLibrary.h"

UWheelRotationComponent::UWheelRotationComponent()
    : MaxTurnRotation(15.0f)
    , bRotateInverse(false)
    , MovementRotationAxis(EAxis::X)
    , TurnRotationAxis(EAxis::Z)
    , WheelRadius(25.f)
    , WheelComponents({})
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UWheelRotationComponent::SetWheelComponents(TMap<FString, USceneComponent*> Wheels)
{
    ResetWheels(true);
    WheelComponents.Append(Wheels);

    for (auto Wheel : Wheels)
    {
        WheelsRotation.Add(Wheel.Value, Wheel.Value->GetRelativeRotation());
    }
}

void UWheelRotationComponent::ResetWheels(bool bClearWheelArray)
{
    if (bClearWheelArray) WheelComponents.Empty();
}

void UWheelRotationComponent::RotateWheels(float YawInput)
{
    SetMovementRotation();
    SetTurnRotation(YawInput);
}

void UWheelRotationComponent::SetMovementRotation()
{
    const float Speed = GetOwner()->GetVelocity().Length();
    if (Speed <= 5.0f) return;
    for (const auto& Pair : WheelComponents)
    {
        if (FRotator* Rotation = WheelsRotation.Find(Pair.Value))
        {
            if (NoMovementRotateWheels.Contains(Pair.Key)) continue;

            float DecreaseRot = 1.0f;
            if (DecreaseRotationWheels.Find(Pair.Key))
                DecreaseRot = *DecreaseRotationWheels.Find(Pair.Key);

            const float RotationRate = UStaticMathLibrary::CalculateMoveRotationBySpeed(GetWorld(), Speed * DecreaseRot, WheelRadius);
            
            const float CurrentRotation = UStaticMathLibrary::GetAxisValue(*Rotation, MovementRotationAxis);
            float NewRotation = CurrentRotation + (bRotateInverse ? -RotationRate : RotationRate);

            NewRotation = FMath::UnwindDegrees(NewRotation);

            UStaticMathLibrary::SetAxisValue(*Rotation, MovementRotationAxis, NewRotation);
    
            Pair.Value->SetRelativeRotation(*Rotation);
        }
    }
}

void UWheelRotationComponent::SetTurnRotation(float TurnAngle)
{
    if (abs(TurnAngle) <= 2.5) TurnAngle = 0.f;
    
    for (const FString& WheelTag : TurnRotateWheels)
    {
        if (WheelComponents.Contains(WheelTag))
        {
            if (FRotator* Rotation = WheelsRotation.Find(WheelComponents[WheelTag]))
            {
                float NewRotation = UStaticMathLibrary::GetAxisValue(*Rotation, TurnRotationAxis);
   
                NewRotation = FMath::FInterpTo(
                    NewRotation,
                    FMath::Clamp(TurnAngle, abs(MaxTurnRotation) * -1, abs(MaxTurnRotation)),
                    GetWorld()->GetDeltaSeconds(),
                    1.f);
            
                USceneComponent* Wheel = WheelComponents[WheelTag];
            
                UStaticMathLibrary::SetAxisValue(*Rotation, TurnRotationAxis, NewRotation);
                Wheel->SetRelativeRotation(*Rotation);
            }
        }
    }
}



