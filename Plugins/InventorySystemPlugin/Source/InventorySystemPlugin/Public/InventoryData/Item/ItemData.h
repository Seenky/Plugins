// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ItemData.generated.h"


USTRUCT(BlueprintType)
struct FItemData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool bConsumable;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		int32 MaxCount;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		int32 CurrentCount;

	FItemData()
		:bConsumable(false)
		,MaxCount(1)
		,CurrentCount(1)
	{}
};
