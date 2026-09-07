"""Non-destructive expansion of city-v3. Run with Blender --background --python."""
import bpy, math, random, json, ast, hashlib, bmesh
from pathlib import Path
from mathutils import Vector
random.seed(407)
OUT=Path(__file__).resolve().parent
BASE=OUT.parent/'city-v3/city-v3.blend'
basehash=hashlib.sha256(BASE.read_bytes()).hexdigest()
bpy.ops.wm.open_mainfile(filepath=str(BASE))
s=bpy.data.scenes['DAY']; night=bpy.data.scenes['NIGHT']; bpy.context.window.scene=s
original={o.name for o in s.objects}
original_game={o['game_id']:[list(row) for row in o.matrix_world] for o in s.objects if o.get('game_id')}
# Reuse original author's geometry helpers, without executing the original build.
src=(OUT/'source_city_v3.py').read_text()
nodes=ast.parse(src)
for node in nodes.body:
 if isinstance(node,(ast.FunctionDef,ast.ClassDef)) and node.name in {'Mesh','smooth','inside','distseg','path','bridge'}:
  exec(compile(ast.Module(body=[node],type_ignores=[]),'<v3 geometry helpers>','exec'))
names=['Land / warm earth','Seawalls / pale sandstone','Pavements / limestone','Asphalt','Lane paint','Parks / meadow','Trees / deep green','Trees / olive green','Trunks','Facade / blue glass','Facade / silver blue','Facade / silver mullions','Facade / ivory stone','Facade / brick','White architecture','Roof / terracotta','Roof / slate','Windows / evening','Port / oxide orange','Port / container blue','Port / container red','Service markings','River Solara / blue green']
mats=[bpy.data.materials[n] for n in names]
EARTH,STONE,PAVE,ROAD,PAINT,GRASS,LEAF,LEAF2,WOOD,GLASS,GLASS2,TRIM,CREAM,BRICK,WHITE,ROOF,ROOF2,LIGHT,CRANE,BLUE,RED,YELLOW,WATER=range(23)
counts={'bridges':0,'new_houses':0,'new_trees':0}
# Compact material slots and weld generated surfaces only.
def finish(mesh,name):
 o=mesh.done(name); bm=bmesh.new();bm.from_mesh(o.data);bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=.000001);bm.to_mesh(o.data);bm.free()
 used=sorted({p.material_index for p in o.data.polygons}); old=list(o.data.materials); ids=[used.index(p.material_index) for p in o.data.polygons];o.data.materials.clear()
 for i in used:o.data.materials.append(old[i])
 for p,i in zip(o.data.polygons,ids):p.material_index=i
 return o
polygons={}
def land(name,pts):
 pp=smooth(pts,3);polygons[name]=pp;m=Mesh();m.poly(pp,-1.2,1.6,EARTH);finish(m,name+' / terrain')
 m=Mesh();path(m,pp,.6,STONE,.42,True);finish(m,name+' / seawall');return pp
np=land('Nuclear coast',[(-284,-76),(-291,-30),(-277,7),(-231,15),(-207,-9),(-210,-54),(-234,-82)])
sp=land('Greenhaven suburb',[(171,-55),(244,-55),(285,-23),(301,46),(289,139),(265,208),(222,229),(184,206),(167,140),(163,66)])
# New districts use the same shared tree mesh datablocks as the original city.
trees=list({o.data for o in s.objects if o.type=='MESH' and ' / tree' in o.name})
def tree(x,y,size,region):
 o=bpy.data.objects.new(region+' / tree',random.choice(trees));s.collection.objects.link(o);o.location=(x,y,.44);o.scale=(size,size,size);o.rotation_euler.z=random.random()*math.tau;counts['new_trees']+=1
# Nuclear facility: remote coast, buffer forest, two open hyperboloid cooling towers.
m=Mesh();m.box(-250,-30,.47,51,45,.12,PAVE);finish(m,'Nuclear coast / serviced platform')
for cx in [-265,-246]:
 cy=-39; m=Mesh();rings=[];N=80
 for j in range(33):
  t=j/32;r=3.4+3.5*((t-.7)/.7)**2;z=1+t*20
  rings.append([(cx+r*math.cos(k*math.tau/N),cy+r*math.sin(k*math.tau/N),z) for k in range(N)])
 for a,b in zip(rings,rings[1:]):
  for k in range(N):q=(k+1)%N;m.face([a[k],a[q],b[q],b[k]],WHITE)
 # Visible inner throat and thick lip, not a solid capped cone.
 a=rings[-1];b=[(cx+(x-cx)*.95,cy+(y-cy)*.95,z-.6) for x,y,z in a]
 for k in range(N):q=(k+1)%N;m.face([a[k],a[q],b[q],b[k]],STONE)
 for k in range(24):
  a=k*math.tau/24;m.beam((cx+6.8*math.cos(a),cy+6.8*math.sin(a),.5),(cx+6.8*math.cos(a+.045),cy+6.8*math.sin(a+.045),2),.18,STONE)
 m.cone(cx,cy,.45,7.2,7.2,.4,STONE,80);o=finish(m,'Nuclear coast / cooling tower')
 for p in o.data.polygons:p.use_smooth=True
for cx in [-267,-252]:
 m=Mesh();m.cone(cx,-14,.55,4.7,4.7,6,WHITE,48)
 for j in range(16):
  a=j*math.pi/32;b=(j+1)*math.pi/32
  m.cone(cx,-14,6.55+4.7*math.sin(a),4.7*math.cos(a),max(.001,4.7*math.cos(b)),4.7*(math.sin(b)-math.sin(a)),WHITE,48)
 finish(m,'Nuclear coast / reactor containment')
m=Mesh();m.box(-232,-18,3.1,12,25,5.2,CREAM)
for y in range(-29,-6,3):
 m.box(-232,y,5.82,11.5,.4,.3,TRIM);m.box(-238.03,y,3.2,.04,1.5,2,GLASS2)
for x in [-240,-225]:
 for y in [-33,-37,-41,-45]:
  m.box(x,y,1.4,1.7,2.2,1.8,GLASS2)
  for dx in [-.5,0,.5]:m.cone(x+dx,y,2.3,.12,.08,.6,TRIM,8)
for y in [-31,-35]:m.beam((-267,y,1),(-231,y,1),.5,TRIM)
finish(m,'Nuclear coast / turbine hall and substation')
m=Mesh();fence=[(-277,-53),(-220,-53),(-220,-5),(-277,-5)]
for a,b in zip(fence,fence[1:]+fence[:1]):
 length=math.dist(a,b)
 for j in range(int(length/2)+1):
  t=j/max(1,int(length/2));x=a[0]+(b[0]-a[0])*t;y=a[1]+(b[1]-a[1])*t;m.box(x,y,1.25,.08,.08,1.6,TRIM)
 for z in [.8,1.4,2]:m.beam((*a,z),(*b,z),.035,TRIM)
route=[(-224,-29),(-215,-29),(-204,-28),(-170,-22),(-143,-15),(-118,-18)]
path(m,route,2.1,ROAD,.52);finish(m,'Nuclear coast / fence and access')
bridge('Nuclear service crossing',(-213,-28),(-143,-15),2.1,False)
for i in range(2100):
 x=random.uniform(-290,-206);y=random.uniform(-81,15)
 if inside(x,y,np) and not(-280<x<-216 and -57<y<0) and min(distseg(x,y,a,b) for a,b in zip(route,route[1:]))>2.5:tree(x,y,random.uniform(.9,1.6),'Nuclear coast')
# Auction waterfront extension keeps existing game sites and dominant tower untouched.
ap=land('Auction',[(-38,-9),(-17,-9),(-17,6),(-38,6)])
m=Mesh();m.box(-27.5,-1,.55,18,12,.2,PAVE)
for i in range(5):m.box(-27.5,-4.5+i*.3,.7+i*.12,12,3-i*.4,.16,STONE)
m.box(-27.5,0,2.9,12,6,4.2,CREAM);m.box(-27.5,-3.08,2.9,10.6,.08,2.8,GLASS)
for x in [-33,-31.2,-29.4,-25.6,-23.8,-22]:
 m.cone(x,-3.5,1.05,.24,.21,4.3,WHITE,16)
m.box(-27.5,0,5.2,13,7.5,.5,WHITE);m.box(-27.5,0,5.55,9,4,.25,GLASS2)
for x in [-32,-23]:
 for y in [-1,1.5]:m.box(x,y,5.85,.9,.9,.4,ROOF2)
o=finish(m,'Auction / main hall');o['game_id']='auction';o['integration_status']='visual_only';o['district']='Crown waterfront'
m=Mesh();path(m,[(-35,-6),(-35,5),(-29,10)],1.3,ROAD,.55)
for x in [-35,-20]:
 for y in [-6,-3,0,3]:tree(x,y,.9,'Auction')
finish(m,'Auction / plaza approach')
# Suburban reusable houses with pitched roofs, window frames, doors, garages and chimneys.
houses=[]
for k in range(12):
 m=Mesh();w=1.7+(k%3)*.25;d=2.2;h=1.3+(k%2)*.6;wall=[CREAM,WHITE,BRICK][k%3];roof=[ROOF,ROOF2][k%2]
 m.box(0,0,h/2,w,d,h,wall);m.poly([(-w/2-.12,-d/2-.15),(w/2+.12,-d/2-.15),(w/2+.12,d/2+.15),(-w/2-.12,d/2+.15)],h,.07,roof)
 m.face([(-w/2-.12,-d/2-.15,h),(0,-d/2-.15,h+.7),(0,d/2+.15,h+.7),(-w/2-.12,d/2+.15,h)],roof)
 m.face([(0,-d/2-.15,h+.7),(w/2+.12,-d/2-.15,h),(w/2+.12,d/2+.15,h),(0,d/2+.15,h+.7)],roof)
 for y in [-d/2,d/2]:m.face([(-w/2,y,h),(w/2,y,h),(0,y,h+.7)],wall)
 for x in [-.5,.5]:
  for y in [-1.11,1.11]:m.box(x,y,.85,.48,.04,.48,GLASS2);m.box(x,y-.03,.85,.035,.02,.49,WHITE)
 m.box(0,-1.13,.45,.35,.05,.9,WOOD);m.box(.55,.6,h+.55,.23,.3,.8,BRICK)
 m.box(w*.72,.2,.45,.9,1.5,.9,wall);m.box(w*.72,-.57,.38,.7,.04,.7,ROOF2)
 o=finish(m,'prototype house');houses.append(o.data);bpy.data.objects.remove(o,do_unlink=True)
# Gently meandering suburban blocks: coherent continuous road network, green gaps.
def bend(y):return 6*math.sin(y*.025)
roads=Mesh();gardens=Mesh();lamps=Mesh();housecoords=[]
xs=[182,196,210,224,238,252,266,280];ys=list(range(-35,209,14))
for x in xs:
 pts=[(x+bend(y),y) for y in range(-42,214)]
 for a,b in zip(pts,pts[1:]):
  if inside(*a,sp) and inside(*b,sp):path(roads,[a,b],1.05,ROAD,.46)
for y in ys:
 for x in range(172,290):
  a=(x+bend(y),y);b=(x+1+bend(y),y)
  if inside(*a,sp) and inside(*b,sp):path(roads,[a,b],1.05,ROAD,.46)
for x in xs[:-1]:
 for y in ys[:-1]:
  # Parkland corridor and park blocks break up the housing.
  park=(random.random()<.20 or (215<x<240 and 50<y<125))
  for dx in [3.2,7,10.7]:
   for dy in [3.4,10.2]:
    xx=x+dx+bend(y+dy);yy=y+dy
    if not all(inside(xx+a,yy+b,sp) for a,b in [(-2,-2),(2,2),(-2,2),(2,-2)]):continue
    if park:
     for j in range(3):tree(xx+random.uniform(-1,1),yy+random.uniform(-1.5,1.5),random.uniform(.8,1.3),'Greenhaven suburb')
     continue
    o=bpy.data.objects.new('Greenhaven suburb / house',random.choice(houses));s.collection.objects.link(o);o.location=(xx,yy,.45);o.rotation_euler.z=0 if dy<7 else math.pi;counts['new_houses']+=1;housecoords.append((xx,yy))
    gardens.box(xx,yy,.435,3.1,5,.04,GRASS);path(gardens,[(xx,yy),(xx, y if dy<7 else y+14)],.48,PAVE,.47)
    tree(xx-1.25,yy+1.7,.65,'Greenhaven suburb')
  xx=x+bend(y)+1.1
  if inside(xx,y+1,sp):
   lamps.box(xx,y+1,1.2,.05,.05,1.5,TRIM);lamps.box(xx,y+1,1.96,.2,.14,.05,LIGHT)
finish(roads,'Greenhaven suburb / street network');finish(gardens,'Greenhaven suburb / gardens and driveways');finish(lamps,'Greenhaven suburb / street lighting')
# Green coast buffer; forest dominates remaining peripheral land.
for i in range(2500):
 x=random.uniform(163,299);y=random.uniform(-53,227)
 if inside(x,y,sp) and min(distseg(x,y,a,b) for a,b in zip(sp,sp[1:]+sp[:1]))<6:tree(x,y,random.uniform(.8,1.35),'Greenhaven suburb')
# Wooded park interiors and gardens soften the low-density district.
for i in range(11500):
 x=random.uniform(168,296);y=random.uniform(-48,222)
 if not inside(x,y,sp):continue
 if min(abs(x-bend(y)-a) for a in xs)<1.8 or min(abs(y-a) for a in ys)<1.7:continue
 if any(abs(x-a)<2 and abs(y-b)<2.9 for a,b in housecoords):continue
 tree(x,y,random.uniform(.75,1.5),'Greenhaven suburb')
bridge('Greenhaven to Riverside',(133,68),(175,68),2.0,True)
bridge('Greenhaven north connection',(144,190),(185,190),2.0,False)
# Connections meet existing urban road grids; no bridge crosses a runway approach.
m=Mesh();path(m,[(130,68),(133,68)],2,ROAD,.49);path(m,[(175,68),(182+bend(68),68)],2,ROAD,.49);path(m,[(140,190),(144,190)],2,ROAD,.49);path(m,[(185,190),(196+bend(190),190)],2,ROAD,.49);finish(m,'Greenhaven suburb / bridge approaches')
# New location anchors carry IDs, not claims of implemented gameplay.
for ident,loc,district in [('nuclear-power',(-250,-30,1),'Nuclear coast'),('suburb-region',(230,80,1),'Greenhaven suburb')]:
 o=bpy.data.objects.new(ident,None);s.collection.objects.link(o);o.location=loc;o['game_id']=ident;o['district']=district;o['integration_status']='visual_only';o.empty_display_size=5
# Organize additions and share them with the existing NIGHT scene.
new=[o for o in s.objects if o.name not in original];groups={}
for o in new:
 name=o.get('district') or o.name.split(' / ')[0]
 if name not in groups:
  c=bpy.data.collections.new('V4 / '+name);s.collection.children.link(c);night.collection.children.link(c);groups[name]=c
 groups[name].objects.link(o)
 if o.name in s.collection.objects:s.collection.objects.unlink(o)
# Simple convex collision proxies for interactive new building footprints, separate hidden collection.
cc=bpy.data.collections.new('V4 / Collision proxies');s.collection.children.link(cc);night.collection.children.link(cc)
for name,loc,dim in [('Auction_main_hall',(-27.5,0,2.9),(12,6,4.2)),('Nuclear_platform',(-250,-30,.2),(51,45,.4))]:
 m=Mesh();m.box(*loc,*dim,STONE);o=finish(m,'UCX_'+name+'_00');cc.objects.link(o);s.collection.objects.unlink(o);o.hide_render=True;o.hide_set(True);o.display_type='WIRE';o['purpose']='collision_proxy'
# Wider overview preserves source orientation, with enough margin for both extensions.
bpy.data.objects['Solara / open water'].scale=(4,4,1)
cam=s.camera;cam.location=(35,-360,300);target=Vector((5,35,0));cam.rotation_euler=(target-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.lens=25;cam.data.clip_end=3000
for sc in [s,night]:
 sc.camera=cam;sc.render.resolution_x=2200;sc.render.resolution_y=1200;sc.render.resolution_percentage=100;sc.cycles.samples=24;sc.cycles.use_denoising=True;sc.render.image_settings.file_format='PNG';sc.render.image_settings.color_mode='RGB';sc.use_fake_user=True
 # Keep inherited mist/compositor and two-world setup.
 sc.world.mist_settings.start=440;sc.world.mist_settings.depth=600
s.frame_set(1);bpy.context.window.scene=s
meshobs=[o for o in s.objects if o.type=='MESH' and not o.hide_render];unique={o.data for o in meshobs}
tri=lambda me:sum(len(p.vertices)-2 for p in me.polygons)
report={'base_sha256':basehash,'objects':len(s.objects),'visible_mesh_objects':len(meshobs),'triangles_instances':sum(tri(o.data) for o in meshobs),'unique_meshes':len(unique),'unique_mesh_triangles':sum(tri(me) for me in unique),'counts_added':counts,'original_game_locations_preserved':all(original_game[o['game_id']]==[list(row) for row in o.matrix_world] for o in s.objects if o.get('game_id') in original_game),'placements':[{'id':o['game_id'],'object':o.name,'district':o.get('district',''),'position':list(o.location)} for o in s.objects if o.get('game_id')],'ue5_verified':False,'gameplay_integrated':False,'collision_status':'two draft proxies only; full map collision not complete','source_file_unchanged':hashlib.sha256(BASE.read_bytes()).hexdigest()==basehash}
(OUT/'city-v4-report.json').write_text(json.dumps(report,ensure_ascii=False,indent=2))
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'city-v4.blend'),compress=True)
print('V4_SAVED',json.dumps({k:v for k,v in report.items() if k!='placements'}),flush=True)
