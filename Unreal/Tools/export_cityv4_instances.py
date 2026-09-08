#!/usr/bin/env python3
"""Read an authored .blend; export indexed geometry, source PBR and instances.

Run with Blender --background --python ... -- --source city-v4.blend --out DIR,
or Python 3.11 with the pinned Blender Foundation bpy package. Never saves .blend.
No decimation, merging, remeshing, browser triangle cap or fabricated UE assets.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from cityv4_format import MAGIC, HEADER, VERTEX, CORNER, U32, audit_mesh, sha256, validate_manifest, write_owned


def material_record(material):
    if material is None:
        return dict(name='Unassigned', base_color=[.8,.8,.8,1], roughness=.5, metallic=0,
                    emission=[0,0,0,1], emission_strength=0, alpha=1, nodes=[], needs_review=True)
    nodes = list(material.node_tree.nodes) if material.use_nodes and material.node_tree else []
    p = next((node for node in nodes if node.type == 'BSDF_PRINCIPLED'), None)
    def value(key, default):
        if p is None or key not in p.inputs:
            return default
        socket = p.inputs[key]
        if socket.is_linked:
            # Preserve the graph description; do not pretend a default is a baked texture.
            return default
        v = socket.default_value
        return list(v) if hasattr(v, '__len__') else float(v)
    graph = []
    for node in nodes:
        inputs = {}
        for socket in node.inputs:
            if socket.is_linked or not hasattr(socket, 'default_value'):
                continue
            v = socket.default_value
            if isinstance(v, (int,float,str,bool)):
                inputs[socket.name] = v
            elif hasattr(v, '__len__'):
                try: inputs[socket.name] = list(v)
                except TypeError: pass
        graph.append(dict(type=node.bl_idname, name=node.name, inputs=inputs,
                          layer_name=str(getattr(node, 'layer_name', ''))))
    return dict(name=material.name, base_color=value('Base Color',list(material.diffuse_color)),
                roughness=value('Roughness',.5), metallic=value('Metallic',0),
                emission=value('Emission Color',[0,0,0,1]), emission_strength=value('Emission Strength',0),
                alpha=value('Alpha',1), nodes=graph,
                links=[dict(from_node=l.from_node.name, from_socket=l.from_socket.name,
                            to_node=l.to_node.name, to_socket=l.to_socket.name) for l in material.node_tree.links] if material.node_tree else [],
                needs_review=p is None or any(s.is_linked for s in p.inputs if s.name in ('Base Color','Roughness','Metallic','Alpha','Emission Color')))


def category(obj, bindings):
    # Linked meshes share a palette; unused slots do not describe this object.
    used = {p.material_index for p in obj.data.polygons}
    text = (obj.name + ' ' + ' '.join(name for i, name in enumerate(bindings) if i in used)).lower()
    if 'open water' in text or 'river solara' in text:
        return 'water'
    if any(v in text for v in ('tree', 'trees /', 'trunks', 'vegetation')):
        return 'vegetation'
    if any(v in text for v in ('terrain','mountain','land /','island base','ridge rock')):
        return 'terrain'
    if any(v in text for v in ('bridge','road','runway','taxiway','seawall','waterfront','asphalt')):
        return 'infrastructure'
    return 'architecture'


def export(source: Path, out: Path) -> dict:
    import bpy
    from mathutils import Matrix, Vector
    source = source.resolve(); out = out.resolve()
    if source.suffix.lower() != '.blend' or not source.is_file():
        raise ValueError('A real source .blend is required')
    if out == source.parent or source.parent in out.parents or any(p.name == 'Assets' for p in (out, *out.parents)):
        raise ValueError('Derived output must be outside authored CityV4/Assets directories')
    original_hash = sha256(source)
    bpy.ops.wm.open_mainfile(filepath=str(source), load_ui=False, use_scripts=False)
    B = Matrix.Diagonal((100.,-100.,100.,1.)); inverse = B.inverted()
    materials = {}; meshes = {}; instances = {}; geometry_cache = {}; scenes = []
    report = dict(schema=1, source_file=source.name, source_sha256=original_hash,
                  exporter_sha256=sha256(Path(__file__)), blender_version=bpy.app.version_string,
                  coordinates='UE_X_NEGY_Z_CM', format='MAIMSH02', meshes=meshes,
                  materials=materials, instances=[], scenes=scenes, source_unchanged=False,
                  unreal_import_verified=False, gpu_render_verified=False)

    def geometry(obj, depsgraph):
        # Unmodified linked data is exported once. Evaluated modifiers are hashed per object.
        matrix = obj.matrix_world.copy()
        mirrored = matrix.to_3x3().determinant() < 0
        # ISM batches use positive-determinant transforms. Preserve mirrored linked
        # meshes as one reflected geometry variant, rather than one mesh per object.
        # Material-linked vertex color layers are part of geometry identity.
        color_bindings = []
        for slot in obj.material_slots:
            material = slot.material
            names = {n.layer_name for n in material.node_tree.nodes if n.type == 'VERTEX_COLOR'} if material and material.node_tree else set()
            if len(names) > 1:
                raise ValueError(f'Multiple vertex-color channels need an explicit material bake: {obj.name}')
            color_bindings.append(next(iter(names)) if names else None)
        cache_key = (obj.data.as_pointer(), mirrored, tuple(color_bindings)) if not obj.modifiers and not obj.data.shape_keys else None
        cols = [matrix.col[i].to_3d() for i in range(3)]
        shear = any(abs(cols[a].dot(cols[b])) > 1e-6 * cols[a].length * cols[b].length for a,b in ((0,1),(0,2),(1,2)))
        if cache_key is not None and not shear and cache_key in geometry_cache:
            return geometry_cache[cache_key]
        evaluated = obj.evaluated_get(depsgraph)
        mesh = evaluated.to_mesh(preserve_all_data_layers=True, depsgraph=depsgraph)
        try:
            # Bake only otherwise-unrepresentable shearing linear transform, keeping translation.
            bake = matrix.to_3x3().to_4x4() if shear else Matrix.Diagonal((-1.,1.,1.,1.)) if mirrored else Matrix.Identity(4)
            points = [bake @ v.co for v in mesh.vertices]
            if not points:
                raise ValueError(f'Empty visible mesh: {obj.name}')
            centre = Vector([(min(p[i] for p in points)+max(p[i] for p in points))/2 for i in range(3)])
            normal_transform = bake.to_3x3().inverted().transposed()
            mesh.calc_loop_triangles()
            slots = max(1, len(obj.material_slots))
            raw = bytearray(HEADER.pack(MAGIC,len(points),len(mesh.loop_triangles),slots))
            for p in points:
                v = B.to_3x3() @ (p-centre)
                raw.extend(VERTEX.pack(*v))
            uv = mesh.uv_layers.active
            color_layers = []
            for binding in color_bindings or [None]:
                attribute = mesh.color_attributes.get(binding) if binding else None
                if attribute is None and (binding == '' or binding is None) and len(mesh.color_attributes):
                    attribute = mesh.color_attributes[mesh.color_attributes.render_color_index]
                if binding is not None and attribute is None:
                    raise ValueError(f'Missing referenced vertex-color attribute {binding!r} on {obj.name}')
                if attribute is not None and attribute.domain not in {'CORNER', 'POINT'}:
                    raise ValueError(f'Unsupported vertex-color domain on {obj.name}')
                color_layers.append(attribute)
            reverse = bake.to_3x3().determinant() >= 0 # one reflection in B; another in bake cancels it
            split = mesh.corner_normals
            for t in mesh.loop_triangles:
                raw.extend(U32.pack(t.material_index))
                order = (0,2,1) if reverse else (0,1,2)
                for k in order:
                    loop = t.loops[k]; index = mesh.loops[loop].vertex_index
                    n = split[loop].vector if len(split) else t.normal
                    n = normal_transform @ n
                    n = Vector((n.x,-n.y,n.z)).normalized()
                    tex = uv.data[loop].uv if uv else (0.,0.)
                    attribute = color_layers[t.material_index]
                    color = attribute.data[loop if attribute.domain == 'CORNER' else index].color if attribute else (1., 1., 1., 1.)
                    raw.extend(CORNER.pack(index,n.x,n.y,n.z,float(tex[0]),1-float(tex[1]),*color))
            digest = hashlib.sha256(raw).hexdigest(); key = digest[:24]
            path = out / 'meshes' / (key+'.maimesh')
            write_owned(path, raw)
            if key not in meshes:
                meshes[key] = dict(file='meshes/'+path.name, source_name=obj.data.name, **audit_mesh(path))
            result = (key, centre.copy(), shear, mirrored)
            if cache_key is not None and not shear:
                geometry_cache[cache_key] = result
            return result
        finally:
            evaluated.to_mesh_clear()

    for scene in sorted(bpy.data.scenes, key=lambda s:s.name):
        if bpy.context.window:
            bpy.context.window.scene = scene
        layer = scene.view_layers[0]
        depsgraph = layer.depsgraph
        visible = [o for o in scene.objects if o.type == 'MESH' and not o.hide_render and o.visible_get(view_layer=layer)]
        raw_unique = {o.data.as_pointer(): o.data for o in visible}
        source_triangles = 0
        for mesh in raw_unique.values():
            mesh.calc_loop_triangles(); source_triangles += len(mesh.loop_triangles)
        scene_record = dict(name=scene.name, visible_mesh_objects=len(visible), source_unique_meshes=len(raw_unique),
                            source_unique_triangles=source_triangles, evaluated_triangles_with_instances=0,
                            resolution=[scene.render.resolution_x,scene.render.resolution_y], cameras=[], lights=[])
        scenes.append(scene_record)
        for n, obj in enumerate(sorted(visible,key=lambda o:o.name)):
            key, centre, shear, mirrored = geometry(obj,depsgraph)
            matrix = obj.matrix_world.copy()
            if shear:
                matrix = Matrix.Translation(matrix.translation)
            elif mirrored:
                matrix = matrix @ Matrix.Diagonal((-1.,1.,1.,1.))
            matrix = B @ matrix @ Matrix.Translation(centre) @ inverse
            bindings = []
            for slot in obj.material_slots:
                name = slot.material.name if slot.material else 'Unassigned'
                materials.setdefault(name,material_record(slot.material)); bindings.append(name)
            if not bindings:
                bindings=['Unassigned'];materials.setdefault('Unassigned',material_record(None))
            identity = hashlib.sha256((obj.name+'\0'+key+'\0'+json.dumps([v for row in matrix for v in row])).encode()).hexdigest()[:24]
            if identity not in instances:
                item = dict(id=identity,name=obj.name,mesh=key,materials=bindings,
                            matrix_cm=[v for row in matrix for v in row],category=category(obj,bindings),scenes=[],
                            game_id=str(obj.get('game_id','')), shear_baked=shear, reflection_baked=mirrored)
                instances[identity] = item
            instances[identity]['scenes'].append(scene.name)
            scene_record['evaluated_triangles_with_instances'] += meshes[key]['triangles']
            if n%5000==0:
                print(f'CityV4 {scene.name}: {n}/{len(visible)} instances, {len(meshes)} unique geometries',flush=True)
        for obj in sorted(scene.objects,key=lambda o:o.name):
            if obj.type == 'CAMERA':
                transform = obj.matrix_world
                loc = B @ transform.translation
                forward = B.to_3x3() @ (transform.to_3x3() @ Vector((0,0,-1)))
                up = B.to_3x3() @ (transform.to_3x3() @ Vector((0,1,0)))
                scene_record['cameras'].append(dict(name=obj.name,active=scene.camera==obj,position_cm=list(loc),
                    forward=list(forward.normalized()),up=list(up.normalized()),type=obj.data.type,
                    ortho_width_cm=obj.data.ortho_scale*100,fov_x_degrees=math.degrees(obj.data.angle_x),
                    clip_start_cm=obj.data.clip_start*100,clip_end_cm=obj.data.clip_end*100))
            elif obj.type == 'LIGHT' and not obj.hide_render:
                scene_record['lights'].append(dict(name=obj.name,type=obj.data.type,energy=obj.data.energy,
                    color=list(obj.data.color),matrix_blender=[v for row in obj.matrix_world for v in row]))
    report['instances'] = sorted(instances.values(),key=lambda i:i['id'])
    report['source_unchanged'] = sha256(source) == original_hash
    if not report['source_unchanged']:
        raise ValueError('Authored source changed during a read-only export')
    manifest = out / 'manifest.json'
    write_owned(manifest, (json.dumps(report,ensure_ascii=False,sort_keys=True,separators=(',',':'))+'\n').encode())
    validate_manifest(manifest,check_geometry=False)
    print(json.dumps({'manifest':str(manifest),'scenes':scenes,'export_unique_geometry':len(meshes),
                      'source_unchanged':True,'unreal_import_verified':False},indent=2),flush=True)
    return report


def main():
    argv=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else sys.argv[1:]
    parser=argparse.ArgumentParser();parser.add_argument('--source',type=Path,required=True);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args(argv);export(args.source,args.out)
if __name__=='__main__': main()
