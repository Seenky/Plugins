// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Objects/InventoryItem.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class INVENTORYSYSTEMPLUGIN_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(Server, Reliable)
	virtual void FirstInitComponent();
	UFUNCTION(Server, Reliable)
	virtual void InitStartItems();
	
	//Delegates
	UPROPERTY(BlueprintAssignable, Category = "Inventory delegates")
		FOnInventoryChanged OnInventoryChanged;
	
	//Server
	/**Server Event. Add item object to inventory (if it has slots)*/
	UFUNCTION(Server, Unreliable, WithValidation, BlueprintCallable, Category = "Inventory")
		void Server_AddItem(UInventoryItem* Item);
		bool Server_AddItem_Validate(UInventoryItem* Item) { return true; }
	/**Server Event. Select item by tag (if inventory has one)*/
	UFUNCTION(Server, Unreliable, WithValidation, BlueprintCallable, Category = "Inventory")
		void Server_SelectItem(const FGameplayTag ItemTag, const int32 ItemIndex);
		bool Server_SelectItem_Validate(const FGameplayTag ItemTag, const int32 ItemIndex) { return true; }
	/**Server Event. Drop selected item (if it can be dropped)*/
	UFUNCTION(Server, Unreliable, WithValidation, BlueprintCallable, Category = "Inventory")
		void Server_DropSelectedItem();
		bool Server_DropSelectedItem_Validate() { return true; }
	/**Server Event. Deselect selected item (move back to inventory)*/
	UFUNCTION(Server, Unreliable, WithValidation, BlueprintCallable, Category = "Inventory")
		void Server_DeselectSelectedItem();
		bool Server_DeselectSelectedItem_Validate() { return true; }

	//Client
	/**Get item actor that selected now*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory")
		AActor* GetCurrentSelectedItemActor() { return CurrentSelectedItemActor.Get(); }
	/**Get item object that selected now*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory")
		UInventoryItem* GetCurrentSelectedItem() { return CurrentSelectedItem; }
	/**Return list of items in inventory*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory")
		TArray<UInventoryItem*> GetAllInventoryItems();

	//Debug
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Debug")
		FString GetCurrentSelectedItemActorName() const { return GetNameSafe(CurrentSelectedItemActor.Get()); }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Debug")
		FString GetCurrentSelectedItemName() const
	{
		if (CurrentSelectedItem) return CurrentSelectedItem->GetItemData().ItemDescription.ItemName.ToString();
		else return FString();
	}

protected:
	virtual void BeginPlay() override;

	/**Set amount of items and their tags that inventory can contain*/
	UPROPERTY(Replicated, EditAnywhere, Category = "Settings")
		TArray<FItemTypeData> ItemTypes;
	/**List of start item, they are create and add on component init*/
	UPROPERTY(Replicated, EditAnywhere, Category = "Settings|Start Items", meta=(DataTable="ItemsData"))
		TArray<FCustomizedDataTableRowHandle> StartItems;
	/**Items data table, to easily select start items*/
	UPROPERTY(Replicated, EditAnywhere, Category = "Settings|Start Items")
		UDataTable* ItemsData;

	UPROPERTY(Replicated)
		TArray<FInventorySlot> InventorySlots;
	UPROPERTY(Replicated)
		TObjectPtr<UInventoryItem> CurrentSelectedItem;
	UPROPERTY(ReplicatedUsing = OnRep_AttachItem)
		TWeakObjectPtr<AActor> CurrentSelectedItemActor;
	UPROPERTY(Replicated)
		FName CurrentSocketName;

private:
	
	/**Inventory handle helpers*/
	FItemTypeData GetItemTypeByTag(const FGameplayTag ItemTag);
	int32 FindSuitableSlot(const FGameplayTag& ItemTag);
	int32 FindSlotByTag(const FGameplayTag ItemTag);
	UInventoryItem* GetItemByTagAndIndex(const FGameplayTag ItemTag, const int32 ItemIndex);
	bool RemoveItem(UInventoryItem* Item);

	/**Item handle helpers*/
	void CleanupOldItem(UInventoryItem* Item, AActor* Owner);
	UInventoryItem* HandleNewItem(const UInventoryItem* Item, AActor* NewOwner);
	void CleanupCurrentItem();
	
	UFUNCTION()
	void OnRep_AttachItem();

	FAttachmentTransformRules AttachItemRules = FAttachmentTransformRules(
		EAttachmentRule::SnapToTarget,
		EAttachmentRule::SnapToTarget,
		EAttachmentRule::KeepRelative,
		false);

	bool bIsInventoryLocked;
};


