// Fill out your copyright notice in the Description page of Project Settings.


#include "PropertyCustomization.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailGroup.h"
#include "ObjectEditorUtils.h"
#include "AssetRegistry/AssetData.h"
#include "Containers/Map.h"
#include "DataTableEditorUtils.h"
#include "Delegates/Delegate.h"
#include "DetailWidgetRow.h"
#include "Editor.h"
#include "IPropertyUtilities.h"
#include "Engine/DataTable.h"
#include "Fonts/SlateFontInfo.h"
#include "Framework/Commands/UIAction.h"
#include "HAL/Platform.h"
#include "HAL/PlatformCrt.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/Text.h"
#include "Misc/Attribute.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyEditorModule.h"
#include "PropertyHandle.h"
#include "Templates/Casts.h"
#include "UObject/Class.h"
#include "UObject/Object.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Text/STextBlock.h"

class SToolTip;

class IDetailPropertyRow;
class SWidget;
class UPackage;

#define LOCTEXT_NAMESPACE "ShowPropertyCustomization"

const FName ShowPropertyCategoriesName("ShowPropertyCategories");
const FName ShowOnlyInnerPropertiesName("ShowOnlyInnerProperties");

struct FCategoryOrGroup
{
	IDetailCategoryBuilder* Category;
	IDetailGroup* Group;

	FCategoryOrGroup(IDetailCategoryBuilder& NewCategory)
		: Category(&NewCategory)
		, Group(nullptr)
	{}

	FCategoryOrGroup(IDetailGroup& NewGroup)
		: Category(nullptr)
		, Group(&NewGroup)
	{}

	FCategoryOrGroup()
		: Category(nullptr)
		, Group(nullptr)
	{}

	IDetailPropertyRow& AddProperty(TSharedRef<IPropertyHandle> PropertyHandle)
	{
		if (Category)
		{
			return Category->AddProperty(PropertyHandle);
		}
		else
		{
			return Group->AddPropertyRow(PropertyHandle);
		}
	}

	IDetailGroup& AddGroup(FName GroupName, const FText& DisplayName)
	{
		if (Category)
		{
			return Category->AddGroup(GroupName, DisplayName);
		}
		else
		{
			return Group->AddGroup(GroupName, DisplayName);
		}
	}

	bool IsValid() const
	{
		return Group || Category;
	}
};


struct FStructPropertyCustomizationGroup
{
	FString RawGroupName;
	FString DisplayName;
	FCategoryOrGroup RootCategory;
	TArray<TSharedPtr<IPropertyHandle>> SimplePropertyHandles;
	TArray<TSharedPtr<IPropertyHandle>> AdvancedPropertyHandles;

	bool IsValid() const
	{
		return !RawGroupName.IsEmpty() && !DisplayName.IsEmpty() && RootCategory.IsValid();
	}

	FStructPropertyCustomizationGroup()
		: RootCategory()
	{}
};

struct FGroupNode
{
	FString Name;
	TMap<FString, TSharedPtr<FGroupNode>> Children;
	TArray<TSharedPtr<FGroupNode>> ChildArray;
	TArray<TSharedPtr<IPropertyHandle>> SimplePropertyHandles;
	TArray<TSharedPtr<IPropertyHandle>> AdvancedPropertyHandles;

	TSharedPtr<FGroupNode> FindOrAddChildNode(const FString& InChildName)
	{
		if (TSharedPtr<FGroupNode>* ChildNode = Children.Find(InChildName))
		{
			return *ChildNode;
		}

		TSharedPtr<FGroupNode> NewNode = MakeShared<FGroupNode>();
		NewNode->Name = InChildName;
		Children.Add(InChildName, NewNode);
		ChildArray.Add(NewNode);
		return NewNode;
	}
};

void FStructPropertyCustomization::CustomizeChildren(
	TSharedRef<IPropertyHandle> StructPropertyHandle,
	class IDetailChildrenBuilder& StructBuilder,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
    uint32 NumChildren = 0;
    FPropertyAccess::Result Result = StructPropertyHandle->GetNumChildren(NumChildren);

    IDetailLayoutBuilder& LayoutBuilder = StructBuilder.GetParentCategory().GetParentLayout();
    TMap<FString, FCategoryOrGroup> NameToCategoryBuilderMap;
    TMap<FString, TSharedPtr<FGroupNode>> RootNodes;

    bool bShowPropertyCategories = StructPropertyHandle->HasMetaData(ShowPropertyCategoriesName);

    if (Result == FPropertyAccess::Success && NumChildren > 0)
    {
        for (uint32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex)
        {
            TSharedPtr<IPropertyHandle> ChildHandle = StructPropertyHandle->GetChildHandle(ChildIndex);
            if (ChildHandle.IsValid() && ChildHandle->GetProperty())
            {
                FProperty* Property = ChildHandle->GetProperty();
                FName CategoryFName = FObjectEditorUtils::GetCategoryFName(Property);
                FString RawCategoryName = CategoryFName.IsNone() ? TEXT("General") : CategoryFName.ToString();

                TArray<FString> CategoryParts;
                RawCategoryName.ParseIntoArray(CategoryParts, TEXT("|"), true);

                FString RootCategoryName = CategoryParts.Num() > 0 ? CategoryParts[0] : RawCategoryName;
            	
                FCategoryOrGroup* RootCategory = NameToCategoryBuilderMap.Find(RootCategoryName);
                if (!RootCategory)
                {
                    if (bShowPropertyCategories)
                    {
                        IDetailCategoryBuilder& NewCategory = LayoutBuilder.EditCategory(*RootCategoryName, FText::GetEmpty(), ECategoryPriority::TypeSpecific);
                        RootCategory = &NameToCategoryBuilderMap.Add(RootCategoryName, FCategoryOrGroup(NewCategory));
                    }
                    else
                    {
                        IDetailGroup& NewGroup = StructBuilder.AddGroup(*RootCategoryName, FText::FromString(RootCategoryName));
                        RootCategory = &NameToCategoryBuilderMap.Add(RootCategoryName, FCategoryOrGroup(NewGroup));
                    }
                }
            	
                TSharedPtr<FGroupNode> RootNode = RootNodes.FindRef(RootCategoryName);
                if (!RootNode)
                {
                    RootNode = MakeShared<FGroupNode>();
                    RootNode->Name = RootCategoryName;
                    RootNodes.Add(RootCategoryName, RootNode);
                }
            	
                FGroupNode* CurrentNode = RootNode.Get();
                for (int32 i = 1; i < CategoryParts.Num(); ++i)
                {
                    FString GroupName = CategoryParts[i].TrimStartAndEnd();
                    CurrentNode = CurrentNode->FindOrAddChildNode(GroupName).Get();
                }
            	
                if (Property->HasAnyPropertyFlags(CPF_AdvancedDisplay))
                {
                    CurrentNode->AdvancedPropertyHandles.Add(ChildHandle);
                }
                else
                {
                    CurrentNode->SimplePropertyHandles.Add(ChildHandle);
                }
            }
        }
    	
        auto ProcessGroupNode = [](FGroupNode& Node, FCategoryOrGroup& ParentCategoryOrGroup, const FString& FullPath, auto&& Self) -> void
        {
            FCategoryOrGroup NodeCategoryOrGroup = (Node.Name == FullPath) ? 
                ParentCategoryOrGroup : 
                FCategoryOrGroup(ParentCategoryOrGroup.AddGroup(*FullPath, FText::FromString(Node.Name)));
        	
            for (auto& SimpleProperty : Node.SimplePropertyHandles)
            {
                NodeCategoryOrGroup.AddProperty(SimpleProperty.ToSharedRef());
            }
        	
            for (auto& ChildNode : Node.ChildArray)
            {
                FString ChildFullPath = FullPath + TEXT("|") + ChildNode->Name;
                Self(*ChildNode, NodeCategoryOrGroup, ChildFullPath, Self);
            }
        	
            if (Node.AdvancedPropertyHandles.Num() > 0)
            {
                FString AdvancedGroupName = FullPath + TEXT("|Advanced");
                IDetailGroup& AdvancedGroup = NodeCategoryOrGroup.AddGroup(*AdvancedGroupName, LOCTEXT("AdvancedGroup", "Advanced"));
                for (auto& AdvancedProperty : Node.AdvancedPropertyHandles)
                {
                    AdvancedGroup.AddPropertyRow(AdvancedProperty.ToSharedRef());
                }
            }
        };
    	
        for (auto& RootNodePair : RootNodes)
        {
            FString RootCategoryName = RootNodePair.Key;
            TSharedPtr<FGroupNode> RootNode = RootNodePair.Value;
            if (FCategoryOrGroup* RootCategory = NameToCategoryBuilderMap.Find(RootCategoryName))
            {
                ProcessGroupNode(*RootNode, *RootCategory, RootCategoryName, ProcessGroupNode);
            }
        }
    }
}

void FStructPropertyCustomization::CustomizeHeader( TSharedRef<IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils )
{
	bool bShowHeader = !StructPropertyHandle->HasMetaData(ShowPropertyCategoriesName) && !StructPropertyHandle->HasMetaData(ShowOnlyInnerPropertiesName);
	if(bShowHeader)
	{
		HeaderRow.NameContent()
		[
			StructPropertyHandle->CreatePropertyNameWidget()
		];

		HeaderRow.ValueContent()
		[
			StructPropertyHandle->CreatePropertyValueWidget()
		];
	}
}
#undef LOCTEXT_NAMESPACE

#define LOCTEXT_NAMESPACE "FSsFixedDataTableCustomisationLayout"

void FDataTablePropertyCustomization::CustomizeHeader(TSharedRef<class IPropertyHandle> InStructPropertyHandle,
	class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	DataTablePropertyHandle = InStructPropertyHandle->GetChildHandle("DataTable");
	RowNamePropertyHandle = InStructPropertyHandle->GetChildHandle("RowName");

	Utils = StructCustomizationUtils.GetPropertyUtilities();

	FString DataTablePropertyName;
	if (InStructPropertyHandle->HasMetaData(TEXT("DataTable")))
	{
		DataTablePropertyName = InStructPropertyHandle->GetMetaData(TEXT("DataTable"));
	}

	if (!DataTablePropertyName.IsEmpty())
	{
		TArray<UObject*> OuterObjects;
		InStructPropertyHandle->GetOuterObjects(OuterObjects);

		for (UObject* OuterObj : OuterObjects)
		{
			if (FProperty* Property = OuterObj->GetClass()->FindPropertyByName(*DataTablePropertyName))
			{
				UDataTable** DataTablePtr = Property->ContainerPtrToValuePtr<UDataTable*>(OuterObj);
				if (DataTablePtr)
				{
					UDataTable* CurrentDataTable = *DataTablePtr;
					UObject* OutDataTable;
					DataTablePropertyHandle->GetValue(OutDataTable);

					const UDataTable* PrevDataTable = Cast<UDataTable>(OutDataTable);

					if (CurrentDataTable != PrevDataTable)
					{
						// Set new DataTable
						DataTablePropertyHandle->SetValue(CurrentDataTable);

						if (CurrentDataTable)
						{
							// Get row names from the DataTable
							TArray<FName> RowNames = CurrentDataTable->GetRowNames();
							if (RowNames.Num() > 0)
							{
								// Set the first row name
								RowNamePropertyHandle->SetValue(RowNames[0]);
							}
							else
							{
								// Handle no row names (empty DataTable)
								RowNamePropertyHandle->SetValue(NAME_None);
							}
						}
						else
						{
							RowNamePropertyHandle->SetValue(NAME_None);
						}
 
						// Force update/refresh the RowName property to ensure it updates immediately
						RowNamePropertyHandle->NotifyFinishedChangingProperties();
					}
				}
			}
		}

		const TSharedPtr<IPropertyHandle> DataTableSourceHandle = 
			InStructPropertyHandle->GetParentHandle()->GetChildHandle(FName(*DataTablePropertyName));

		if (DataTableSourceHandle.IsValid())
		{
			DataTableSourceHandle->SetOnPropertyValueChanged(
				FSimpleDelegate::CreateSP(this, &FDataTablePropertyCustomization::OnDataTableSourceChanged)
			);
		}
	}

	FPropertyComboBoxArgs ComboArgs(RowNamePropertyHandle, 
			FOnGetPropertyComboBoxStrings::CreateSP(this, &FDataTablePropertyCustomization::OnGetRowStrings), 
			FOnGetPropertyComboBoxValue::CreateSP(this, &FDataTablePropertyCustomization::OnGetRowValueString));
	ComboArgs.ShowSearchForItemCount = 1;

	TSharedRef<SWidget> BrowseTableButton = PropertyCustomizationHelpers::MakeBrowseButton(
		FSimpleDelegate::CreateSP(this, &FDataTablePropertyCustomization::BrowseTableButtonClicked),
		LOCTEXT("SsBrowseToDatatable", "Browse to DataTable in Content Browser"));

	HeaderRow
		.NameContent()
		[InStructPropertyHandle->CreatePropertyNameWidget()]
		.ValueContent()
		.MaxDesiredWidth(0.0f) // don't constrain the combo button width
		[ 
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				PropertyCustomizationHelpers::MakePropertyComboBox(ComboArgs)
			]
			+ SHorizontalBox::Slot()
			.Padding(2.0f)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.AutoWidth()
			[
				BrowseTableButton
			]
		];

	FDataTableEditorUtils::AddSearchForReferencesContextMenu(
		HeaderRow,
		FExecuteAction::CreateSP(this, &FDataTablePropertyCustomization::OnSearchForReferences));
}

bool FDataTablePropertyCustomization::GetCurrentValue(UDataTable*& OutDataTable, FName& OutName) const
{
	if (RowNamePropertyHandle.IsValid() && RowNamePropertyHandle->IsValidHandle() && DataTablePropertyHandle.IsValid() && DataTablePropertyHandle->IsValidHandle())
	{
		// If either handle is multiple value or failure, fail
		UObject* SourceDataTable = nullptr;
		if (DataTablePropertyHandle->GetValue(SourceDataTable) == FPropertyAccess::Success)
		{
			OutDataTable = Cast<UDataTable>(SourceDataTable);

			if (RowNamePropertyHandle->GetValue(OutName) == FPropertyAccess::Success)
			{
				return true;
			}
		}
	}
	return false;
}

void FDataTablePropertyCustomization::OnSearchForReferences()
{
	UDataTable* DataTable;
	FName RowName;

	if (GetCurrentValue(DataTable, RowName) && DataTable)
	{
		TArray<FAssetIdentifier> AssetIdentifiers;
		AssetIdentifiers.Add(FAssetIdentifier(DataTable, RowName));

		FEditorDelegates::OnOpenReferenceViewer.Broadcast(AssetIdentifiers, FReferenceViewerParams());
	}
}

void FDataTablePropertyCustomization::OnGetRowStrings(TArray<TSharedPtr<FString>>& OutStrings,
	TArray<TSharedPtr<SToolTip>>& OutToolTips, TArray<bool>& OutRestrictedItems) const
{
	UDataTable* DataTable = nullptr;
	FName IgnoredRowName;

	// Ignore return value as we will show rows if table is the same but row names are multiple values
	GetCurrentValue(DataTable, IgnoredRowName);

	TArray<FName> AllRowNames;
	if (DataTable != nullptr)
	{
		for (TMap<FName, uint8*>::TConstIterator Iterator(DataTable->GetRowMap()); Iterator; ++Iterator)
		{
			AllRowNames.Add(Iterator.Key());
		}

		// Sort the names alphabetically.
		AllRowNames.Sort(FNameLexicalLess());
	}

	for (const FName& RowName : AllRowNames)
	{
		OutStrings.Add(MakeShared<FString>(RowName.ToString()));
		OutRestrictedItems.Add(false);
	}
}

FString FDataTablePropertyCustomization::OnGetRowValueString() const
{
	if (!RowNamePropertyHandle.IsValid() || !RowNamePropertyHandle->IsValidHandle())
	{
		return FString();
	}

	FName RowNameValue;
	const FPropertyAccess::Result RowResult = RowNamePropertyHandle->GetValue(RowNameValue);
	if (RowResult == FPropertyAccess::Success)
	{
		if (RowNameValue.IsNone())
		{
			return LOCTEXT("DataTable_None", "None").ToString();
		}
		return RowNameValue.ToString();
	}
	else if (RowResult == FPropertyAccess::Fail)
	{
		return LOCTEXT("DataTable_None", "None").ToString();
	}
	else
	{
		return LOCTEXT("MultipleValues", "Multiple Values").ToString();
	}
}

void FDataTablePropertyCustomization::BrowseTableButtonClicked()
{
	if (DataTablePropertyHandle.IsValid())
	{
		UObject* SourceDataTable = nullptr;
		if (DataTablePropertyHandle->GetValue(SourceDataTable) == FPropertyAccess::Success)
		{
			TArray<FAssetData> Assets;
			Assets.Add(SourceDataTable);
			GEditor->SyncBrowserToObjects(Assets);
		}
	}	
}

void FDataTablePropertyCustomization::OnDataTableSourceChanged()
{
	if (Utils) Utils->RequestForceRefresh();
}
#undef LOCTEXT_NAMESPACE
