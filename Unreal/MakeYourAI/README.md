# Make Your AI — C++ project preparation

**UE5 reference gate: NOT VERIFIED. This is not a playable vertical slice.**

The repository now has a C++ project descriptor, Game/Editor targets and one runtime module. No fake gameplay classes, .umap or .uasset placeholders are supplied. Core, Gameplay, Economy, Persistence and UI responsibilities are separated under Source; only the required primary module is implemented. No extra plugin is needed yet.

## Engine selection and build

No UE5 installation was available in the editing environment, so EngineAssociation is intentionally empty. Do not insert an assumed release. Python 3.10+ is used only for preparation tooling; it is not a runtime game dependency.

On a machine with a real UE5 installation, set UE_ROOT to its root directory (the directory containing Engine), then run from the repository root:

```sh
python Unreal/Tools/ue5.py audit
python Unreal/Tools/ue5.py pin-engine
python Unreal/Tools/ue5.py generate
python Unreal/Tools/ue5.py build
```

`--engine-root` can be supplied explicitly to every command instead of UE_ROOT. `pin-engine` reads the actual Engine/Build/Build.version, checks Editor/Editor-Cmd/UBT/build-script file presence, stores the exact metadata plus its SHA-256 in UE5Engine.lock.json, and sets the portable major.minor EngineAssociation. Review and commit the resulting pin and descriptor together. It does not store an installation path or machine-specific engine GUID. A changed pin is rejected rather than silently overwritten. Source-built engines should still be opened through their explicit Editor path, not an assumed launcher association.

The target files use BuildSettingsVersion.Latest **from the pinned installation**. Until an actual pin and successful build exist this is a preparation target, not a reproducibly compiled release. The lock records build metadata, not cryptographic integrity of an entire custom engine/toolchain.

Generation invokes UBT's ProjectFiles mode through the installed build wrapper. Build requests MakeYourAIEditor, Development, on Linux or Win64. Linux and Windows command construction has unit coverage; neither platform has a real UE integration build in this run. macOS commands are intentionally rejected until configured and tested.

Each command writes a new Saved/Verification/<run-id>/result.json and command logs. An explicit --output must name a new directory. Exit 2 means a blocked preparation step; 127 means the command could not start; 124 means a wrapper timeout. process_exit_code is null when the underlying process was not executed. A successful wrapper, pin, header check or test of Python code never grants the reference gate. Inspect the actual UBT log before recording compile verified.

## Editor launch after a successful build

Run the selected installation's Engine/Binaries/Linux/UnrealEditor (Linux) or Engine/Binaries/Win64/UnrealEditor.exe (Windows), passing the path to Unreal/MakeYourAI/MakeYourAI.uproject and -log. Paths are resolved on that machine, not hard-coded in this project. Confirm the module loads and retain the real Editor log. No default project map is configured because a real map has not been authored yet.

Only then create L_Reference_GarageCity and follow [the gate report](../UE5_GATE_REPORT.md): real FBX import, collision traces, a real skeletal LOD chain, Nanite, Lumen, World Partition and instancing. Do not advance to gameplay until the evidence is reviewed.

## Checks available without UE

```sh
python -m unittest discover -s Unreal/Tools/tests -v
```

These are **preparation-tool tests, not Unreal Automation tests**. The preparation workflow also reads the four original FBX files and compares binary headers and Git blob hashes. This proves source availability/integrity only, not importability, rig behavior, collisions or rendering.

Generated folders are ignored. Never commit Binaries, Intermediate, Saved, DerivedDataCache or .vs. Curated evidence belongs outside those folders under Unreal/Evidence, with its source commit and test scope recorded.

## Primary references consulted

- [Epic: modules](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-modules)
- [Epic: project generation / UBT ProjectFiles](https://dev.epicgames.com/documentation/en-us/unreal-engine/how-to-generate-unreal-engine-project-files-for-your-ide)
- [Epic: target rules](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-build-tool-target-reference)

These documents guided the prepared structure; they do not constitute evidence that this project compiled.
