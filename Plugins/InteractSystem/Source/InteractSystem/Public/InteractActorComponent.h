// Florist Game. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/InteractWidgetInfoInterface.h"
#include "StatusesInfo.h"
#include "Components/ActorComponent.h"
#include "Interfaces/InteractInterface.h"
#include "InteractActorComponent.generated.h"

class UInteractWidgetManager;

UCLASS(Abstract, Blueprintable, ClassGroup=(InteractSystem), meta=(BlueprintSpawnableComponent), Abstract)
class INTERACTSYSTEM_API UInteractActorComponent : public UActorComponent, public IInteractInterface, public IInteractWidgetInfoInterface
{
	GENERATED_BODY()

public:	
	UInteractActorComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	// Functions
	// Getter
	
	UFUNCTION(BlueprintCallable, Category="Interact Info")
		const bool GetIsAlreadyUse() const { return bIsAlreadyUse; }

	FORCEINLINE bool GetIsShowDebug() const { return bIsShowDebug; }
	
	// Setter
	
	UFUNCTION(BlueprintCallable, Category="Interact Info")
		void SetIsAlreadyUse(const bool bNewIsAlreadyUse) { bIsAlreadyUse = bNewIsAlreadyUse; }
	UFUNCTION(BlueprintCallable, Category="Interact")
		void SetCanInteractPrimitiveComps(TArray<UPrimitiveComponent*>& CanInteractComps) { CanInteractPrimitiveComps = CanInteractComps; }
	
	// Interact Interface
	
	virtual bool CanInteract_Implementation(FHitResult HitResult, AActor* QueryFromActor) override final;
	virtual void CancelCanInteract_Implementation(FHitResult HitResult) override final;
	virtual bool StartInteract_Implementation(FHitResult HitResult, AActor* InteractByActor) override final;
	virtual void FailedTryInteract_Implementation(FHitResult HitResult) override final;
	virtual void StopInteract_Implementation(FHitResult HitResult) override final;
	virtual void TraceHit_Implementation(FHitResult HitResult, AActor* QueryFromActor) override final;
	virtual void GetInteractWidgetLocation_Implementation(FVector& WidgetLocation) const override final;
	virtual void GetInteractWidget_Implementation(TSubclassOf<UUserWidget>& InteractWidgetClass, UObject*& OwnerInteractObject) override final;
	virtual const bool GetIsShowInteractWidget_Implementation() const override final;
	
protected:
	
	// Interact Interface Support Functions
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Interact")
		bool StartInteractLogic(FHitResult HitResult, AActor* InteractByActor);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Interact")
		bool CanInteractLogic(FHitResult HitResult, AActor* QueryFromActor);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Interact")
		void CancelCanInteractLogic(FHitResult HitResult);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Interact")
		void StopInteractLogic(FHitResult HitResult);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Interact")
		void FailedTryInteractLogic(FHitResult HitResult);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Interact")
		void TraceHitLogic(FHitResult HitResult, AActor* QueryFromActor);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Interact")
		void GetInteractWidgetLocationLogic(FVector& WidgetLocation) const;
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Interact")
		void GetInteractWidgetLogic(TSubclassOf<UUserWidget>& InteractWidgetClass, UObject*& OwnerInteractObject);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Interact")
		const bool GetIsShowInteractWidgetLogic() const; 
	
	// Show Debug

	virtual void ShowDebug();
	
private:

	// Variables
	
	// If InverseCondition you can set Cant Interact Statuses
	UPROPERTY(EditAnywhere, Category="Interact Info", meta=(AllowPrivateAccess))
		TArray<FStatusesFindInfo> CanInteractWithStatuses;

	// Interact widget class
	UPROPERTY(EditAnywhere, Category="Interact Info|Widget", meta=(AllowPrivateAccess))
		TSubclassOf<UUserWidget> InteractWidgetClassRef = nullptr;
	
	// Can Interact Logic
	
	const bool GetCanInteractByStatuses(const AActor* StatusCompOwner);
	const bool CanInteractByPrimitiveComps(UPrimitiveComponent* InteractMeshComponent);

	// Show debug
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ShowDebug", meta=(AllowPrivateAccess))
		bool bIsShowDebug = false;
	
	TArray<UPrimitiveComponent*> CanInteractPrimitiveComps = {};
	bool bIsAlreadyUse = false;

};