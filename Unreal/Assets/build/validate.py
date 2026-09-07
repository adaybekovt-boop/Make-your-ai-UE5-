"""Read-only validation: exported FBX round trip, scale, skin weights and glTF structure."""
import bpy,json,sys,struct,math
from pathlib import Path
from mathutils import Vector
OUT=Path(__file__).resolve().parents[1];results=[]
def bounds(obs):
 bpy.context.view_layer.update();v=[o.matrix_world@Vector(c) for o in obs for c in o.bound_box]
 return [max(p[i] for p in v)-min(p[i] for p in v) for i in range(3)]
for path in sorted(OUT.glob('*/*/asset.json')):
 try:
  a=json.loads(path.read_text())
  if '--' in sys.argv and a['group'] not in sys.argv[sys.argv.index('--')+1:]:continue
  folder=path.parent;id=a['id'];bpy.ops.wm.open_mainfile(filepath=str(folder/(id+'.blend')));s=bpy.context.scene;s.frame_set(1)
  obs=[o for o in s.objects if o.type=='MESH' and not o.get('presentation') and not o.get('collision') and not o.get('enclosure')]
  actual=sum(len(o.data.polygons) for o in obs);nonfinite=sum(not all(math.isfinite(c) for c in v.co) for o in obs for v in o.data.vertices);ngons=sum(len(p.vertices)!=3 for o in obs for p in o.data.polygons);degenerate=sum(p.area<1e-13 for o in obs for p in o.data.polygons)
  rigs=[o for o in s.objects if o.type=='ARMATURE'];unweighted=0
  if rigs:
   for o in obs:
    for v in o.data.vertices:
     if sum(g.weight for g in v.groups)<=.999:unweighted+=1
   # Ensure the preview cycle actually deforms vertices.
   deps=bpy.context.evaluated_depsgraph_get();p1=[v.co.copy() for o in obs for v in o.evaluated_get(deps).data.vertices];s.frame_set(7);deps=bpy.context.evaluated_depsgraph_get();p7=[v.co.copy() for o in obs for v in o.evaluated_get(deps).data.vertices];movement=max((a-b).length for a,b in zip(p1,p7));s.frame_set(1)
  else:movement=None
  raw=(folder/(id+'.glb')).read_bytes();length,kind=struct.unpack_from('<II',raw,12);gltf=json.loads(raw[20:20+length]);prims=[p for m in gltf.get('meshes',[]) for p in m['primitives']];tangent=all('TANGENT' in p['attributes'] for p in prims if 'NORMAL' in p['attributes']);gltri=sum(gltf['accessors'][p['indices']]['count']//3 for p in prims)
  # Fresh FBX import, not a successful-export assumption.
  bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=str(folder/(id+'.fbx')),use_anim=True);imported=[o for o in bpy.context.scene.objects if o.type=='MESH' and not o.name.startswith('UCX_')];dims=bounds(imported);rig2=[o for o in bpy.context.scene.objects if o.type=='ARMATURE'];bones=sum(len(o.data.bones) for o in rig2);scale_error=max(abs(x-y)/max(.01,y) for x,y in zip(dims,a['dimensions_m']));fbtri=sum(sum(len(p.vertices)-2 for p in o.data.polygons) for o in imported)
  row={'id':id,'source_blend_triangles':actual,'fbx_import_triangles':fbtri,'glb_triangles':gltri,'fbx_dimensions_m':dims,'relative_scale_error':scale_error,'nonfinite_vertices':nonfinite,'nontriangle_faces':ngons,'degenerate_faces':degenerate,'unweighted_vertices':unweighted,'bones_after_fbx_import':bones,'animation_max_vertex_motion_m':movement,'gltf_tangents_present':tangent,'fbx_armature_actions':len(bpy.data.actions),'unreal_editor_tested':False}
  row['passed']=actual==a['triangles'] and fbtri==actual and gltri==actual and nonfinite==0 and ngons==0 and unweighted==0 and bones==a['bones'] and scale_error<.02 and (movement is None or movement>.02)
  results.append(row);print('VALIDATED',id,row['passed'],scale_error,degenerate,flush=True)
 except Exception as exc:
  results.append({"id":path.parent.name,"passed":False,"error":str(exc)});print("VALIDATION_ERROR",path.parent.name,str(exc),flush=True)
(OUT/('validation-partial.json' if '--' in sys.argv else 'validation.json')).write_text(json.dumps(results,indent=2));print('VALIDATION_COMPLETE',len(results),sum(r['passed'] for r in results),flush=True)
