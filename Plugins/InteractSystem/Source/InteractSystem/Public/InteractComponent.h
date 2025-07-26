//GAIDJIIN. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "InteractComponent.generated.h"

class UInteractWidgetManager;
class UWidgetComponent;

UENUM(BlueprintType)
enum class ECanInteractState : uint8
{
    None = 0 UMETA(Hidden),
    CanNotInteract = 1 << 0,
    UsedNow = 1 << 1,
    CanInteract = 1 << 2
};

UENUM(BlueprintType)
enum class EFocusInteractVisualizationMode : uint8
{
    None = 0 UMETA(Hidden),
    Widget = 1 << 0,
    Outliner = 1 << 1
};

enum class EFocusState
{
    None = 0,
    Find = 1 << 0,
    Lost = 1 << 1
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSuccessStartInteract);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFindInteract);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLostInteract);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChangeCanInteractState, ECanInteractState, CanInteractState);

class UStatusesComponent;
class IInteractInterface;

UCLASS( Blueprintable, ClassGroup=(InteractSystem), meta=(BlueprintSpawnableComponent))
class INTERACTSYSTEM_API UInteractComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UInteractComponent();

protected:
	virtual void BeginPlay() override;

public:
    
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    
    // Main Logic

    // Interact

    UFUNCTION(BlueprintCallable, Category="Interact Component")
        void StartInteract();
    UFUNCTION(BlueprintCallable, Category="Interact Component")
        void StopInteract();
    UFUNCTION(BlueprintCallable, Category="Interact Component")
        void Interact();
    UFUNCTION(BlueprintCallable, Category="Interact Component")
        void InteractCheck();

    // Setter
    
    UFUNCTION(BlueprintCallable, Category="Interact Component")
        void SetStopInteractCheck(const bool bIsInteractCheck);
    UFUNCTION(BlueprintCallable,Category="Interact Component")
        void SetUseActorEyes(bool bNewbIsUseActorEyes) { bIsUseActorEyes = bNewbIsUseActorEyes; }
    
    // Getter

    UFUNCTION(BlueprintCallable,Category="Interact Component")
        FORCEINLINE bool GetIsValidInteractObject() const { return InteractableObject.IsValid() && InteractableObject.Get(); } 
    UFUNCTION(BlueprintCallable,Category="Interact Component")
        const void GetInteractItemWidgetLocation(FVector& WidgetLocation) const;
    UFUNCTION(BlueprintCallable, Category="Interact Component")
        const ECanInteractState GetCurrentInputState() const;
    
private:
    
    // Variables

    // If true - use start point camera location
    UPROPERTY(EditAnywhere, Category="Interact Setup", meta=(AllowPrivateAccess))
        bool bIsUseActorEyes = false;
    UPROPERTY(EditAnywhere, Category="Interact Setup", meta=(AllowPrivateAccess))
        EFocusInteractVisualizationMode FocusInteractVisualizationMode = EFocusInteractVisualizationMode::Widget;
    UPROPERTY(Replicated, EditAnywhere, Category="Interact Setup", meta=(AllowPrivateAccess))
        bool bStopInteractCheck = false;
    UPROPERTY(EditAnywhere, Category="Interact Setup", meta=(AllowPrivateAccess))
        float MaxInteractionDistance = 1000.f;
    UPROPERTY(EditAnywhere, Category="Interact Setup", meta=(AllowPrivateAccess))
        float MaxInteractionRadius = 20.f;
    UPROPERTY(EditAnywhere, Category="Interact Setup", meta=(ClampMin="0.01", AllowPrivateAccess))
        float InteractionFrequency = 0.1f;
    UPROPERTY(EditAnywhere, Category="Interact Setup", meta=(AllowPrivateAccess))
        TEnumAsByte<ECollisionChannel> InteractChannel = ECC_Visibility;
    UPROPERTY(EditAnywhere,  Category="Debug", meta=(AllowPrivateAccess))
        bool bShowInteractDebug = false;
    // Show Widget Comp location by sphere
    UPROPERTY(EditAnywhere, Category="Debug",meta=(AllowPrivateAccess, EditConditionHides, EditCondition="bShowInteractDebug"))
        bool bShowWidgetLocSphere = false;
    UPROPERTY(EditAnywhere, Category="Statuses", meta=(AllowPrivateAccess))
        FGameplayTag InteractNowTag;
    UPROPERTY(EditAnywhere, Category="Statuses", meta=(AllowPrivateAccess))
        FGameplayTagContainer CantInteractTag;

    // CPP
    // Service

    EFocusState FocusState = EFocusState::None;
    TWeakObjectPtr<UStatusesComponent> StatusesComp;
    FTimerHandle CheckInteractTraceHandle;

    // Replicated
    
    UPROPERTY(VisibleAnywhere, Replicated, Category="Service")
        bool bStopNow = false;
    UPROPERTY(VisibleAnywhere, Replicated, Category="Service")
        ECanInteractState CurrentCanInteractState = ECanInteractState::None;
    
    // Widget Info

    // Widget manager for manipulate of interact widgets
    TObjectPtr<UInteractWidgetManager> InteractWidgetManager = nullptr;
    TObjectPtr<UWidgetComponent> InteractWidgetComp = nullptr;
    
    // Current Interact Info
    // Object for interact now (actor or actor comp)
    UPROPERTY(VisibleAnywhere, Replicated, Category="Service Interact Info")
        TWeakObjectPtr<AActor> InteractableObject;
    UPROPERTY(VisibleAnywhere, Replicated, Category="Service Interact Info")
        FHitResult CurrentHitResult;
    
    // Functions

    FORCEINLINE bool IsLocallyControlled() const;
    void FirstInitializeComp();
    
    // Interact

    // Internal
    void StartInteract_Internal();
    void StopInteract_Internal();
    void Interact_Internal();
    void InteractCheck_Internal();
    void SetInteractObject_Internal(AActor* NewInteractObject) { InteractableObject = NewInteractObject; }
    void SetStopInteractCheck_Internal(const bool bIsInteractCheck);
    
    // Service
    
    bool TraceFromCamera(FHitResult& OutHit);
    void SetInteractCheck(const bool bIsCheckInteract);
    
    // Clear interact info
    void ClearCurrentInteractInfo();
    
    // Check can interact state
    const void CheckCanInteractState();
    void CancelInteractFromItem();
    // Get Interactable object from actor
    UObject* GetInteractObject(AActor* InteractActor) const;
    // Get Interactable object from saved InteractableObject
    UObject* GetInteractObject() { return GetInteractObject(InteractableObject.Get()); }
    // Get const Interactable object from saved InteractableObject
    UObject* GetConstInteractObject() const { return GetInteractObject(InteractableObject.Get()); }
    
    // Call delegates or find or lost object
    void OnFindOrLostInteractObject(const bool bIsFindInteract) const;
    // Call on change interact focus
    void CheckChangeInteractFocusState();

    //--------------------------------------------Replicated Methods--------------------------------------------------//

    // Start Interact
    UFUNCTION(Server, Unreliable, WithValidation, Category="Interact Component")
        void Server_StartInteract();
    UFUNCTION(Client, Unreliable, BlueprintCallable,Category="Interact Component")
        void Client_StartInteract();

    // Stop Interact
    UFUNCTION(Server, Unreliable, WithValidation, Category="Interact Component")
        void Server_StopInteract();
    UFUNCTION(Client, Unreliable, Category="Interact Component")
        void Client_StopInteract();

    // Interact
    UFUNCTION(Server, Unreliable, WithValidation, Category="Interact Component")
        void Server_Interact();
    UFUNCTION(Client, Unreliable, Category="Interact Component")
        void Client_Interact();

    // Interact Check
    UFUNCTION(Server, Unreliable, WithValidation, Category="Interact Component")
        void Server_InteractCheck();
    UFUNCTION(Client, Unreliable, Category="Interact Component")
        void Client_InteractCheck();
    UFUNCTION(Server, Unreliable, WithValidation, Category="Interact Component")
        void Server_SetInteractObject(AActor* NewInteractActor);

    // Set Interact Check
    UFUNCTION(Server, Unreliable, WithValidation, Category="Interact Component")
        void Server_SetStopInteractCheck(const bool bIsInteractCheck);

    // Set Interact highlight
    UFUNCTION(Client, Unreliable, Category="Interact Highlight")
        void Client_ToggleHighlightMeshes(const AActor* ActorToHighlight, bool bIsHighlight);
    
    // Delegate call methods on client
    UFUNCTION(Client, Unreliable, Category="Replicate Delegates")
        void Client_CallOnSuccessStartInteract() const;
        void Client_CallOnSuccessStartInteract_Implementation() const { OnSuccessStartInteract.Broadcast(); }
    UFUNCTION(Client, Unreliable, Category="Replicate Delegates")
        void Client_CallOnFindInteract() const;
        void Client_CallOnFindInteract_Implementation() const { OnFindInteract.Broadcast(); }
    UFUNCTION(Client, Unreliable, Category="Replicate Delegates")
        void Client_CallOnLostInteract() const;
        void Client_CallOnLostInteract_Implementation() const { OnLostInteract.Broadcast(); }
    UFUNCTION(Client, Unreliable, Category="Replicate Delegates")
        void Client_CallOnChangeCanInteractState(ECanInteractState NewCanInteractState) const;
        void Client_CallOnChangeCanInteractState_Implementation(ECanInteractState NewCanInteractState) const
        { OnChangeCanInteractState.Broadcast(NewCanInteractState); }

    // Debug
    UFUNCTION(Client, Unreliable, Category="Debug")
        void Client_ShowDebugInteractLine(const FHitResult& HitResult);
    
    //--------------------------------------------------------------------------------------------------------------------//

    
    // Debug

    #if !UE_BUILD_SHIPPING
        void ShowDebug();
        const FString GetCurrentCanInteractStateStringDebug() const;
    #endif
    
    // Delegates
    
    UPROPERTY(BlueprintAssignable)
        FOnSuccessStartInteract OnSuccessStartInteract;
    UPROPERTY(BlueprintAssignable)
        FOnFindInteract OnFindInteract;
    UPROPERTY(BlueprintAssignable)
        FOnLostInteract OnLostInteract;
    UPROPERTY(BlueprintAssignable)
        FOnChangeCanInteractState OnChangeCanInteractState;
    
};