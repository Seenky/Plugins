// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/InventoryInterface.h"
#include "FastenItemActor.generated.h"

UCLASS()
class INVENTORYSYSTEMPLUGIN_API AFastenItemActor : public AActor, public IInventoryInterface
{
	GENERATED_BODY()

public:
	AFastenItemActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
		UStaticMeshComponent* Mesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
		USceneComponent* DefaultRoot = nullptr;

	virtual void SetAttachParent_Implementation(USceneComponent* AttachParent) override { AttachmentReplication.AttachComponent = AttachParent; }

	UStaticMeshComponent* GetStaticMeshComponent() const { return Mesh; }
	void SetStaticMesh(UStaticMesh* StaticMesh) const { Mesh->SetStaticMesh(StaticMesh); }
};
