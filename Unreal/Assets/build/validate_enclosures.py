import bpy,json
from pathlib import Path
OUT=Path(__file__).resolve().parents[1];rows=json.loads((OUT/'validation.json').read_text())
for path in sorted(OUT.glob('interiors/*/asset.json')):
 a=json.loads(path.read_text());row=next(r for r in rows if r['id']==a['id'])
 try:
  bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=str(path.parent/(a['id']+'-enclosure.fbx')));obs=[o for o in bpy.context.scene.objects if o.type=='MESH' and not o.name.startswith('UCX_')];tri=sum(sum(len(p.vertices)-2 for p in o.data.polygons) for o in obs);cols=sum(o.name.startswith('UCX_') for o in bpy.context.scene.objects);passed=tri==a['enclosure_triangles'] and cols==6;row['enclosure_fbx']={'triangles':tri,'collision_meshes':cols,'passed':passed};row['passed']=row['passed'] and passed;print('ENCLOSURE',a['id'],passed,tri,cols,flush=True)
 except Exception as e:row['enclosure_fbx']={'passed':False,'error':str(e)};row['passed']=False;print('ENCLOSURE_ERROR',a['id'],str(e),flush=True)
(OUT/'validation.json').write_text(json.dumps(rows,indent=2))
