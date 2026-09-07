# Make your AI — Unreal migration

This repository preserves the current web game and adds the Blender/FBX asset core prepared for the Unreal Engine migration.

## Added content

- `Unreal/Assets/` — upgraded vehicles, characters, server racks, location interiors, procedural material maps, previews, manifests, and Blender build scripts.
- `Unreal/CityV4/` — expanded city with the auction house, remote nuclear power station, and Greenhaven suburb. Includes Blender source, FBX export, day/night renders, close-ups, validation reports, and reproducible scripts.

The repository does not contain a `.uproject` yet. The first Unreal task is to create the C++ project, pin the engine version, import a reference Garage/map fragment, then verify Nanite and a skeletal LOD chain before full migration.

Large binary assets are stored with Git LFS. Run `git lfs install` before cloning or pulling on a development machine.
