"""Run inside a compiled Unreal Editor. Creates real assets/maps; never fabricates their bytes."""
from pathlib import Path
import json
import os
import traceback
import unreal

BOOT = '/Game/Scaffold/Maps/L_Campaign_Boot'
GARAGE = '/Game/Scaffold/Maps/L_Campaign_Garage'
DATA = '/Game/Scaffold/Data/DA_Campaign'


def main(report: dict) -> None:
    project = Path(unreal.Paths.project_dir()).resolve()
    classes = {name: unreal.load_class(None, '/Script/MakeYourAI.' + name)
               for name in ('MaiCampaignAsset', 'MaiGarageInterior', 'MaiGameMode')}
    if not all(classes.values()):
        raise RuntimeError('Campaign C++ classes unavailable. Compile Development Editor first.')
    # Creation-only: re-running must not overwrite hand-edited rooms or profiles.
    for path in (BOOT, GARAGE, DATA):
        if unreal.EditorAssetLibrary.does_asset_exist(path):
            raise RuntimeError('Existing asset will not be overwritten: ' + path)
    report['engine_version'] = unreal.SystemLibrary.get_engine_version()
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', classes['MaiCampaignAsset'])
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        'DA_Campaign', '/Game/Scaffold/Data', classes['MaiCampaignAsset'], factory)
    if not asset or not unreal.EditorAssetLibrary.save_loaded_asset(asset):
        raise RuntimeError('Campaign DataAsset creation/save failed')
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for path in (GARAGE, BOOT):
        # Garage is an ordinary streamable interior, not a partitioned city.
        if not levels.new_level(path):
            raise RuntimeError('Map creation failed: ' + path)
        world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        world.get_world_settings().set_editor_property('default_game_mode', classes['MaiGameMode'])
        if path == GARAGE:
            room = actors.spawn_actor_from_class(classes['MaiGarageInterior'], unreal.Vector(0, 0, 0))
            if not room:
                raise RuntimeError('Garage bootstrap actor could not be created')
            room.set_actor_label('Walkable Garage runtime graybox - not FBX validation')
            light = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 600))
            light.set_actor_rotation(unreal.Rotator(-60, -30, 0), False)
            light.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
            light.light_component.set_intensity(3.0)
            actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 300))
        if not levels.save_current_level():
            raise RuntimeError('Editor failed to save real map: ' + path)
        relative = path.removeprefix('/Game/') + '.umap'
        file = project / 'Content' / relative
        if not file.is_file() or file.stat().st_size < 128:
            raise RuntimeError('Expected Editor-generated binary map is absent')
        report['maps'].append({'path': path, 'bytes': file.stat().st_size})
    # Default map changes only AFTER those binary maps really exist.
    ini = project / 'Config/DefaultEngine.ini'
    text = ini.read_text(encoding='utf-8')
    text = text.replace('GameDefaultMap=/Engine/Maps/Entry', 'GameDefaultMap=' + BOOT)
    text = text.replace('EditorStartupMap=/Engine/Maps/Entry', 'EditorStartupMap=' + BOOT)
    ini.write_text(text, encoding='utf-8')
    report.update(ok=True, campaign_data_asset=DATA,
                  portrait_created=False, portrait_slot='/Game/Scaffold/Portraits/T_ElonMax_Fictional',
                  widgets='native programmatic UMG classes, no fake widget assets',
                  imported_city='/Game/Scaffold/Maps/L_Scaffold_City (optional, built by build_scaffold.py)',
                  garage='explicit runtime graybox; existing FBX reference import is a separate pipeline',
                  editor_assets_created=True, visual_verified=False, gameplay_verified=False,
                  collision_verified=False, reference_gate='UE5 reference gate: NOT VERIFIED')


if __name__ == '__main__':
    result = {'ok': False, 'maps': [], 'editor_assets_created': False,
              'visual_verified': False, 'gameplay_verified': False}
    try:
        main(result)
    except Exception as error:
        result['error'] = str(error)
        result['traceback'] = traceback.format_exc()
        unreal.log_error(result['traceback'])
    finally:
        destination = Path(os.environ.get('MAI_SCRIPT_REPORT',
            str(Path(unreal.Paths.project_dir()) / 'Saved/CampaignBuild/result.json')))
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    if not result['ok']:
        raise RuntimeError('Campaign Editor bootstrap failed; inspect the retained report')
