// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Data/MassTrafficGateStates.h"
#include "GameFramework/Actor.h"
#include "Interfaces/MassTrafficGatesInterface.h"
#include "MassTrafficGateBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOpenGate, bool, bOpen);

UCLASS()
class MASSTRAFFIC_API AMassTrafficGateBase : public AActor, public IMassTrafficGatesInterface
{
	GENERATED_BODY()

public:
	AMassTrafficGateBase();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	//Interface methods to interact from processor
	virtual void OpenGates_Implementation(bool bOpen) override;
	virtual void AddEntityToQueueList_Implementation(const int32& InEntity) override { EntityQueueList.AddUnique(InEntity); }
	virtual void RemoveEntityFromQueueList_Implementation(const int32& InEntity) override { EntityQueueList.Remove(InEntity); }
	virtual TArray<int32> GetEntityQueueList_Implementation() override { return EntityQueueList; }
	virtual void ClearEntityQueueList_Implementation() override { EntityQueueList.Empty(); }
	virtual void SetNearestEntity_Implementation(const int64 InEntity) override { NearestEntity = InEntity; }
	virtual int64 GetNearestEntity_Implementation() override { return NearestEntity; }
	virtual EMassTrafficGateStates GetGateState_Implementation() override { return CurrentGateState; }
	//We should reset pending to close manually after moving logic done otherwise it wont work
	virtual void SendPendingToClose_Implementation() override { bPendingToClose = true; }
	virtual bool GetPendingToClose_Implementation() override { return bPendingToClose; }
	virtual void AddEntityUniqueIDToList_Implementation(const int64& InID) override { EntityUniqueIDList.AddUnique(InID); }
	virtual void RemoveEntityUniqueIDFromList_Implementation(const int64& InID) override { EntityUniqueIDList.Remove(InID); }
	virtual void ClearEntityUniqueIDList_Implementation() override { EntityUniqueIDList.Empty(); }
	virtual TArray<int64> GetEntityUniqueIDList_Implementation() override { return EntityUniqueIDList; }

	UPROPERTY(BlueprintAssignable, Category="MassTrafficGates")
	FOnOpenGate OnOpenGate;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MassTrafficGateState")
	EMassTrafficGateStates CurrentGateState = EMassTrafficGateStates::Closed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MassTrafficGateState")
	bool bPendingToClose = false;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	UBoxComponent* GateLaneLocationBoxComponent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	UArrowComponent* GateDirectionArrowComponent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	USceneComponent* SceneComponent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	UStaticMeshComponent* HandStaticMeshComponent;

private:
	TArray<int32> EntityQueueList;
	TArray<int64> EntityUniqueIDList;
	int64 NearestEntity = INDEX_NONE;
};
