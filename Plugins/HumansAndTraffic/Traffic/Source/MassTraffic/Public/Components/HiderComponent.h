// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HiderComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHide);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MASSTRAFFIC_API UHiderComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHiderComponent();
	
	UPROPERTY(BlueprintAssignable, Category = "HiderComponent")
		FOnHide OnHide;

	UFUNCTION(BlueprintCallable, Category = "HiderComponent")
		void SetHidden(const bool bHidden) { OnHide.Broadcast(); bIsHidden = bHidden; }
	UFUNCTION(BlueprintPure, Category = "HiderComponent")
		bool GetIsHidden() const { return bIsHidden; }

private:
	bool bIsHidden;
};
