using UnrealBuildTool;

public class MakeYourAI : ModuleRules
{
    public MakeYourAI(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        // Follow the actual UE installation's C++ standard. The portable domain
        // also compiles separately as C++17, but must not downgrade engine headers.
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "UMG" });
        PrivateDependencyModuleNames.AddRange(new string[] { "InputCore", "Slate", "SlateCore" });
    }
}
