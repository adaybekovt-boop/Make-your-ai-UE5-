"""Import authored furnished FBX rooms, preserving their PBR material slots."""
import hashlib
import json
from pathlib import Path
import re
import unreal

REPO = Path(__file__).resolve().parents[2]
ROOT = '/Game/Generated/Interiors/AuthoredV2'
assets = unreal.EditorAssetLibrary
edit = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
report = []
for key in ('garage', 'workshop', 'technopark', 'server-hall', 'campus', 'dc-north', 'dc-south'):
    folder = REPO / 'Unreal/Assets/interiors' / key
    source = folder / (key + '.fbx')
    digest = hashlib.sha256(source.read_bytes()).hexdigest()
    name = key.replace('-', '_')
    destination = ROOT + '/' + name
    mesh_path = destination + '/SM_' + name
    mesh = assets.load_asset(mesh_path) if assets.does_asset_exist(mesh_path) else None
    if mesh and assets.get_metadata_tag(mesh, 'MAI_SourceSHA256') != digest:
        raise RuntimeError('Refusing to overwrite changed content: ' + mesh_path)
    if not mesh:
        options = unreal.FbxImportUI()
        for field, value in dict(automated_import_should_detect_type=False, import_mesh=True,
                                import_as_skeletal=False, import_materials=False,
                                import_textures=False, import_animations=False).items():
            options.set_editor_property(field, value)
        options.set_editor_property('mesh_type_to_import', unreal.FBXImportType.FBXIT_STATIC_MESH)
        data = options.get_editor_property('static_mesh_import_data')
        for field, value in dict(combine_meshes=True, auto_generate_collision=False,
                                convert_scene=True, convert_scene_unit=True, import_uniform_scale=1.).items():
            data.set_editor_property(field, value)
        task = unreal.AssetImportTask()
        for field, value in dict(filename=str(source), destination_path=destination,
                                destination_name='SM_' + name, automated=True,
                                replace_existing=False, save=True, options=options).items():
            task.set_editor_property(field, value)
        tools.import_asset_tasks([task])
        meshes = [assets.load_asset(p) for p in task.get_editor_property('imported_object_paths')]
        meshes = [m for m in meshes if isinstance(m, unreal.StaticMesh)]
        if len(meshes) != 1:
            raise RuntimeError('Expected one furnished room: ' + key)
        mesh = meshes[0]
        if mesh.get_path_name().split('.')[0] != mesh_path:
            raise RuntimeError('Unexpected room package: ' + mesh.get_path_name())
        definitions = json.loads((folder / 'materials.json').read_text(encoding='utf-8'))
        def material_key(value):
            number = re.match(r'\d+', value)
            return number.group() if number else re.sub(r'[^a-z0-9]', '', value.lower())
        definitions = {material_key(m['name']): m for m in definitions}
        for i, slot in enumerate(mesh.get_editor_property('static_materials')):
            slot_name = str(slot.get_editor_property('imported_material_slot_name'))
            prefix = material_key(slot_name)
            definition = definitions.get(prefix)
            if not definition:
                raise RuntimeError('Unmapped authored material: ' + slot_name)
            material_name = 'M_' + name + '_' + prefix
            material = tools.create_asset(material_name, destination, unreal.Material, unreal.MaterialFactoryNew())
            if not material:
                raise RuntimeError('Material creation failed: ' + material_name)
            for cls, field, value, output in [
                (unreal.MaterialExpressionConstant3Vector, 'constant', unreal.LinearColor(*definition['base_color_linear']), unreal.MaterialProperty.MP_BASE_COLOR),
                (unreal.MaterialExpressionConstant, 'r', max(.4, definition['roughness_scalar']), unreal.MaterialProperty.MP_ROUGHNESS),
                (unreal.MaterialExpressionConstant, 'r', definition['metallic'], unreal.MaterialProperty.MP_METALLIC),
            ]:
                expression = edit.create_material_expression(material, cls)
                expression.set_editor_property(field, value)
                if not edit.connect_material_property(expression, '', output):
                    raise RuntimeError('Unconnected authored material')
            edit.recompile_material(material)
            assets.save_loaded_asset(material)
            mesh.set_material(i, material)
        mesh.get_editor_property('body_setup').set_editor_property('collision_trace_flag', unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
        assets.set_metadata_tag(mesh, 'MAI_SourceSHA256', digest)
        if not assets.save_loaded_asset(mesh):
            raise RuntimeError('Failed to save room: ' + key)
    bounds = mesh.get_bounds()
    report.append(dict(id=key, asset=mesh.get_path_name(), source_sha256=digest,
                       origin=[bounds.origin.x, bounds.origin.y, bounds.origin.z],
                       half_size=[bounds.box_extent.x, bounds.box_extent.y, bounds.box_extent.z]))
output = REPO / 'Unreal/MakeYourAI/Saved/Verification/authored-interiors-import.json'
output.write_text(json.dumps(report, indent=2), encoding='utf-8')
unreal.log('MAI_AUTHORED_INTERIORS_READY: ' + str(len(report)))
