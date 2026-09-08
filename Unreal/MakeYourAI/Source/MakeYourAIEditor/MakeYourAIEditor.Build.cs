using UnrealBuildTool;
public class MakeYourAIEditor : ModuleRules {
    public MakeYourAIEditor(ReadOnlyTargetRules Target) : base(Target) {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[]{"Core","CoreUObject","Engine"});
        PrivateDependencyModuleNames.AddRange(new[]{"UnrealEd","AssetRegistry","AssetTools","MeshDescription","StaticMeshDescription","Json","RenderCore","RHI"});
    }
}
