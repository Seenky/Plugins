// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "FlowItemActionTypes.generated.h"

class UFlowItemAction;

USTRUCT()
struct FItemActionsTraitBase
{
    GENERATED_BODY()
};

USTRUCT(BlueprintType)
struct FFlowActionEntry
{
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action")
    FGameplayTagContainer TargetTags;
    
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = "Action", meta = (ShowOnlyInnerProperties))
    TArray<UFlowItemAction*> Actions;
    
    bool operator==(const FFlowActionEntry& Other) const
    {
        return TargetTags.HasAnyExact(Other.TargetTags);
    }
    
    bool operator==(const FGameplayTag Tag) const
    {
        return TargetTags.HasTagExact(Tag);
    }
};

USTRUCT(BlueprintType)
struct FItemInteractActionsMapTrait : public FItemActionsTraitBase
{
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actions", meta = (ShowOnlyInnerProperties))
    TArray<FFlowActionEntry> ActionEntries;
};

USTRUCT(BlueprintType)
struct FItemInteractActionsArrayTrait : public FItemActionsTraitBase
{
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = "Actions", meta = (ShowOnlyInnerProperties))
    TArray<UFlowItemAction*> Actions;
};

USTRUCT(BlueprintType)
struct FItemEquipActionsTrait : public FItemActionsTraitBase
{
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = "Actions", meta = (ShowOnlyInnerProperties))
    TArray<UFlowItemAction*> Actions;
};

USTRUCT(BlueprintType)
struct FItemUnequipActionsTrait : public FItemActionsTraitBase
{
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = "Actions", meta = (ShowOnlyInnerProperties))
    TArray<UFlowItemAction*> Actions;
};

USTRUCT(BlueprintType)
struct FItemDropActionsTrait : public FItemActionsTraitBase
{
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = "Actions", meta = (ShowOnlyInnerProperties))
    TArray<UFlowItemAction*> Actions;
};