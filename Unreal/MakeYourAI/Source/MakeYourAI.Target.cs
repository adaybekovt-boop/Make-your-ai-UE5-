using UnrealBuildTool;

public class MakeYourAITarget : TargetRules
{
    public MakeYourAITarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        // The exact installed engine must be pinned before generating/building.
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        ExtraModuleNames.Add("MakeYourAI");
    }
}
