"""Create an Editor map from imported project assets, without replacing an existing map."""
from pathlib import Path
import unreal

MAP = '/Game/Scaffold/Maps/L_Scaffold_City'


def create_scene(project: Path, meshes: dict, markers: dict, classes: dict, report: dict) -> None:
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if unreal.EditorAssetLibrary.does_asset_exist(MAP):
        raise RuntimeError('Existing map will not be overwritten')
    try:
        created = levels.new_level(MAP, is_partitioned_world=True)
        report['partition_creation_requested'] = True
    except TypeError:
        created = levels.new_level(MAP)
        report['partition_creation_requested'] = False
        report['warnings'].append('Partition conversion is required on this Editor version.')
    if not created:
        raise RuntimeError('Map creation failed')
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property('default_game_mode', classes['MaiGameMode'])

    def place(mesh, label, position=None):
        actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, position or unreal.Vector(0, 0, 0))
        if not actor:
            raise RuntimeError('Actor creation failed: ' + label)
        actor.static_mesh_component.set_static_mesh(mesh)
        actor.set_actor_label(label)
        actor.set_editor_property('is_spatially_loaded', False)
        return actor

    place(meshes['city'], 'CityV4 full reference - not spatial streaming chunks')
    root = actors.spawn_actor_from_class(classes['MaiScaffoldWorld'], unreal.Vector(0, 0, 0))
    root.set_editor_property('imported_city', True)
    root.set_editor_property('is_spatially_loaded', False)
    root.set_editor_property('marker_positions', {key: unreal.Vector(*v['candidate_ue_cm']) for key, v in markers['markers'].items()})
    root.set_editor_property('garage_interior_mesh', meshes['garage'])
    root.set_editor_property('garage_enclosure_mesh', meshes['enclosure'])
    entry = markers['markers']['garage']
    x, y = entry['candidate_ue_cm'][:2]
    z = entry['bounds_m'][0][2] * 100
    garage = place(meshes['garage'], 'Garage source reference')
    origin, extent = garage.get_actor_bounds(False)
    report['garage_dimensions_cm'] = [extent.x * 2, extent.y * 2, extent.z * 2]
    if not 300 <= extent.z * 2 <= 450:
        raise RuntimeError('Garage height is not approximately 368 cm; inspect import units')
    garage.set_actor_location(unreal.Vector(x - origin.x, y - origin.y, z - origin.z + extent.z), False, False)
    shell = place(meshes['enclosure'], 'Separate Garage enclosure collision', garage.get_actor_location())
    shell.static_mesh_component.set_visibility(False)
    person = actors.spawn_actor_from_class(unreal.SkeletalMeshActor, unreal.Vector(x + 500, y, z))
    person.skeletal_mesh_component.set_skeletal_mesh_asset(meshes['person'])
    person.set_actor_label('Person1 original skeleton - LOD gate outstanding')
    person.set_editor_property('is_spatially_loaded', False)
    sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 30000), unreal.Rotator(-40, -35, 0))
    sun.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sun.light_component.set_intensity(3.0)
    actors.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0))
    sky = actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 1000))
    sky.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sky.light_component.set_real_time_capture_enabled(True)
    if not levels.save_current_level():
        raise RuntimeError('Map save failed')
    physical = project / 'Content/Scaffold/Maps/L_Scaffold_City.umap'
    if not physical.is_file() or physical.stat().st_size < 128:
        raise RuntimeError('Saved map binary is missing')
    report.update(map=MAP, map_file_bytes=physical.stat().st_size,
                  marker_axis_verified=False, runtime_collision_traces_verified=False,
                  skeletal_lod_triangles=[None, None, None], runtime_cell_size_cm=None,
                  runtime_loading_range_cm=None, proposed_cell_size_cm=12800,
                  proposed_loading_range_cm=51200, data_layers=[], city_streaming_ready=False)
    report['warnings'].append('Combined city preserves composition but is not fine-grained World Partition streaming. Camera, marker orientation, collisions and rendering still require verification.')
