using System.IO;
using UnrealBuildTool;
public class QuickJS : ModuleRules {
    public QuickJS(ReadOnlyTargetRules Target) : base(Target) {
        Type=ModuleType.External;
        string Root=Path.GetFullPath(Path.Combine(ModuleDirectory,"../../..","ThirdParty","QuickJS"));
        string Library=Target.Platform==UnrealTargetPlatform.Win64
            ? Path.Combine(Root,"build","Win64","Release","qjs.lib")
            : Path.Combine(Root,"build","Linux","libqjs.a");
        if(!File.Exists(Library)||!File.Exists(Path.Combine(Root,"source","quickjs.h")))
            throw new BuildException("Pinned rules dependency is missing. Run Unreal/Tools/Run-NativeParity.ps1 -Stage generate first.");
        PublicSystemIncludePaths.Add(Path.Combine(Root,"source"));PublicAdditionalLibraries.Add(Library);
    }
}
