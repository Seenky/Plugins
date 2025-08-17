// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MassAIWithTraffic : ModuleRules
{
	public MassAIWithTraffic(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core", 
				"Engine", 
				"MassAIBehavior", 
				"MassCommon", 
				"MassMovement", 
				"MassNavigation", 
				"AnimationBudgetAllocator",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"ZoneGraph",
				"AnimationBudgetAllocator",
				"DeveloperSettings", 
				"AnimGraphRuntime",
				"MassActors",
				"MassCrowd",
				"MassCommon",
				"MassEntity",
				"Niagara",
                "ChaosVehicles",
				"ChaosVehicles",
				"ChaosVehiclesCore",
				"Chaos",
				"ChaosCore",
				"MassRepresentation",
				"MassLOD",
				// ... add private dependencies that you statically link with here ...	
			}
			);

        if (Target.bBuildEditor)
		{
            PrivateDependencyModuleNames.AddRange(
            new string[]
			{
                "PropertyEditor"
            }
			);
    }
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
