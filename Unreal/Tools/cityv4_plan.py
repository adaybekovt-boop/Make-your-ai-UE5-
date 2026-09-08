"""Pure, testable spatial/material grouping; no polygon or object budget cuts."""
from __future__ import annotations
from collections import defaultdict
import hashlib
import json
import math
from pathlib import Path
from cityv4_format import validate_manifest


def make_plan(manifest: Path, cell_cm: float = 2500, scene_name: str = 'DAY') -> dict:
    data = validate_manifest(manifest)
    if not math.isfinite(cell_cm) or cell_cm <= 0:
        raise ValueError('Invalid spatial cell size')
    scene = next((s for s in data['scenes'] if s['name'] == scene_name), None)
    if scene is None:
        raise ValueError('Requested authored scene is absent')
    camera = next((c for c in scene['cameras'] if c['active']), None)
    if camera is None:
        raise ValueError('No authored active camera')
    batches = defaultdict(list)
    for item in data['instances']:
        if scene_name not in item['scenes']:
            continue
        m = item['matrix_cm']
        key = (item['category'], math.floor(m[3] / cell_cm), math.floor(m[7] / cell_cm),
               item['mesh'], tuple(item['materials']), item['game_id'])
        batches[key].append(item)
    groups = []
    for key, items in sorted(batches.items()):
        identity = hashlib.sha256(json.dumps(key).encode()).hexdigest()[:24]
        groups.append(dict(id=identity, category=key[0], cell=[key[1], key[2]], mesh=key[3],
                           materials=list(key[4]), game_id=key[5],
                           matrices=[i['matrix_cm'] for i in items], ids=[i['id'] for i in items]))
    count = sum(len(g['ids']) for g in groups)
    triangles = sum(len(g['ids']) * data['meshes'][g['mesh']]['triangles'] for g in groups)
    if count != scene['visible_mesh_objects'] or triangles != scene['evaluated_triangles_with_instances']:
        raise ValueError('Grouping lost instances or geometry')
    return dict(scene=scene_name, camera=camera, cell_cm=cell_cm, batches=groups,
                instance_count=count, triangles_with_instances=triangles, unique_geometry=len(data['meshes']))
