"""Lossless-topology CityV4 interchange, independent of Blender and Unreal.

Positions are centred mesh-local UE centimetres. Instance matrices already include
that centre. Reflection Blender(X,Y,Z)->UE(X,-Y,Z) is accompanied by winding reversal.
The importer must not apply FBX axis conversion or mesh reduction a second time.
"""
from __future__ import annotations
import hashlib
import json
import math
from pathlib import Path
import struct

MAGIC = b'MAIMSH01'
HEADER = struct.Struct('<8sIII')
VERTEX = struct.Struct('<3f')
CORNER = struct.Struct('<I5f')
U32 = struct.Struct('<I')
MAX_VERTICES = 10_000_000
MAX_TRIANGLES = 10_000_000


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(block)
    return digest.hexdigest()


def write_owned(path: Path, payload: bytes) -> None:
    """Idempotent; never overwrite an existing, different author-owned file."""
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.exists():
        if path.read_bytes() != payload:
            raise ValueError(f'Refusing to overwrite different existing derivative: {path}')
        return
    temporary = path.with_name(path.name + '.partial')
    if temporary.exists():
        raise ValueError(f'Incomplete previous export: inspect {temporary} before retry')
    with temporary.open('xb') as stream:
        stream.write(payload)
    temporary.replace(path)


def finite(values, label: str) -> None:
    if any(not isinstance(v, (int, float)) or not math.isfinite(v) for v in values):
        raise ValueError(f'Non-finite {label}')


def audit_mesh(path: Path, expected: dict | None = None) -> dict:
    raw = path.read_bytes()
    if len(raw) < HEADER.size:
        raise ValueError('Truncated mesh header')
    magic, vertices, triangles, slots = HEADER.unpack_from(raw)
    if magic != MAGIC or not 0 < vertices <= MAX_VERTICES or not 0 < triangles <= MAX_TRIANGLES or not 0 < slots <= 4096:
        raise ValueError('Invalid mesh format/counts')
    if len(raw) != HEADER.size + vertices * VERTEX.size + triangles * (U32.size + 3 * CORNER.size):
        raise ValueError('Mesh size does not match declared topology')
    low, high = [math.inf] * 3, [-math.inf] * 3
    cursor = HEADER.size
    positions = []
    for _ in range(vertices):
        p = VERTEX.unpack_from(raw, cursor); cursor += VERTEX.size
        finite(p, 'position'); positions.append(p)
        for axis in range(3):
            low[axis] = min(low[axis], p[axis]); high[axis] = max(high[axis], p[axis])
    used = set(); degenerate = 0
    for _ in range(triangles):
        material, = U32.unpack_from(raw, cursor); cursor += U32.size
        if material >= slots:
            raise ValueError('Triangle material slot is outside mesh bindings')
        used.add(material); ids = []
        for _ in range(3):
            index, nx, ny, nz, u, v = CORNER.unpack_from(raw, cursor); cursor += CORNER.size
            if index >= vertices:
                raise ValueError('Triangle vertex is outside mesh')
            finite((nx, ny, nz, u, v), 'corner'); ids.append(index)
            normal_length = nx * nx + ny * ny + nz * nz
            if normal_length < .5 or normal_length > 1.5:
                raise ValueError('Invalid corner normal')
        a, b, c = (positions[i] for i in ids)
        ab = [b[i] - a[i] for i in range(3)]; ac = [c[i] - a[i] for i in range(3)]
        cross = (ab[1]*ac[2]-ab[2]*ac[1], ab[2]*ac[0]-ab[0]*ac[2], ab[0]*ac[1]-ab[1]*ac[0])
        if sum(v*v for v in cross) < 1e-16:
            degenerate += 1
    result = dict(vertices=vertices, triangles=triangles, material_slots=slots,
                  used_slots=sorted(used), bounds_min_cm=low, bounds_max_cm=high,
                  degenerate_triangles=degenerate, sha256=hashlib.sha256(raw).hexdigest(), bytes=len(raw))
    if expected:
        for key in ('vertices', 'triangles', 'material_slots', 'sha256'):
            if expected.get(key) != result[key]:
                raise ValueError(f'Mesh {path.name}: {key} disagrees with manifest')
    return result


def validate_manifest(path: Path, check_geometry: bool = True) -> dict:
    data = json.loads(path.read_text(encoding='utf-8'))
    if data.get('schema') != 1 or data.get('coordinates') != 'UE_X_NEGY_Z_CM':
        raise ValueError('Unsupported CityV4 manifest')
    if not isinstance(data.get('meshes'), dict) or not data['meshes']:
        raise ValueError('No exported meshes')
    for key, mesh in data['meshes'].items():
        relative = Path(mesh['file'])
        if relative.is_absolute() or '..' in relative.parts or key != mesh['sha256'][:24]:
            raise ValueError('Unsafe geometry path or invalid mesh key')
        if check_geometry:
            audit_mesh(path.parent / relative, mesh)
    identities = set()
    for instance in data['instances']:
        if instance['id'] in identities:
            raise ValueError('Duplicate instance identity')
        identities.add(instance['id'])
        mesh = data['meshes'].get(instance['mesh'])
        if not mesh or len(instance['materials']) != mesh['material_slots']:
            raise ValueError('Missing geometry/material bindings')
        if any(name not in data['materials'] for name in instance['materials']):
            raise ValueError('Unknown material')
        m = instance['matrix_cm']
        if len(m) != 16:
            raise ValueError('Invalid instance matrix')
        finite(m, 'instance matrix')
        # Column-vector affine matrix. Never silently discard shearing transforms.
        if any(abs(m[12+i] - (1 if i == 3 else 0)) > 1e-8 for i in range(4)):
            raise ValueError('Non-affine instance transform')
        cols = [[m[r*4+c] for r in range(3)] for c in range(3)]
        lengths = [math.sqrt(sum(v*v for v in c)) for c in cols]
        if any(v < 1e-10 for v in lengths):
            raise ValueError('Singular instance transform')
        for a, b in ((0,1),(0,2),(1,2)):
            if abs(sum(cols[a][i]*cols[b][i] for i in range(3))) > 1e-5 * lengths[a] * lengths[b]:
                raise ValueError('Shear must be baked by the exporter, not lost in FTransform')
    for scene in data['scenes']:
        visible = [i for i in data['instances'] if scene['name'] in i['scenes']]
        if len(visible) != scene['visible_mesh_objects']:
            raise ValueError('Scene instance count mismatch')
        actual = sum(data['meshes'][i['mesh']]['triangles'] for i in visible)
        if actual != scene['evaluated_triangles_with_instances']:
            raise ValueError('Scene evaluated triangle count mismatch')
    return data
