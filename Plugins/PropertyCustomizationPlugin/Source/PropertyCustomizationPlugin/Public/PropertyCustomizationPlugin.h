// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

class FPropertyCustomizationPluginModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	virtual bool SupportsDynamicReloading() override
	{
		return true;
	}

protected:
	void RegisterPropertyTypeCustomizations();
	void RegisterCustomPropertyTypeLayout(FName PropertyTypeName, const FOnGetPropertyTypeCustomizationInstance& PropertyTypeLayoutDelegate);

private:
	TSet< FName > RegisteredClassNames;
	TSet< FName > RegisteredPropertyTypes;
};
