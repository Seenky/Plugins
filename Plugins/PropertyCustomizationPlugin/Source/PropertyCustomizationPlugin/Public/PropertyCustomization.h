// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

class IDetailPropertyRow;
class SWidget;
class UPackage;

class PROPERTYCUSTOMIZATIONPLUGIN_API FStructPropertyCustomization final : public IPropertyTypeCustomization
{
public:
	
	static TSharedRef<IPropertyTypeCustomization> MakeInstance( )
	{
		return MakeShareable(new FStructPropertyCustomization());
	}

public:
	virtual void CustomizeChildren( TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils ) override;
	virtual void CustomizeHeader( TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils ) override;
};

class PROPERTYCUSTOMIZATIONPLUGIN_API FDataTablePropertyCustomization final : public IPropertyTypeCustomization
{
public:
	
	static TSharedRef<IPropertyTypeCustomization> MakeInstance( )
	{
		return MakeShareable(new FDataTablePropertyCustomization());
	}

public:
	virtual void CustomizeHeader(TSharedRef<class IPropertyHandle> InStructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	// Not needed, but must be implemented
	virtual void CustomizeChildren(TSharedRef<class IPropertyHandle> InStructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override {}

protected:
	bool GetCurrentValue(UDataTable*& OutDataTable, FName& OutName) const;
	void OnSearchForReferences();
	void OnGetRowStrings(TArray< TSharedPtr<FString> >& OutStrings, TArray<TSharedPtr<SToolTip>>& OutToolTips, TArray<bool>& OutRestrictedItems) const;
	FString OnGetRowValueString() const;
	void BrowseTableButtonClicked();
	
	TSharedPtr<IPropertyHandle> DataTablePropertyHandle;
	TSharedPtr<IPropertyHandle> RowNamePropertyHandle;

private:
	void OnDataTableSourceChanged();
	TSharedPtr<IPropertyUtilities> Utils;
};

