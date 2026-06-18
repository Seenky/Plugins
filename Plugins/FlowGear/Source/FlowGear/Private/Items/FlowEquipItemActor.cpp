// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/FlowEquipItemActor.h"


AFlowEquipItemActor::AFlowEquipItemActor()
{
	PrimaryActorTick.bCanEverTick = true;
	
	Scene = CreateDefaultSubobject<USceneComponent>(FName("Scene"));
	SetRootComponent(Scene);
	
	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>("StaticMesh");
	StaticMesh->SetupAttachment(Scene);
	
	SkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(FName("SkeletalMesh"));
	SkeletalMesh->SetupAttachment(Scene);
	
	SetActorEnableCollision(false);
}

void AFlowEquipItemActor::BeginPlay()
{
	Super::BeginPlay();
	
	if (!StaticMesh->GetStaticMesh())
	{
		StaticMesh->DestroyComponent();
	}
	
	if (!SkeletalMesh->GetSkeletalMeshAsset())
	{
		SkeletalMesh->DestroyComponent();
	}
}

void AFlowEquipItemActor::ConstructItem(UFlowItemInstance* InItemInstance)
{
	Super::ConstructItem(InItemInstance);
	
	ItemInstance = InItemInstance;
	
	SetActorEnableCollision(false);
}




