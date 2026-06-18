// Fill out your copyright notice in the Description page of Project Settings.


#include "ItemSelectionCustomization.h"

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

#define LOCTEXT_NAMESPACE "FSsFixedDataTableCustomisationLayout"

void FItemSelectionCustomization::CustomizeHeader(TSharedRef<class IPropertyHandle> InStructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	DataTablePropertyHandle = InStructPropertyHandle->GetChildHandle("DataTable");
	RowNamePropertyHandle = InStructPropertyHandle->GetChildHandle("RowName");

	Utils = StructCustomizationUtils.GetPropertyUtilities();

	FString DataTablePropertyName;
	if (InStructPropertyHandle->HasMetaData(TEXT("DataTable")))
	{
		UE_LOG(LogTemp, Warning, TEXT("REGISTER"));
		DataTablePropertyName = InStructPropertyHandle->GetMetaData(TEXT("DataTable"));
	}

	if (!DataTablePropertyName.IsEmpty())
	{
		TArray<UObject*> OuterObjects;
		InStructPropertyHandle->GetOuterObjects(OuterObjects);
		
		UE_LOG(LogTemp, Warning, TEXT("SEARCH FOR PROPERTY"));

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
				FSimpleDelegate::CreateSP(this, &FItemSelectionCustomization::OnDataTableSourceChanged)
			);
		}
	}

	FPropertyComboBoxArgs ComboArgs(RowNamePropertyHandle, 
			FOnGetPropertyComboBoxStrings::CreateSP(this, &FItemSelectionCustomization::OnGetRowStrings), 
			FOnGetPropertyComboBoxValue::CreateSP(this, &FItemSelectionCustomization::OnGetRowValueString));
	ComboArgs.ShowSearchForItemCount = 1;

	TSharedRef<SWidget> BrowseTableButton = PropertyCustomizationHelpers::MakeBrowseButton(
		FSimpleDelegate::CreateSP(this, &FItemSelectionCustomization::BrowseTableButtonClicked),
		LOCTEXT("SsBrowseToDatatable", "Browse to DataTable in Content Browser"));

	HeaderRow
		.NameContent()
		[InStructPropertyHandle->CreatePropertyNameWidget()]
		.ValueContent()
		.MaxDesiredWidth(0.0f) // don't constrain the combo button width
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
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
			]
		];

	FDataTableEditorUtils::AddSearchForReferencesContextMenu(
		HeaderRow,
		FExecuteAction::CreateSP(this, &FItemSelectionCustomization::OnSearchForReferences));
}

void FItemSelectionCustomization::CustomizeChildren(TSharedRef<class IPropertyHandle> InStructPropertyHandle,
	class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	uint32 NumChildren;
	InStructPropertyHandle->GetNumChildren(NumChildren);
	
	for (uint32 Index = 0; Index < NumChildren; ++Index)
	{
		TSharedPtr<IPropertyHandle> ChildHandle = InStructPropertyHandle->GetChildHandle(Index);
		if (ChildHandle.IsValid() && ChildHandle->GetProperty())
		{
			FName PropertyName = ChildHandle->GetProperty()->GetFName();
			
			if (PropertyName != GET_MEMBER_NAME_CHECKED(FDataTableRowHandle, DataTable) &&
				PropertyName != GET_MEMBER_NAME_CHECKED(FDataTableRowHandle, RowName))
			{
				StructBuilder.AddProperty(ChildHandle.ToSharedRef());
			}
		}
	}
}

bool FItemSelectionCustomization::GetCurrentValue(UDataTable*& OutDataTable, FName& OutName) const
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

void FItemSelectionCustomization::OnSearchForReferences()
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

void FItemSelectionCustomization::OnGetRowStrings(TArray<TSharedPtr<FString>>& OutStrings,
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

FString FItemSelectionCustomization::OnGetRowValueString() const
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

void FItemSelectionCustomization::BrowseTableButtonClicked()
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

void FItemSelectionCustomization::OnDataTableSourceChanged()
{
	if (Utils) Utils->RequestForceRefresh();
}
#undef LOCTEXT_NAMESPACE
