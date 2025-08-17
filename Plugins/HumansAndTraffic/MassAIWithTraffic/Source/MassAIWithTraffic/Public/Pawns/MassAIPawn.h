// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameFramework/Character.h"
#include "Interface/PawnInteractInterface.h"
#include "Managers/NiagaraManager.h"
#include "MassAIPawn.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChangePawnVisibility, const bool, bVisible);

UCLASS()
class MASSAIWITHTRAFFIC_API AMassAIPawn : public ACharacter, public IPawnInteractInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AMassAIPawn(const class FObjectInitializer& ObjectInitializer);

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void Destroyed() override;
	virtual void Tick(float DeltaTime) override;

	//Interface
	virtual void SetPawnVisibility_Implementation(const bool bVisible, const float& NewLerpSpeed) override;
	virtual void UpdatePerformance_Implementation(EMassLOD::Type LOD) override;
	virtual bool GetIsVisible_Implementation() const override { return GetMesh()->IsVisible(); }
	virtual bool GetIsLerpingNow_Implementation() const override { return bShouldLerpOpacity; }

	//Delegates
	UPROPERTY(BlueprintAssignable, Category = "Delegates")
		FOnChangePawnVisibility OnChangePawnVisibility;


protected:

	UFUNCTION(BlueprintImplementableEvent, Category = "Niagara Manager")
		void NiagaraManagerPrepared(ANiagaraManager* Manager);

	UFUNCTION(BlueprintPure, Category = "Managers")
		FORCEINLINE ANiagaraManager* GetNiagaraManager() const { return NiagaraManager.Get(); }
	
	UFUNCTION(BlueprintCallable, Category = "Human Prepare")
		void UpdateNiagaraPawn(const  FPawnMeshData& PawnData);
	
	//Parameter names
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings|Opacity Parameters")
		FName OpacityAlphaParameterName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings|Opacity Parameters")
		FName UseOpacityAlphaParameterName;

private:
	void CheckMeshVisibility(const float& Time);
	void DestroyMeshes();
	void SetPawnNiagaraParameters();
	void UpdateClothSettings(EMassLOD::Type LOD) const;
	void StartOpacityLerp(const bool bVisible);
	void UpdateAnimation(const float& DeltaTime) const;

private:
	TWeakObjectPtr<ANiagaraManager> NiagaraManager;

	//Opacity vars
	bool bIsVisibleNow;
	bool bShouldLerpOpacity;
	float CurrentOpacity;
	float TargetOpacity;

	//Niagara crowd settings parameters
	bool bUseNiagaraParticlesForCrowd;
	float LerpSpeed;
	bool bUseParticlesOnly;
};
