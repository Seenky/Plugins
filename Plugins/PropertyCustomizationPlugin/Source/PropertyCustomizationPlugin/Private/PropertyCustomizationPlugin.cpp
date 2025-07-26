// Copyright Epic Games, Inc. All Rights Reserved.

#include "PropertyCustomizationPlugin.h"

#include "PropertyCustomization.h"

void FPropertyCustomizationPluginModule::StartupModule()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	RegisterPropertyTypeCustomizations();

	PropertyModule.NotifyCustomizationModuleChanged();
}

void FPropertyCustomizationPluginModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

		// Unregister all classes customized by name
		for (auto It = RegisteredClassNames.CreateConstIterator(); It; ++It)
		{
			if (It->IsValid())
			{
				PropertyModule.UnregisterCustomClassLayout(*It);
			}
		}

		// Unregister all structures
		for (auto It = RegisteredPropertyTypes.CreateConstIterator(); It; ++It)
		{
			if(It->IsValid())
			{
				PropertyModule.UnregisterCustomPropertyTypeLayout(*It);
			}
		}
	
		PropertyModule.NotifyCustomizationModuleChanged();
	}
}

void FPropertyCustomizationPluginModule::RegisterPropertyTypeCustomizations()
{
	RegisterCustomPropertyTypeLayout("RainSettings", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FStructPropertyCustomization::MakeInstance));
	RegisterCustomPropertyTypeLayout("ButtonSettings", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FStructPropertyCustomization::MakeInstance));
	RegisterCustomPropertyTypeLayout("CustomizedDataTableRowHandle", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FDataTablePropertyCustomization::MakeInstance));
}

void FPropertyCustomizationPluginModule::RegisterCustomPropertyTypeLayout(const FName PropertyTypeName, const FOnGetPropertyTypeCustomizationInstance& PropertyTypeLayoutDelegate)
{
	check(PropertyTypeName != NAME_None);

	RegisteredPropertyTypes.Add(PropertyTypeName);

	static FName PropertyEditor("PropertyEditor");
	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(PropertyEditor);
	PropertyModule.RegisterCustomPropertyTypeLayout(PropertyTypeName, PropertyTypeLayoutDelegate);
}
	
IMPLEMENT_MODULE(FPropertyCustomizationPluginModule, PropertyCustomizationPlugin)