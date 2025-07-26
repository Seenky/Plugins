// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/Items/EquipItemActorBase.h"

#include "Misc/OutputDeviceNull.h"

AEquipItemActorBase::AEquipItemActorBase()
{
	PrimaryActorTick.bCanEverTick = true;
	
	SkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>("SkeletalMesh");
	SkeletalMesh->SetupAttachment(DefaultRoot);
	SkeletalMesh->SetIsReplicated(true);
}

void AEquipItemActorBase::InventoryItemInteract_Implementation(FGameplayTagContainer InteractTags, const bool bInteract)
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, "SHOOT");
	for (FGameplayTag Tag : InteractTags)
	{
		if (const FString* FunctionName = InteractByTag.Find(Tag))
		{
			FString FunctionCallString = *FunctionName + FString::Printf(TEXT(" %hd"), bInteract);
			FOutputDeviceNull OutputDeviceNull;
			const TCHAR* CmdAndParams = GetData(FunctionCallString);
			this->CallFunctionByNameWithArguments(CmdAndParams, OutputDeviceNull, nullptr, true);
		}
	}
}


