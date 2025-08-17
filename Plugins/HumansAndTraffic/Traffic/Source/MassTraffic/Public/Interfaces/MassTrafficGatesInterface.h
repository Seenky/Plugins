// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Data/MassTrafficGateStates.h"
#include "MassTrafficGatesInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, Blueprintable)
class UMassTrafficGatesInterface : public UInterface
{
	GENERATED_BODY()
};

class MASSTRAFFIC_API IMassTrafficGatesInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void OpenGates(bool bOpen);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void AddEntityToQueueList (const int32& InEntity);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void RemoveEntityFromQueueList (const int32& InEntity);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void AddEntityUniqueIDToList(const int64& InID);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void RemoveEntityUniqueIDFromList(const int64& InID);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void ClearEntityUniqueIDList();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	TArray<int64> GetEntityUniqueIDList();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	TArray<int32> GetEntityQueueList();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void ClearEntityQueueList();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	bool GetVehiclePassedGate(const FVector& InLocation);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void SetNearestEntity(const int64 InEntity);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	int64 GetNearestEntity();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	EMassTrafficGateStates GetGateState();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void SendPendingToClose();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	bool GetPendingToClose();
};
