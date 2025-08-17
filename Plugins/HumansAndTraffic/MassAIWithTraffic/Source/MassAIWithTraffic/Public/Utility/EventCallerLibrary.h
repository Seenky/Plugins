// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EventCallerLibrary.generated.h"


UCLASS()
class MASSAIWITHTRAFFIC_API UEventCallerLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	//Template functions
	template <typename InterfaceType, class SpecifiedClass = UObject>
	static void CallInterfaceMethodByName(UWorld* WorldContext, FName MethodName)
	{
		const FString ExecuteFunctionName = *MethodName.ToString();
		const FName ExecuteFName(*ExecuteFunctionName);
		
		const UClass* InterfaceClass = InterfaceType::UClassType::StaticClass();
		
		for (TObjectIterator<SpecifiedClass> ObjectItr; ObjectItr; ++ObjectItr)
		{
			UObject* Object = *ObjectItr;
			
			if (Object && Object->GetWorld() == WorldContext && Object->GetClass()->ImplementsInterface(InterfaceClass))
			{
				UFunction* Function = Object->FindFunction(ExecuteFName);
				if (Function)
				{
					Object->ProcessEvent(Function, nullptr);
				}
			}
		}
	}
};
