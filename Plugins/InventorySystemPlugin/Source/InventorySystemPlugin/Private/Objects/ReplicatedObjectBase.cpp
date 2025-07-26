// Fill out your copyright notice in the Description page of Project Settings.


#include "Objects/ReplicatedObjectBase.h"

#include "Net/UnrealNetwork.h"

void UReplicatedObjectBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	UObject::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(UReplicatedObjectBase, Owner);
}

int32 UReplicatedObjectBase::GetFunctionCallspace(UFunction* Function, FFrame* Stack)
{
	if (AActor* MyOwner = GetOwner())
	{
		return MyOwner->GetFunctionCallspace(Function, Stack);
	}
	return FunctionCallspace::Local;
}

bool UReplicatedObjectBase::CallRemoteFunction(UFunction* Function, void* Parms, struct FOutParmRec* OutParms, FFrame* Stack)
{
	bool bProcessed = false;

	if (AActor* MyOwner = GetOwner())
	{
		FWorldContext* const Context = GEngine->GetWorldContextFromWorld(GetWorld());
		if (Context != nullptr)
		{
			for (FNamedNetDriver& Driver : Context->ActiveNetDrivers)
			{
				if (Driver.NetDriver != nullptr && Driver.NetDriver->ShouldReplicateFunction(MyOwner, Function))
				{
					Driver.NetDriver->ProcessRemoteFunction(MyOwner, Function, Parms, OutParms, Stack, this);
					bProcessed = true;
				}
			}
		}
	}

	return bProcessed;
}