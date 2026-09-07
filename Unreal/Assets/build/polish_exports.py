"""Final degenerate-face cleanup and export refresh; rendering only for visible changes."""
import bpy,bmesh,sys,json
from pathlib import Path
from mathutils import Vector
HERE=Path(__file__).resolve().parent;sys.path.insert(0,str(HERE));import pipeline,enhance
OUT=HERE.parent
for path in sorted(OUT.glob('*/*/asset.json')):
 a=json.loads(path.read_text());id=a['id'];folder=path.parent
 if '--' in sys.argv and id not in sys.argv[sys.argv.index('--')+1:]:continue
 bpy.ops.wm.open_mainfile(filepath=str(folder/(id+'.blend')));s=bpy.context.scene;s.frame_set(1)
 obs=[o for o in s.objects if o.type=='MESH' and not o.get('presentation') and not o.get('collision') and not o.get('enclosure')]
 enclosure=[o for o in s.objects if o.type=='MESH' and o.get('enclosure')];rigs=[o for o in s.objects if o.type=='ARMATURE'];cols=[o for o in s.objects if o.get('collision') and not o.name.startswith('UCX_Enclosure')]
 # Ceiling fixtures don't carry collision; separate enclosure collision names all begin UCX_Enclosure.
 removed=0
 if a['group']=='interiors' and id in ['technopark','campus','hq'] and not a.get('detailed_foliage'):
  w,d,h=a['interior_dimensions'];left=-w/2;back=d/2
  positions=[(left+.55,-d/3,1),(-left-.55,back-2.4,1)] if id=='technopark' else [(left+.65,back-1.3,1.4),(-left-.65,back-1.3,1.4)] if id=='campus' else [(left+.65,back-.6,1.4)]
  foliage=next(m for m in bpy.data.materials if 'foliage' in m.name)
  oldleaves=[o for o in obs if len(o.data.materials)==1 and o.data.materials[0]==foliage]
  for o in oldleaves:obs.remove(o);bpy.data.objects.remove(o,do_unlink=True)
  new=enhance.detailed_plants(positions,foliage);ob=enhance.join(new,'SM_'+id.replace('-','_')+'_Foliage');ob['asset_id']=id;pipeline.uv(ob);obs.append(ob);a['detailed_foliage']=True
 for o in obs+enclosure:
  bm=bmesh.new();bm.from_mesh(o.data);bmesh.ops.triangulate(bm,faces=list(bm.faces));bad=[f for f in bm.faces if f.calc_area()<1e-13];removed+=len(bad)
  if bad:bmesh.ops.delete(bm,geom=bad,context='FACES')
  bm.to_mesh(o.data);bm.free();o.data.update()
 if a['group']=='vehicles':
  if not a.get('wheels_circularized'):
   ratio=6.3/7.5 if id=='car-5' else 6.0/7.0 if id=='car-2' else 1.0 if id=='car-4' else 5.05/6.5
   for ob in obs:
    if ob.get('part')=='wheel':
     for vert in ob.data.vertices:vert.co.y*=ratio
   a['wheels_circularized']=True
  for o in obs:
   if 'cabin' in o.name.lower() or 'glazing' in o.name.lower():
    for p in o.data.polygons:p.use_smooth=True
  lo,hi=pipeline.bounds(obs);size=max(hi-lo);target=(lo+hi)/2
  key=bpy.data.objects.get('Key')
  if key:key.location=target+Vector((-1.8,.15,2.1))*size;key.rotation_euler=(target-key.location).to_track_quat('-Z','Y').to_euler()
  # Keep a low-intensity fill in the windshield; the key now highlights the body shoulder.
  fill=bpy.data.objects.get('Fill')
  if fill and not a.get('studio_polished'):fill.data.energy*=.55
  a['studio_polished']=True
  s.render.filepath=str(folder/(id+'.png'));bpy.ops.render.render(write_still=True)
 if a.get('detailed_foliage'):
  s.render.filepath=str(folder/(id+'.png'));bpy.ops.render.render(write_still=True)
 pipeline.export_fbx(folder/(id+'.fbx'),obs+rigs+cols,animated=bool(rigs))
 if enclosure:
  ec=[o for o in s.objects if o.get('collision') and o.name.startswith('UCX_Enclosure')];pipeline.export_fbx(folder/(id+'-enclosure.fbx'),enclosure+ec)
 pipeline.select(obs+rigs);bpy.ops.export_scene.gltf(filepath=str(folder/(id+'.glb')),export_format='GLB',use_selection=True,export_animations=bool(rigs),export_cameras=False,export_lights=False,export_extras=True,export_tangents=True)
 for o in enclosure+[o for o in s.objects if o.get('collision')]:o.hide_set(True);o.hide_render=True
 materialfile=folder/'materials.json';md=json.loads(materialfile.read_text())
 for entry in md:
  material=bpy.data.materials.get(entry['name'])
  if material:
   p=material.node_tree.nodes.get('Principled BSDF');entry['coat_roughness']=p.inputs['Coat Roughness'].default_value;entry['normal_strength']=next((n.inputs['Strength'].default_value for n in material.node_tree.nodes if n.type=='NORMAL_MAP'),1.0)
 materialfile.write_text(json.dumps(md,ensure_ascii=False,indent=2))
 a['objects']=len(obs);a['triangles']=sum(len(o.data.polygons) for o in obs);a['enclosure_triangles']=sum(len(o.data.polygons) for o in enclosure);a['removed_degenerate_faces']=removed;a['fbx_bytes']=(folder/(id+'.fbx')).stat().st_size;a['glb_bytes']=(folder/(id+'.glb')).stat().st_size;s['export_triangles']=a['triangles'];path.write_text(json.dumps(a,ensure_ascii=False,indent=2));bpy.ops.wm.save_as_mainfile(filepath=str(folder/(id+'.blend')),compress=True)
 print('POLISHED',id,removed,flush=True)
