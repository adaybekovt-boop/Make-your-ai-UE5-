"""Safe high-detail replacements for the repository's modelling helpers."""
import bpy,math
from mathutils import Vector

def active(o):
 bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o

def mat(name,c,metal=0,rough=.65,emit=0):
 m=bpy.data.materials.new(name);m.diffuse_color=(*c,1);m.use_nodes=True;p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(*c,1);p.inputs['Metallic'].default_value=metal;p.inputs['Roughness'].default_value=rough;p.inputs['Emission Color'].default_value=(*c,1);p.inputs['Emission Strength'].default_value=emit;return m

def bevel(o,width,segments=3):
 if width<=0:return o
 active(o);m=o.modifiers.new('Manufactured edge radius','BEVEL');m.width=width;m.segments=segments;bpy.ops.object.modifier_apply(modifier=m.name)
 for p in o.data.polygons:p.use_smooth=True
 m=o.modifiers.new('Weighted corner normals','WEIGHTED_NORMAL');m.keep_sharp=True;bpy.ops.object.modifier_apply(modifier=m.name);return o

def cube(name,pos,size,m,bevel=0):
 bpy.ops.mesh.primitive_cube_add(size=1,location=pos);o=bpy.context.object;o.name=name;o.scale=size;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);o.data.materials.append(m)
 if bevel:globals()['bevel'](o,bevel,4)
 return o

def cylinder(name,pos,r,depth,m,vertices=48,rotation=None):
 bpy.ops.mesh.primitive_cylinder_add(vertices=max(32,vertices),radius=r,depth=depth,location=pos);o=bpy.context.object;o.name=name;o.data.materials.append(m)
 if rotation:o.rotation_euler=rotation
 for p in o.data.polygons:p.use_smooth=len(p.vertices)==4
 return o

def line(name,points,r,m):
 curve=bpy.data.curves.new(name,'CURVE');curve.dimensions='3D';curve.resolution_u=12;curve.bevel_depth=r;curve.bevel_resolution=3
 sp=curve.splines.new('POLY');sp.points.add(len(points)-1)
 for p,v in zip(sp.points,points):p.co=(*v,1)
 o=bpy.data.objects.new(name,curve);bpy.context.collection.objects.link(o);o.data.materials.append(m);active(o);bpy.ops.object.convert(target='MESH');o=bpy.context.object;o.name=name;return o

def text(name,body,pos,size,m,rotation=(math.pi/2,0,0)):
 cu=bpy.data.curves.new(name,'FONT');cu.body=body;cu.size=size;cu.align_x='CENTER';cu.extrude=.0003;cu.resolution_u=5;o=bpy.data.objects.new(name,cu);bpy.context.collection.objects.link(o);o.location=pos;o.rotation_euler=rotation;cu.materials.append(m);active(o);bpy.ops.object.convert(target='MESH');return bpy.context.object

def join(obs,name):
 obs=list(obs)
 if not obs:return
 active(obs[0])
 for o in obs:o.select_set(True)
 bpy.ops.object.join();o=bpy.context.object;o.name=name;bpy.context.scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR');return o

def batch_materials():
 for m in list(bpy.data.materials):join([o for o in bpy.context.scene.objects if o.type=='MESH' and len(o.data.materials)==1 and o.data.materials[0]==m],m.name)
def triangles():return sum(sum(len(p.vertices)-2 for p in o.data.polygons) for o in bpy.context.scene.objects if o.type=='MESH')
def preview(*a,**k):pass

def sphere(name,pos,scale,m):
 bpy.ops.mesh.primitive_uv_sphere_add(segments=32,ring_count=20,radius=1,location=pos);o=bpy.context.object;o.name=name;o.scale=scale;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);o.data.materials.append(m)
 for p in o.data.polygons:p.use_smooth=True
 return o

def torus(name,pos,major,minor,m,rotation=None):
 bpy.ops.mesh.primitive_torus_add(major_segments=64,minor_segments=12,location=pos,major_radius=major,minor_radius=minor);o=bpy.context.object;o.name=name;o.data.materials.append(m)
 if rotation:o.rotation_euler=rotation
 for p in o.data.polygons:p.use_smooth=True
 return o
