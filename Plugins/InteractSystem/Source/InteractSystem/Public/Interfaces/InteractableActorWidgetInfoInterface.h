// Florist Game. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractInfo.h"
#include "InteractableActorWidgetInfoInterface.generated.h"

class UInteractComponent;

UINTERFACE()
class UInteractableActorWidgetInfoInterface : public UInterface
{
	GENERATED_BODY()
	
};

class INTERACTSYSTEM_API IInteractableActorWidgetInfoInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable,BlueprintNativeEvent, Category="Interact Interface Widget")
		const bool GetIsShowInteractWidgetFromInteractable(const FInteractInputInfo& InteractInputInfo) const;
	UFUNCTION(BlueprintCallable,BlueprintNativeEvent, Category="Interact Interface Widget")
		void GetInteractWidgetLocationFromInteractable(FVector& WidgetLocation, const FInteractInputInfo& InteractInputInfo) const;
	
};
