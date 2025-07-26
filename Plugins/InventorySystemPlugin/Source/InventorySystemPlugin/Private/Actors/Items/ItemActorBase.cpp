// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/Items/ItemActorBase.h"

#include "Net/UnrealNetwork.h"


AItemActorBase::AItemActorBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bReplicateUsingRegisteredSubObjectList = true;

	DefaultRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultRoot"));
	DefaultRoot->SetIsReplicated(false);
	SetRootComponent(DefaultRoot);
	
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>("Mesh");
	Mesh->SetupAttachment(DefaultRoot);
	Mesh->SetIsReplicated(true);
}

void AItemActorBase::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AItemActorBase, ItemObject);
}

void AItemActorBase::BeginPlay()
{
	Super::BeginPlay();

	InitItemObject();
}

void AItemActorBase::InitItemObject()
{
	if (HasAuthority() && !ItemObject)
	{
		if (!ItemInfo.DataTable || !ItemsData) return;

		if (const FItems* FindRow =
			ItemsData->FindRow<FItems>(ItemInfo.RowName, "FindItem", true))
		{
			ItemObject = NewObject<UInventoryItem>(this, UInventoryItem::StaticClass());
			ItemObject->Server_SetItemData(FindRow->ItemData);
			ItemObject->Server_SetDynamicItemData(FindRow->DynamicItemData);
			ItemObject->Server_SetPickupItemData(FindRow->PickupItemData);
			ItemObject->Server_SetItemTag(FindRow->ItemTag);
			AddReplicatedSubObject(ItemObject);
		}
	}
}

void AItemActorBase::Server_EnableCollision_Implementation(const bool bEnable)
{
	Mesh->bReplicatePhysicsToAutonomousProxy = true;
	Mesh->SetEnableGravity(bEnable);
	Mesh->SetSimulatePhysics(bEnable);
	Mesh->SetCollisionEnabled(bEnable ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
}

void AItemActorBase::ClearEmptyComponents() const
{
	TArray<UMeshComponent*> Components;
	GetComponents<UMeshComponent>(Components);
	for (auto Component : Components)
	{
		if (UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(Component))
		{
			if (!StaticMeshComponent->GetStaticMesh())
			{
				StaticMeshComponent->DestroyComponent();
				continue;
			}
		}
		
		if (USkeletalMeshComponent* SkeletonComponent = Cast<USkeletalMeshComponent>(Component))
		{
			if (!SkeletonComponent->GetSkeletalMeshAsset())
			{
				SkeletonComponent->DestroyComponent();
				continue;
			}
		}
	}
}








