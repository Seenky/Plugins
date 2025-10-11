// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "StructUtils/InstancedStruct.h"
#include "InventoryData.generated.h"

class AFastenItemActor;
class UInventoryItem;

USTRUCT(BlueprintType)
struct FInventoryItemDataDescription
{
	GENERATED_BODY()

public:
	/*Item icon (for ui purposes)*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TSoftObjectPtr<UTexture2D> ItemIcon;

	/*Item name (for ui purposes)*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FText ItemName;

	/*Item description (for ui purposes)*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FText ItemDescription;

	FInventoryItemDataDescription()
		: ItemIcon(nullptr)
		, ItemName(FText::FromString("None"))
		, ItemDescription(FText::FromString("None"))
	{}
};

USTRUCT(BlueprintType)
struct FInventoryItemData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FInventoryItemDataDescription ItemDescription;

	/*Pickup class for item, it needs to select what item class need to be dropped*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TSubclassOf<AActor> ItemPickupClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TObjectPtr<UStaticMesh> LightweightMesh;

	/*Use class for item, it needs to select what item class need to be selected*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TSubclassOf<AActor> ItemUseClass;

	FInventoryItemData()
		: ItemDescription(FInventoryItemDataDescription{})
		, ItemPickupClass(nullptr)
		, LightweightMesh(nullptr)
		, ItemUseClass(nullptr)
	{}
};

USTRUCT(BlueprintType)
struct FItemTypeData
{
	GENERATED_BODY()

public:
	/*Item tag that inventory can hold*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FGameplayTag ItemTag;
	/*Max amount of items with selected tag in inventory*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		int32 MaxCount;
	/*Enable or disable item drop (note that if you pick up item with bCanBeDropped=false, you cant drop it anymore)*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool bCanBeDropped;
	/*Socket name for items with selected tags should attach*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FName AttachSocketName;
	/*Socket names for items with selected tags should attach when don't select (if none item won't attach)
	 * Each socket linked to item index
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<FName> FastenSocketNames;

	FItemTypeData()
		: ItemTag(FGameplayTag::EmptyTag)
		, MaxCount(0)
		, bCanBeDropped(true)
	{}
};

USTRUCT(BlueprintType)
struct FFastenItemsData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FGameplayTag ItemTag;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		int32 ItemIndex;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		AFastenItemActor* FastenItemActor;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool bIsFasten;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FName FastenSocketName;

	bool operator==(const FFastenItemsData& Other) const
	{
		return Other.ItemTag == ItemTag && Other.ItemIndex == ItemIndex &&
			   Other.FastenItemActor == FastenItemActor && Other.FastenSocketName == FastenSocketName;
	}

	FFastenItemsData()
		: ItemIndex(0)
		, FastenItemActor(nullptr)
		, bIsFasten(false)
	{
	}

	FFastenItemsData(const FGameplayTag& Tag, const int32& Index, AFastenItemActor* Actor, const FName& AttachSocket)
		: ItemTag(Tag)
		, ItemIndex(Index)
		, FastenItemActor(Actor)
		, bIsFasten(false)
		, FastenSocketName(AttachSocket)
	{
	}
};


USTRUCT(BlueprintType)
struct FPickupItemData
{
	GENERATED_BODY()

public:
	/*Runtime parameter, to check if item was used before drop*/
	UPROPERTY()
		bool bIsUsing;
	/*Enable or disable auto use item when we dropped it*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool bShouldUseOnDrop;
	/*Timer for dropped item use(isn't working yet)*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool bHasUsageTimer;
	/*Timer delay for dropped item use(isn't working yet)*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float UsageTimer;

	FPickupItemData()
		: bIsUsing(false)
		, bShouldUseOnDrop(false)
		, bHasUsageTimer(false)
		, UsageTimer(0.0f)
	{}
};

USTRUCT(BlueprintType)
struct FInventorySlot
{
	GENERATED_BODY()
	
	UPROPERTY()
	FGameplayTag SlotTag;
	
	UPROPERTY()
	int32 MaxCount = 1;
	
	UPROPERTY()
	TArray<UInventoryItem*> Items;

	bool IsFull() const { return Items.Num() >= MaxCount; }
	bool IsEmpty() const { return Items.Num() == 0; }
};

USTRUCT(BlueprintType)
struct FItems : public FTableRowBase
{
	GENERATED_BODY()

public:
	/*Base item info*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FInventoryItemData ItemData;
	/*Dynamic item info, you can select struct for this info*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FInstancedStruct DynamicItemData;
	/*Pickup info for dropped items*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FPickupItemData PickupItemData;
	/*Item tag that will be used for inventory slot select*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag ItemTag;
};

USTRUCT(BlueprintType)
struct FCustomizedDataTableRowHandle : public FDataTableRowHandle
{
	GENERATED_BODY()
};





