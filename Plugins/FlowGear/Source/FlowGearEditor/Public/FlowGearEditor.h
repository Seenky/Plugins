#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FFlowGearEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

protected:
    void RegisterClassCustomizations();
    void RegisterCustomClassLayout(FName ClassName, const FOnGetDetailCustomizationInstance& DetailLayoutDelegate);
    
    void RegisterPropertyTypeCustomizations();
    void RegisterCustomPropertyTypeLayout(FName PropertyTypeName, const FOnGetPropertyTypeCustomizationInstance& PropertyTypeLayoutDelegate);

private:
    TSet< FName > RegisteredClassNames;
    TSet< FName > RegisteredPropertyTypes;
};
