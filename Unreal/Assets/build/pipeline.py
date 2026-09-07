import bpy,sys,json,math,re,hashlib,bmesh
from pathlib import Path
from mathutils import Vector,Matrix
from blender_common import *
OUT=Path(__file__).resolve().parents[1]

def meshes():return [o for o in bpy.context.scene.objects if o.type=='MESH' and not o.get('presentation') and not o.get('collision')]
def bounds(obs):
 bpy.context.view_layer.update();v=[o.matrix_world@Vector(c) for o in obs for c in o.bound_box];return Vector(tuple(min(p[i] for p in v) for i in range(3))),Vector(tuple(max(p[i] for p in v) for i in range(3)))
def select(obs):
 bpy.ops.object.select_all(action='DESELECT')
 for o in obs:o.hide_set(False);o.select_set(True)
 if obs:bpy.context.view_layer.objects.active=obs[0]

def surface(m):
 name=m.name.lower();p=m.node_tree.nodes.get('Principled BSDF')
 if not p:return None
 if any(w in name for w in ['glass','glaz','lens','halo','led','indicator','illumination','screen','features','iris','pupil','sclera','light']):return None
 if any(w in name for w in ['shirt','trouser','fabric','silk','print']):kind='fabric'
 elif 'skin' in name:kind='skin'
 elif any(w in name for w in ['oak','wood','timber']):kind='oak'
 elif any(w in name for w in ['floor','plaster','wall','ivory']):kind='concrete'
 elif any(w in name for w in ['rubber','tyre','recess','cable','trim']):kind='rubber'
 elif any(w in name for w in ['shoe','hair']):kind='leather'
 elif any(w in name for w in ['paint','coat','metal','alloy','steel','alumin','chrome','rotor','copper','shell','metal']):kind='metal'
 else:return None
 for suffix,socket in [('Roughness','Roughness'),('Normal','Normal')]:
  im=bpy.data.images.load(str(OUT/'textures'/f'T_{kind}_{suffix}.png'),check_existing=True);im.colorspace_settings.name='Non-Color';n=m.node_tree.nodes.new('ShaderNodeTexImage');n.image=im;n.name='PBR_'+suffix;n.label='Shared seamless '+kind+' '+suffix;n.interpolation='Linear'
  if suffix=='Normal':
   normal=m.node_tree.nodes.new('ShaderNodeNormalMap');normal.inputs['Strength'].default_value=.45 if kind in ['metal','skin'] else .75;m.node_tree.links.new(n.outputs['Color'],normal.inputs['Color']);m.node_tree.links.new(normal.outputs['Normal'],p.inputs['Normal'])
  else:m.node_tree.links.new(n.outputs['Color'],p.inputs[socket])
 return kind

def uv(o):
 if not o.data.uv_layers:o.data.uv_layers.new(name='UV0_Surface')
 layer=o.data.uv_layers.active
 for p in o.data.polygons:
  axis=max(range(3),key=lambda i:abs(p.normal[i]));axes=[i for i in range(3) if i!=axis]
  for li in p.loop_indices:
   co=o.data.vertices[o.data.loops[li].vertex_index].co
   layer.data[li].uv=(co[axes[0]]*2,co[axes[1]]*2)
 # UV0 uses a 0.5 m tile. Deliberately not a unique lightmap atlas.

def collision_box(name,lo,hi,material):
 o=cube(name,(lo+hi)/2,hi-lo,material);o['collision']=True;o.display_type='WIRE';o.hide_render=True;return o

def export_fbx(path,obs,animated=False):
 select(obs);bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,object_types={'MESH','ARMATURE'},global_scale=1.0,apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',use_mesh_modifiers=True,mesh_smooth_type='FACE',use_tspace=True,add_leaf_bones=False,armature_nodetype='NULL',bake_anim=animated,bake_anim_use_all_actions=False,bake_anim_use_nla_strips=False,bake_anim_simplify_factor=0,path_mode='COPY',embed_textures=True,use_custom_props=True)

def studio(asset_id,obs,folder,interior=False,person=False):
 s=bpy.context.scene;lo,hi=bounds(obs);centre=(lo+hi)/2;size=max(hi-lo);s.world=bpy.data.worlds.new('Neutral studio');s.world.use_nodes=True;s.world.node_tree.nodes['Background'].inputs[0].default_value=(.11,.14,.19,1);s.world.node_tree.nodes['Background'].inputs[1].default_value=.35
 groundmat=mat('Presentation floor',(.055,.065,.08),.12,.42);ground=cube('Studio floor',(0,0,lo.z-.065),(size*200,size*200,.1),groundmat);ground['presentation']=True
 target=centre.copy()
 if interior:target.z=.8
 if person:direction=Vector((.45,-2.4,.65));ortho=(hi-lo).z*1.32
 elif interior:direction=Vector((1.05,-1.5,1.38));ortho=size*1.23
 else:direction=Vector((1.2,-1.7,.92));ortho=size*1.32
 camdata=bpy.data.cameras.new('Asset review');cam=bpy.data.objects.new('Asset review',camdata);s.collection.objects.link(cam);cam.location=target+direction*size;cam.rotation_euler=(target-cam.location).to_track_quat('-Z','Y').to_euler();camdata.type='ORTHO';camdata.ortho_scale=ortho;s.camera=cam
 if interior:
  rotation=cam.rotation_euler.to_matrix().transposed();projected=[rotation@(o.matrix_world@Vector(c)-target) for o in obs for c in o.bound_box];camdata.ortho_scale=max(max(v[i] for v in projected)-min(v[i] for v in projected) for i in [0,1])*1.13
 for name,vec,power,col in [('Key',(-1.1,-1.4,2.1),180,(1,.84,.68)),('Fill',(1.5,-.3,1.1),85,(.66,.81,1)),('Rim',(.4,1.7,2.2),240,(.76,.86,1))]:
  data=bpy.data.lights.new(name,'AREA');o=bpy.data.objects.new(name,data);s.collection.objects.link(o);o.location=target+Vector(vec)*size;data.energy=power*size*size;data.shape='DISK';data.size=size*.95;data.color=col;o.rotation_euler=(target-o.location).to_track_quat('-Z','Y').to_euler()
 s.render.engine='CYCLES';s.cycles.samples=32;s.cycles.use_denoising=True;s.render.resolution_x=1200;s.render.resolution_y=1200;s.render.resolution_percentage=100
 s.render.image_settings.file_format='PNG';s.view_settings.view_transform='AgX';s.view_settings.exposure=-.45;s.render.filepath=str(folder/(asset_id+'.png'))
 bpy.ops.render.render(write_still=True)

def finish(asset_id,group,info,source):
 print('FINISH_BEGIN',asset_id,flush=True);folder=OUT/group/asset_id;folder.mkdir(parents=True,exist_ok=True);s=bpy.context.scene;s.unit_settings.system='METRIC';s.unit_settings.scale_length=1.;s['asset_id']=asset_id;s['source_repository']='adaybekovt-boop/Make-your-Ai';s['pipeline']='UE5 preparation / editor not tested'
 obs=meshes();enclosure=[o for o in obs if o.get('enclosure')];obs=[o for o in obs if not o.get('enclosure')]
 # Keep articulated/static modules separate, batch only unrelated static decorations by material.
 if group in ['vehicles','interiors']:
  eligible=[o for o in obs if not o.get('part')]
  if group=='interiors':eligible=obs
  for m in list(bpy.data.materials):
   subset=[o for o in list(bpy.context.scene.objects) if o.type=='MESH' and not o.get('enclosure') and not o.get('part') and len(o.data.materials)==1 and o.data.materials[0]==m]
   if subset:
    name='SM_'+asset_id.replace('-','_')+'_'+re.sub('[^A-Za-z0-9]+','_',m.name).strip('_');ob=join(subset,name)
  obs=[o for o in meshes() if not o.get('enclosure')]
 for o in obs+enclosure:
  # Freeze world transforms for stable pivots except wheel assemblies and rigged parts.
  if not o.parent and not o.get('part'):
   active(o);bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
  o['asset_id']=asset_id
  bm=bmesh.new();bm.from_mesh(o.data);bmesh.ops.triangulate(bm,faces=list(bm.faces));bmesh.ops.delete(bm,geom=[f for f in bm.faces if f.calc_area()<1e-13],context='FACES');bm.to_mesh(o.data);bm.free();o.data.update();uv(o)
 mats=list(dict.fromkeys(m for o in obs+enclosure for m in o.data.materials if m));materialdata=[]
 for m in mats:
  kind=surface(m);p=m.node_tree.nodes.get('Principled BSDF')
  materialdata.append({'name':m.name,'base_color_linear':list(p.inputs['Base Color'].default_value),'metallic':p.inputs['Metallic'].default_value,'roughness_scalar':p.inputs['Roughness'].default_value,'coat_weight':p.inputs['Coat Weight'].default_value,'emission_color_linear':list(p.inputs['Emission Color'].default_value),'emission_strength':p.inputs['Emission Strength'].default_value,'texture_family':kind,'normal_convention':'OpenGL +Y; flip green in Unreal','uv0':'box projected, tiling; not a lightmap UV'})
 (folder/'materials.json').write_text(json.dumps(materialdata,ensure_ascii=False,indent=2))
 lo,hi=bounds(obs);collisions=[];cm=mat('Collision debug',(.7,.1,.1))
 if group in ['vehicles','racks']:
  # One conservative static-prop hull; vehicles need Chaos-specific collision later.
  # Name follows the main body object, imported with meshes combined only when appropriate.
  host=next((o for o in obs if 'Chassis' in o.name),max([o for o in obs if not o.get('part')],key=lambda o:len(o.data.polygons)))
  collisions.append(collision_box('UCX_'+host.name+'_00',lo,hi,cm))
 elif group=='interiors':
  w,d,h=info['interior_dimensions'];host=obs[0]
  for j,(a,b) in enumerate([((-w/2,-d/2,-.38),(w/2,d/2,0)),((-w/2,d/2-.1,0),(w/2,d/2+.1,h)),((-w/2-.1,-d/2,0),(-w/2+.1,d/2,1.24)),((w/2-.1,-d/2,0),(w/2+.1,d/2,.54))]):collisions.append(collision_box('UCX_'+host.name+f'_{j:02d}',Vector(a),Vector(b),cm))
 rigs=[o for o in s.objects if o.type=='ARMATURE']
 for o in enclosure:o.hide_render=True;o.hide_set(True)
 export_fbx(folder/(asset_id+'.fbx'),obs+rigs+collisions,animated=bool(rigs))
 if enclosure:
  ec=[]
  for o in enclosure:
   if o.get('skip_collision'):continue
   a,b=bounds([o]);ec.append(collision_box('UCX_'+o.name+'_00',a,b,cm))
  export_fbx(folder/(asset_id+'-enclosure.fbx'),enclosure+ec)
  for o in enclosure+ec:o.hide_render=True;o.hide_set(True)
 select(obs+rigs)
 bpy.ops.export_scene.gltf(filepath=str(folder/(asset_id+'.glb')),export_format='GLB',use_selection=True,export_animations=bool(rigs),export_cameras=False,export_lights=False,export_extras=True,export_materials='EXPORT',export_texcoords=True,export_normals=True,export_tangents=True)
 tri=sum(sum(len(p.vertices)-2 for p in o.data.polygons) for o in obs);etr=sum(sum(len(p.vertices)-2 for p in o.data.polygons) for o in enclosure)
 info.update(id=asset_id,group=group,source=source,objects=len(obs),triangles=tri,enclosure_triangles=etr,dimensions_m=list(hi-lo),bones=sum(len(r.data.bones) for r in rigs),fbx_bytes=(folder/(asset_id+'.fbx')).stat().st_size,glb_bytes=(folder/(asset_id+'.glb')).stat().st_size,unreal_editor_verified=False)
 (folder/'asset.json').write_text(json.dumps(info,ensure_ascii=False,indent=2))
 for o in collisions:o.hide_set(True)
 studio(asset_id,obs,folder,group=='interiors',group=='characters')
 for im in bpy.data.images:
  if im.source=='FILE':
   try:im.pack()
   except Exception:pass
 # Texture paths are packed; the material maps are also supplied explicitly for Unreal.
 select(obs+rigs);s['export_triangles']=tri;bpy.ops.wm.save_as_mainfile(filepath=str(folder/(asset_id+'.blend')),compress=True)
 print('ASSET_DONE',asset_id,tri,len(obs),flush=True)
