// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/FlowItemInstance.h"
#include "GameplayTagContainer.h"
#include "Blueprint/BlueprintExceptionInfo.h"
#include "Inventory/ItemDefinitionAsset.h"

#define LOCTEXT_NAMESPACE "FlowItemInstance"

void UFlowItemInstance::SetDefinitionAsset(UItemDefinitionAsset* InDefinitionAsset)
{
	if (DefinitionAsset) return;
	DefinitionAsset = InDefinitionAsset;
	AddItemTraits(DefinitionAsset->ItemTraits);
}

void UFlowItemInstance::AddItemTraits(const TArray<FInstancedStruct>& InTraits, const bool bAppend, const bool bReplace)
{
	if (bAppend)
	{
		for (const FInstancedStruct& Trait : InTraits)
		{
			AddItemTraitInternal(Trait, bReplace);
		}
	}
	else
	{
		ItemTraits = InTraits;
		
		TypeToTrait.Empty();
		for (int32 i = 0; i < ItemTraits.Num(); ++i)
		{
			if (const UScriptStruct* SS = ItemTraits[i].GetScriptStruct())
			{
				TypeToTrait.Add(SS, i);
			}
		}
	}
}

DEFINE_FUNCTION(UFlowItemInstance::execAddItemTrait)
{
	Stack.StepCompiledIn<FStructProperty>(nullptr);
	FStructProperty* Prop = CastField<FStructProperty>(Stack.MostRecentProperty);
	void* SrcPtr = Stack.MostRecentPropertyAddress;
	
	Stack.StepCompiledIn<FBoolProperty>(nullptr);
	bool bReplace = *(bool*)Stack.MostRecentPropertyAddress;

	P_FINISH;

	if (!Prop || !Prop->Struct)
	{
		return;
	}

	P_NATIVE_BEGIN
	{
		UE_LOG(LogTemp, Warning, TEXT("ADD TRAIT"));
		
		const uint8* DataPtr = static_cast<const uint8*>(SrcPtr);

		FInstancedStruct InstancedTrait;
		InstancedTrait.InitializeAs(Prop->Struct, DataPtr);

		P_THIS->AddItemTraitInternal(InstancedTrait, bReplace);
	}
	P_NATIVE_END
}

DEFINE_FUNCTION(UFlowItemInstance::execGetItemTrait)
{
	Stack.StepCompiledIn<FStructProperty>(nullptr);
	FStructProperty* ValueProp = CastField<FStructProperty>(Stack.MostRecentProperty);
	void* ValuePtr = Stack.MostRecentPropertyAddress;
    
	P_FINISH;
    
	if (!ValueProp || !ValuePtr)
	{
		const FBlueprintExceptionInfo ExceptionInfo(
		   EBlueprintExceptionType::AccessViolation,
		   LOCTEXT("GetItemTrait_MissingOutputProperty", "Failed to resolve the output parameter for GetItemTrait.")
		);
		FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
		*StaticCast<bool*>(RESULT_PARAM) = false;
		return;
	}
    
	bool bResult = false;
    
	P_NATIVE_BEGIN
	   if (FInstancedStruct* FoundTrait = P_THIS->GetTraitPtr(ValueProp->Struct))
	   {
	   		ValueProp->CopySingleValue(ValuePtr, FoundTrait->GetMemory());
	   		bResult = true;
	   }
	P_NATIVE_END
	
	*StaticCast<bool*>(RESULT_PARAM) = bResult;
}

void UFlowItemInstance::AddDynamicState(const FGameplayTag DynamicStateTag)
{
	DynamicStateTags.AddTag(DynamicStateTag);
}

void UFlowItemInstance::RemoveDynamicState(const FGameplayTag DynamicStateTag)
{
	DynamicStateTags.RemoveTag(DynamicStateTag);
}

bool UFlowItemInstance::ContainsDynamicState(const FGameplayTag DynamicStateTag) const
{
	return DynamicStateTags.HasTagExact(DynamicStateTag);
}

bool UFlowItemInstance::HasAnyDynamicStates(const FGameplayTagContainer DynamicStateTag) const
{
	return DynamicStateTags.HasAny(DynamicStateTag);
}

void UFlowItemInstance::AddItemTraitInternal(const FInstancedStruct& InTrait, const bool bReplace)
{
	const UScriptStruct* TargetStruct = InTrait.GetScriptStruct();
	if (!TargetStruct) return;

	FInstancedStruct* TraitPtr = GetTraitPtr(TargetStruct);
	if (TraitPtr && !bReplace) return;
    
	if (TraitPtr)
	{
		*TraitPtr = InTrait;
	}
	else
	{
		int32 NewIndex = ItemTraits.Add(InTrait);
		TypeToTrait.Add(TargetStruct, NewIndex);
	}
}

FInstancedStruct* UFlowItemInstance::GetTraitPtr(const UScriptStruct* InTrait)
{
	if (!InTrait) return nullptr;
	
	if (const uint32* Index = TypeToTrait.Find(InTrait))
	{
		if (ItemTraits.IsValidIndex(*Index))
		{
			return &ItemTraits[*Index];
		}
	}
	return nullptr;
}

const FInstancedStruct* UFlowItemInstance::GetTraitPtr(const UScriptStruct* InTrait) const
{
	return const_cast<UFlowItemInstance*>(this)->GetTraitPtr(InTrait);
}

FGameplayTag UFlowItemInstance::GetItemTag() const
{
	if (!DefinitionAsset) return FGameplayTag::EmptyTag;
	
	return DefinitionAsset->Tag;
}

FText UFlowItemInstance::GetItemName() const
{
	if (!DefinitionAsset) return FText::GetEmpty();
	
	return DefinitionAsset->FriendlyName;
}

TSoftObjectPtr<UTexture2D> UFlowItemInstance::GetItemIcon() const
{
	if (!DefinitionAsset) return nullptr;
	return DefinitionAsset->Icon;
}

TSoftClassPtr<AFlowEquipItemActor> UFlowItemInstance::GetItemActorClass() const
{
	if (!DefinitionAsset) return nullptr;
	return DefinitionAsset->EquipItemActorClass;
}

TSoftClassPtr<AFlowPickupActor> UFlowItemInstance::GetPickupActorClass() const
{
	if (!DefinitionAsset) return nullptr;
	return DefinitionAsset->PickupItemActorClass;
}

TSoftObjectPtr<UStaticMesh> UFlowItemInstance::GetDefaultPickupMesh() const
{
	if (!DefinitionAsset) return nullptr;
	return DefinitionAsset->DefaultPickupMesh;
}

TSoftObjectPtr<UInputMappingContext> UFlowItemInstance::GetInputMappingContext() const
{
	if (!DefinitionAsset) return nullptr;
	return DefinitionAsset->ItemInputMappingContext;
}

int32 UFlowItemInstance::GetInputPriority() const
{
	if (!DefinitionAsset) return -1;
	return DefinitionAsset->InputContextPriority;
}

TMap<class UInputAction*, FGameplayTag> UFlowItemInstance::GetInputBindings() const
{
	if (!DefinitionAsset) return {};
	return DefinitionAsset->InputBindings;
}

bool UFlowItemInstance::IsStackable() const
{
	if (!DefinitionAsset) return false;
	return DefinitionAsset->bIsStackable;
}

int32 UFlowItemInstance::GetStackSize() const
{
	if (!DefinitionAsset) return INDEX_NONE;
	return DefinitionAsset->MaxStackSize;
}

#undef LOCTEXT_NAMESPACE