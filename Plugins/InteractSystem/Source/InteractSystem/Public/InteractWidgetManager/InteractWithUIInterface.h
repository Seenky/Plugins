// Florist Game. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractWithUIInterface.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UInteractWithUIInterface : public UInterface
{
	GENERATED_BODY()
};


class INTERACTSYSTEM_API IInteractWithUIInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	// Owner Interact Object - object that owns the interact widget
	UFUNCTION(BlueprintNativeEvent, Category="Interact with UI")
		void SetupWidget(UObject* InteractOwner);
	// Call only on widget was created
	UFUNCTION(BlueprintNativeEvent, Category="Interact with UI")
		void ConstructWidget();
	// Clear previous interact owner info
	UFUNCTION(BlueprintNativeEvent, Category="Interact with UI")
		void ClearInfoOnChangeInteractOwner(UObject* OldInteractOwner);
};
