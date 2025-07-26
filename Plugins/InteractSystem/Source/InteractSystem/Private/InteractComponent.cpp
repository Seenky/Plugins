//GAIDJIIN. All rights reserved.


#include "InteractComponent.h"
#include "StatusesComponent.h"
#include "Components/WidgetComponent.h"
#include "Interfaces/InteractInterface.h"
#include "Kismet/KismetSystemLibrary.h"
#include "InteractWidgetManager/InteractWidgetManager.h"
#include "Interfaces/InteractOutlinerInterface.h"
#include "Interfaces/InteractWidgetInfoInterface.h"
#include "Net/UnrealNetwork.h"

UInteractComponent::UInteractComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

    if(IsLocallyControlled())
    {
        InteractWidgetComp = CreateDefaultSubobject<UWidgetComponent>("WidgetComponent");
    }
}

bool UInteractComponent::IsLocallyControlled() const
{
    auto LocalOwner = GetOwner();
    if(!LocalOwner) return false;

    const ENetMode OwnerNetMode = LocalOwner->GetNetMode();

    if(OwnerNetMode == NM_Standalone)
    {
        return true;
    }
    
    if(OwnerNetMode == NM_Client && GetOwnerRole() == ROLE_AutonomousProxy)
    {
        return true;
    }

    if(LocalOwner->GetRemoteRole() != ROLE_AutonomousProxy && GetOwnerRole() == ROLE_Authority)
    {
        return true;
    }

    return false;
}

// First Initialize needed info
void UInteractComponent::FirstInitializeComp()
{
    if(!GetOwner() || !GetWorld()) return;
    
    if(IsLocallyControlled())
    {
        if(FocusInteractVisualizationMode == EFocusInteractVisualizationMode::Widget)
        {
            InteractWidgetManager = NewObject<UInteractWidgetManager>(this, UInteractWidgetManager::StaticClass(),"InteractWidgetManager");
            checkf(InteractWidgetComp && InteractWidgetManager, TEXT("No set Interact Widget Comp or Interact Widget Manager"))
            InteractWidgetManager->SetupInteractWidgetComp(InteractWidgetComp, GetOwner());
        }
        else
        {
            if(InteractWidgetManager) InteractWidgetManager->ConditionalBeginDestroy();
            if(InteractWidgetComp) InteractWidgetComp->DestroyComponent();
        }
    }
    else
    {
        if(InteractWidgetManager) InteractWidgetManager->ConditionalBeginDestroy();
        if(InteractWidgetComp) InteractWidgetComp->DestroyComponent();
    }
    
    SetInteractCheck(true);
}

void UInteractComponent::BeginPlay()
{
	Super::BeginPlay();
	SetComponentTickEnabled(false);

    // Setup comp only if owner locally controlled
    StatusesComp = GetOwner()->FindComponentByClass<UStatusesComponent>();
    FirstInitializeComp();

    SetComponentTickEnabled(true);
}

void UInteractComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	ShowDebug();
}

void UInteractComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION(UInteractComponent, InteractableObject, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UInteractComponent, CurrentHitResult, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UInteractComponent, bStopInteractCheck, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UInteractComponent, bStopNow, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UInteractComponent, CurrentCanInteractState, COND_OwnerOnly);
}

//---------------------------------------------Start Interact---------------------------------------------------------//

void UInteractComponent::StartInteract()
{
    Server_StartInteract();
}

void UInteractComponent::Server_StartInteract_Implementation()
{
    StartInteract_Internal();
    Client_StartInteract();
}

void UInteractComponent::Client_StartInteract_Implementation()
{
    // Play animation, sound and etc
}

bool UInteractComponent::Server_StartInteract_Validate()
{
    return true;
}

void UInteractComponent::StartInteract_Internal()
{
    auto LocalResolvedInteractableObject = GetInteractObject();
    if(!LocalResolvedInteractableObject) return;

    if(StatusesComp.IsValid())
    {
        if(StatusesComp->GetIsContainStatuses(CantInteractTag,false,true))
        {
            IInteractInterface::Execute_FailedTryInteract(LocalResolvedInteractableObject,CurrentHitResult);
            return;
        }
    }

    /*
     * Check is can interact with Interactable Object.
     * If cant interact with Interactable Object - return
     */
    if(!IInteractInterface::Execute_CanInteract(LocalResolvedInteractableObject,CurrentHitResult,GetOwner()))
    {
        if(bShowInteractDebug) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red,
            FString("Cant Interact With Object"));

        IInteractInterface::Execute_FailedTryInteract(LocalResolvedInteractableObject,CurrentHitResult);
        return;
    }
    
    bool LocalInteractSuccess = IInteractInterface::Execute_StartInteract(LocalResolvedInteractableObject,CurrentHitResult, GetOwner());
    if(LocalInteractSuccess)
    {
        if(bShowInteractDebug) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
            FString("Interact With Object!!!"));
        
        Client_CallOnSuccessStartInteract();
    }
}

//--------------------------------------------------------------------------------------------------------------------//

//----------------------------------------------Stop Interact---------------------------------------------------------//

void UInteractComponent::StopInteract()
{
    Server_StopInteract();
}

void UInteractComponent::Server_StopInteract_Implementation()
{
    StopInteract_Internal();
    Client_StopInteract();
}

void UInteractComponent::Client_StopInteract_Implementation()
{
    // Play animation, sound and etc
}

bool UInteractComponent::Server_StopInteract_Validate()
{
    return true;
}

void UInteractComponent::StopInteract_Internal()
{
    auto LocalResolvedInteractableObject = GetInteractObject();
    if(!LocalResolvedInteractableObject) return;
    IInteractInterface::Execute_StopInteract(LocalResolvedInteractableObject, CurrentHitResult);
}

//--------------------------------------------------------------------------------------------------------------------//

//---------------------------------------------------Interact---------------------------------------------------------//

void UInteractComponent::Interact()
{
    Server_Interact();
}

void UInteractComponent::Server_Interact_Implementation()
{
    Interact_Internal();
    Client_Interact();
}

void UInteractComponent::Client_Interact_Implementation()
{
    // Play animation, sound and etc
}

bool UInteractComponent::Server_Interact_Validate()
{
    return true;
}

void UInteractComponent::Interact_Internal()
{
    if(!StatusesComp.IsValid()) return;
    bool bInteractNow = StatusesComp->GetIsContainStatus(FGameplayTag(InteractNowTag),true);
    bInteractNow ?  StopInteract() : StartInteract();
}

//--------------------------------------------------------------------------------------------------------------------//

//------------------------------------------------Interact Check------------------------------------------------------//

void UInteractComponent::Server_SetInteractObject_Implementation(AActor* NewInteractObject)
{ SetInteractObject_Internal(NewInteractObject); }

bool UInteractComponent::Server_SetInteractObject_Validate(AActor* NewInteractObject)
{
    return true;
}

void UInteractComponent::Server_InteractCheck_Implementation()
{
    InteractCheck_Internal();
}

bool UInteractComponent::Server_InteractCheck_Validate()
{
    return true;
}

void UInteractComponent::InteractCheck()
{
    Server_InteractCheck();
}

void UInteractComponent::Client_InteractCheck_Implementation()
{
    InteractCheck_Internal();
}

void UInteractComponent::InteractCheck_Internal()
{
    // Check is can interact now
    auto LocalResolvedInteractableObject = GetInteractObject();
    
    const bool bLocalIsValidInteractItem = LocalResolvedInteractableObject && InteractableObject.IsValid();
    bool bCantCheckNow = false;
    bool bInteractNow = false;
    if(StatusesComp.IsValid())
    {
        bCantCheckNow = StatusesComp->GetIsContainStatuses(CantInteractTag,false,true);
        bInteractNow = StatusesComp->GetIsContainStatus(FGameplayTag(InteractNowTag),true);
    }
    
    bool bStopInteract = bStopInteractCheck || bInteractNow || bCantCheckNow;
    
    if (bStopNow)
    {
        // Return if stop now
        if(bStopInteract) return;
        bStopNow = false;
    }

    // Cancel from interact if cant interact by statuses info or if interact check was stopped
    if(bStopInteract)
    {
        if(!bLocalIsValidInteractItem) return;
        CancelInteractFromItem();
        if(bStopInteractCheck || bCantCheckNow) ClearCurrentInteractInfo();
        bStopNow = true;
        return;
    }

    // Try find interact object
    FHitResult LocalOutHit;
    bool LocalIsHit = TraceFromCamera(LocalOutHit);
    CurrentHitResult = LocalOutHit;
    
    // Cancel from interact if no hit
    if(!LocalIsHit)
    {
        if(!bLocalIsValidInteractItem) return;
        CancelInteractFromItem();
        ClearCurrentInteractInfo();
        return;
    }

    // Find Interactable Object
    UObject* LocalInteractableItem = nullptr;
    if(LocalOutHit.GetActor()) LocalInteractableItem = GetInteractObject(LocalOutHit.GetActor());
    
    // Cancel if new interact object is different as previous
    if(bLocalIsValidInteractItem && LocalResolvedInteractableObject != LocalInteractableItem)
    {
        CancelInteractFromItem();
    }

    // Clear Interactable Object if current finded object is empty
    if(!LocalInteractableItem)
    {
        ClearCurrentInteractInfo();
        return;
    }
    
    // Save if finded Interact Object is new
    const bool bLocalIsFindedObjectNew = LocalInteractableItem != LocalResolvedInteractableObject;
    
    // Save Interact Object
    Server_SetInteractObject(LocalOutHit.GetActor());
    
    // Execute Trace Hit. Call after find valid InteractableObject
    IInteractInterface::Execute_TraceHit(LocalInteractableItem, CurrentHitResult, GetOwner());

    // Show or Hide widget
    auto LocalIsShowInteractWidget = false;

    if(LocalInteractableItem->Implements<UInteractWidgetInfoInterface>())
    {
        LocalIsShowInteractWidget = IInteractWidgetInfoInterface::Execute_GetIsShowInteractWidget(LocalInteractableItem);
        // Setup interact widget by interact widget manager
        TSubclassOf<UUserWidget> LocalInteractWidgetRef;
        UObject* LocalInteractWidgetOwnerObject;
        IInteractWidgetInfoInterface::Execute_GetInteractWidget(LocalInteractableItem, LocalInteractWidgetRef, LocalInteractWidgetOwnerObject);

        if(auto LocalOwnerPawn = Cast<APawn>(GetOwner()))
        {
            auto LocalPlayerController = LocalOwnerPawn->GetController<APlayerController>();
            if(InteractWidgetManager && LocalPlayerController)
            {
                InteractWidgetManager->ToggleVisibilityWidget(LocalPlayerController, LocalInteractWidgetRef, LocalInteractWidgetOwnerObject,
                GetOwner(), LocalIsShowInteractWidget);
            }
        }
    }
    
    // Set Interact Widget Location if widget comp is valid and InteractWidgetLocation != ComponentLocation()
    FVector LocalInteractItemWidgetLocation;
    GetInteractItemWidgetLocation(LocalInteractItemWidgetLocation);
    if(LocalIsShowInteractWidget && InteractWidgetComp && LocalInteractItemWidgetLocation != InteractWidgetComp->GetComponentLocation())
        InteractWidgetComp->SetWorldLocation(LocalInteractItemWidgetLocation);

    // Call is find new interact object
    if(bLocalIsFindedObjectNew)
    {
        FocusState = EFocusState::None;
        OnFindOrLostInteractObject(true);
    }
    
    // Check Can Interact State - call in end after all calculations
    CheckCanInteractState();

    // Check is changed interact focus state
    CheckChangeInteractFocusState();
}

//--------------------------------------------------------------------------------------------------------------------//

// Set Interact highlight
void UInteractComponent::Client_ToggleHighlightMeshes_Implementation(const AActor* ActorToHighlight,
    bool bIsHighlight)
{
    if(!IsLocallyControlled() || !ActorToHighlight || FocusInteractVisualizationMode != EFocusInteractVisualizationMode::Outliner) return;
    
    if(!ActorToHighlight) return;

    // If no implement interface needed for highlight - return  
    if(!ActorToHighlight->Implements<UInteractOutlinerInterface>()) return;

    const auto LocalHighlightMeshes = IInteractOutlinerInterface::Execute_GetComponentsForHighlight(ActorToHighlight);

    // Toggle highlight state meshes
    for (const auto& LocalHighlightMesh : LocalHighlightMeshes)
    {
        if (!LocalHighlightMesh) return;

        LocalHighlightMesh->SetRenderCustomDepth(bIsHighlight);
        LocalHighlightMesh->CustomDepthStencilValue = bIsHighlight ? 252 : 0;
    }
}

void UInteractComponent::OnFindOrLostInteractObject(const bool bIsFindInteract) const
{
    bIsFindInteract ? Client_CallOnFindInteract() : Client_CallOnLostInteract();
}

void UInteractComponent::CheckChangeInteractFocusState()
{
    // Check is changed interact focus state
    switch (CurrentCanInteractState)
    {
    case ECanInteractState::None:
    case ECanInteractState::UsedNow:
    case ECanInteractState::CanNotInteract:
        {
            if(FocusState == EFocusState::None || FocusState == EFocusState::Find)
            {
                IInteractInterface::Execute_OnInteractTargetFocusChanged(InteractableObject.Get(), false, CurrentHitResult, GetOwner());
                Client_ToggleHighlightMeshes(InteractableObject.Get(), false);
                FocusState = EFocusState::Lost;
            }
            break;
        }
        
    case ECanInteractState::CanInteract:
        {
            if(FocusState == EFocusState::None || FocusState == EFocusState::Lost)
            {
                IInteractInterface::Execute_OnInteractTargetFocusChanged(InteractableObject.Get(), true, CurrentHitResult, GetOwner());
                Client_ToggleHighlightMeshes(InteractableObject.Get(), true);
                FocusState = EFocusState::Find;
            }
            break;
        }
        
    default:
        break;
    }
}

void UInteractComponent::SetStopInteractCheck(const bool bIsInteractCheck)
{
    Server_SetStopInteractCheck(bIsInteractCheck);
}

void UInteractComponent::Server_SetStopInteractCheck_Implementation(const bool bIsInteractCheck)
{
    SetStopInteractCheck_Internal(bIsInteractCheck);
}

bool UInteractComponent::Server_SetStopInteractCheck_Validate(const bool bIsInteractCheck)
{
    return true;
}

void UInteractComponent::SetStopInteractCheck_Internal(const bool bIsInteractCheck)
{
    bStopInteractCheck = bIsInteractCheck;
    if(bStopInteractCheck)
    {
        if(!InteractableObject.IsValid()) return;
        CancelInteractFromItem();
        CurrentHitResult = FHitResult();
        Server_SetInteractObject(nullptr);
    }
}

void UInteractComponent::CancelInteractFromItem()
{
    if(!InteractableObject.IsValid()) return;
    IInteractInterface::Execute_CancelCanInteract(GetInteractObject(), CurrentHitResult);
    if(InteractWidgetManager) InteractWidgetManager->HideVisibilityWidget();
    
    IInteractInterface::Execute_OnInteractTargetFocusChanged(InteractableObject.Get(), false, CurrentHitResult, GetOwner());
    Client_ToggleHighlightMeshes(InteractableObject.Get(), false);
    FocusState = EFocusState::None;
    
    OnFindOrLostInteractObject(false);
}

// Trace from camera
bool UInteractComponent::TraceFromCamera(FHitResult& OutHit)
{
    FVector PlayerViewLoc;
    FRotator PlayerViewRot;

    // If no use actor eyes - try take player view point
    if(!bIsUseActorEyes)
    {
        if(auto LocalOwnerPawn = Cast<APawn>(GetOwner()))
        {
            auto LocalPlayerController = LocalOwnerPawn->GetController<APlayerController>();
            if(!LocalPlayerController || !LocalPlayerController->PlayerCameraManager || !GetWorld()) return false;
            LocalPlayerController->GetPlayerViewPoint(PlayerViewLoc,PlayerViewRot);
        }
    }
    // Else use actor eyes
    else GetOwner()->GetActorEyesViewPoint(PlayerViewLoc,PlayerViewRot);

    // Calculate Start End loc
    const FVector Start = PlayerViewLoc;
    const FVector End = PlayerViewLoc + PlayerViewRot.Vector()*MaxInteractionDistance;
    
    const bool bHitTrace = UKismetSystemLibrary::SphereTraceSingle
    (
        GetWorld(),
        Start,
        End,
        MaxInteractionRadius,
        UEngineTypes::ConvertToTraceType(InteractChannel),
        false,
        {GetOwner()},
        EDrawDebugTrace::None,
        OutHit,
        true,
        FLinearColor::Red,
        FLinearColor::Green,
        InteractionFrequency
        );

    Client_ShowDebugInteractLine(OutHit);
    
    return bHitTrace;
}

void UInteractComponent::SetInteractCheck(const bool bIsCheckInteract)
{
    if(!GetWorld()) return;

    // Set timer to check trace
    if(bIsCheckInteract && !bStopInteractCheck && GetOwnerRole() == ROLE_Authority)
    {
        if(!GetWorld()->GetTimerManager().IsTimerActive(CheckInteractTraceHandle))
            GetWorld()->GetTimerManager().SetTimer(CheckInteractTraceHandle, this,
                &UInteractComponent::InteractCheck, InteractionFrequency, true);
    }
    // Clear timer if stop interact OR force stop interact
    else GetWorld()->GetTimerManager().ClearTimer(CheckInteractTraceHandle);
}

// Clear interact info
void UInteractComponent::ClearCurrentInteractInfo()
{
    Server_SetInteractObject(nullptr);
    CurrentHitResult = FHitResult();
    CurrentCanInteractState = ECanInteractState::None;
}

// Getter
const void UInteractComponent::GetInteractItemWidgetLocation(FVector& WidgetLocation) const
{
    if(!InteractableObject.IsValid()) WidgetLocation = FVector::Zero();
    if(!InteractableObject.Get()->Implements<UInteractWidgetInfoInterface>()) return;
    
    IInteractWidgetInfoInterface::Execute_GetInteractWidgetLocation(GetConstInteractObject(), WidgetLocation);
}

const ECanInteractState UInteractComponent::GetCurrentInputState() const
{
    ECanInteractState LocalNewCanInteractState = CurrentCanInteractState;
        
    if(!InteractableObject.IsValid()) LocalNewCanInteractState = ECanInteractState::None;
        
    // else if( Is Used Now Condition ) LocalNewCanInteractState = ECanInteractState::UsedNow;
        
    else
    {
        auto LocalResolvedInteractableObject = GetConstInteractObject();
        if(!LocalResolvedInteractableObject) return ECanInteractState::None;
        
        const bool bLocalCanInteract = IInteractInterface::Execute_CanInteract(LocalResolvedInteractableObject,CurrentHitResult,GetOwner());
        LocalNewCanInteractState = bLocalCanInteract ? ECanInteractState::CanInteract : ECanInteractState::CanNotInteract;
    }

    return MoveTemp(LocalNewCanInteractState);
}

const void UInteractComponent::CheckCanInteractState()
{
    ECanInteractState LocalNewCanInteractState = GetCurrentInputState();
    
    // Call Delegate if new state
    if(CurrentCanInteractState != LocalNewCanInteractState)
    {
        CurrentCanInteractState = LocalNewCanInteractState;
        Client_CallOnChangeCanInteractState(CurrentCanInteractState);
    }
}

UObject* UInteractComponent::GetInteractObject(AActor* InteractActor) const
{
    if(!InteractActor) return nullptr;
    UObject* LocalInteractableObject = InteractActor;
    if(InteractActor->Implements<UInteractInterface>()) return LocalInteractableObject;
    
    const auto LocalInteractComponents = InteractActor->GetComponentsByInterface(UInteractInterface::StaticClass());
    if(LocalInteractComponents.IsEmpty()) return nullptr;
    
    for(const auto InteractComp : LocalInteractComponents)
    {
        if(InteractComp->Implements<UInteractInterface>()) return InteractComp;
    }
    
    return nullptr;
}

#if !UE_BUILD_SHIPPING

    // Debug Show Trace
    void UInteractComponent::Client_ShowDebugInteractLine_Implementation(const FHitResult& HitResult)
    {
        if(!bShowInteractDebug) return;
    
        FVector Start = HitResult.TraceStart; FVector End = HitResult.TraceEnd;
        FVector Direction = (End-Start).GetSafeNormal();
        bool bHitTrace = HitResult.bBlockingHit;
        float LocalAllDistance = FVector::Distance(Start, End);
        // Show debug on locally controlled on client
        if(IsLocallyControlled())
        {
            FVector LocalEndPoint = HitResult.Location;
            float LocalDistanceTo = bHitTrace ? HitResult.Distance : LocalAllDistance;
            DrawDebugCylinder(GetWorld(), Start, Start + Direction * LocalDistanceTo, MaxInteractionRadius, 12, FColor::Red, false, InteractionFrequency);
            if(bHitTrace) DrawDebugCylinder(GetWorld(), LocalEndPoint, LocalEndPoint + Direction * (LocalAllDistance - HitResult.Distance), MaxInteractionRadius, 12, FColor::Green, false, InteractionFrequency);
        }
    }

    // Debug show logic
    void UInteractComponent::ShowDebug()
    {
        if(!bShowInteractDebug) return;
        FString DebugText;
        if(InteractableObject.IsValid())
        {
            DebugText += "Interactable Object is: " + FString(InteractableObject.Get() ? "True" : "False");
            DebugText += "\nInteractable Name: " + InteractableObject->GetName();
            if(InteractWidgetComp && !InteractWidgetComp->bHiddenInGame)
            {
                UKismetSystemLibrary::DrawDebugString(
                    GetWorld(),
                    InteractWidgetComp->GetComponentLocation(),
                    GetNameSafe(InteractableObject.Get())
                    );
            }
        }

        DebugText += "\nInteract State: " + GetCurrentCanInteractStateStringDebug();
        DebugText += "\nIs stop now: " + FString(bStopNow ? "True" : "False");

        // Get debug info from Interact Widget Manager
        if(InteractWidgetManager) DebugText += "\n" + InteractWidgetManager->GetDebugInteractWidgetsString() + "\n";
        
        UKismetSystemLibrary::PrintString(GetWorld(),
               DebugText,
               true,
               false,
               FLinearColor::Red,
               0.0f);

        if(InteractWidgetComp && bShowWidgetLocSphere && InteractWidgetManager) UKismetSystemLibrary::DrawDebugSphere(GetOwner(),
            InteractWidgetComp->GetComponentLocation(),30.f,12.f, InteractWidgetManager->GetIsWidgetVisibleNow() ?
            FLinearColor::Green : FLinearColor::Red, 0.f, 2.f);
        
    }

    const FString UInteractComponent::GetCurrentCanInteractStateStringDebug() const
    {
        switch (CurrentCanInteractState)
        {
            case ECanInteractState::None:
                return "None";
            case ECanInteractState::CanInteract:
                return "CanInteract";
            case ECanInteractState::UsedNow:
                return "UsedNow";
            case ECanInteractState::CanNotInteract:
                return "CanNotInteract";
            default:
                return "Default";
        }
    }
#endif