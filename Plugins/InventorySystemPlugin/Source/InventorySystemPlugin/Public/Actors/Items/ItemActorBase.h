// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventoryData/Weapon/WeaponData.h"
#include "Interfaces/InventoryInterface.h"
#include "GameFramework/Actor.h"
#include "Objects/InventoryItem.h"
#include "ItemActorBase.generated.h"

UCLASS(Abstract)
class INVENTORYSYSTEMPLUGIN_API AItemActorBase : public AActor, public IInventoryInterface
{
	GENERATED_BODY()

public:
	AItemActorBase();
	
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
		UStaticMeshComponent* Mesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
		USceneComponent* DefaultRoot = nullptr;
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void SetInventoryItemObject_Implementation(UInventoryItem* Item) override { Server_SetItemObject(Item); }
	virtual UInventoryItem* GetInventoryItemObject_Implementation() override { return GetItemObject(); }
	virtual void EnableSimulatePhysics_Implementation(const bool bEnable) override { Server_EnableCollision(bEnable); }
	virtual void SetAttachParent_Implementation(USceneComponent* AttachParent) override { AttachmentReplication.AttachComponent = AttachParent; }
	
	UFUNCTION(Server, Unreliable)
		void Server_SetItemObject(UInventoryItem* Item);
		void Server_SetItemObject_Implementation(UInventoryItem* Item) { ItemObject = Item; }
	UFUNCTION(Server, Unreliable)
		void Server_EnableCollision(const bool bEnable);

	//Getter
	/*Get Item Object (non interface method)*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item Info")
		FORCEINLINE UInventoryItem* GetItemObject() const { return ItemObject.Get(); }

protected:
	/*Base item info, fill it to make item parameters*/
	UPROPERTY(EditDefaultsOnly, Category = "Settings")
		UDataTable* ItemsData;
	UPROPERTY(EditDefaultsOnly, Category = "Settings", meta=(DataTable="ItemsData"))
		FCustomizedDataTableRowHandle ItemInfo;
	UPROPERTY(Replicated)
		TObjectPtr<UInventoryItem> ItemObject;

	virtual void OnRep_AttachmentReplication() override {}

private:
	//Clear empty components on actor init
	void ClearEmptyComponents() const;
	//Init item object and general info
	void InitItemObject();
};

