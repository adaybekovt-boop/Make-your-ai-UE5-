"""Create reusable master materials through the Editor API. Never writes fake .uasset bytes."""
from __future__ import annotations
import unreal

BASE = '/Game/Scaffold/Materials'
MASTERS = (
    ('M_ArchitectureConcrete', 'Opaque architecture and concrete'),
    ('M_GlassFacade', 'Glass and facade with controlled fresnel'),
    ('M_MetalIndustrial', 'Metal and port industrial surfaces'),
    ('M_Water', 'Water; expensive transparency is opt-in'),
    ('M_RoadMarking', 'Roads and markings'),
    ('M_LandscapeRock', 'Landscape and rock'),
    ('M_Foliage', 'Vegetation'),
    ('M_StatusSurface', 'Locked / Available / Owned interactive surfaces'),
    ('M_EmissiveAccent', 'City emissive accents for night readability'),
)


def _material(name: str, blend: 'unreal.BlendMode', shading: 'unreal.MaterialShadingModel') -> unreal.Material:
    path = BASE + '/' + name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        existing = unreal.load_asset(path)
        if not isinstance(existing, unreal.Material):
            raise RuntimeError('Existing asset is not a Material: ' + path)
        return existing
    factory = unreal.MaterialFactoryNew()
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, BASE, unreal.Material, factory)
    if not asset:
        raise RuntimeError('Could not create material: ' + name)
    asset.set_editor_property('blend_mode', blend)
    asset.set_editor_property('shading_model', shading)
    unreal.EditorAssetLibrary.save_asset(path)
    return asset


def create_master_materials(report: dict) -> dict:
    report.setdefault('materials', [])
    created = {}
    opaque = unreal.BlendMode.BLEND_OPAQUE
    translucent = unreal.BlendMode.BLEND_TRANSLUCENT
    default = unreal.MaterialShadingModel.MSM_DEFAULT_LIT
    for name, note in MASTERS:
        blend = translucent if name in ('M_GlassFacade', 'M_Water') else opaque
        asset = _material(name, blend, default)
        created[name] = asset
        report['materials'].append({
            'path': BASE + '/' + name,
            'note': note,
            'blend': str(blend),
            'shader_compile': 'NOT_VERIFIED',
        })
    collection_path = '/Game/Scaffold/Materials/MPC_DayNight'
    if not unreal.EditorAssetLibrary.does_asset_exist(collection_path):
        factory = unreal.MaterialParameterCollectionFactoryNew()
        collection = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            'MPC_DayNight', BASE, unreal.MaterialParameterCollection, factory)
        if not collection:
            raise RuntimeError('Could not create MPC_DayNight')
        unreal.EditorAssetLibrary.save_asset(collection_path)
    report['parameter_collection'] = collection_path
    report['warnings'].append(
        'Master materials are parameter-driven shells. Shader compile and visual review require a GPU Editor session.')
    return created
