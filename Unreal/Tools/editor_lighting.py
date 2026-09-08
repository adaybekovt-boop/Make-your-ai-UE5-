"""One day/night parameter system. Does not invent a second incompatible light rig."""
from __future__ import annotations
import unreal

MPC = '/Game/Scaffold/Materials/MPC_DayNight'


def _find_or_spawn(world, cls, label: str):
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    matches = [a for a in actors if isinstance(a, cls)]
    if len(matches) > 1:
        raise RuntimeError('Several lights of this type exist; inspect duplicates. No actor was deleted or added.')
    if matches:
        existing = matches[0]
        if existing.get_actor_label() != label:
            raise RuntimeError('An authored/CityV4 light already exists. Use its rig; a duplicate will not be spawned or overwrite it.')
        return existing
    spawned = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).spawn_actor_from_class(cls, unreal.Vector(0, 0, 30000))
    if not spawned:
        raise RuntimeError('Could not spawn ' + label)
    spawned.set_actor_label(label)
    return spawned


def apply_day_night(report: dict, mode: str = 'day') -> None:
    if mode not in ('day', 'night'):
        raise ValueError('mode must be day or night')
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    sun = _find_or_spawn(world, unreal.DirectionalLight, 'MAI_Sun')
    sky = _find_or_spawn(world, unreal.SkyLight, 'MAI_Sky')
    sun.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sky.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    if mode == 'day':
        sun.set_actor_rotation(unreal.Rotator(-40, -35, 0), False)
        sun.light_component.set_intensity(3.0)
        sun.light_component.set_light_color(unreal.LinearColor(1.0, 0.96, 0.88, 1))
        sky.light_component.set_intensity(1.0)
    else:
        sun.set_actor_rotation(unreal.Rotator(-8, -35, 0), False)
        sun.light_component.set_intensity(0.15)
        sun.light_component.set_light_color(unreal.LinearColor(0.35, 0.42, 0.7, 1))
        sky.light_component.set_intensity(0.25)
    if unreal.EditorAssetLibrary.does_asset_exist(MPC):
        collection = unreal.load_asset(MPC)
        report['mpc'] = MPC
        report['mpc_applied'] = bool(collection)
    else:
        report['mpc'] = None
        report['warnings'].append('MPC_DayNight is missing; run editor_materials.create_master_materials first.')
    report['lighting_mode'] = mode
    report['lumen_requested'] = True
    report['lumen_verified'] = False
    report['virtual_shadow_maps_requested'] = True
    report['virtual_shadow_maps_verified'] = False
    report['screenshot'] = None
    report['warnings'].append('Day/night actors were configured. Visual proof requires a dated PNG from this Editor.')
