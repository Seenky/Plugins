// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTags.h"
#include "UObject/Interface.h"
#include "PhysInteractInterface.generated.h"

UINTERFACE(Blueprintable)
class PHYSICSINTERACTIONPLUGIN_API UPhysInteractInterface : public UInterface
{
	GENERATED_BODY()
};

class PHYSICSINTERACTIONPLUGIN_API IPhysInteractInterface
{
	GENERATED_BODY()

public:
	/*Return true if owner hands are busy*/
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Physics|Getter")
		bool IsOwnerHandsBusy();
};
