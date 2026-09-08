"""Real Editor import helpers. Importing this module requires Unreal Editor."""
from __future__ import annotations
import hashlib
from pathlib import Path
import unreal

BASE = '/Game/Scaffold'


def digest(path: Path) -> str:
    h = hashlib.sha256()
    with path.open('rb') as stream:
        for part in iter(lambda: stream.read(1024 * 1024), b''):
            h.update(part)
    return h.hexdigest()


def import_mesh(repo: Path, relative: str, name: str, report: dict, skeletal: bool = False):
    # Retire the destructive single-mesh CityV4 route before any assets are written.
    if 'cityv4' in relative.lower() or name.lower().startswith('cityv4'):
        raise RuntimeError('CityV4 requires export_cityv4_instances.py and build_city_content.py; combined FBX import is disabled.')
    source = repo / relative
    source_hash = digest(source)
    package = BASE + '/Imported/' + name
    if unreal.EditorAssetLibrary.does_asset_exist(package):
        result = unreal.load_asset(package)
        if unreal.EditorAssetLibrary.get_metadata_tag(result, 'MAI_SourceSHA256') != source_hash:
            raise RuntimeError('Will not overwrite an untracked or changed asset: ' + package)
        report['reused'].append({'asset': package, 'source_sha256': source_hash})
        return result
    options = unreal.FbxImportUI()
    options.set_editor_property('automated_import_should_detect_type', False)
    options.set_editor_property('import_mesh', True)
    options.set_editor_property('import_as_skeletal', skeletal)
    options.set_editor_property('mesh_type_to_import', unreal.FBXImportType.FBXIT_SKELETAL_MESH if skeletal else unreal.FBXImportType.FBXIT_STATIC_MESH)
    options.set_editor_property('import_materials', False)
    options.set_editor_property('import_textures', False)
    options.set_editor_property('import_animations', skeletal)
    data = options.get_editor_property('skeletal_mesh_import_data' if skeletal else 'static_mesh_import_data')
    data.set_editor_property('convert_scene', True)
    data.set_editor_property('convert_scene_unit', True)
    data.set_editor_property('import_uniform_scale', 1.0)
    if not skeletal:
        data.set_editor_property('combine_meshes', True)
        data.set_editor_property('auto_generate_collision', False)
    task = unreal.AssetImportTask()
    task.set_editor_property('filename', str(source))
    task.set_editor_property('destination_path', BASE + '/Imported')
    task.set_editor_property('destination_name', name)
    task.set_editor_property('automated', True)
    task.set_editor_property('replace_existing', False)
    task.set_editor_property('save', True)
    task.set_editor_property('options', options)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    outputs = list(task.get_editor_property('imported_object_paths'))
    expected = unreal.SkeletalMesh if skeletal else unreal.StaticMesh
    meshes = [asset for asset in (unreal.load_asset(path) for path in outputs) if isinstance(asset, expected)]
    if len(meshes) != 1:
        raise RuntimeError(f'Expected one combined reference mesh from {relative}; got {len(meshes)}: {outputs}')
    result = meshes[0]
    collisions = None
    if not skeletal:
        static = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
        settings = static.get_nanite_settings(result)
        settings.set_editor_property('enabled', True)
        static.set_nanite_settings(result, settings, True)
        collisions = static.get_simple_collision_count(result)
    unreal.EditorAssetLibrary.set_metadata_tag(result, 'MAI_SourceSHA256', source_hash)
    if not unreal.EditorAssetLibrary.save_loaded_asset(result):
        raise RuntimeError('Imported mesh was not saved')
    if digest(source) != source_hash:
        raise RuntimeError('Source FBX changed during import')
    report['imports'].append({'source': relative, 'source_sha256': source_hash, 'asset': result.get_path_name(),
                              'outputs': outputs, 'skeletal': skeletal, 'nanite_requested': not skeletal,
                              'simple_collision_count': collisions})
    return result


def create_catalog(catalog_class):
    package = BASE + '/Data/DA_ScaffoldCatalog'
    if unreal.EditorAssetLibrary.does_asset_exist(package):
        return unreal.load_asset(package)
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', catalog_class)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset('DA_ScaffoldCatalog', BASE + '/Data', catalog_class, factory)
    if not asset:
        raise RuntimeError('Catalog DataAsset creation failed')
    unreal.EditorAssetLibrary.save_loaded_asset(asset)
    return asset


def create_material():
    package = BASE + '/Materials/M_ScaffoldBase'
    if unreal.EditorAssetLibrary.does_asset_exist(package):
        return unreal.load_asset(package)
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_ScaffoldBase', BASE + '/Materials', unreal.Material, unreal.MaterialFactoryNew())
    if not material:
        raise RuntimeError('Material creation failed')
    color = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -400, 0)
    color.set_editor_property('parameter_name', 'BaseTint')
    color.set_editor_property('default_value', unreal.LinearColor(0.19, 0.19, 0.21, 1))
    rough = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionConstant, -400, 150)
    rough.set_editor_property('r', 0.72)
    unreal.MaterialEditingLibrary.connect_material_property(color, '', unreal.MaterialProperty.MP_BASE_COLOR)
    unreal.MaterialEditingLibrary.connect_material_property(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    return material
