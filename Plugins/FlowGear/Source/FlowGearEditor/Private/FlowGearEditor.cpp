#include "FlowGearEditor.h"

#include "ItemSelectionCustomization.h"
#include "ItemDefinitionAssetCustomization.h" 
#include "Inventory/FlowInventoryTypes.h"
#include "Inventory/ItemDefinitionAsset.h"    

#define LOCTEXT_NAMESPACE "FFlowGearEditorModule"

void FFlowGearEditorModule::StartupModule()
{
    FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

    RegisterClassCustomizations();        
    RegisterPropertyTypeCustomizations();

    PropertyModule.NotifyCustomizationModuleChanged();   
}

void FFlowGearEditorModule::ShutdownModule()
{
    if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
    {
       FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
       
       // Unregister all classes
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

void FFlowGearEditorModule::RegisterClassCustomizations()
{
    RegisterCustomClassLayout(
        UItemDefinitionAsset::StaticClass()->GetFName(), 
        FOnGetDetailCustomizationInstance::CreateStatic(&FItemDefinitionAssetCustomization::MakeInstance)
    );
}

void FFlowGearEditorModule::RegisterCustomClassLayout(FName ClassName, const FOnGetDetailCustomizationInstance& DetailLayoutDelegate)
{
    check(ClassName != NAME_None);

    RegisteredClassNames.Add(ClassName);

    static FName PropertyEditor("PropertyEditor");
    FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(PropertyEditor);
    PropertyModule.RegisterCustomClassLayout(ClassName, DetailLayoutDelegate);
}

void FFlowGearEditorModule::RegisterPropertyTypeCustomizations()
{
    RegisterCustomPropertyTypeLayout(FItemRowHandle::StaticStruct()->GetFName(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FItemSelectionCustomization::MakeInstance));
}

void FFlowGearEditorModule::RegisterCustomPropertyTypeLayout(FName PropertyTypeName, const FOnGetPropertyTypeCustomizationInstance& PropertyTypeLayoutDelegate)
{
    check(PropertyTypeName != NAME_None);

    RegisteredPropertyTypes.Add(PropertyTypeName);

    static FName PropertyEditor("PropertyEditor");
    FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(PropertyEditor);
    PropertyModule.RegisterCustomPropertyTypeLayout(PropertyTypeName, PropertyTypeLayoutDelegate);
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FFlowGearEditorModule, FlowGearEditor)