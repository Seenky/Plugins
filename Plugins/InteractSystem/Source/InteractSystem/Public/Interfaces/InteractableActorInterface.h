// Florist Game. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "InteractInfo.h"
#include "UObject/Interface.h"
#include "InteractableActorInterface.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UInteractableActorInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Need add in Owner of InteractableActorComponent
 */
class INTERACTSYSTEM_API IInteractableActorInterface
{
	GENERATED_BODY()

public:
	/*
	 * Cancel if new interact object is different as previous
	 * Cancel from interact if no hit
	 * Cancel from interact if cant interact by statuses info or if interact check was stopped
	 */
	UFUNCTION(BlueprintCallable,BlueprintNativeEvent, Category="Interact Interface Logic")
		bool CanInteract(const FInteractInputInfo& InteractInputInfo, FHitResult HitResult, AActor* QueryFromActor);
	
};
