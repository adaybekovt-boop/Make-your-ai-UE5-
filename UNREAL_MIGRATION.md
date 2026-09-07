# Make your AI — Unreal migration

This repository preserves the current web game and adds the Blender/FBX asset core prepared for the Unreal Engine migration.

## Added content

- `Unreal/Assets/` — upgraded vehicles, characters, server racks, location interiors, procedural material maps, previews, manifests, and Blender build scripts.
- `Unreal/CityV4/` — expanded city with the auction house, remote nuclear power station, and Greenhaven suburb. Includes Blender source, FBX export, day/night renders, close-ups, validation reports, and reproducible scripts.

The migration starts with a C++ project, an engine version selected from an actual installation, a reference Garage/map fragment, and verified Nanite/collision/skeletal LODs before gameplay migration. A project descriptor alone is not a completed migration. See `Unreal/UE5_STATUS.md`, `Unreal/UE5_GATE_REPORT.md` and `Unreal/UE5_PROGRESS.md` for the observed status as those reports are added.

## Binary storage correction — 2026-09-07

Binary assets are stored in ordinary Git, not Git LFS. `.gitattributes` declares binary file types and does not configure an LFS filter. The original instruction to run `git lfs install` was incorrect for this repository. No LFS migration, pointer replacement or history rewrite is authorized by this change.

Source audited: `cda0b460257e656c57b31c8ccebfb79097023d4a`. The browser source and Blender authoring files remain unchanged.
