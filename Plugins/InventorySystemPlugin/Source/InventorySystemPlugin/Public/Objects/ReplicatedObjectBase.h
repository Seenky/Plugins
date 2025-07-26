// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ReplicatedObjectBase.generated.h"


UCLASS(Blueprintable)
class INVENTORYSYSTEMPLUGIN_API UReplicatedObjectBase : public UObject
{
	GENERATED_BODY()

public:
	UReplicatedObjectBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) { Owner = GetTypedOuter<AActor>(); }
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool IsSupportedForNetworking() const override { return true; }
	virtual void PostInitProperties() override { UObject::PostInitProperties(); if (GetWorld()) ObjectBeginPlay(); }
	virtual void ObjectBeginPlay() { Owner = GetTypedOuter<AActor>(); }

	virtual int32 GetFunctionCallspace(UFunction* Function, FFrame* Stack) override;
	virtual bool CallRemoteFunction(UFunction* Function, void* Parms, struct FOutParmRec* OutParms, FFrame* Stack) override;

	/*Get Object Owner*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Replicated Object")
	virtual AActor* GetOwner() const { return Owner; }
	/*Set Object Owner*/
	UFUNCTION(Server, Unreliable, BlueprintCallable, Category="Replicated Object")
	virtual void Server_SetOwner(AActor* NewOwner);
	void Server_SetOwner_Implementation(AActor* NewOwner) { if (NewOwner) Owner = NewOwner; }

private:
	UPROPERTY(Replicated)
	AActor* Owner;
};
