// Florist Game. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractWidgetInfoInterface.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UInteractWidgetInfoInterface : public UInterface
{
	GENERATED_BODY()
	
};


class INTERACTSYSTEM_API IInteractWidgetInfoInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable,BlueprintNativeEvent, Category="Interact Interface Widget")
		const bool GetIsShowInteractWidget() const;
	UFUNCTION(BlueprintCallable,BlueprintNativeEvent, Category="Interact Interface Widget")
		void GetInteractWidgetLocation(FVector& WidgetLocation) const;
	UFUNCTION(BlueprintCallable,BlueprintNativeEvent, Category="Interact Interface Widget")
		void GetInteractWidget(TSubclassOf<UUserWidget>& InteractWidgetClass, UObject*& OwnerInteractObject);
};
