//Florist Game. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, Blueprintable)
class UInteractInterface : public UInterface
{
	GENERATED_BODY()
};
class INTERACTSYSTEM_API IInteractInterface
{
	GENERATED_BODY()

public:

    // Call always on hit actor to check can youi interact with it now
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Interact Interface Logic")
        bool CanInteract(FHitResult HitResult, AActor* QueryFromActor);
    /*
     * Cancel if new interact object is different as previous
     * Cancel from interact if no hit
     * Cancel from interact if cant interact by statuses info or if interact check was stopped
     */
    // @DEPRECATED
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Interact Interface Logic")
        void CancelCanInteract(FHitResult HitResult);
    // @DEPRECATED
    
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Interact Interface Logic")
        bool StartInteract(FHitResult HitResult, AActor* InteractByActor);
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Interact Interface Logic")
        void StopInteract(FHitResult HitResult);
    // Call if start interact is failed (can interact return false)
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Interact Interface Logic")
        void FailedTryInteract(FHitResult HitResult);
    // Call always when you hit actor
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Interact Interface Logic")
        void TraceHit(FHitResult HitResult, AActor* QueryFromActor);

    // Call on find or lost interact target (or if can interact equal false/true)
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Interact Interface Logic")
        void OnInteractTargetFocusChanged(bool bIsFindInteractTarget, FHitResult HitResult, AActor* QueryFromActor);
    
};