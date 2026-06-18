using UnrealBuildTool;

public class FlowGearEditor : ModuleRules
{
    public FlowGearEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore", 
                "FlowGear",
                "UnrealEd",
                "PropertyEditor",
                "EnhancedInput",
            }
        );
    }
}