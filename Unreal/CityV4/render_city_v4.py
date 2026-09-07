import bpy,sys
from pathlib import Path
from mathutils import Vector
OUT=Path(__file__).resolve().parent
bpy.ops.wm.open_mainfile(filepath=str(OUT/'city-v4.blend'))
s=bpy.data.scenes['DAY'];bpy.context.window.scene=s
mode=sys.argv[-1]
if mode=='preview':
 s.render.resolution_percentage=50;s.cycles.samples=8;s.render.filepath=str(OUT/'preview.png');bpy.ops.render.render(write_still=True)
elif mode=='export':
 bpy.ops.object.select_all(action='DESELECT')
 for o in s.objects:
  if o.type=='MESH' and not o.hide_render:o.select_set(True)
 bpy.ops.export_scene.fbx(filepath=str(OUT/'city-v4.fbx'),use_selection=True,object_types={'MESH'},use_mesh_modifiers=True,add_leaf_bones=False,bake_anim=False,path_mode='AUTO',axis_forward='-Y',axis_up='Z')
 print('FBX_BYTES',(OUT/'city-v4.fbx').stat().st_size,flush=True)
else:
 for name in ['DAY','NIGHT']:
  sc=bpy.data.scenes[name];bpy.context.window.scene=sc;sc.frame_set(1 if name=='DAY' else 2);sc.render.filepath=str(OUT/('city-v4-'+name.lower()+'.png'));bpy.ops.render.render(write_still=True)
 bpy.context.window.scene=s;s.frame_set(1)
 for name,loc,target,lens in [('nuclear',(-320,-120,90),(-250,-26,4),48),('suburb',(320,-80,130),(232,64,0),45),('auction',(-52,-40,30),(-26,0,2),48)]:
  if mode=='polished' and name!='suburb':continue
  s.camera.location=loc;s.camera.rotation_euler=(Vector(target)-s.camera.location).to_track_quat('-Z','Y').to_euler();s.camera.data.lens=lens;s.render.resolution_x=1500;s.render.resolution_y=1000;s.render.filepath=str(OUT/('city-v4-'+name+'.png'));bpy.ops.render.render(write_still=True)
