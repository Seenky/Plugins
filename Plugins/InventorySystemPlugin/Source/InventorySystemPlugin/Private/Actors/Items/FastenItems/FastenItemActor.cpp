// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/Items/FastenItems/FastenItemActor.h"


AFastenItemActor::AFastenItemActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	DefaultRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultRoot"));
	DefaultRoot->SetIsReplicated(false);
	SetRootComponent(DefaultRoot);
	
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>("Mesh");
	Mesh->SetupAttachment(DefaultRoot);
	Mesh->SetIsReplicated(true);

	SetActorEnableCollision(false);
}


