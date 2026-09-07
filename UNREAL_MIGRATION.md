# Make Your AI — Unreal migration

The browser game and its original Blender/FBX/GLB sources remain intact. Binary assets use ordinary Git; no LFS migration has been performed.

The branch `agent/ue5-core-vertical-slice` now contains an expanded C++ project at `Unreal/MakeYourAI/MakeYourAI.uproject`, an executable portable simulation domain, UE subsystem adapters, native UMG, graybox world/interaction/NPC code, real Editor/Blender preparation tools and tests.

Status: **SOURCE_SCAFFOLD**. UE Development Editor compilation, UHT/UBT, Editor verification, visual verification and gameplay verification remain **NOT VERIFIED**. Native C++ compilation is separately verified and must not be represented as an Unreal build. No real .umap/.uasset or UE screenshot was produced in the editing environment.

Current reports: [progress and verified commands](Unreal/UE5_PROGRESS.md), [architecture and scope](Unreal/UE5_SCAFFOLD.md), [environment](Unreal/UE5_STATUS.md), [reference gate](Unreal/UE5_GATE_REPORT.md), [launch instructions](Unreal/MakeYourAI/README.md).

Existing sources: `Unreal/Assets/` contains the original standalone asset library; `Unreal/CityV4/` contains the original authored city and reports. They are not replaced with a new graybox city. The Editor builder imports these sources only when executed on a machine with a real, pinned UE installation.
