// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Actors/HumansHider.h"
#include "Managers/NiagaraManager.h"
#include "Subsystems/WorldSubsystem.h"
#include "NiagaraCrowdSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNiagaraManagerPrepared, ANiagaraManager*, NiagaraManager);

UCLASS()
class MASSAIWITHTRAFFIC_API UNiagaraCrowdSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UNiagaraCrowdSubsystem();
	virtual ~UNiagaraCrowdSubsystem();
	virtual void Deinitialize() override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	UFUNCTION(BlueprintPure, Category = "Niagara")
		ANiagaraManager* GetNiagaraManager() const { return NiagaraManager; }

	UPROPERTY(BlueprintAssignable, Category = "Niagara Manager")
		FOnNiagaraManagerPrepared OnNiagaraManagerPrepared;

	TArray<AHumansHider*> GetAllHiders() const { return HumansHiders; }

private:
	TArray<TWeakObjectPtr<UActorComponent>> SwapperComponents;
	TArray<TWeakObjectPtr<APawn>> OwnerPawns;
	TWeakObjectPtr<APlayerController> PC;
	float DistanceToSwap;
	bool bUseNiagaraParticles;
	FSoftObjectPath PathToAsset;
	TSoftObjectPtr<UNiagaraSystem> NiagaraSystem;
	UPROPERTY()
	ANiagaraManager* NiagaraManager;
	TArray<AHumansHider*> HumansHiders;
};

