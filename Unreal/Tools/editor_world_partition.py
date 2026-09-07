"""Apply World Partition cell size and streaming range only on a real partitioned map."""
from __future__ import annotations
import unreal

PROPOSED_CELL_CM = 12800
PROPOSED_LOADING_CM = 51200


def configure_streaming(report: dict, cell_cm: int = PROPOSED_CELL_CM, loading_cm: int = PROPOSED_LOADING_CM) -> None:
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    settings = world.get_world_settings()
    report['proposed_cell_size_cm'] = cell_cm
    report['proposed_loading_range_cm'] = loading_cm
    report['runtime_cell_size_cm'] = None
    report['runtime_loading_range_cm'] = None
    report['world_partition_enabled'] = False
    partition = None
    for name in ('world_partition', 'WorldPartition'):
        try:
            partition = settings.get_editor_property(name)
            if partition:
                break
        except Exception:
            continue
    if not partition:
        report['warnings'].append(
            'World Partition object was not found. Combined CityV4 is not a streaming layout; do not claim WP is active.')
        return
    applied = {}
    for prop, value in (
        ('default_cell_size', cell_cm),
        ('default_placement_grid_cell_size', cell_cm),
        ('loading_range', loading_cm),
    ):
        try:
            partition.set_editor_property(prop, value)
            applied[prop] = value
        except Exception as error:
            report['warnings'].append(f'Could not set {prop}: {error}')
    report['world_partition_enabled'] = True
    report['applied_partition_properties'] = applied
    report['runtime_cell_size_cm'] = applied.get('default_cell_size') or applied.get('default_placement_grid_cell_size')
    report['runtime_loading_range_cm'] = applied.get('loading_range')
    report['warnings'].append(
        'Streaming numbers are Editor-applied requests. Verify with stat streaming that the whole city is not resident at start.')
