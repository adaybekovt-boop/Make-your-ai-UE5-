using UnrealBuildTool;

public class MakeYourAIEditorTarget : TargetRules
{
    public MakeYourAIEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        // Uses the defaults of the engine recorded in UE5Engine.lock.json.
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        ExtraModuleNames.AddRange(new[] { "MakeYourAI", "MakeYourAIEditor" });
    }
}
