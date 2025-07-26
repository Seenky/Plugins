// Florist Game. All rights reserved.


#include "InteractWidgetManager/InteractWidgetManager.h"
#include "InteractWidgetManager/InteractWithUIInterface.h"

// Create Interact Widget
UUserWidget* UInteractWidgetManager::CreateOrGetInteractWidget(APlayerController* WidgetOwningObject, TSubclassOf<UUserWidget> InteractWidgetClass, UObject* OwnerInteractObject)
{
	if(!InteractWidgetClass || !WidgetOwningObject || !OwnerInteractObject) return nullptr;
	
	// Try find widget in interact widgets pool
	auto LocalInteractWidget = GetInteractWidget(InteractWidgetClass);
	if(LocalInteractWidget.Key)
	{
		// Setup widget if it implements interface and new Interact object id
		if(LocalInteractWidget.Key->Implements<UInteractWithUIInterface>() && OwnerInteractObject != LocalInteractWidget.Value)
		{
			// Clear logic from previous interact owner (unbind delegates and etc)
			IInteractWithUIInterface::Execute_ClearInfoOnChangeInteractOwner(LocalInteractWidget.Key, LocalInteractWidget.Value.Get());
			// Setup new interact owner in UI (binding delegates and etc)
			IInteractWithUIInterface::Execute_SetupWidget(LocalInteractWidget.Key, OwnerInteractObject);
			// Save new info in interact widget pool
			ToggleInteractWidget(MoveTemp(LocalInteractWidget.Key),OwnerInteractObject, true);
		}
		
		return MoveTemp(LocalInteractWidget.Key);
	}
	
	// Create widget if not contains widget
	LocalInteractWidget.Key = CreateWidget(WidgetOwningObject, InteractWidgetClass);
	// Setup and construct widget if it implements interface
	if(LocalInteractWidget.Key && LocalInteractWidget.Key->Implements<UInteractWithUIInterface>())
	{
		IInteractWithUIInterface::Execute_ConstructWidget(LocalInteractWidget.Key);
		IInteractWithUIInterface::Execute_SetupWidget(LocalInteractWidget.Key, OwnerInteractObject);
	}
	ToggleInteractWidget(MoveTemp(LocalInteractWidget.Key),OwnerInteractObject, true);

	return MoveTemp(LocalInteractWidget.Key);
}

void UInteractWidgetManager::SetupInteractWidgetComp(TWeakObjectPtr<UWidgetComponent> NewInteractWidgetComp,
	TWeakObjectPtr<AActor> NewInteractWidgetCompOwner)
{
	if(!NewInteractWidgetComp.IsValid() || !NewInteractWidgetComp.Get()) return;
	
	InteractWidgetCompOwner = NewInteractWidgetCompOwner;
	InteractWidgetComp = NewInteractWidgetComp;
	NewInteractWidgetComp.Get()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	NewInteractWidgetComp.Get()->SetDrawAtDesiredSize(true);
	NewInteractWidgetComp.Get()->SetWidgetSpace(EWidgetSpace::Screen);
	NewInteractWidgetComp.Get()->SetVisibleInRayTracing(false);
	NewInteractWidgetComp.Get()->bVisibleInReflectionCaptures = false;
	NewInteractWidgetComp.Get()->bVisibleInRealTimeSkyCaptures = false;
	NewInteractWidgetComp.Get()->SetReceivesDecals(false);
}

void UInteractWidgetManager::ToggleVisibilityWidget(APlayerController* NewWidgetOwningObject, TSubclassOf<UUserWidget> NewInteractWidgetClass,
		UObject* NewOwnerInteractObject, TWeakObjectPtr<AActor> NewInteractWidgetCompOwner, const bool bIsShow)
{
	if(!InteractWidgetComp.IsValid() || !InteractWidgetComp.Get()) return;
    
	SetupInteractWidgetComp(InteractWidgetComp, NewInteractWidgetCompOwner);
	InteractWidgetComp->SetHiddenInGame(!bIsShow);

	// Try Set New Interact Widget
	auto LocalInteractWidget = CreateOrGetInteractWidget(NewWidgetOwningObject, NewInteractWidgetClass, NewOwnerInteractObject);

	// Show or Hide Interact Widget
	if(LocalInteractWidget != InteractWidgetComp->GetWidget()) InteractWidgetComp->SetWidget(MoveTemp(LocalInteractWidget));
	
	InteractWidgetComp->SetTickMode(bIsShow ? ETickMode::Automatic : ETickMode::Disabled);

	// Attach or detach
	if(!bIsShow && NewInteractWidgetCompOwner.IsValid() && NewInteractWidgetCompOwner.Get())
		InteractWidgetComp->AttachToComponent(NewInteractWidgetCompOwner->GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	else InteractWidgetComp->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
}

bool UInteractWidgetManager::GetIsWidgetVisibleNow() const
{
	if(!InteractWidgetComp.IsValid() || !InteractWidgetComp.Get()) return false;
	return InteractWidgetComp->GetWidget() && !InteractWidgetComp->bHiddenInGame;
}

// @todo Fix always 
// Get debug string of interact widgets
FString UInteractWidgetManager::GetDebugInteractWidgetsString() const
{
	/*if(InteractWidgetsPool.IsEmpty()) return "No Interact Widgets in pool!!!";
	auto It = InteractWidgetsPool.CreateConstIterator();
	if(!It) return "No Valid Widgets Pool Iterator";
	
	uint32 LocalWidgetIndex = 0;
	FString LocalDebugString = "\n-----------------------------Interact Widgets Pool-----------------------------------\n";
	for (; It; ++It)
	{
		if(!It) continue;
		if(!It.Key() || !It.Value().Key || !It.Value().Value.Get() || !It.Value().Value.IsValid()) continue;
		
		LocalDebugString += FString::FromInt(LocalWidgetIndex++) + FString("  :  \nInteract Widget Class : ") + GetNameSafe(It.Key());
		LocalDebugString += FString("\n      Interact Widget Instance : ") + GetNameSafe(It.Value().Key);
		LocalDebugString += FString("\n      Interact Widget Owner Ref : ") + GetNameSafe(It.Value().Value.Get());
		LocalDebugString += "\n";
	}
	LocalDebugString += "\n--------------------------------------------------------------------------------------";

	return LocalDebugString;*/

	return "DEPRECATE DEBUG FROM INTERACT WIDGET MANAGER";
}