"""Reference-directed dense estuary metropolis. All generated files stay beside this script."""
import bpy, math, random, json, os
from pathlib import Path
from mathutils import Vector
random.seed(631)
OUT=Path(__file__).resolve().parent
SRC=Path(os.environ.get('CITY_SOURCE',str(OUT.parent/'city-repo/artifacts/city/Metropolis_Reference_v5.blend')))
bpy.ops.wm.open_mainfile(filepath=str(SRC))
s=bpy.context.scene;s.name='DAY'
keep=[o for o in s.objects if o.get('game_id') or o.type in {'LIGHT','CAMERA'}]
for o in list(s.objects):
 if o not in keep:bpy.data.objects.remove(o,do_unlink=True)
sun=next(o for o in s.objects if o.type=='LIGHT');cam=s.camera
mats=[]
def mat(name,c,metal=0,rough=.65,emit=0):
 m=bpy.data.materials.new(name);m.diffuse_color=(*c,1);m.use_nodes=True;p=m.node_tree.nodes['Principled BSDF'];p.inputs['Base Color'].default_value=(*c,1);p.inputs['Roughness'].default_value=rough;p.inputs['Metallic'].default_value=metal;p.inputs['Emission Color'].default_value=(*c,1);p.inputs['Emission Strength'].default_value=emit;mats.append(m);return len(mats)-1
EARTH=mat('Land / warm earth',(.24,.29,.17));STONE=mat('Seawalls / pale sandstone',(.47,.43,.32));PAVE=mat('Pavements / limestone',(.57,.55,.45));ROAD=mat('Asphalt',(.075,.089,.092));PAINT=mat('Lane paint',(.73,.72,.59));GRASS=mat('Parks / meadow',(.14,.25,.065));LEAF=mat('Trees / deep green',(.055,.16,.043));LEAF2=mat('Trees / olive green',(.13,.24,.061));WOOD=mat('Trunks',(.16,.105,.06));GLASS=mat('Facade / blue glass',(.10,.24,.31),.48,.21);GLASS2=mat('Facade / silver blue',(.27,.38,.40),.40,.27);TRIM=mat('Facade / silver mullions',(.64,.64,.55),.38,.35);CREAM=mat('Facade / ivory stone',(.68,.59,.44));BRICK=mat('Facade / brick',(.37,.20,.115));WHITE=mat('White architecture',(.78,.77,.67),.12,.4);ROOF=mat('Roof / terracotta',(.39,.16,.085));ROOF2=mat('Roof / slate',(.19,.25,.26));LIGHT=mat('Windows / evening',(.95,.68,.31),rough=.4,emit=.03);CRANE=mat('Port / oxide orange',(.64,.23,.075),.20,.5);BLUE=mat('Port / container blue',(.055,.22,.31));RED=mat('Port / container red',(.46,.115,.065));YELLOW=mat('Service markings',(.75,.56,.14));WATER=mat('River Solara / blue green',(.025,.135,.19),.22,.22)
# Procedural surface detail; no image textures.
for idx in [EARTH,STONE,PAVE,GRASS,CREAM,BRICK,ROOF]:
 m=mats[idx];p=m.node_tree.nodes['Principled BSDF'];n=m.node_tree.nodes.new('ShaderNodeTexNoise');n.inputs['Scale'].default_value=7;bu=m.node_tree.nodes.new('ShaderNodeBump');bu.inputs['Strength'].default_value=.12;bu.inputs['Distance'].default_value=.035;m.node_tree.links.new(n.outputs['Fac'],bu.inputs['Height']);m.node_tree.links.new(bu.outputs['Normal'],p.inputs['Normal'])
m=mats[WATER];p=m.node_tree.nodes['Principled BSDF'];n=m.node_tree.nodes.new('ShaderNodeTexNoise');n.inputs['Scale'].default_value=2.8;n.inputs['Detail'].default_value=3;bu=m.node_tree.nodes.new('ShaderNodeBump');bu.inputs['Strength'].default_value=.16;bu.inputs['Distance'].default_value=.065;m.node_tree.links.new(n.outputs['Fac'],bu.inputs['Height']);m.node_tree.links.new(bu.outputs['Normal'],p.inputs['Normal'])
class Mesh:
 def __init__(self):self.v=[];self.f=[];self.mi=[]
 def face(self,vs,mi):
  k=len(self.v);self.v.extend(vs);self.f.append(tuple(range(k,k+len(vs))));self.mi.append(mi)
 def box(self,x,y,z,w,d,h,mi):
  a=x-w/2;b=x+w/2;c=y-d/2;e=y+d/2;l=z-h/2;u=z+h/2
  v=[(a,c,l),(b,c,l),(b,e,l),(a,e,l),(a,c,u),(b,c,u),(b,e,u),(a,e,u)]
  for ids in [(0,3,2,1),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)]:self.face([v[i] for i in ids],mi)
 def poly(self,pts,z,h,mi):
  self.face([(x,y,z+h) for x,y in pts],mi);self.face([(x,y,z) for x,y in pts[::-1]],mi)
  for a,b in zip(pts,pts[1:]+pts[:1]):self.face([(*a,z),(*b,z),(*b,z+h),(*a,z+h)],mi)
 def cone(self,x,y,z,r1,r2,h,mi,n=8):
  a=[(x+r1*math.cos(i*math.tau/n),y+r1*math.sin(i*math.tau/n),z) for i in range(n)]
  b=[(x+r2*math.cos(i*math.tau/n),y+r2*math.sin(i*math.tau/n),z+h) for i in range(n)]
  self.face(a[::-1],mi);self.face(b,mi)
  for i in range(n):j=(i+1)%n;self.face([a[i],a[j],b[j],b[i]],mi)
 def beam(self,a,b,w,mi,depth=None):
  a=Vector(a);b=Vector(b);v=b-a;u=v.cross(Vector((0,0,1)))
  if u.length<.001:u=Vector((1,0,0))
  u.normalize();u*=w/2;t=v.normalized().cross(u)
  if depth is not None:t=t.normalized()*depth/2
  p=[a-u-t,a+u-t,a+u+t,a-u+t,b-u-t,b+u-t,b+u+t,b-u+t]
  for ids in [(0,3,2,1),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)]:self.face([p[i] for i in ids],mi)
 def done(self,name):
  me=bpy.data.meshes.new(name);me.from_pydata(self.v,[],self.f);me.update();o=bpy.data.objects.new(name,me);s.collection.objects.link(o)
  for m in mats:me.materials.append(m)
  for p,i in zip(me.polygons,self.mi):p.material_index=i
  return o
# Smooth coastlines via corner cutting, keeping channels irregular.
def smooth(pts,n=2):
 for _ in range(n):
  res=[]
  for a,b in zip(pts,pts[1:]+pts[:1]):res.extend([(a[0]*.75+b[0]*.25,a[1]*.75+b[1]*.25),(a[0]*.25+b[0]*.75,a[1]*.25+b[1]*.75)])
  pts=res
 return pts
def inside(x,y,p):
 yes=False
 for (a,b),(c,d) in zip(p,p[1:]+p[:1]):
  if (b>y)!=(d>y) and x<(c-a)*(y-b)/(d-b)+a:yes=not yes
 return yes
def distseg(x,y,a,b):
 v=Vector((b[0]-a[0],b[1]-a[1]));w=Vector((x-a[0],y-a[1]));t=max(0,min(1,w.dot(v)/max(.0001,v.length_squared)));return (w-v*t).length

def path(m,pts,w,mi,z=.46,closed=False):
 pp=pts+[pts[0]] if closed else pts
 for a,b in zip(pp,pp[1:]):m.beam((*a,z),(*b,z),w,mi,.035)
# Authoring footprint matches the main reference: central river, crown island, port left, airport lower right.
regions=[
 ('Crown',[(-44,10),(-48,40),(-42,72),(-25,88),(-2,87),(17,75),(21,47),(16,17),(3,3),(-22,1)],'cbd'),
 ('Westgate',[(-104,12),(-101,59),(-84,89),(-58,87),(-52,65),(-56,33),(-62,9),(-82,3)],'west'),
 ('Eastridge',[(26,9),(29,47),(28,74),(45,87),(59,78),(61,47),(57,20),(44,6)],'east'),
 ('Riverside',[(68,-5),(66,34),(69,75),(90,100),(126,102),(143,74),(143,23),(121,1),(91,-12)],'res'),
 ('North reaches',[(-96,101),(-80,143),(-27,156),(4,137),(13,105),(-18,96),(-48,99)],'north'),
 ('North east',[(21,104),(36,141),(75,150),(115,133),(126,111),(91,108),(65,92),(46,95)],'north'),
 ('Southgate',[(-50,-76),(-37,-96),(-7,-101),(16,-84),(25,-59),(22,-38),(3,-27),(-23,-31),(-40,-49)],'south'),
 ('Valor',[(-55,-22),(-47,-10),(-34,-9),(-23,-17),(-22,-28),(-34,-34),(-49,-31)],'park'),
 ('Harbour',[(-125,-75),(-122,-15),(-112,0),(-87,-3),(-72,-16),(-72,-65),(-87,-81)],'port'),
 ('Skyport',[(39,-111),(36,-62),(41,-29),(62,-19),(115,-16),(126,-50),(124,-103),(91,-119)],'airport'),
 ('Far horizon',[(-110,162),(-108,241),(-50,258),(50,248),(155,233),(168,174),(130,155),(75,158),(20,153),(-35,163)],'north'),
 ('West hills',[(-190,-22),(-190,252),(-139,270),(-112,233),(-102,183),(-112,97),(-123,54),(-132,15),(-130,-20)],'forest')]
polys={};coasts={}
for name,pts,kind in regions:
 pp=smooth(pts,2);polys[name]=pp;m=Mesh();m.poly(pp,-1.2,1.55,EARTH if kind!='airport' else GRASS);m.done(name+' / terrain')
 edge=Mesh();path(edge,pp,.55,STONE,.37,True);coasts[name]=pp
 if kind!='forest':path(edge,pp,1.25,PAVE,.40,True)
 edge.done(name+' / waterfront')
m=Mesh();m.box(0,25,-1.35,1200,1200,.18,WATER);m.done('Solara / open water')
# Original game buildings are appended and relocated, maintaining all twelve IDs.
places={'garage':(-30,-75,2.0,'Southgate / founders'), 'workshop':(-24,-72,2.3,'Southgate / founders'), 'technopark':(-25,16,3.2,'Crown / technology'), 'server-hall':(5,21,3.0,'Crown / technology'), 'campus':(-25,32,3.2,'Crown / premium campus'), 'orbit':(-10,43,2.7,'Crown / anchor cluster'), 'horizon':(36,32,2.4,'Eastridge'), 'meridian':(-3,-48,2.1,'Southgate / waterfront'), 'dc-north':(-19,116,1.2,'North reaches / data campus'), 'dc-south':(93,71,1.1,'Riverside / data campus'), 'chip-depot':(-27,-64,1.4,'Southgate / founders'), 'silicon-market':(-25,24,1.5,'Crown / technology')}
reserved=[];placements=[]
for o in keep:
 if not o.get('game_id'):continue
 key=o['game_id'];x,y,f,desc=places[key];vs=[o.matrix_world@v.co for v in o.data.vertices];lo=Vector([min(v[i] for v in vs) for i in range(3)]);hi=Vector([max(v[i] for v in vs) for i in range(3)]);c=(lo+hi)/2
 for v,p in zip(o.data.vertices,vs):v.co=((p.x-c.x)*f+x,(p.y-c.y)*f+y,(p.z-lo.z)*f+.42)
 o.matrix_world.identity();w=(hi.x-lo.x)*f;d=(hi.y-lo.y)*f;reserved.append((x,y,w/2+1,d/2+1));o['district']=desc;placements.append(dict(id=key,node=o.name,position=[x,y,.42],scale=f,district=desc))
reserved.append((-9,17,11,11))
def blocked(x,y,w,d):return any(abs(x-a)<w/2+c and abs(y-b)<d/2+e for a,b,c,e in reserved)
# Shared architectural templates: varied silhouettes, facades, rooftop plant and parapets.
protos={'high':[],'mid':[],'low':[]}
def make_building(kind,i):
 m=Mesh();w=random.uniform(1.8,3.3) if kind=='high' else random.uniform(1.25,2.2);d=w*random.uniform(.72,1.2)
 h=random.uniform(17,40) if kind=='high' else random.uniform(4,11) if kind=='mid' else random.uniform(.85,2.5)
 wall=[CREAM,WHITE,BRICK,GLASS2][i%4];glass=GLASS if i%2 else GLASS2
 if kind=='high':
  m.box(0,0,.55,w+1,d+1,1.1,CREAM)
  tiers=3 if i%3==0 else 2 if i%3==1 else 1
  for t in range(tiers):
   tw=w*(1-t*.17);td=d*(1-t*.15);hh=h/tiers;zz=1+t*hh
   m.box(0,0,zz+hh/2,tw,td,hh,glass)
   floors=max(3,int(hh/.65))
   for k in range(floors+1):m.box(0,0,zz+k*hh/floors,tw+.04,td+.04,.045,TRIM if i%3 else CREAM)
   for u in [-.48,-.24,0,.24,.48]:
    for side in [-1,1]:
     m.box(u*tw,side*td/2,zz+hh/2,.035,.035,hh,TRIM)
     m.box(side*tw/2,u*td,zz+hh/2,.035,.035,hh,TRIM)
   for k in range(2,floors,4):
    for u in [-.28,.19]:m.box(u*tw,-td/2-.02,zz+k*hh/floors,.15,.015,.20,LIGHT)
  m.box(0,0,h+1.15,w*.47,d*.47,.35,CREAM)
  if i%4==0:m.cone(0,0,h+1.3,.035,.012,2.2,TRIM,6)
 else:
  m.box(0,0,h/2,w,d,h,wall);nf=max(1,int(h/.65))
  for k in range(nf):
   z=.40+k*h/nf
   for side in [-1,1]:
    for u in [-.30,0,.30]:
     m.box(u*w,side*(d/2+.012),z,w*.19,.025,.29,glass)
     if kind=='mid':m.box(side*(w/2+.012),u*d,z,.025,d*.19,.29,glass)
  if kind=='low' and i%3:
   roof=ROOF if i%2 else ROOF2
   a=(-w/2-.1,-d/2-.1,h);b=(w/2+.1,-d/2-.1,h);c=(w/2+.1,d/2+.1,h);e=(-w/2-.1,d/2+.1,h);u=(0,-d/2-.1,h+.5);v=(0,d/2+.1,h+.5)
   for vs in [[a,b,u],[e,v,c],[a,u,v,e],[u,b,c,v]]:m.face(vs,roof)
  else:
   m.box(0,0,h+.08,w+.12,d+.12,.16,PAVE);m.box(.2,.1,h+.28,w*.25,d*.30,.25,ROOF2)
   for side in [-1,1]:m.box(0,side*d/2,h+.20,w,.055,.24,CREAM)
 o=m.done('template');data=o.data;bpy.data.objects.remove(o,do_unlink=True);return data,w,d,h
for kind,n in [('high',24),('mid',24),('low',24)]:
 for i in range(n):protos[kind].append(make_building(kind,i))
# Shared broadleaf and conifer forms, richer than the previous cone rows.
tree_protos=[]
for ti in range(5):
 m=Mesh();m.cone(0,0,0,.07,.035,.7,WOOD,6)
 if ti<4:
  for dx,dy,z,r in [(0,0,.63,.46),(.20,.08,.88,.32),(-.16,-.1,.96,.33)]:
   m.cone(dx,dy,z-.26,r*.65,r,.28,LEAF if ti%2 else LEAF2,7);m.cone(dx,dy,z+.02,r,.04,.44,LEAF if ti%2 else LEAF2,7)
 else:
  m.cone(0,0,.35,.42,0,1.15,LEAF,8);m.cone(0,0,.8,.30,0,.85,LEAF2,8)
 o=m.done('template tree');tree_protos.append(o.data);bpy.data.objects.remove(o,do_unlink=True)
counts={'buildings':0,'trees':0,'bridges':0,'boats':0};occupied={}
def tree(x,y,size=1,region='Park',z=.43):
 o=bpy.data.objects.new(region+' / tree',random.choice(tree_protos));s.collection.objects.link(o);o.location=(x,y,z);f=random.uniform(.8,1.25)*size;o.scale=(f,f,f);o.rotation_euler.z=random.random()*6.28;counts['trees']+=1

def building(x,y,kind,region,scale=1):
 data,w,d,h=random.choice(protos[kind]);o=bpy.data.objects.new(region+' / '+kind,data);s.collection.objects.link(o);o.location=(x,y,.44);o.scale=(scale,scale,scale*random.uniform(.88,1.1));o.rotation_euler.z=random.choice([0,math.pi/2]);counts['buildings']+=1;occupied.setdefault(region,[]).append((x,y,w*scale/2+.2,d*scale/2+.2));return o
# Gridded urban fabric clipped to organically shaped land: sidewalks, intersections and blocks.
for name,_,kind in regions:
 if kind in ['park','port','airport','forest']:continue
 poly=polys[name];mnx=min(x for x,y in poly);mxx=max(x for x,y in poly);mny=min(y for x,y in poly);mxy=max(y for x,y in poly)
 step=8 if kind in ['cbd','west','east'] else 7
 ox=math.floor(mnx/step)*step;oy=math.floor(mny/step)*step
 roads=Mesh();parks=Mesh();furniture=Mesh()
 # Dense street grid, each small strip clipped by point sampling.
 for xi in range(int(ox),int(mxx)+1,step):
  for y in range(math.floor(mny),math.ceil(mxy)):
   if inside(xi,y,poly) and inside(xi,y+1,poly):
    roads.box(xi,y+.5,.415,1.18,1.05,.035,PAVE);roads.box(xi,y+.5,.44,.78,1.05,.025,ROAD)
    if y%3==0:roads.box(xi,y+.5,.46,.035,.40,.006,PAINT)
 for yi in range(int(oy),int(mxy)+1,step):
  for x in range(math.floor(mnx),math.ceil(mxx)):
   if inside(x,yi,poly) and inside(x+1,yi,poly):
    roads.box(x+.5,yi,.415,1.05,1.18,.035,PAVE);roads.box(x+.5,yi,.44,1.05,.78,.025,ROAD)
    if x%3==0:roads.box(x+.5,yi,.46,.40,.035,.006,PAINT)
 for bx in range(int(ox),int(mxx),step):
  for by in range(int(oy),int(mxy),step):
   park=random.random()<(.13 if kind in ['south','res','north'] else .08)
   for dx in [2.1,4.1,6.1] if step==8 else [1.8,3.7,5.5]:
    for dy in [2.1,4.1,6.1] if step==8 else [1.8,3.7,5.5]:
     x=bx+dx;y=by+dy
     if not all(inside(x+a,y+b,poly) for a,b in [(-1,-1),(1,1),(-1,1),(1,-1)]):continue
     if blocked(x,y,1.8,1.8):continue
     if park:
      parks.box(x,y,.43,1.8,1.8,.04,GRASS)
      for j in range(3):tree(x+random.uniform(-.6,.6),y+random.uniform(-.6,.6),1,name)
      continue
     high=False
     if kind=='cbd':high=(dx==4.1 and dy==4.1) or (random.random()<.16 and y>27)
     elif kind in ['west','east']:high=(dx==4.1 and dy==4.1 and random.random()<.68)
     if high:
      # One tower takes the entire block, with smaller fabric omitted nearby.
      if dx!=4.1 or dy!=4.1:continue
      building(x,y,'high',name,1.05 if kind=='cbd' else .82)
     elif kind in ['cbd','west','east']:
      if dx==4.1 and dy==4.1:building(x,y,'mid',name,1.0)
      elif dx!=4.1 and dy!=4.1:building(x,y,'mid',name,.77)
     else:building(x,y,'mid' if random.random()<(.65 if kind in ['cbd','west','east'] else .22) else 'low',name,.72 if step==7 else .82)
   for tx,ty in [(bx+1,by+1),(bx+step-1,by+1)]:
    if inside(tx,ty,poly) and not blocked(tx,ty,.4,.4):tree(tx,ty,.85,name)
   if inside(bx+.7,by+.7,poly):
    furniture.box(bx+.7,by+.7,.91,.035,.035,.92,TRIM);furniture.box(bx+.7,by+.7,1.39,.14,.10,.04,LIGHT)
 roads.done(name+' / complete street network');parks.done(name+' / green courts');furniture.done(name+' / lamps')
 # Waterfront boulevard follows the coastline, with irregular trees.
 m=Mesh();cx=sum(x for x,y in poly)/len(poly);cy=sum(y for x,y in poly)/len(poly);inner=[(cx+(x-cx)*.95,cy+(y-cy)*.95) for x,y in poly];path(m,inner,.7,ROAD,.46,True);m.done(name+' / coastal boulevard')
 for a,b in zip(poly,poly[1:]+poly[:1]):
  le=math.dist(a,b)
  for j in range(max(1,int(le/1.3))):
   t=j/max(1,int(le/1.3));x=a[0]*(1-t)+b[0]*t;y=a[1]*(1-t)+b[1]*t;x=cx+(x-cx)*.975;y=cy+(y-cy)*.975
   if not blocked(x,y,.5,.5):tree(x,y,.9,name)
print('URBAN_FABRIC',counts,flush=True)
# Crown landmark: flared structural base, tapered ivory fins, dark glass shaft.
m=Mesh();cx=-9;cy=17
for r,z,h,mi in [(11,.37,.08,PAVE),(9.8,.45,.05,GRASS),(8,.50,.08,PAVE),(6.5,.58,.5,CREAM)]:m.cone(cx,cy,z,r,r,h,mi,96)
for radius in [8.7,10.5]:
 pts=[(cx+radius*math.cos(i*math.tau/96),cy+radius*math.sin(i*math.tau/96)) for i in range(96)];path(m,pts,.14,WHITE,.57,True)
for i in range(20):
 a=i*math.tau/20
 m.beam((cx+6.5*math.cos(a),cy+6.5*math.sin(a),.9),(cx+2.8*math.cos(a),cy+2.8*math.sin(a),18),.20,WHITE)
 path(m,[(cx+7*math.cos(a),cy+7*math.sin(a)),(cx+10.7*math.cos(a),cy+10.7*math.sin(a))],.30,PAVE,.58)
for k in range(88):
 z=1+k*.66;r=3.3*(1-k/100)**.68
 m.cone(cx,cy,z,r,r*.993,.66,GLASS,24)
 m.cone(cx,cy,z,r+.04,r+.035,.06,TRIM,24)
 for i in range(12):
  a=i*math.tau/12;m.beam((cx+r*math.cos(a),cy+r*math.sin(a),z),(cx+r*.993*math.cos(a),cy+r*.993*math.sin(a),z+.66),.075,WHITE)
m.cone(cx,cy,59,1.05,.15,7,WHITE,24);m.cone(cx,cy,66,.15,.014,7,TRIM,12)
m.done('Crown / signature tower and radial gardens')
for i in range(40):
 a=i*math.tau/40;tree(cx+9.4*math.cos(a),cy+9.4*math.sin(a),.75,'Crown plaza')
# Park island with a civic hall, promenades and compact marinas.
m=Mesh();poly=polys['Valor'];m.poly(poly,.355,.02,GRASS)
for x,y in [(-40,-18),(-31,-25)]:
 m.box(x,y,1.6,5,3.5,2.3,CREAM);m.box(x,y,2.85,5.7,4.2,.2,WHITE)
 for dx in [-2,-1,0,1,2]:
  for sy in [-1,1]:m.cone(x+dx,y+sy*1.8,.45,.12,.12,2.2,WHITE,8)
 m.poly([(x-2.8,y-2),(x+2.8,y-2),(x+2.8,y+2),(x-2.8,y+2)],2.95,.1,PAVE)
path(m,[(-51,-23),(-42,-23),(-36,-20),(-27,-22)],.8,PAVE,.44)
m.done('Valor / civic gardens')
for i in range(320):
 x=random.uniform(-54,-24);y=random.uniform(-32,-12)
 if inside(x,y,poly) and abs(y+23)>1 and math.hypot(x+40,y+18)>3.6 and math.hypot(x+31,y+25)>3.6:tree(x,y,1.1,'Valor')
# Long span bridges with ramps, guardrails, pylons and fan cables.
def bridge(name,a,b,width=1.6,cable=False):
 m=Mesh();a=Vector((*a,0));b=Vector((*b,0));v=b-a;n=Vector((-v.y,v.x,0)).normalized()
 def at(t):
  p=a+v*t;p.z=.5+min(t/.18,(1-t)/.18,1)*1.4;return p
 for k in range(40):
  p=at(k/40);q=at((k+1)/40);m.beam(p,q,width,ROAD,.18)
  for sign in [-1,1]:m.beam(p+n*width*.53*sign+Vector((0,0,.20)),q+n*width*.53*sign+Vector((0,0,.20)),.045,WHITE)
  if k%2==0:m.beam(p,q,.035,PAINT,.01)
 for t in [.23,.50,.77]:
  p=at(t);m.box(p.x,p.y,.4,.45,.45,3,STONE)
 for t in [.30,.70] if cable else []:
  p=at(t)
  for sign in [-1,1]:
   foot=p+n*width*.60*sign;top=p+n*width*.24*sign+Vector((0,0,10));m.beam(foot,top,.22,WHITE)
   for dt in [-.22,-.16,-.10,-.04,.04,.10,.16,.22]:m.beam(top,at(t+dt)+n*width*.48*sign,.028,TRIM)
 for k in range(2,39,5):
  p=at(k/40)+n*width*.57;m.beam(p,p+Vector((0,0,1)),.035,TRIM);m.box(p.x,p.y,p.z+1.03,.16,.12,.05,LIGHT)
 m.done('Bridge / '+name);counts['bridges']+=1
bridge('Aurora / main estuary',(45,16),(56,-34),2.4,True)
bridge('Crown west',(-60,23),(-39,21),1.9,True)
bridge('Crown east',(12,33),(35,34),1.8,True)
bridge('Westgate port',(-90,13),(-103,-20),2,True)
bridge('Port Valor',(-82,-34),(-43,-24),1.1)
bridge('Valor Southgate',(-34,-28),(-27,-48),1.0)
bridge('Southgate Skyport',(12,-65),(52,-66),2.0,True)
bridge('Eastridge Riverside',(50,24),(85,23),1.6)
bridge('Crown North',(-19,72),(-29,111),1.5)
bridge('Eastridge North',(44,72),(47,110),1.6)
bridge('North cross river',(-17,117),(45,122),1.5,True)
bridge('Northern banks',(-83,125),(-121,130),1.4)
# Harbour: sawtooth warehouses, container stacks, fingers and large gantry cranes.
m=Mesh();pp=polys['Harbour']
for y in [-67,-50,-33,-16]:
 for x in [-113,-96]:
  m.box(x,y,1.85,11,7,2.7,CREAM)
  for j in range(5):
   yy=y-3.5+j*1.4
   m.face([(x-5.6,yy,3.25),(x+5.6,yy,3.25),(x+5.6,yy+1,3.95),(x-5.6,yy+1,3.95)],ROOF2)
   m.face([(x-5.6,yy+1,3.95),(x+5.6,yy+1,3.95),(x+5.6,yy+1.4,3.25),(x-5.6,yy+1.4,3.25)],GLASS2)
  for dx in [-4,-2,0,2,4]:m.box(x+dx,y-3.53,1.5,1.2,.06,1.7,ROOF2)
for x in [-121,-104,-86]:path(m,[(x,-72),(x,-7)],1.3,ROAD,.43)
for y in [-75,-59,-42,-25,-8]:path(m,[(-122,y),(-75,y)],1.2,ROAD,.43)
# Piers projecting into a shared harbour basin.
for y in [-64,-48,-32,-16]:
 m.box(-71,y,-.15,24,5,1.1,STONE);m.box(-71,y,.43,24,4.8,.06,PAVE)
 for x in [-80,-73,-65]:
  for dx in [-.9,.9]:
   for dy in [-1.7,1.7]:m.beam((x+dx,y+dy,.45),(x+dx*.7,y+dy,6.3),.20,CRANE)
  m.box(x,y,6.35,2.4,4.2,.25,CRANE);m.box(x+2,y,7.15,7.8,.25,.24,CRANE)
  m.beam((x-1.8,y,6.9),(x+1,y,9),.09,CRANE);m.beam((x+1,y,9),(x+5.5,y,7.2),.08,CRANE)
  m.beam((x+5.4,y,7.2),(x+5.4,y,2.6),.035,TRIM);m.box(x+5.4,y,2.5,1.1,.5,.12,CRANE)
  for j in range(4):m.beam((x-1.5+j*1.4,y,7.1),(x-.8+j*1.4,y,7.7),.06,CRANE)
for i in range(650):
 x=random.choice([-119,-117,-115,-92,-90,-88,-81,-79,-77]);y=random.uniform(-71,-8)
 if not inside(x,y,pp):continue
 z=.44+random.choice([.4,.4,1.2]);mi=[RED,BLUE,WHITE,CRANE,ROOF2][i%5];m.box(x,y,z,.9,2.0,.76,mi)
 for yy in [-.75,-.25,.25,.75]:m.box(x+.46,y+yy,z,.022,.05,.7,TRIM if i%13==0 else mi)
m.done('Harbour / logistics piers and 12 gantry cranes')
# Shared ships, small craft and marina jetties.
def ship(x,y,length=8,width=1.8,angle=0,cargo=True):
 m=Mesh();pts=[(-width/2,-length*.37),(-width*.43,length*.38),(0,length*.5),(width*.43,length*.38),(width/2,-length*.37),(width*.25,-length*.46),(-width*.25,-length*.46)];m.poly(pts,-.28,.66,ROOF2);m.poly([(a*.85,b*.95) for a,b in pts],.39,.06,PAVE)
 if cargo:
  for xx in [-width*.23,width*.23]:
   for yy in [-length*.19,0,length*.19]:m.box(xx,yy,.77,width*.4,length*.16,.68,random.choice([RED,BLUE,CREAM]))
  m.box(0,-length*.33,1.05,width*.78,length*.14,1,WHITE);m.box(0,-length*.33,1.62,width*.65,length*.12,.15,GLASS)
 else:m.box(0,-length*.05,.58,width*.60,length*.40,.38,WHITE)
 o=m.done('Ship');o.location=(x,y,-.10);o.rotation_euler.z=angle;counts['boats']+=1
for y in [-65,-49,-33,-17]:ship(-64,y-4,10,2.2,math.pi/2)
for x,y,a in [(-97,-92,.25),(-60,-85,-.4),(-66,0,.7),(18,-13,-.5),(70,-7,.2)]:ship(x,y,8,1.7,a)
for cx,cy in [(-15,-94),(-40,-37),(-67,18),(5,4)]:
 m=Mesh();path(m,[(cx-4,cy),(cx+4,cy)],.35,WOOD,.43)
 for j in range(-3,4):path(m,[(cx+j,cy),(cx+j,cy-2.6)],.16,WOOD,.43)
 m.done('Marina / timber pontoons')
 for j in range(-3,4):ship(cx+j+.28,cy-1.2,1.45,.40,0,False)
# Airport: two long runways, taxiways, service roads, terminal concourses and aircraft.
m=Mesh()
for x in [94,111]:
 m.box(x,-67,.42,4.0,82,.045,ROAD)
 for y in range(-101,-27,4):m.box(x,y,.46,.10,1.8,.012,PAINT)
 for y in [-102,-32]:
  for dx in [-1.5,-1,-.5,.5,1,1.5]:m.box(x+dx,y,.47,.19,3.4,.012,WHITE)
 for y in range(-106,-25,3):
  for dx in [-2.1,2.1]:m.box(x+dx,y,.50,.075,.075,.08,LIGHT)
for x in [86,103]:m.box(x,-67,.42,1.4,81,.04,ROAD)
for y in [-101,-83,-64,-46,-30]:
 m.box(99,y,.43,26,1.5,.03,ROAD);m.box(99,y,.46,26,.035,.01,YELLOW)
m.box(69,-67,.425,29,60,.04,PAVE);m.box(54,-66,1.75,4.7,48,2.6,GLASS2);m.box(54,-66,3.1,5.2,48.4,.18,WHITE)
for y in [-83,-67,-51]:
 m.box(64,y,1.3,19,2.2,1.7,GLASS);m.box(64,y,2.2,19.4,2.5,.17,WHITE)
 for x in [58,63,68,73]:
  for side in [-1,1]:
   m.box(x,y+side*2.1,1.10,.65,2.6,.55,WHITE)
   path(m,[(x-1.8,y+side*4.5),(x+1.8,y+side*4.5)],.035,YELLOW,.47)
for y in [-89,-80,-71,-62,-53,-44]:
 m.box(44,y,.43,9,6,.025,ROAD)
 for x in [41,43,45,47]:
  for dy in [-1.8,0,1.8]:
   m.box(x,y+dy,.47,1.3,.035,.01,WHITE);m.box(x,y+dy+.55,.67,.60,1.0,.38,random.choice([WHITE,ROOF2,RED]))
for x in [58,68,78]:m.box(x,-106,1.65,7,6,2.5,CREAM);m.box(x,-106,2.95,7.2,6.2,.10,ROOF2)
m.cone(49,-33,.44,.55,.4,6,WHITE,12);m.cone(49,-33,6.44,1.1,1.1,1,GLASS,12);m.cone(49,-33,7.44,1.2,1.2,.10,WHITE,12)
path(m,[(42,-108),(40,-85),(40,-53),(46,-32),(58,-23)],1.3,ROAD,.45)
m.done('Skyport / two runways taxiways terminal and parking')
# Aircraft meshes, shared across gates.
def plane_proto():
 m=Mesh();# fuselage along local Y
 n=12;rows=[]
 for yy,rr in [(-3.8,.05),(-3.1,.27),(-2,.31),(2.4,.31),(3.1,.21),(3.55,0)]:rows.append([(rr*math.cos(k*math.tau/n),yy,.85+rr*math.sin(k*math.tau/n)) for k in range(n)])
 for a,b in zip(rows,rows[1:]):
  for k in range(n):q=(k+1)%n;m.face([a[k],a[q],b[q],b[k]],WHITE)
 for side in [-1,1]:
  m.poly([(0,.8),(side*3.4,-1.3),(side*3.55,-2),(side*.3,-.6)],.82,.07,WHITE)
  m.poly([(0,-2.7),(side*1.3,-3.3),(side*1.3,-3.65),(0,-3.3)],.95,.06,WHITE)
  m.box(side*1.15,-.4,.65,.33,.9,.34,GLASS2)
  for y in [-2,-1.4,-.8,-.2,.4,1,1.6,2.2]:m.box(side*.303,y,.94,.018,.17,.10,GLASS)
 m.face([(0,-2.5,.95),(0,-3.5,.95),(0,-3.3,2.25),(0,-2.85,2.25)],BLUE)
 o=m.done('template aircraft');data=o.data;bpy.data.objects.remove(o,do_unlink=True);return data
plane=plane_proto()
for y in [-83,-67,-51]:
 for x in [59,68,77]:
  for side in [-1,1]:
   o=bpy.data.objects.new('Skyport / passenger jet',plane);s.collection.objects.link(o);o.location=(x,y+side*5.3,.15);o.rotation_euler.z=math.pi if side==1 else 0
# Curving motorway and cloverleaf interchange near airport.
m=Mesh();route=smooth([(35,-105),(31,-84),(29,-63),(31,-40),(41,-22),(64,-16),(81,-8)],3);path(m,route,2.0,ROAD,.75)
for offset in [-.20,.20]:path(m,[(x+offset,y) for x,y in route],.04,PAINT,.78)
for cx,cy in [(36,-34),(40,-29)]:
 pts=[(cx+3.7*math.cos(i*math.tau/48),cy+4.2*math.sin(i*math.tau/48)) for i in range(49)];path(m,pts,.65,ROAD,.64)
m.done('Skyport / coastal motorway interchange')
# Forested, continuous mountain side of the horizon.
def hill(x,y):
 ridge=math.exp(-((x+143)/19)**2);und=16+9*math.sin(y*.058)+5*math.sin(y*.17+1)+3*math.sin(x*.13+y*.1)
 return .35+ridge*max(5,und)*min(1,max(0,(y+20)/22))
m=Mesh();p=polys['West hills']
# Sample terrain with boundary interpolation so the forested mountain merges into its coast.
def coast_distance(x,y):return min(distseg(x,y,a,b) for a,b in zip(p,p[1:]+p[:1]))
def terrain_height(x,y):return .35+(hill(x,y)-.35)*min(1,coast_distance(x,y)/9)
# Dense rows fill the interior; edge fans close the boundary without exposed square cliffs.
from mathutils.geometry import tessellate_polygon
pts=[Vector((x,y,terrain_height(x,y))) for x,y in p]
# Surface uses Delaunay triangulation with the exact coastline as constrained edges.
from mathutils.geometry import delaunay_2d_cdt
verts=[Vector((x,y)) for x,y in p]
for x in range(-189,-101,3):
 for y in range(-20,269,3):
  if inside(x,y,p):verts.append(Vector((x,y)))
edges=[(i,(i+1)%len(p)) for i in range(len(p))]
vv,ee,ff,_,_,_=delaunay_2d_cdt(verts,edges,[list(range(len(p)))],1,.0001)
for f in ff:
 xc=sum(vv[i].x for i in f)/len(f);yc=sum(vv[i].y for i in f)/len(f)
 if not inside(xc,yc,p):continue
 m.face([(vv[i].x,vv[i].y,terrain_height(vv[i].x,vv[i].y)) for i in f],STONE if terrain_height(xc,yc)>25 else EARTH)
m.done('West hills / smooth continuous terrain')
for i in range(10000):
 x=random.uniform(-187,-108);y=random.uniform(-15,262)
 if inside(x,y,p):tree(x,y,random.uniform(.9,1.6),'West hills',terrain_height(x,y))
# Distant mountains complete the horizon behind the north urban shore.
m=Mesh()
for ix in range(100):
 for iy in range(14):
  def pt(i,j):
   x=-190+i*4.0;y=251+j*4
   z=.1+math.sin(math.pi*j/14)*max(1,13+7*math.sin(i*.29)+4*math.sin(i*.68))
   return (x,y,z)
  m.face([pt(ix,iy),pt(ix+1,iy),pt(ix+1,iy+1)],EARTH);m.face([pt(ix,iy),pt(ix+1,iy+1),pt(ix,iy+1)],EARTH)
m.done('Far horizon / mountain silhouette')
# Tiny vegetated islands in the estuary.
for x,y,rx,ry in [(-42,-62,3,1.7),(-49,-73,2,1.2),(-30,-104,4,2),(-64,-60,2,1),(-68,-80,2,1.3)]:
 m=Mesh();pp=[(x+rx*math.cos(i*math.tau/24),y+ry*math.sin(i*math.tau/24)) for i in range(24)];m.poly(pp,-.5,.85,STONE);m.poly(pp,.35,.04,GRASS);m.done('Estuary / green islet')
 for j in range(12):
  a=random.random()*6.28;r=random.random()*.8;tree(x+rx*r*math.cos(a),y+ry*r*math.sin(a),.8,'Islet')
# Populate roads with small vehicle instances using economical shared meshes.
vehicle=[]
for mi in [WHITE,RED,BLUE,ROOF2,YELLOW]:
 m=Mesh();m.box(0,0,.17,.32,.67,.24,mi);m.box(0,-.04,.34,.27,.34,.16,GLASS2);o=m.done('template car');vehicle.append(o.data);bpy.data.objects.remove(o,do_unlink=True)
for name,_,kind in regions:
 if kind not in ['cbd','west','east','res','south','north']:continue
 pp=polys[name];step=8 if kind in ['cbd','west','east'] else 7
 for j in range(170):
  x=round(random.uniform(min(x for x,y in pp),max(x for x,y in pp))/step)*step+.18;y=random.uniform(min(y for x,y in pp),max(y for x,y in pp))
  if inside(x,y,pp):
   o=bpy.data.objects.new(name+' / car',random.choice(vehicle));s.collection.objects.link(o);o.location=(x,y,.46)
# Reference camera: broad oblique view with port left, airport right, layered city behind.
cam.location=(29,-246,192);target=Vector((-8,31,1));cam.rotation_euler=(target-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='PERSP';cam.data.lens=38;cam.data.clip_end=2500
sun.data.energy=3.0;sun.data.color=(1,.87,.70);sun.data.angle=.08;sun.rotation_euler=(.55,-.35,-.6)
s.world.node_tree.nodes['Background'].inputs[0].default_value=(.36,.49,.65,1);s.world.node_tree.nodes['Background'].inputs[1].default_value=.65
s.render.engine='CYCLES';s.cycles.samples=40;s.cycles.use_denoising=True;s.render.resolution_x=2200;s.render.resolution_y=1800;s.render.resolution_percentage=100;s.view_settings.view_transform='AgX';s.view_settings.exposure=.35

if os.environ.get('CITY_PREVIEW'):
 s.render.resolution_x=1400;s.render.resolution_y=1100;s.cycles.samples=12
s.render.image_settings.file_format='PNG';s.render.image_settings.color_mode='RGB'
# Day/night share meshes. Lighting keeps the source world + sun model.
p=mats[LIGHT].node_tree.nodes['Principled BSDF'].inputs['Emission Strength'];p.default_value=.03;p.keyframe_insert('default_value',frame=1);p.default_value=2.2;p.keyframe_insert('default_value',frame=2)
s.frame_set(1);night=s.copy();night.name='NIGHT';night.use_fake_user=True;night.world=s.world.copy();night.world.name='Solara / evening';night.world.node_tree.nodes['Background'].inputs[0].default_value=(.11,.18,.29,1);night.world.node_tree.nodes['Background'].inputs[1].default_value=.48;night.collection.objects.unlink(sun);ns=sun.copy();ns.data=sun.data.copy();night.collection.objects.link(ns);ns.data.energy=.45;ns.data.color=(.55,.69,.88);night.frame_set(2)
bpy.context.window.scene=s;s.frame_set(1)
meshobs=[o for o in s.objects if o.type=='MESH'];tri=sum(sum(len(p.vertices)-2 for p in o.data.polygons) for o in meshobs);unique={o.data for o in meshobs};utri=sum(sum(len(p.vertices)-2 for p in me.polygons) for me in unique)
report=dict(counts,objects=len(s.objects),mesh_objects=len(meshobs),triangles_instances=tri,unique_meshes=len(unique),unique_mesh_triangles=utri,placements=placements,source_revision='fd270196525c4c6fe61217327c81274e67a58d4b')
(OUT/'city-v3-report.json').write_text(json.dumps(report,ensure_ascii=False,indent=2));print('BUILT',json.dumps({k:v for k,v in report.items() if k!='placements'}),flush=True)
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'city-v3.blend'),compress=True)
s.render.filepath=str(OUT/('preview.png' if os.environ.get('CITY_PREVIEW') else 'city-v3-day.png'));bpy.ops.render.render(write_still=True)
if os.environ.get('CITY_PREVIEW'):raise SystemExit(0)
bpy.context.window.scene=night;night.frame_set(2);night.render.filepath=str(OUT/'city-v3-night.png');bpy.ops.render.render(write_still=True)
bpy.context.window.scene=s;s.frame_set(1)
# Export supports GPU instancing in this Blender exporter; verify available RNA rather than assume.
args=dict(filepath=str(OUT/'city-v3.glb'),export_format='GLB',use_active_scene=True,export_animations=False,export_cameras=False,export_lights=False,export_extras=True,export_draco_mesh_compression_enable=True,export_draco_mesh_compression_level=7,export_draco_position_quantization=16)
props=bpy.ops.export_scene.gltf.get_rna_type().properties
if 'export_gpu_instances' in props:args['export_gpu_instances']=True
bpy.ops.export_scene.gltf(**args)
report['glb_bytes']=(OUT/'city-v3.glb').stat().st_size;report['gpu_instancing_requested']=args.get('export_gpu_instances',False);(OUT/'city-v3-report.json').write_text(json.dumps(report,ensure_ascii=False,indent=2));print('COMPLETE',report['glb_bytes'],flush=True)
