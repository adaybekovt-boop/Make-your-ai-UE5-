#!/usr/bin/env python3
"""Run with Blender --background --python this_file -- --repo-root PATH.
Read the original city without saving it. Write only derived JSON under Saved.
Object origins are NOT used: the source's interactive geometry has baked vertices.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import sys

PREFIXES = {
    'garage': ('location_garage',),
    'workshop': ('location_workshop',),
    'technopark': ('location_technopark',),
    'server-hall': ('location_server_hall',),
    'campus': ('location_campus',),
    'dc-north': ('location_dc_north',),
    'dc-south': ('location_dc_south',),
    'auction': ('auction', 'location_auction'),
    'nuclear-power': ('nuclear', 'location_nuclear'),
    'greenhaven': ('greenhaven', 'location_greenhaven'),
}
REQUIRED = {'garage', 'workshop', 'technopark', 'server-hall', 'campus', 'auction', 'nuclear-power', 'greenhaven'}


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for part in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(part)
    return digest.hexdigest()


def normalized(name: str) -> str:
    return name.lower().replace('-', '_').replace(' ', '_')


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo-root', type=Path, required=True)
    args = parser.parse_args(argv)
    repo = args.repo_root.resolve()
    source = repo / 'Unreal/CityV4/city-v4.blend'
    out = repo / 'Unreal/MakeYourAI/Saved/ScaffoldSource'
    original = sha256(source)
    import bpy
    from mathutils import Vector
    bpy.ops.wm.open_mainfile(filepath=str(source))
    graph = bpy.context.evaluated_depsgraph_get()
    markers = {}
    for key, prefixes in PREFIXES.items():
        objects = [obj for obj in bpy.context.scene.objects if obj.type == 'MESH' and any(normalized(obj.name).startswith(p) for p in prefixes)]
        corners = []
        for obj in objects:
            evaluated = obj.evaluated_get(graph)
            corners.extend(evaluated.matrix_world @ Vector(corner) for corner in evaluated.bound_box)
        if not corners:
            continue
        low = [min(c[i] for c in corners) for i in range(3)]
        high = [max(c[i] for c in corners) for i in range(3)]
        center = [(low[i] + high[i]) / 2 for i in range(3)]
        markers[key] = {'objects': [o.name for o in objects], 'bounds_m': [low, high],
                        'candidate_ue_cm': [center[0] * 100, -center[1] * 100, high[2] * 100 + 200]}
    if sha256(source) != original:
        raise RuntimeError('Original source changed unexpectedly')
    missing = sorted(REQUIRED - markers.keys())
    report = {'schema': 1, 'source': str(source.relative_to(repo)), 'source_sha256': original,
              'blender_version': bpy.app.version_string, 'source_unchanged': True,
              'marker_basis': 'X, -Y, Z; meters to centimeters; candidate requires UE orientation check',
              'axis_verified_in_unreal': False, 'markers': markers, 'missing_required': missing}
    out.mkdir(parents=True, exist_ok=True)
    (out / 'city-markers.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(report, indent=2))
    if missing:
        raise RuntimeError('Required authored marker bounds were not found: ' + ', '.join(missing))
    return 0


if __name__ == '__main__':
    arguments = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
    raise SystemExit(main(arguments))
