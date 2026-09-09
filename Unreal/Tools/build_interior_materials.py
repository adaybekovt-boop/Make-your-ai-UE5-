"""Run inside Unreal Editor. Reproducible, opaque interior surface library."""
import unreal

ROOT = '/Game/Generated/Interiors/V1'
OWNER = 'MakeYourAI.Interiors.V1'
assets = unreal.EditorAssetLibrary
edit = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
palette = {
    'Floor': ((.13, .17, .18, 1), .82, 0),
    'Wall': ((.42, .47, .45, 1), .9, 0),
    'Trim': ((.065, .095, .11, 1), .65, .2),
    'Rack': ((.025, .04, .05, 1), .58, .35),
    'Metal': ((.23, .28, .3, 1), .5, .6),
    'Accent': ((.12, .36, .29, 1), .65, 0),
}
for name, (color, roughness, metallic) in palette.items():
    path = ROOT + '/M_' + name
    if assets.does_asset_exist(path):
        material = assets.load_asset(path)
        if assets.get_metadata_tag(material, 'MAI_GeneratedOwner') != OWNER:
            raise RuntimeError('Refusing to overwrite unowned asset: ' + path)
        continue
    material = tools.create_asset('M_' + name, ROOT, unreal.Material, unreal.MaterialFactoryNew())
    if not material:
        raise RuntimeError('Could not create ' + path)
    material.set_editor_property('blend_mode', unreal.BlendMode.BLEND_OPAQUE)
    for cls, value, prop in [
        (unreal.MaterialExpressionConstant3Vector, unreal.LinearColor(*color), unreal.MaterialProperty.MP_BASE_COLOR),
        (unreal.MaterialExpressionConstant, roughness, unreal.MaterialProperty.MP_ROUGHNESS),
        (unreal.MaterialExpressionConstant, metallic, unreal.MaterialProperty.MP_METALLIC),
    ]:
        expression = edit.create_material_expression(material, cls)
        expression.set_editor_property('constant' if cls == unreal.MaterialExpressionConstant3Vector else 'r', value)
        if not edit.connect_material_property(expression, '', prop):
            raise RuntimeError('Unconnected material output: ' + path)
    edit.recompile_material(material)
    assets.set_metadata_tag(material, 'MAI_GeneratedOwner', OWNER)
    if not assets.save_loaded_asset(material):
        raise RuntimeError('Could not save ' + path)
unreal.log('MAI_INTERIOR_MATERIALS_READY: 6 opaque non-emissive surfaces')
