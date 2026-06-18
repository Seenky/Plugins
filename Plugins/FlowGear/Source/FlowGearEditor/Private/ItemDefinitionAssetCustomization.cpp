// Fill out your copyright notice in the Description page of Project Settings.

#include "ItemDefinitionAssetCustomization.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailGroup.h"
#include "PropertyCustomizationHelpers.h"
#include "Inventory/ItemDefinitionAsset.h"
#include "StructUtils/InstancedStruct.h" // Нужно для чтения FInstancedStruct

TSharedRef<IDetailCustomization> FItemDefinitionAssetCustomization::MakeInstance()
{
    return MakeShareable(new FItemDefinitionAssetCustomization());
}

void FItemDefinitionAssetCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
    DetailBuilder.HideCategory("Item Data");
    DetailBuilder.HideCategory("Input");
    DetailBuilder.HideCategory("Stacking");
    DetailBuilder.HideCategory("ItemDefinitionAsset");
    
    IDetailCategoryBuilder& IdentityCategory = DetailBuilder.EditCategory("Core Identity", FText::GetEmpty(), ECategoryPriority::Important);
    IdentityCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UItemDefinitionAsset, FriendlyName));
    IdentityCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UItemDefinitionAsset, Tag));
    
    IDetailCategoryBuilder& VisualsCategory = DetailBuilder.EditCategory("Visuals & Actors", FText::GetEmpty(), ECategoryPriority::TypeSpecific);
    VisualsCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UItemDefinitionAsset, Icon));
    VisualsCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UItemDefinitionAsset, DefaultPickupMesh));
    VisualsCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UItemDefinitionAsset, EquipItemActorClass));
    VisualsCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UItemDefinitionAsset, PickupItemActorClass));
    
    IDetailCategoryBuilder& StackingCategory = DetailBuilder.EditCategory("Inventory & Stacking", FText::GetEmpty(), ECategoryPriority::Default);
    StackingCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UItemDefinitionAsset, bIsStackable));
    StackingCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UItemDefinitionAsset, MaxStackSize));
    
    IDetailCategoryBuilder& InputCategory = DetailBuilder.EditCategory("Input Settings", FText::GetEmpty(), ECategoryPriority::Default);
    InputCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UItemDefinitionAsset, ItemInputMappingContext));
    InputCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UItemDefinitionAsset, InputContextPriority));
    InputCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UItemDefinitionAsset, InputBindings));
    
    IDetailCategoryBuilder& TraitsCategory = DetailBuilder.EditCategory("Item Traits", FText::GetEmpty(), ECategoryPriority::Important);
    TraitsCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UItemDefinitionAsset, ItemTraits));
}