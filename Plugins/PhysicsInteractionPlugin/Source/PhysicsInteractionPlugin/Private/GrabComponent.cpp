// Fill out your copyright notice in the Description page of Project Settings.


#include "GrabComponent.h"

#include "Camera/CameraComponent.h"
#include "Chaos/PBDConstraintBaseData.h"
#include "Chaos/PBDJointConstraintData.h"
#include "Components/ArrowComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "Interfaces/PhysInteractInterface.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY(LogGrabComponent);

TAutoConsoleVariable<int32> CVarEnableGrabDebug(
    TEXT("Grab.EnableDebug"),    
    0,                               
    TEXT("Enable debug for grab component\n") 
    " 0: Disable debug\n"
    " 1: Enable debug");   

UGrabComponent::UGrabComponent()
	: DefaultGrabDistance(200.0f)
    , MaxGrabHeightMass(50.0f)
    , PhysSettingsInterpSpeed(5.0f)
	, GrabbedComponentMassScale(3.0f)
    , MaxLinearDriveForce(5000.0f)
    , MaxFriction(10.0f)
    , MinFriction(0.7f)
    , bEnableDebug(false)
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = true;
}

#if WITH_EDITOR
void UGrabComponent::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    const FName PropertyName = PropertyChangedEvent.GetPropertyName();
    
    if (PropertyName == FName("InterpolationSpeed"))
    {
        DefaultInterpolationSpeed = InterpolationSpeed;
    }
    else if (PropertyName == FName("LinearStiffness"))
    {
        DefaultLinearStiffness = LinearStiffness;
    }
    else if (PropertyName == FName("LinearDamping"))
    {
        DefaultLinearDamping = LinearDamping;
    }
    else if (PropertyName == FName("DefaultGrabDistance"))
    {
        CurrentGrabDistance = DefaultGrabDistance;
    }
}
#endif

void UGrabComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME(UGrabComponent, SocketComponent);
    DOREPLIFETIME_CONDITION(UGrabComponent, CurrentGrabDistance, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UGrabComponent, CurrentGrabLocation, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UGrabComponent, GrabVelocity, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UGrabComponent, GrabDistance, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UGrabComponent, TargetGrabRotation, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UGrabComponent, TargetGrabLocation, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UGrabComponent, MaxLinearStiffness, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UGrabComponent, MaxLinearDamping, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UGrabComponent, bLastHit, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UGrabComponent, CurrentResistanceVelocity, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UGrabComponent, CurrentGravityZ, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UGrabComponent, CurrentMassRatio, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UGrabComponent, CurrentForce, COND_OwnerOnly);
}

void UGrabComponent::BeginPlay()
{
    Super::BeginPlay();
    SetupPhysMaterialSettings();
    SetComponentTickEnabled(false);
    
    CurrentGrabDistance = DefaultGrabDistance;
    DefaultInterpolationSpeed = InterpolationSpeed;
    DefaultLinearStiffness = LinearStiffness;
    DefaultLinearDamping = LinearDamping;

#if !UE_BUILD_SHIPPING
    BindCvars();
#endif
}

void UGrabComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

    Server_StopGrabObject();
}

void UGrabComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    CalculateCurrentGrabVelocity();
    CalculateCurrentGrabRotation();
    HoldPhysicsObject();
#if !UE_BUILD_SHIPPING
    DrawDebugInfo();
#endif
}

/*************************************************Setup****************************************************************/
void UGrabComponent::SetupPhysMaterialSettings()
{
    if (!PhysMaterial) return;
    
    DefaultPhysMaterialSettings.Friction = PhysMaterial->Friction;
    DefaultPhysMaterialSettings.StaticFriction = PhysMaterial->StaticFriction;
    DefaultPhysMaterialSettings.FrictionCombineMode = PhysMaterial->FrictionCombineMode;
    DefaultPhysMaterialSettings.OverrideFrictionCombineMode = PhysMaterial->bOverrideFrictionCombineMode;
    DefaultPhysMaterialSettings.Restitution = PhysMaterial->Restitution;
    DefaultPhysMaterialSettings.RestitutionCombineMode = PhysMaterial->RestitutionCombineMode;
    DefaultPhysMaterialSettings.OverrideRestitutionCombineMode = PhysMaterial->bOverrideRestitutionCombineMode;
    DefaultPhysMaterialSettings.Density = PhysMaterial->Density;
    DefaultPhysMaterialSettings.SleepLinearVelocityThreshold = PhysMaterial->SleepLinearVelocityThreshold;
    DefaultPhysMaterialSettings.SleepAngularVelocityThreshold = PhysMaterial->SleepAngularVelocityThreshold;
    DefaultPhysMaterialSettings.SleepCounterThreshold = PhysMaterial->SleepCounterThreshold;
    DefaultPhysMaterialSettings.RaiseMassToPower = PhysMaterial->RaiseMassToPower;
    DefaultPhysMaterialSettings.TensileStrength = PhysMaterial->Strength.TensileStrength;
    DefaultPhysMaterialSettings.CompressionStrength = PhysMaterial->Strength.CompressionStrength;
    DefaultPhysMaterialSettings.ShearStrength = PhysMaterial->Strength.ShearStrength;
    DefaultPhysMaterialSettings.DamageThresholdMultiplier = PhysMaterial->DamageModifier.DamageThresholdMultiplier;
}

bool UGrabComponent::SetupSocketComponent(AActor* Owner, const float& Distance)
{
    if (!Owner)
        return false;
    
    SocketComponent = Cast<UArrowComponent>(Owner->AddComponentByClass(
                                            UArrowComponent::StaticClass(),
                                            true,FTransform::Identity,true));

    if (!SocketComponent)
        return false;

    SocketComponent->SetMobility(EComponentMobility::Movable);

    if (UCameraComponent* OwnerCamera = Owner->FindComponentByClass<UCameraComponent>())
    {
        SocketComponent->SetupAttachment(OwnerCamera);
        SocketComponent->SetRelativeLocation(FVector(Distance, 0, 0));
    }
    else
    {
        SocketComponent->SetupAttachment(Owner->GetRootComponent());
        SocketComponent->SetRelativeLocation(FVector(Distance, 0, 0));
    }

    if (bEnableDebug)
    {
        SocketComponent->SetVisibility(true);
        SocketComponent->SetHiddenInGame(false);
        SocketComponent->SetArrowSize(0.4f);
        SocketComponent->SetArrowLength(100.0f);
    }

    Owner->FinishAddComponent(SocketComponent, true, SocketComponent->GetRelativeTransform());

    return true;
}
/**********************************************************************************************************************/

/***********************************************Main Server Logic******************************************************/
void UGrabComponent::Server_StopGrabObject_Implementation()
{
    if (!SocketComponent) return;
    
    SetComponentTickEnabled(false);

    if (GrabbedComponent)
    {
        FCollisionResponseContainer CollisionResponseContainer;
        CollisionResponseContainer.SetAllChannels(ECR_Block);
        GrabbedComponent->SetCollisionResponseToChannels(CollisionResponseContainer);

        if (PhysMaterial && GEngine && GrabbedComponent->GetBodyInstance()->GetSimplePhysicalMaterial() == PhysMaterial)
            GrabbedComponent->SetPhysMaterialOverride(nullptr);

        if (PhysMaterial)
            FChaosEngineInterface::UpdateMaterial(*PhysMaterial->MaterialHandle, PhysMaterial);
        
        GrabbedComponent->GetBodyInstance()->UpdatePhysicalMaterials();
        GrabbedComponent->GetBodyInstance()->UpdateMassProperties();
    }
    
    ReleaseComponent();
    
    if (SocketComponent)
        SocketComponent->DestroyComponent();

    SetDefaultPhysMaterialSettings();
    CurrentGrabLocation = FVector::ZeroVector;
    GrabVelocity = FVector::ZeroVector;
    TargetGrabRotation = FRotator::ZeroRotator;
}

void UGrabComponent::Server_TryGrabObject_Implementation(UPrimitiveComponent* PickupComponent)
{
    AActor* ComponentOwner = GetOwner();
    if (!ComponentOwner || !PickupComponent || PickupComponent->GetMass() > MaxGrabHeightMass)
        return;
    
    if (IPhysInteractInterface::Execute_IsOwnerHandsBusy(GetOwner()))
    {
        if (bEnableDebug)
            UE_LOG(LogGrabComponent, Display, TEXT("Owner hand busy"));
        return;
    }
    
    if (GetIsCharacterOnGrabObject(ComponentOwner, PickupComponent))
    {
        if (bEnableDebug)
            UE_LOG(LogGrabComponent, Display, TEXT("Char standing on grab object"));
        return;
    }

    float InitialDistance;
    if (const UCameraComponent* OwnerCamera = ComponentOwner->FindComponentByClass<UCameraComponent>())
    {
        InitialDistance = FVector::Distance(OwnerCamera->GetComponentLocation(), PickupComponent->GetComponentLocation());
    }
    else
    {
        InitialDistance = FVector::Distance(ComponentOwner->GetActorLocation(), PickupComponent->GetComponentLocation());
    }
    
    if (InitialDistance > DefaultGrabDistance)
    {
        if (bEnableDebug)
        {
            UE_LOG(LogGrabComponent, Display, TEXT("Grab object is failed cause of distance : %f"), InitialDistance);
        }
        Server_StopGrabObject();
        return;
    }
    
    SetupSocketComponent(ComponentOwner, InitialDistance);

    if (!SocketComponent)
    {
        if (bEnableDebug)
            UE_LOG(LogGrabComponent, Display, TEXT("Grab object is failed cause of pointer : %s"),
                        *GetNameSafe(SocketComponent));
        Server_StopGrabObject();
        return;
    }
    
    Server_SetGrabDistance(InitialDistance);
    FVector ClosestGrabLocation;
    PickupComponent->GetClosestPointOnCollision(SocketComponent->GetComponentLocation(),
                                             ClosestGrabLocation);
    
    GrabComponentAtLocationWithRotation(PickupComponent, NAME_None, 
                                        ClosestGrabLocation, PickupComponent->GetComponentRotation());
    
    CurrentGrabLocation = ClosestGrabLocation;

    CurrentMassRatio = FMath::Clamp(PickupComponent->GetMass() / MaxGrabHeightMass, 0.0f, 1.0f);
    CalculatePhysSettings(MaxLinearStiffness,MaxLinearDamping,
                            CurrentMassRatio, true, false);
    SetPhysSettings(true);
    
    if (PhysMaterial && GEngine && GrabbedComponent->GetBodyInstance()->GetSimplePhysicalMaterial() == GEngine->DefaultPhysMaterial)
        PickupComponent->SetPhysMaterialOverride(PhysMaterial);

    SetComponentTickEnabled(true);
}
/**********************************************************************************************************************/

/**************************************************MainLogic***********************************************************/
void UGrabComponent::HoldPhysicsObject_Implementation()
{
    if (!SocketComponent || !GrabbedComponent)
        return;

    float ToSocketDistance;
    if (const UCameraComponent* OwnerCamera = GetOwner()->FindComponentByClass<UCameraComponent>())
    {
        ToSocketDistance = FVector::Distance(OwnerCamera->GetComponentLocation(), GrabbedComponent->GetComponentLocation());
    }
    else
    {
        ToSocketDistance = FVector::Distance(GetOwner()->GetActorLocation(), GrabbedComponent->GetComponentLocation());
    }

    if (ToSocketDistance > DefaultGrabDistance)
    {
        Server_StopGrabObject();
        return;
    }
    
    if (PhysMaterial && GrabbedComponent->GetBodyInstance()->GetSimplePhysicalMaterial() == PhysMaterial)
    {
        PhysMaterial->Friction = FMath::Lerp(MinFriction, MaxFriction,CurrentMassRatio);
        FChaosEngineInterface::UpdateMaterial(*PhysMaterial->MaterialHandle, PhysMaterial);
        GrabbedComponent->GetBodyInstance()->UpdatePhysicalMaterials();
        GrabbedComponent->GetBodyInstance()->UpdateMassProperties();
    }
    
    SetPhysSettings(false);
    
    SetTargetLocationAndRotation(TargetGrabLocation, TargetGrabRotation);
}
/**********************************************************************************************************************/

/***************************************************Setter*************************************************************/
void UGrabComponent::SetDefaultPhysMaterialSettings()
{
    if (!PhysMaterial) return;
    
    PhysMaterial->Friction = DefaultPhysMaterialSettings.Friction;
    PhysMaterial->StaticFriction = DefaultPhysMaterialSettings.StaticFriction;
    PhysMaterial->FrictionCombineMode = DefaultPhysMaterialSettings.FrictionCombineMode;
    PhysMaterial->bOverrideFrictionCombineMode = DefaultPhysMaterialSettings.OverrideFrictionCombineMode;
    PhysMaterial->Restitution = DefaultPhysMaterialSettings.Restitution;
    PhysMaterial->RestitutionCombineMode = DefaultPhysMaterialSettings.RestitutionCombineMode;
    PhysMaterial->bOverrideRestitutionCombineMode = DefaultPhysMaterialSettings.OverrideRestitutionCombineMode;
    PhysMaterial->Density = DefaultPhysMaterialSettings.Density;
    PhysMaterial->SleepLinearVelocityThreshold = DefaultPhysMaterialSettings.SleepLinearVelocityThreshold;
    PhysMaterial->SleepAngularVelocityThreshold = DefaultPhysMaterialSettings.SleepAngularVelocityThreshold;
    PhysMaterial->SleepCounterThreshold = DefaultPhysMaterialSettings.SleepCounterThreshold;
    PhysMaterial->RaiseMassToPower = DefaultPhysMaterialSettings.RaiseMassToPower;
    DefaultPhysMaterialSettings.TensileStrength = PhysMaterial->Strength.TensileStrength;
    DefaultPhysMaterialSettings.CompressionStrength = PhysMaterial->Strength.CompressionStrength;
    PhysMaterial->Strength.ShearStrength = DefaultPhysMaterialSettings.ShearStrength;
    PhysMaterial->DamageModifier.DamageThresholdMultiplier = DefaultPhysMaterialSettings.DamageThresholdMultiplier;
}

void UGrabComponent::SetPhysSettings(const bool bImmediate)
{
    float CalculatedLinearStiffness = DefaultLinearStiffness;
    float CalculatedLinearDamping =  DefaultLinearDamping;
    
    CalculatePhysSettings(CalculatedLinearStiffness,
                          CalculatedLinearDamping,
                          CurrentMassRatio, true);
    
    float FinalLinearStiffness;
    float FinalLinearDamping;
    if (bImmediate)
    {
        FinalLinearStiffness = CalculatedLinearStiffness;
        FinalLinearDamping = CalculatedLinearDamping;
    }
    else
    {
        FinalLinearStiffness = FMath::FInterpTo(LinearStiffness, CalculatedLinearStiffness,
                                                        GetWorld()->GetDeltaSeconds(), PhysSettingsInterpSpeed);
        FinalLinearDamping =   FMath::FInterpTo(LinearDamping, CalculatedLinearDamping,
                                                        GetWorld()->GetDeltaSeconds(), PhysSettingsInterpSpeed);
    }

    const float MaxForce = GrabbedComponent->GetMass() * MaxLinearDriveForce;
    const float VelocityForce = FMath::Abs(FMath::Clamp(GrabVelocity.Length(), 1.0f, GrabVelocity.Length() * 0.01f));
    const float NewForce = FMath::Min(MaxForce * VelocityForce, MaxForce * 1.5f);

    if (bImmediate)
    {
        CurrentForce = NewForce;
    }
    else
    {
        CurrentForce = FMath::FInterpTo(CurrentForce, NewForce,
                        GetWorld()->GetDeltaSeconds(), InterpolationSpeed);
    }
    
    if (ConstraintHandle.IsValid() && ConstraintHandle.Constraint->IsType(Chaos::EConstraintType::JointConstraintType))
	{
		FPhysicsCommand::ExecuteWrite(ConstraintHandle, [&](const FPhysicsConstraintHandle& InConstraintHandle)
		{
			if (Chaos::FJointConstraint* Constraint = static_cast<Chaos::FJointConstraint*>(ConstraintHandle.Constraint))
			{
			    Constraint->SetLinearDriveForceMode(Chaos::EJointForceMode::Force);
			    Constraint->SetLinearSoftForceMode(Chaos::EJointForceMode::Force);
			    Constraint->SetLinearDriveMaxForce(Chaos::FVec3(CurrentForce));
			    Constraint->SetMassConditioningEnabled(true);

			    FVector OutLin;
			    FVector OutAng;
			    FPhysInterface_Chaos::GetForce(ConstraintHandle, OutLin, OutAng);
			    GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, FString::Printf(TEXT("Linear force : %s"), *OutLin.ToString()));
			    GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, FString::Printf(TEXT("Max Force : %f"), CurrentForce));
			}
		});
	}
    //GrabbedComponent->AddForce(GravityForceVector , NAME_None, true);
    SetLinearStiffness(FinalLinearStiffness);
    SetLinearDamping(FinalLinearDamping);
}

/**********************************************************************************************************************/

/*************************************************Getter***************************************************************/
bool UGrabComponent::GetIsCharacterOnGrabObject(AActor* Owner, const UPrimitiveComponent* PickupComponent) const
{
    if (!Owner || !PickupComponent)
        return false;
    
    const FVector TraceDownDist = FVector(0, 0, 1000.0f);
    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(Owner);
    bool bHit;
    if (const ACharacter* OwnerCharacter = Cast<ACharacter>(Owner))
    {
        const float CapsuleRadius = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();
        const float CapsuleHeight = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
        const FQuat CapsuleRot = OwnerCharacter->GetCapsuleComponent()->GetRelativeRotation().Quaternion();
        const FCollisionShape Shape = FCollisionShape::MakeCapsule(CapsuleRadius, CapsuleHeight);
        
        bHit = GetWorld()->SweepSingleByChannel(Hit,
                                              Owner->GetActorLocation(),
                                               Owner->GetActorLocation() - TraceDownDist,
                                                   CapsuleRot,ECC_Visibility,Shape, Params);
    }
    else
    {
        bHit = GetWorld()->LineTraceSingleByChannel(Hit,
                                                  Owner->GetActorLocation(),
                                                   Owner->GetActorLocation() - TraceDownDist,
                                                   ECC_Visibility, Params);
    }

    if (bHit)
    {
        if (Hit.GetComponent() && Hit.GetComponent() == PickupComponent)
            return true;
    }

    return false;
}
/**********************************************************************************************************************/

/***************************************************Calculations*******************************************************/
void UGrabComponent::CalculateCurrentGrabVelocity_Implementation()
{
    if (!SocketComponent || !GrabbedComponent) return;

    TargetGrabLocation = SocketComponent->GetComponentLocation();
    
    GrabVelocity = (TargetGrabLocation - CurrentGrabLocation) / GetWorld()->GetDeltaSeconds();
    GrabDistance = FVector::Distance(TargetGrabLocation, CurrentGrabLocation);
    CurrentGrabLocation = SocketComponent->GetComponentLocation();
}

void UGrabComponent::CalculateCurrentGrabRotation_Implementation()
{
    if (!GrabbedComponent || !SocketComponent) return;
    
    TargetGrabRotation = GrabbedComponent->GetComponentRotation();
}

void UGrabComponent::CalculatePhysSettings(
    float& CalculatedLinearStiffness,
    float& CalculatedLinearDamping,
    const float& MassRatio,
    const bool bInvertRatio,
    const bool bUseMaxValues) const
{
    const float Ratio = bInvertRatio ? 1.0f - MassRatio : MassRatio;
    if (LinearStiffnessCurve)
    {
        CalculatedLinearStiffness = DefaultLinearStiffness * LinearStiffnessCurve->GetFloatValue(1.0f - Ratio);
    }
    else
    {
        CalculatedLinearStiffness = DefaultLinearStiffness * (Ratio);
    }

    if (LinearDampingCurve)
    {
        CalculatedLinearDamping = DefaultLinearDamping * LinearDampingCurve->GetFloatValue(1.0f - Ratio);
    }
    else
    {
        CalculatedLinearDamping = DefaultLinearDamping * (Ratio);
    }

    if (bUseMaxValues)
    {
        CalculatedLinearStiffness = FMath::Clamp(CalculatedLinearStiffness, 0.0f, MaxLinearStiffness);
        CalculatedLinearDamping = FMath::Clamp(CalculatedLinearDamping, 0.0f, MaxLinearDamping);
    }
}
/**********************************************************************************************************************/

/*************************************************Debug****************************************************************/
#if !UE_BUILD_SHIPPING
void UGrabComponent::DrawDebugInfo() const
{
    if (bEnableDebug)
    {
        FString FrictionDebug = "";
        if (PhysMaterial)
            FrictionDebug = FString::Printf(TEXT("Friction : %f"), PhysMaterial->Friction);
        
        FString DebugString = FString::Printf(TEXT("Current location : %s\n"
                                                   "Current velocity : %f\n"
                                                   "Current resistance velocity : %f\n"
                                                   "Current mass ration : %f\n"
                                                   "Current gravity force : %f\n"
                                                   "Current interp speed : %f\n"
                                                   "Current linear stiffness : %f\n"
                                                   "Current linear damping : %f\n"
                                                   "Max linear stiffness : %f\n"
                                                   "Max linear damping : %f\n"),
                                                   *TargetGrabLocation.ToString(), GrabVelocity.Length(),
                                                   CurrentResistanceVelocity.Length(), CurrentMassRatio, CurrentGravityZ.Length(),
                                                   InterpolationSpeed, LinearStiffness, LinearDamping,
                                                   MaxLinearStiffness, MaxLinearDamping);
        DebugString += FrictionDebug;
        
        DrawDebugDirectionalArrow(GetWorld(), TargetGrabLocation,
                            TargetGrabLocation + CurrentResistanceVelocity,
                            10.0f, FColor::Red, false, 0.0f, 1.0f);
        DrawDebugDirectionalArrow(GetWorld(), TargetGrabLocation,
                            TargetGrabLocation + GrabVelocity,
                            10.0f, FColor::Green, false, 0.0f, 1.0f);

        DrawDebugSphere(GetWorld(), TargetGrabLocation,
                  5.0f, 12, FColor::Green, false, 0.0f, 1.0f);

        if (GrabbedComponent)
        {
            DrawDebugString(GetWorld(), GrabbedComponent->GetComponentLocation() + FVector(0, 0, 60.0f),
                            DebugString, nullptr, FColor::Red, 0.0f);
        
            if (GrabDistance > 1.0f)
            {
                DrawDebugLine(GetWorld(), TargetGrabLocation, GrabbedComponent->GetComponentLocation(),
                              FColor::Purple, false, 0.0f, 1.0f, 1.0f);
            }

            DrawDebugSphere(GetWorld(), GrabbedComponent->GetComponentLocation() + ConstraintLocalPosition,
                      3.0f, 12, FColor::Purple, false, 0.0f, 1.0f);
        }
    }
}

void UGrabComponent::BindCvars()
{
    const FConsoleVariableDelegate CheckCvarValues = FConsoleVariableDelegate::CreateUObject(this, &UGrabComponent::FetchCVars);

    CVarEnableGrabDebug->Set(bEnableDebug);
    CVarEnableGrabDebug->SetOnChangedCallback(CheckCvarValues);
}

void UGrabComponent::FetchCVars(IConsoleVariable* Var)
{
    bEnableDebug = static_cast<bool>(CVarEnableGrabDebug.GetValueOnGameThread());
}
#endif
/**********************************************************************************************************************/





