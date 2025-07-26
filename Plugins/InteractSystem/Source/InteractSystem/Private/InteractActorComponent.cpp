// Florist Game. All rights reserved.


#include "InteractActorComponent.h"
#include "StatusesComponent.h"
#include "Blueprint/UserWidget.h"


UInteractActorComponent::UInteractActorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

}

void UInteractActorComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if(bIsShowDebug) ShowDebug();
}

void UInteractActorComponent::FailedTryInteractLogic_Implementation(FHitResult HitResult)
{
}

void UInteractActorComponent::TraceHitLogic_Implementation(FHitResult HitResult, AActor* QueryFromActor)
{
}

void UInteractActorComponent::ShowDebug()
{
}

//------------------------------------Interact Interface Support Functions--------------------------------------------//

const bool UInteractActorComponent::GetIsShowInteractWidgetLogic_Implementation() const
{
	return true;
}

bool UInteractActorComponent::CanInteractLogic_Implementation(FHitResult HitResult, AActor* QueryFromActor)
{
	return true;
}

bool UInteractActorComponent::StartInteractLogic_Implementation(FHitResult HitResult, AActor* InteractByActor)
{
	return true;
}

void UInteractActorComponent::CancelCanInteractLogic_Implementation(FHitResult HitResult)
{
}

void UInteractActorComponent::StopInteractLogic_Implementation(FHitResult HitResult)
{
}

void UInteractActorComponent::GetInteractWidgetLocationLogic_Implementation(FVector& WidgetLocation) const
{
	if(!GetOwner()) WidgetLocation = FVector::Zero();
	WidgetLocation = GetOwner()->GetActorLocation();
}

void UInteractActorComponent::GetInteractWidgetLogic_Implementation(TSubclassOf<UUserWidget>& InteractWidgetClass, UObject*& OwnerInteractObject)
{
	
}

//--------------------------------------------------------------------------------------------------------------------//


//-------------------------------------------Interface Implementations------------------------------------------------//

//-----------------------------------------------Interact Interface---------------------------------------------------//

bool UInteractActorComponent::CanInteract_Implementation(FHitResult HitResult, AActor* QueryFromActor)
{
	// Get is owner can interact
	bool bLocalOwnerIsCanInteract = true;
	if(GetOwner()->Implements<UInteractInterface>()) bLocalOwnerIsCanInteract = IInteractInterface::Execute_CanInteract(GetOwner(), HitResult, QueryFromActor);
		
	return GetCanInteractByStatuses(QueryFromActor) && CanInteractByPrimitiveComps(HitResult.GetComponent()) &&
		CanInteractLogic(HitResult, QueryFromActor) && bLocalOwnerIsCanInteract && !bIsAlreadyUse;
}

void UInteractActorComponent::CancelCanInteract_Implementation(FHitResult HitResult)
{
	// Cancel can interact in owner
	if(GetOwner()->Implements<UInteractInterface>()) IInteractInterface::Execute_CancelCanInteract(GetOwner(), HitResult);
	
	CancelCanInteractLogic(HitResult);
}

bool UInteractActorComponent::StartInteract_Implementation(FHitResult HitResult, AActor* InteractByActor)
{
	if(bIsAlreadyUse) return false;

	// Start Interact in owner
	bool bLocalIsSuccessStartInteract = true;
	if(GetOwner()->Implements<UInteractInterface>()) bLocalIsSuccessStartInteract = IInteractInterface::Execute_StartInteract(GetOwner(), HitResult, InteractByActor);
		
	return bLocalIsSuccessStartInteract && StartInteractLogic(HitResult, InteractByActor);
}

void UInteractActorComponent::FailedTryInteract_Implementation(FHitResult HitResult)
{
	// Failed try interact in owner
	if(GetOwner()->Implements<UInteractInterface>()) IInteractInterface::Execute_FailedTryInteract(GetOwner(), HitResult);
	
	FailedTryInteractLogic(HitResult);
}

void UInteractActorComponent::StopInteract_Implementation(FHitResult HitResult)
{
	// Stop Interact in owner
	if(GetOwner()->Implements<UInteractInterface>()) IInteractInterface::Execute_StopInteract(GetOwner(), HitResult);
	
	StopInteractLogic(HitResult);
}

void UInteractActorComponent::TraceHit_Implementation(FHitResult HitResult, AActor* QueryFromActor)
{
	// Trace Hit in owner
	if(GetOwner()->Implements<UInteractInterface>()) IInteractInterface::Execute_TraceHit(GetOwner(), HitResult, QueryFromActor);
	
	TraceHitLogic(HitResult, QueryFromActor);
}

//--------------------------------------------Interact Widget Interact------------------------------------------------//

/*
 * If add IInteractWidgetInfoInterface information (widget and location) will be taken only from the owner
 */

void UInteractActorComponent::GetInteractWidgetLocation_Implementation(FVector& WidgetLocation) const
{
	GetInteractWidgetLocationLogic(WidgetLocation);

	// Get Interact Widget Location from owner
	FVector LocalWidgetLocationFromOwner = FVector::Zero();
	if(GetOwner()->Implements<UInteractWidgetInfoInterface>()) IInteractWidgetInfoInterface::Execute_GetInteractWidgetLocation(GetOwner(), LocalWidgetLocationFromOwner);

	// Doesnt return loc from owner if widget loc = FVector::Zero
	if(!LocalWidgetLocationFromOwner.IsNearlyZero()) WidgetLocation = LocalWidgetLocationFromOwner;
}

void UInteractActorComponent::GetInteractWidget_Implementation(TSubclassOf<UUserWidget>& InteractWidgetClass, UObject*& OwnerInteractObject)
{
	InteractWidgetClass = InteractWidgetClassRef;
	OwnerInteractObject = this;
	
	GetInteractWidgetLogic(InteractWidgetClass, OwnerInteractObject);

	// Get Interact Widget from owner
	if(GetOwner()->Implements<UInteractWidgetInfoInterface>()) IInteractWidgetInfoInterface::Execute_GetInteractWidget(GetOwner(), InteractWidgetClass, OwnerInteractObject);
}

const bool UInteractActorComponent::GetIsShowInteractWidget_Implementation() const
{
	// Get Is Show Interact Widget
	bool bLocalIsShowWidgetFromOwner = true;
	if(GetOwner()->Implements<UInteractWidgetInfoInterface>()) bLocalIsShowWidgetFromOwner = IInteractWidgetInfoInterface::Execute_GetIsShowInteractWidget(GetOwner());
		
	return bLocalIsShowWidgetFromOwner && GetIsShowInteractWidgetLogic();
}

//--------------------------------------------------------------------------------------------------------------------//


//----------------------------------------------Can Interact Logic----------------------------------------------------//

// Get is can interact if player contains this statuses
const bool UInteractActorComponent::GetCanInteractByStatuses(const AActor* StatusCompOwner)
{
	if(!StatusCompOwner || CanInteractWithStatuses.IsEmpty()) return true;
	const auto LocalDialogueComp = Cast<UStatusesComponent>(StatusCompOwner->GetComponentByClass(UStatusesComponent::StaticClass())); 
	if(!LocalDialogueComp) return true;
	bool bLocalIsCanInteract = false;
    
	for (const auto CanInteractStatuses : CanInteractWithStatuses)
	{
		bLocalIsCanInteract = bLocalIsCanInteract || LocalDialogueComp->GetIsContainStatuses(
		CanInteractStatuses.StatusesToFind,
		CanInteractStatuses.bCheckAll,
		CanInteractStatuses.bExactCheck,
		CanInteractStatuses.bInverseCondition);
	}
    
	return bLocalIsCanInteract;
}

// Get primitives that can be interacted with
const bool UInteractActorComponent::CanInteractByPrimitiveComps(UPrimitiveComponent* InteractMeshComponent)
{
	return CanInteractPrimitiveComps.IsEmpty() || !InteractMeshComponent ? true : CanInteractPrimitiveComps.Contains(InteractMeshComponent);
}

//--------------------------------------------------------------------------------------------------------------------//