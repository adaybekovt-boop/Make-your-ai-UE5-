"""Unreal Editor entry point; invoked by scaffold_runner.py after native compilation."""
from pathlib import Path
import json
import os
import sys
import traceback
import unreal

sys.path.insert(0, str(Path(__file__).resolve().parent))
from editor_assets import digest, import_mesh, create_material, create_catalog
from editor_scene import create_scene, MAP


def main(report: dict) -> None:
    project = Path(unreal.Paths.project_dir()).resolve()
    repo = project.parents[1]
    markers = json.loads((project / 'Saved/ScaffoldSource/city-markers.json').read_text(encoding='utf-8'))
    required = {'garage', 'workshop', 'technopark', 'server-hall', 'campus', 'auction', 'nuclear-power', 'greenhaven'}
    if markers.get('missing_required') or not required.issubset(markers.get('markers', {})):
        raise RuntimeError('Incomplete authored marker bounds; run the Blender extraction tool')
    if markers.get('source_sha256') != digest(repo / 'Unreal/CityV4/city-v4.blend'):
        raise RuntimeError('Marker metadata is stale')
    if unreal.EditorAssetLibrary.does_asset_exist(MAP):
        raise RuntimeError('Existing scaffold map will not be overwritten')
    names = ('MaiScaffoldWorld', 'MaiCatalogAsset', 'MaiGameMode')
    classes = {name: unreal.load_class(None, '/Script/MakeYourAI.' + name) for name in names}
    if not all(classes.values()):
        raise RuntimeError('Native classes unavailable: compile Development Editor first')
    report['engine_version'] = unreal.SystemLibrary.get_engine_version()
    meshes = {
        'city': import_mesh(repo, 'Unreal/CityV4/city-v4.fbx', 'SM_CityV4_Reference', report),
        'garage': import_mesh(repo, 'Unreal/Assets/interiors/garage/garage.fbx', 'SM_Garage', report),
        'enclosure': import_mesh(repo, 'Unreal/Assets/interiors/garage/garage-enclosure.fbx', 'SM_GarageEnclosure', report),
        'person': import_mesh(repo, 'Unreal/Assets/characters/person-1/person-1.fbx', 'SK_Person1_Source', report, True),
    }
    for name in ('rack-basic', 'rack-cooled', 'rack-enterprise'):
        meshes[name] = import_mesh(repo, f'Unreal/Assets/racks/{name}/{name}.fbx', 'SM_' + name.replace('-', '_'), report)
    material = create_material()
    for mesh in (meshes['city'], meshes['garage'], meshes['enclosure']):
        for index in range(len(mesh.get_editor_property('static_materials'))):
            mesh.set_material(index, material)
        unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    create_catalog(classes['MaiCatalogAsset'])
    create_scene(project, meshes, markers, classes, report)
    if markers['source_sha256'] != digest(repo / 'Unreal/CityV4/city-v4.blend'):
        raise RuntimeError('Original city changed unexpectedly')
    report.update(ok=True, source_unchanged=True)


report = {'schema': 1, 'ok': False, 'status': 'SOURCE_SCAFFOLD', 'imports': [], 'reused': [], 'warnings': [],
          'visual_verified': False, 'gameplay_verified': False, 'reference_gate': 'UE5 reference gate: NOT VERIFIED'}
try:
    main(report)
except Exception:
    report['ok'] = False
    report['error'] = traceback.format_exc()
    unreal.log_error(report['error'])
finally:
    target = Path(os.environ['MAI_SCRIPT_REPORT'])
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
if not report['ok']:
    raise RuntimeError('Editor scene builder failed; inspect the per-run report')
