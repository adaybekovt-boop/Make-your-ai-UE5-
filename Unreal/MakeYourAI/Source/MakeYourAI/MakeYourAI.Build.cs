using UnrealBuildTool;

public class MakeYourAI : ModuleRules
{
    public MakeYourAI(ReadOnlyTargetRules Target) : base(Target)
    {
        bUseUnity = false; // keep the embedded C API isolated from unrelated UE translation units
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        // Follow the actual UE installation's C++ standard. The portable domain
        // also compiles separately as C++17, but must not downgrade engine headers.
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "UMG", "Json", "AudioMixer" });
        RuntimeDependencies.Add("$(ProjectDir)/Content/Rules/mai-rules.js", StagedFileType.UFS);
        RuntimeDependencies.Add("$(ProjectDir)/Content/Rules/mai-rules.manifest.json", StagedFileType.UFS);
        RuntimeDependencies.Add("$(ProjectDir)/Content/Rules/city-content.json", StagedFileType.UFS);
        PrivateDependencyModuleNames.AddRange(new string[] { "InputCore", "Slate", "SlateCore", "QuickJS", "RHI", "RenderCore" });
    }
}
