"""Regenerate upgrades from immutable repository builders, never execute their write/export tails.
Usage: blender -b -t 8 --python build_all.py -- vehicles|characters|racks|interiors [optional id]
"""
import bpy,sys,json,math,random
from pathlib import Path
HERE=Path(__file__).resolve().parent;sys.path.insert(0,str(HERE))
import blender_common,enhance,pipeline
from blender_common import *
ROOT=HERE/'source'
args=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else ['vehicles'];category=args[0];only=args[1] if len(args)>1 else None
inventory=json.loads((HERE.parent/'source-inventory.json').read_text())
def sourcepath(id):return next(a['source'] for a in inventory if a['id']==id)
def finish(id,info):pipeline.finish(id,category,info,sourcepath(id))
base=dict(bpy=bpy,sys=sys,json=json,math=math,random=random,Path=Path,Vector=__import__('mathutils').Vector,bmesh=__import__('bmesh'),S=1.0,**{n:getattr(blender_common,n) for n in ['mat','cube','cylinder','line','text','join','triangles','batch_materials']})
if category=='vehicles':
 src=(ROOT/'build_reference_vehicles.py').read_text();defs=src[src.index('def rounded'):src.index('for idx in [')];body=src[src.index('for idx in ['):src.index(' batch_materials();count=')];body=body.replace('for idx in [1,2,3,5,6]:','for idx in requested:')
 requested=[int(only.split('-')[1])] if only and only!='car-4' else [1,2,3,5,6] if not only else []
 g=base.copy();exec(defs,g);g.update(shell=enhance.shell,requested=requested,enhance=enhance,finish=finish)
 body+=' info=enhance.vehicle(globals());finish(f"car-{idx}",info)\n';exec(body,g)
 if only in [None,'car-4']:
  bpy.ops.wm.read_factory_settings(use_empty=True);src=(ROOT/'build_hero_coupe.py').read_text();src=src[src.index("paint=mat("):src.index('# Smooth surfaces')];g=base.copy();exec(src,g);g['idx']=4;finish('car-4',enhance.vehicle(g,hero=True))
elif category=='characters':
 src=(ROOT/'build_reference_people.py').read_text();src=src[src.index('for variant,label'):src.index(' for o in bpy.context.scene.objects:\n  if o.name.startswith')]
 start=src.index(' def batch(');end=src.index(' waist=',start)
 src=src[:start]+''' def batch(obs,name,pivot=(0,0,0)):
  obj=join(obs,name);bpy.context.scene.cursor.location=pivot;bpy.ops.object.origin_set(type='ORIGIN_CURSOR');obj['joint']=name;return obj
'''+src[end:]
 src=src.replace('segments=segments,ring_count=rings','segments=max(32,segments),ring_count=max(20,rings)')
 src=src.replace('  vs=[];faces=[]','  segments=max(32,segments)\n  rings=enhance.interp(sorted(rings),4)\n  vs=[];faces=[]')
 src=src.replace("bpy.context.collection.objects.link(o);return o","bpy.context.collection.objects.link(o)\n  for p in me.polygons:p.use_smooth=len(p.vertices)==4\n  return o")
 src=src.replace('(.94,waist+.01,.10,0,0)', '(.88,waist+.01,.10,0,0)').replace('(0,-.109,.917)', '(0,-.111,.853)')
 # All new parts retain their PBR material slots. The tiny-scale vertex palette is no longer used.
 src=src.replace(" bpy.ops.wm.read_factory_settings(use_empty=True);female=variant>=3"," if only and only!=f'person-{variant+1}':continue\n bpy.ops.wm.read_factory_settings(use_empty=True);female=variant>=3")
 src+=' info=enhance.person(globals());finish(f"person-{variant+1}",info)\n';g=base.copy();g.update(enhance=enhance,finish=finish,only=only);exec(src,g)
elif category=='racks':
 for key in ['rack-basic','rack-cooled','rack-enterprise']:
  if only and key!=only:continue
  finish(key,enhance.rack(key))
elif category=='interiors':
 src=(ROOT/'build_interiors.py').read_text();src=src[src.index('for key,n'):src.index(' batch_materials();count=')]
 src=src.replace(" bpy.ops.wm.read_factory_settings(use_empty=True);random.seed(17)"," if only and only!=key:continue\n bpy.ops.wm.read_factory_settings(use_empty=True);random.seed(17)")
 src=src.replace('subdivisions=1,radius=.24','subdivisions=3,radius=.24');src+=' info=enhance.interior(globals());finish(key,info)\n';g=base.copy();g.update(enhance=enhance,finish=finish,only=only);exec(src,g)
else:raise ValueError(category)
