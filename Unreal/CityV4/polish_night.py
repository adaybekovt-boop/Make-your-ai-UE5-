import bpy,json
from pathlib import Path
OUT=Path(__file__).resolve().parent
bpy.ops.wm.open_mainfile(filepath=str(OUT/'city-v4.blend'))
s=bpy.data.scenes['DAY'];night=bpy.data.scenes['NIGHT'];bpy.context.window.scene=s
light=bpy.data.materials['Windows / evening'];sock=light.node_tree.nodes['Principled BSDF'].inputs['Emission Strength'];sock.default_value=.03;sock.keyframe_insert('default_value',frame=1);sock.default_value=8;sock.keyframe_insert('default_value',frame=2)
meshes=sorted({o.data for o in s.objects if o.name.startswith('Greenhaven suburb / house')},key=lambda m:m.name)
for k,me in enumerate(meshes):
 if k%3==0:continue
 for i,ma in enumerate(me.materials):
  if ma.name=='Facade / silver blue':me.materials[i]=light
for name,loc,energy,size in [('Nuclear working light',(-250,-28,26),2600,42),('Auction plaza light',(-27.5,-4,12),500,15)]:
 data=bpy.data.lights.new(name,'AREA');data.energy=energy;data.color=(1,.72,.42);data.shape='DISK';data.size=size;o=bpy.data.objects.new(name,data);night.collection.objects.link(o);o.location=loc
s.frame_set(1);bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'city-v4.blend'),compress=True)
print('NIGHT_POLISHED',flush=True)
