// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "Data/NiagaraCrowdData.h"
#include "NiagaraManager.generated.h"

USTRUCT(BlueprintType)
struct MASSAIWITHTRAFFIC_API FMassCrowdInstanceCustomData
{
	GENERATED_BODY()

	FMassCrowdInstanceCustomData() = default;
	
	FMassCrowdInstanceCustomData(const struct FMassCrowdPackedInstanceCustomData& PackedCustomData);

	static FMassCrowdInstanceCustomData MakeCrowdCustomData(const FLinearColor Color, const FLinearColor Color2, const FLinearColor Color3);

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FLinearColor Color = FLinearColor::Transparent;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FLinearColor Color2 = FLinearColor::Transparent;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FLinearColor Color3 = FLinearColor::Transparent;
};

/**
 * FMassTrafficVehicleInstanceCustomData packed into a single 32 bit float to be passed as ISMC PerInstanceCustomData
 * which is currently limited to a single float for Nanite ISMCs. We also pass this to PrimitiveComponent's via 
 * UPrimitiveComponent::SetCustomPrimitiveDataFloat
 *
 * @see FMassTrafficVehicleInstanceCustomData
 */
USTRUCT(BlueprintType)
struct MASSAIWITHTRAFFIC_API FMassCrowdPackedInstanceCustomData
{
	GENERATED_BODY()

	FMassCrowdPackedInstanceCustomData() {};
	
	explicit FMassCrowdPackedInstanceCustomData(const float InPackedParam1, const float InPackedParam2, const float InPackedParam3)
		: PackedParamColorRGB(InPackedParam1)
		, PackedParam2ColorRGB(InPackedParam2)
		, PackedParam3ColorRGB(InPackedParam3)
	{}
	
	FMassCrowdPackedInstanceCustomData(const FMassCrowdInstanceCustomData& UnpackedCustomData);

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	float PackedParamColorRGB = 0.0f;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	float PackedParam2ColorRGB = 0.0f;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	float PackedParam3ColorRGB = 0.0f;
};

UCLASS()
class MASSAIWITHTRAFFIC_API ANiagaraManager : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ANiagaraManager();
	
	UFUNCTION(BlueprintPure, Category = "NiagaraManager")
		bool GetIsSystemActivated() const { return bSystemActivated; }
	
	UFUNCTION(BlueprintPure, Category = "NiagaraManager")
		FORCEINLINE UNiagaraComponent* GetNiagaraComponent() { return NiagaraComponent; }

	//Setter for owner of particle
	UFUNCTION(BlueprintCallable, Category = "NiagaraManager")
		void SetPawnInstance(const FPawnMeshData& MeshData);

	//Setters for niagara variables
	UFUNCTION(BlueprintCallable, Category = "NiagaraManager")
	void SetNiagaraArrayColor(FLinearColor Color)
	{
		ColorsArray.Add(Color);
		UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayColor(NiagaraComponent, ColorsArrayName, ColorsArray);
	}
	
	UFUNCTION(BlueprintCallable, Category = "NiagaraManager")
	void SetNiagaraArrayAnimOffset(const float& Offset)
	{
		AnimOffsetArray.Add(Offset);
		UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayFloat(NiagaraComponent, OffsetArrayName, AnimOffsetArray);
	}

	//System setter
	void SetNiagaraSystem(UNiagaraSystem* System);

	void SetTranslatedLocation(const TArray<FVector>& InLocations) { TranslatedLocations = InLocations; }
	void SetTranslatedForwardVectors(const TArray<FVector>& InForwardVectors) { TranslatedForwardVectors = InForwardVectors; }
	void SetTranslatedVelocity(const TArray<FVector>& InVelocities) { TranslatedVelocity = InVelocities; }
	void SetTranslatedVisibility(const TArray<int32>& InVisibility) { TranslatedVisibility = InVisibility; }
	void SetTranslatedLerpSpeed(const TArray<float>& NewLerpSpeedArray) { TranslatedLerpSpeedArray = NewLerpSpeedArray;}

protected:
	
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	
	//Components
	UPROPERTY(EditAnywhere,	BlueprintReadWrite, Category = "Components")
		TObjectPtr<UNiagaraComponent> NiagaraComponent;
	UPROPERTY(EditAnywhere,	BlueprintReadWrite, Category = "Assets", meta = (ExposeOnSpawn = "true"))
		TObjectPtr<UNiagaraSystem> NiagaraSystem;

	//Debug settings
	UPROPERTY(EditAnywhere,	BlueprintReadWrite, Category = "Debug")
		bool bEnableDebug;

private:
	//Main logic for update particles
	void UpdateParticlesInfoTranslated() const;

	//Setup and update logic
	void ActivateSystem();
	void SetupNiagaraMesh();
	void UpdateSystem();
	
	TMap<int32, FPawnMeshData> PawnList;

	//Niagara variables
	TArray<FLinearColor> ColorsArray;
	TArray<float> AnimOffsetArray;
	float LerpSpeed;
	int32 MeshesCount;

	//Setup dependencies
	int32 IDCount;
	FTimerHandle UpdateSystemHandle;

	//ParameterNames
	FName LocationArrayName;
	FName ForwardVectorArrayName;
	FName VisibilityArrayName;
	FName VelocityArrayName;
	FName MeshIndexArrayName;
	FName ColorsArrayName;
	FName OffsetArrayName;
	FName LerpSpeedName;

	TArray<FVector> TranslatedLocations;
	TArray<FVector> TranslatedForwardVectors;
	TArray<FVector> TranslatedVelocity;
	TArray<int32> TranslatedVisibility;
	TArray<float> TranslatedLerpSpeedArray;

	bool bSystemActivated;
};
