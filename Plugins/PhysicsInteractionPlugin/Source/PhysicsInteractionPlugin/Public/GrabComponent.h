// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GrabData.h"
#include "Chaos/ChaosEngineInterface.h"
#include "Components/ActorComponent.h"
#include "Components/ArrowComponent.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"
#include "GrabComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogGrabComponent, Log, All);

extern TAutoConsoleVariable<int32> CVarEnableGrabDebug;

UCLASS(ClassGroup=(Components), meta=(BlueprintSpawnableComponent), DontCollapseCategories)
class PHYSICSINTERACTIONPLUGIN_API UGrabComponent : public UPhysicsHandleComponent
{
	GENERATED_BODY()

public:
	UGrabComponent();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/**Try grab primitive component*/
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "PickupComponent")
		void Server_TryGrabObject(UPrimitiveComponent* PickupComponent);
		bool Server_TryGrabObject_Validate(UPrimitiveComponent* PickupComponent) { return true; }
	/**Drop primitive component from hand if it was*/
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "PickupComponent")
		void Server_StopGrabObject();
		bool Server_StopGrabObject_Validate() { return true; }

protected:
	/**Default grab distance (this is max distance to grab object)*/
	UPROPERTY(EditDefaultsOnly, Category = "Settings|Grab Settings", meta = (ClampMin = "1.0"))
		float DefaultGrabDistance;
	/**Max object mass we can grab*/
	UPROPERTY(EditDefaultsOnly, Category = "Settings|Grab Settings", meta = (ClampMin = "1.0"))
		float MaxGrabHeightMass;
	/**Interp speed of changing phys settings*/
	UPROPERTY(EditDefaultsOnly, Category = "Settings|Grab Settings", meta = (ClampMin = "0.1"))
		float PhysSettingsInterpSpeed;
	/**Mass multiplier of grabbed component, to stabilize mass ratio*/
	UPROPERTY(EditDefaultsOnly, Category = "Settings|Grab Settings", meta = (ClampMin = "1.0"))
		float GrabbedComponentMassScale;
	UPROPERTY(EditDefaultsOnly, Category = "Settings|Grab Settings", meta = (ClampMin = "1.0"))
		float MaxLinearDriveForce;
	/**Curve for linear stiffness change, must be from 1 to 0*/
	UPROPERTY(EditDefaultsOnly, Category = "Settings|Grab Settings|Curve")
		UCurveFloat* LinearStiffnessCurve;
	/**Curve for linear damping change, must be from 1 to 0*/
	UPROPERTY(EditDefaultsOnly, Category = "Settings|Grab Settings|Curve")
		UCurveFloat* LinearDampingCurve;
	/**Phys material applied to grabbed object*/
	UPROPERTY(EditDefaultsOnly, Category = "Settings|Material")
		UPhysicalMaterial* PhysMaterial;
	UPROPERTY(EditDefaultsOnly, Category = "Settings|Material")
		float MaxFriction;
	UPROPERTY(EditDefaultsOnly, Category = "Settings|Material")
		float MinFriction;
	/**Enable debug for this component (also can be enabled with cvar Grab.EnableDebug 0/1)*/
	UPROPERTY(EditDefaultsOnly, Category = "Settings|DebugSettings")
		bool bEnableDebug;

	/**Set current grab distance*/
	UFUNCTION(Server, Unreliable, WithValidation, BlueprintCallable, Category="Grab Component")
		void Server_SetGrabDistance(const float NewGrabDistance);
		bool Server_SetGrabDistance_Validate(const float NewGrabDistance) { return true; }
		void Server_SetGrabDistance_Implementation(const float NewGrabDistance) { CurrentGrabDistance = NewGrabDistance; }
	/**Return current grab distance*/
	UFUNCTION(BlueprintPure, Category="Grab Component")
		float GetGrabDistance() const { return CurrentGrabDistance; }
	UFUNCTION(BlueprintPure, Category="Grab Component")
		bool GetIsHoldingItem() const { return IsValid(GrabbedComponent); }
	
private:
	/**Promote all calculations while hold phys object (works on tick)*/
	UFUNCTION(Server, Unreliable)
	void HoldPhysicsObject();
	
	/**Return current grab velocity*/
	FVector GetCurrentGrabVelocity() const { return GrabVelocity; }
	/**Return if owner standing on grab object*/
	bool GetIsCharacterOnGrabObject(AActor* Owner, const UPrimitiveComponent* PickupComponent) const;
	
	/**Set default material settings from phys material instance*/
	void SetDefaultPhysMaterialSettings();
	/**Set interp speed, linear stiffness and linear damping from mass ratio and overlaps*/
	void SetPhysSettings(const bool bImmediate = false);
	
	/**Calculate current grab velocity and target location (works on tick)*/
	UFUNCTION(Server, Unreliable)
	void CalculateCurrentGrabVelocity();
	/**Calculate current grab rotation (works on tick)*/
	UFUNCTION(Server, Unreliable)
	void CalculateCurrentGrabRotation();
	/**Calculate interp speed, linear stiffness and linear damping from curves or mass ratio*/
	void CalculatePhysSettings(
		float& CalculatedLinearStiffness,
		float& CalculatedLinearDamping,
		const float& MassRatio,
		const bool bInvertRatio = true,
		const bool bUseMaxValues = true) const;
	
	/**Setup default material parameters from phys material*/
	void SetupPhysMaterialSettings();
	/**Setup socket component with default settings*/
	bool SetupSocketComponent(AActor* Owner, const float& Distance);

#if !UE_BUILD_SHIPPING
	/**Draw debug sphere and lines for velocity and parameters*/
	void DrawDebugInfo() const;
	/**Change parameters from cvar*/
	void BindCvars();
	void FetchCVars(IConsoleVariable* Var);
#endif
	
	UPROPERTY(Replicated)
		UArrowComponent* SocketComponent;
	UPROPERTY(Replicated)
		float CurrentGrabDistance;
	UPROPERTY(Replicated)
		FVector CurrentGrabLocation;
	UPROPERTY(Replicated)
		FVector TargetGrabLocation;
	UPROPERTY(Replicated)
		float MaxLinearStiffness;
	UPROPERTY(Replicated)
		float MaxLinearDamping;
	UPROPERTY(Replicated)
		FVector GrabVelocity;
	UPROPERTY(Replicated)
		float GrabDistance;
	UPROPERTY(Replicated)
		FRotator TargetGrabRotation;
	UPROPERTY(Replicated)
		bool bLastHit;
	UPROPERTY(Replicated)
		FVector CurrentResistanceVelocity;
	UPROPERTY(Replicated)
		FVector CurrentGravityZ;
	UPROPERTY(Replicated)
		float CurrentMassRatio;
	UPROPERTY(Replicated)
		float CurrentForce;

	float DefaultInterpolationSpeed;
	float DefaultLinearStiffness;
	float DefaultLinearDamping;
	FDefaultPhysMaterialSettings DefaultPhysMaterialSettings;
};



