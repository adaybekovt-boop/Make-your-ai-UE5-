"""Read a saved Editor scene; do not certify rendering or gameplay."""
import json
import os
from pathlib import Path
import traceback
import unreal


def inspect() -> dict:
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not levels.load_level('/Game/Scaffold/Maps/L_Scaffold_City'):
        raise RuntimeError('Scaffold map could not be loaded')
    subsystem = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    result = {'ok': True, 'engine_version': unreal.SystemLibrary.get_engine_version(), 'meshes': {}}
    for name in ('SM_CityV4_Reference', 'SM_Garage', 'SM_GarageEnclosure'):
        mesh = unreal.load_asset('/Game/Scaffold/Imported/' + name)
        if not isinstance(mesh, unreal.StaticMesh):
            raise RuntimeError('Missing StaticMesh: ' + name)
        result['meshes'][name] = {
            'nanite_enabled_setting': subsystem.get_nanite_settings(mesh).get_editor_property('enabled'),
            'simple_collision_count': subsystem.get_simple_collision_count(mesh),
        }
    person = unreal.load_asset('/Game/Scaffold/Imported/SK_Person1_Source')
    if not isinstance(person, unreal.SkeletalMesh):
        raise RuntimeError('Skeletal reference is missing')
    result['skeletal_source_lod_count'] = unreal.get_editor_subsystem(unreal.SkeletalMeshEditorSubsystem).get_lod_count(person)
    result['skeletal_lod_triangles'] = [None, None, None]
    result['actors'] = [a.get_actor_label() for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()]
    return result


try:
    report = inspect()
except Exception:
    report = {'ok': False, 'error': traceback.format_exc()}
    unreal.log_error(report['error'])
report.update(schema=1, visual_verified=False, gameplay_verified=False,
              runtime_collision_traces_verified=False,
              reference_gate='UE5 reference gate: NOT VERIFIED')
Path(os.environ['MAI_SCRIPT_REPORT']).write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
if not report['ok']:
    raise RuntimeError('Editor inspection failed')
