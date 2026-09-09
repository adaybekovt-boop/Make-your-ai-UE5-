"""Original deterministic near-tree geometry, not a lossless source conversion.

MAIMSH02 output uses the source bounds, pivot and material slot indices. The
original source mesh must remain a separate distant LOD; never replace all
21,062 instances with this near geometry at every distance.
"""
from __future__ import annotations
import hashlib
import math
import random
from cityv4_format import HEADER, MAGIC, VERTEX, CORNER, U32


def add(a, b):
    return tuple(x+y for x, y in zip(a, b))


def mul(a, scale):
    return tuple(x*scale for x in a)


def cross(a, b):
    return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])


def unit(a):
    length = math.sqrt(sum(x*x for x in a))
    if length < 1.e-9:
        raise ValueError('Degenerate tree vector')
    return mul(a, 1/length)


def build_tree(source: dict, colors: dict[int, tuple]) -> bytes:
    name = source['source_name']
    if name not in ('template tree.001', 'template tree.002', 'template tree.004'):
        raise ValueError('Only the three audited city tree templates are supported')
    if source['material_slots'] <= 8:
        raise ValueError('Tree source material bindings changed')
    rng = random.Random(int.from_bytes(hashlib.sha256(name.encode()).digest()[:8], 'little'))
    conifer = name.endswith('.004')
    points, triangles = [], []

    def vertex(p):
        points.append(p)
        return len(points)-1

    def face(ids, slot, shade=1., normals=None):
        triangles.append((tuple(ids), slot, shade, normals))

    def branch(start, end, radius, tip):
        axis = unit(add(end, mul(start, -1)))
        side = unit(cross(axis, (0, 1, 0)))
        other = cross(axis, side)
        rings = []
        for position, size in ((start, radius), (end, tip)):
            rings.append([vertex(add(position, add(mul(side, size*math.cos(i*math.tau/8)),
                                                  mul(other, size*math.sin(i*math.tau/8))))) for i in range(8)])
        for i in range(8):
            j = (i+1) % 8
            ni=add(mul(side,math.cos(i*math.tau/8)),mul(other,math.sin(i*math.tau/8)))
            nj=add(mul(side,math.cos(j*math.tau/8)),mul(other,math.sin(j*math.tau/8)))
            face((rings[0][i], rings[0][j], rings[1][j]), 8, normals=(ni,nj,nj))
            face((rings[0][i], rings[1][j], rings[1][i]), 8, normals=(ni,nj,ni))
        for ring, reverse in ((rings[0], True), (rings[1], False)):
            for i in range(1, 7):
                face((ring[0], ring[i+1], ring[i]) if reverse else (ring[0], ring[i], ring[i+1]), 8)

    def leaf(center, direction, length, width):
        length *= .48
        width *= .48
        forward = unit(direction)
        side = unit(cross(forward, (0, 0, 1)))
        normal = unit(cross(forward, side))
        outline = [vertex(add(center,add(mul(forward,length*math.cos(i*math.tau/8)),
                                         mul(side,width*math.sin(i*math.tau/8))))) for i in range(8)]
        top = vertex(add(center, mul(normal, width*.045)))
        bottom = vertex(add(center, mul(normal, -width*.02)))
        slot, shade = rng.choice((6, 7)), rng.uniform(.78, 1.16)
        for i in range(8):
            face((outline[i], outline[(i+1) % 8], top), slot, shade, (normal,)*3)
            face((outline[(i+1) % 8], outline[i], bottom), slot, shade*.9, (mul(normal,-1),)*3)

    # Tapered trunk and branching are true geometry. Every leaf is closed, so
    # neither translucent sorting nor two-sided alpha overdraw is required.
    branch((0, 0, 0), (.025, -.02, .94), .038, .0015)
    levels = 5 if conifer else 4
    for level in range(levels):
        z = .24+level*(.135 if conifer else .17)
        reach = (.49*(1-z)+.08) if conifer else (.34 if level in (1, 2) else .25)
        count = 6
        for arm in range(count):
            angle = arm*math.tau/count+level*2.399+rng.uniform(-.15, .15)
            direction = (math.cos(angle), math.sin(angle), .2)
            start = (.025*z, -.02*z, z)
            end = add(start, (reach*direction[0], reach*direction[1], .08))
            branch(start, end, .010, .002)
            for index in range(38 if conifer else 54):
                t = rng.uniform(.32, 1.08)
                center = add(start, mul(add(end, mul(start, -1)), t))
                spread = .065 if conifer else .10
                center = add(center, (rng.uniform(-spread, spread), rng.uniform(-spread, spread), rng.uniform(-.06, .12)))
                yaw = angle+rng.uniform(-1.6, 1.6)
                leaf(center, (math.cos(yaw), math.sin(yaw), rng.uniform(-.35, .5)),
                     rng.uniform(.035, .075), rng.uniform(.012, .025) if conifer else rng.uniform(.024, .045))
    for index in range(48):
        yaw=index*2.399
        leaf((.025+.03*math.cos(yaw),-.02+.03*math.sin(yaw),.91+rng.uniform(-.015,.04)),
             (math.cos(yaw),math.sin(yaw),.35),.06,.018 if conifer else .035)

    # Normalize the authored replacement into the exact existing mesh envelope.
    # This preserves pivots, source placement and maximum forest silhouette size.
    low = tuple(min(p[i] for p in points) for i in range(3))
    high = tuple(max(p[i] for p in points) for i in range(3))
    source_low, source_high = source['bounds_min_cm'], source['bounds_max_cm']
    if any(not math.isfinite(x) for x in (*source_low, *source_high)) or any(source_high[i] <= source_low[i] for i in range(3)):
        raise ValueError('Invalid tree source bounds')
    scale=tuple((source_high[i]-source_low[i])/(high[i]-low[i]) for i in range(3))
    points = [tuple(source_low[i]+(p[i]-low[i])*scale[i] for i in range(3)) for p in points]
    raw = bytearray(HEADER.pack(MAGIC, len(points), len(triangles), source['material_slots']))
    for p in points:
        raw.extend(VERTEX.pack(*p))
    for ids, slot, shade, normals in triangles:
        a, b, c = (points[i] for i in ids)
        normal = unit(cross(add(b, mul(a, -1)), add(c, mul(a, -1))))
        color = tuple(max(0., min(1., value*shade)) for value in colors[slot][:3])+(1.,)
        raw.extend(U32.pack(slot))
        for corner,(index, uv) in enumerate(zip(ids, ((0., 0.), (1., 0.), (.5, 1.)))):
            shading=unit(tuple(normals[corner][axis]/scale[axis] for axis in range(3))) if normals else normal
            raw.extend(CORNER.pack(index, *shading, *uv, *color))
    return bytes(raw)
