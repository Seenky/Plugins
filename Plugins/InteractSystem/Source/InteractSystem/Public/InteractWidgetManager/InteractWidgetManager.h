// Florist Game. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/WidgetComponent.h"
#include "UObject/Object.h"
#include "InteractWidgetManager.generated.h"

class UWidgetComponent;


/*
 * Interace widget manager for spawn and contain interact widgets for items
 */
UCLASS()
class INTERACTSYSTEM_API UInteractWidgetManager : public UObject
{
	GENERATED_BODY()

public:
	
	// Setter
	
	/*
	 * Create Interact Widget
	 * Owner Interact Object - object that owns the interact widget
	 */
	UUserWidget* CreateOrGetInteractWidget(APlayerController* WidgetOwningObject, TSubclassOf<UUserWidget> InteractWidgetClass, UObject* OwnerInteractObject);

	// Set Interact Widget Comp
	void SetupInteractWidgetComp(TWeakObjectPtr<UWidgetComponent> NewInteractWidgetComp, TWeakObjectPtr<AActor> NewInteractWidgetCompOwner);
	
	/*
	 * Widget Owning Object - owning object for create widget
	 * Interact Widget Class - interact widget class
	 * Owner Interact Object - owner interact object for bindings logic
	 */
	void ToggleVisibilityWidget(APlayerController* NewWidgetOwningObject, TSubclassOf<UUserWidget> NewInteractWidgetClass,
		UObject* NewOwnerInteractObject, TWeakObjectPtr<AActor> NewInteractWidgetCompOwner, const bool bIsShow);
	
	void HideVisibilityWidget()
	{
		ToggleVisibilityWidget(nullptr, nullptr, nullptr,
			InteractWidgetCompOwner.Get(), false);
	}
	
	// Getter

	// Get Interact Widget
	FORCEINLINE TPair<UUserWidget*, TWeakObjectPtr<>> GetInteractWidget(TSubclassOf<UUserWidget> InteractWidgetClass) const
	{
		return InteractWidgetsPool.FindRef(InteractWidgetClass);
	}

	// Get Interact Widget
	FORCEINLINE TWeakObjectPtr<UWidgetComponent> GetInteractWidgetComp() const
	{
		return InteractWidgetComp;
	}

	bool GetIsWidgetVisibleNow() const;
	
	// Get debug string of interact widgets
	FString GetDebugInteractWidgetsString() const;
	
private:

	// Functions

	// Add or Remove Widget
	FORCEINLINE void ToggleInteractWidget(UUserWidget* InteractWidget, TWeakObjectPtr<> OwnerInteractObject, const bool bIsAdd)
	{
		if(bIsAdd) InteractWidgetsPool.Add(InteractWidget->GetClass(), TPair<UUserWidget*, TWeakObjectPtr<>>(InteractWidget, OwnerInteractObject));
		else InteractWidgetsPool.Remove(InteractWidget->GetClass());
	}

	
	// Variables
	
	/*
	 * Interact Widgets Pool
	 * Key: User Widget Subclass
	 * Value: {UUserWidget*, CurrentInteractOwnerObject}
	 */
	TMap<TSubclassOf<UUserWidget>, TPair<UUserWidget*, TWeakObjectPtr<>>> InteractWidgetsPool = {};
	// Interact Widget Comp
	TWeakObjectPtr<UWidgetComponent> InteractWidgetComp = nullptr;
	TWeakObjectPtr<AActor> InteractWidgetCompOwner = nullptr;
};
