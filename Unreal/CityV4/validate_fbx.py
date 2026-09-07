import bpy,json
from pathlib import Path
OUT=Path(__file__).resolve().parent
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=str(OUT/'city-v4.fbx'),use_anim=False)
obs=[o for o in bpy.context.scene.objects if o.type=='MESH']
report=json.loads((OUT/'city-v4-report.json').read_text())
result={'mesh_objects':len(obs),'triangles_instances':sum(sum(len(p.vertices)-2 for p in o.data.polygons) for o in obs),'unique_meshes':len({o.data for o in obs}),'fbx_bytes':(OUT/'city-v4.fbx').stat().st_size,'blender_roundtrip_only':True,'ue5_import_tested':False}
result['counts_match']=result['mesh_objects']==report['visible_mesh_objects'] and result['triangles_instances']==report['triangles_instances']
(OUT/'city-v4-fbx-validation.json').write_text(json.dumps(result,indent=2))
print('VALIDATED',json.dumps(result),flush=True)
assert result['counts_match'],result
