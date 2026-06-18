// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"
#include "Engine/DataTable.h"
#include "GameFramework/Actor.h"
#include "Widgets/SToolTip.h"

class IDetailPropertyRow;
class SWidget;
class UPackage;


class FLOWGEAREDITOR_API FItemSelectionCustomization : public IPropertyTypeCustomization
{
public:
	
	static TSharedRef<IPropertyTypeCustomization> MakeInstance( )
	{
		return MakeShareable(new FItemSelectionCustomization());
	}

public:
	virtual void CustomizeHeader(TSharedRef<class IPropertyHandle> InStructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<class IPropertyHandle> InStructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	
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
