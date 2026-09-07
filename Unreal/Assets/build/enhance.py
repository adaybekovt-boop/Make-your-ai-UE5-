import bpy,math,random,bmesh
from mathutils import Vector,Matrix
from blender_common import *

def interp(points,samples=5):
 out=[]
 for i in range(len(points)-1):
  a,b,c,d=points[max(i-1,0)],points[i],points[i+1],points[min(i+2,len(points)-1)]
  for j in range(samples):
   t=j/samples;out.append(tuple(.5*((2*q)+(-p+r)*t+(2*p-5*q+4*r-s)*t*t+(-p+3*q-3*r+s)*t*t*t) for p,q,r,s in zip(a,b,c,d)))
 return out+[points[-1]]

def shell(name,sections,m):
 sections=interp(sections,5);N=96;vs=[];fs=[]
 for y,w,base,top in sections:
  for k in range(N):
   a=k*math.tau/N;c=math.cos(a);s=math.sin(a);vs.append((w*math.copysign(abs(c)**.45,c),y,(top+base)/2+(top-base)/2*math.copysign(abs(s)**.45,s)))
 for j in range(len(sections)-1):
  for k in range(N):fs.append((j*N+k,j*N+(k+1)%N,(j+1)*N+(k+1)%N,(j+1)*N+k))
 fs.extend([tuple(range(N-1,-1,-1)),tuple(range((len(sections)-1)*N,len(sections)*N))]);me=bpy.data.meshes.new(name);me.from_pydata(vs,[],fs);me.materials.append(m);me.update();bm=bmesh.new();bm.from_mesh(me);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(me);bm.free();o=bpy.data.objects.new(name,me);bpy.context.collection.objects.link(o)
 for p in me.polygons:p.use_smooth=True
 return o

def vehicle(g,hero=False):
 idx=g.get('idx',4);w=1.0 if hero else g['width'];L=4.9 if hero else g['length'];r=.39 if hero else g['radius'];axles=[-1.52,1.48] if hero else g['axles'];s=1 if hero else .16
 rubber=g['rubber'];alloy=g.get('chrome',g.get('alloy'));paint=g['paint'];lamp=g.get('led',g.get('lamp'));glass=g['glass'];red=g['red']
 rotor=mat('Vented rotor steel',(.22,.235,.25),.82,.38);caliper=mat('Brake caliper ceramic red',(.48,.027,.012),.45,.30)
 for side in [-1,1]:
  for y in axles:
   wx=side*(w+.020*s/.16) if not hero else side*1.112
   # Functional wheel islands: tyre tread, annular rim, drilled rotor, hub bolts and caliper.
   torus('Rim rolled lip',(wx,y,r),r*.72,r*.018,alloy,(0,math.pi/2,0))
   cylinder('Ventilated brake rotor',(wx-side*r*.05,y,r),r*.53,r*.035,rotor,96,(0,math.pi/2,0))
   for k in range(30):
    a=k*math.tau/30
    for rr in [.39,.47]:cylinder('Rotor cooling bore',(wx+side*r*.002,y+r*rr*math.cos(a),r+r*rr*math.sin(a)),r*.022,r*.004,rubber,12,(0,math.pi/2,0))
   cube('Brake caliper',(wx+side*r*.025,y+r*.40,r), (r*.10,r*.22,r*.48),caliper,r*.035)
   for k in range(5):
    a=k*math.tau/5;cylinder('Wheel hex fastener',(wx+side*r*.065,y+r*.20*math.cos(a),r+r*.20*math.sin(a)),r*.039,r*.045,alloy,6,(0,math.pi/2,0))
   # Tread blocks are actual geometry, shared later by static mesh material batching.
   for k in range(64):
    a=k*math.tau/64
    for row in [-1,1]:
     o=cube('Tyre shoulder tread',(side*(w-.005)+row*r*.12,y+r*.997*math.sin(a),r+r*.997*math.cos(a)),(r*.17,r*.047,r*.015),rubber)
     o.rotation_euler.x=-a;o.rotation_euler.z=row*.22
   for rr in [.83,.92]:torus('Tyre sidewall moulding',(wx-side*r*.065,y,r),r*rr,r*.004,rubber,(0,math.pi/2,0))
  # Indicator repeaters and mirror lenses.
  if not hero:
   z=.285 if idx==2 else .244
   if idx!=5:
    cube('Mirror reflective insert',(side*(w+.02),-.098,z),(.021,.002,.013),alloy,.003)
    cube('Mirror repeater',(side*(w+.033),-.12,z-.002),(.001,.025,.0025),lamp,.0005)
   for yy in [-L*.18,L*.18]:
    cylinder('Parking ultrasonic sensor',(side*w*.75,side*0+L*.507,.125 if idx!=5 else .20),.0023,.002,rubber,20,(math.pi/2,0,0))
 # Grille depth, individual slats and a proper plate surround.
 if not hero:
  for x in range(-7,8):cube('Grille vertical fin',(x*w*.05,-L*.509,.166),(.0012,.004,.021),alloy,.0003)
  for x in [-w*.69,w*.69]:
   for z in [-.005,0,.005]:cube('Headlight internal optic',(x,-L*.522,(.219 if idx==5 else .177)+z),(w*.32,.001,.0014),lamp,.0004)
  text('License plate engraving',f'NRN {idx:02d}',(0,-L*.511,.133),.010,rubber)
  for sign in [-1,1]:
   y=L*.503
   cylinder('Exhaust polished sleeve',(sign*w*.60,y,.092),.010,.034,alloy,48,(math.pi/2,0,0));cylinder('Exhaust dark core',(sign*w*.60,y+.018,.092),.0075,.002,rubber,32,(math.pi/2,0,0))
  if idx==5:
   for y in [-.38,-.18,.02,.22,.42]:
    for side in [-1,1]:
     for z in [.305,.327]:cube('Passenger belt trim',(side*(w+.004),y,z),(.003,.18,.002),alloy,.0005)
   text('Coach destination','NEURON / CAMPUS',(0,-L*.505,.475),.022,lamp)
  elif idx==2:
   for side in [-1,1]:
    cube('Sliding rail channel',(side*(w+.001),.15,.26),(.003,.32,.008),rubber,.001)
    cube('Sliding rail insert',(side*(w+.003),.15,.26),(.002,.30,.003),alloy,.0006)
   text('Fleet rear badge','NEURON',(0,L*.503,.30),.027,alloy,(-math.pi/2,0,math.pi))
  else:
   for side in [-1,1]:line('Wiper arm',[(side*.055,-.14,.231),(side*.025,-.106,.264),(side*.077,-.10,.265)],.0012,rubber)
 # Flush glazing sill and lamp housings, correcting gaps in the mini-game generator.
 if not hero:
  for ob in list(bpy.context.scene.objects):
   if ob.name.startswith('Integrated cabin'):
    for vert in ob.data.vertices:
     if abs(vert.co.y+.15)<.0001:vert.co.z=min(vert.co.z,.2115)
   if ob.name.startswith('Swept headlamp'):
    ob.location.y=-L*.502;ob.scale.y=.4
   if ob.name.startswith('Rear lamp'):
    ob.location.y=L*.502;ob.scale.y=.5
 # Transparent glazing deliberately restrained: authoring look rather than invisible black slabs.
 p=glass.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(.012,.026,.033,1);p.inputs['Metallic'].default_value=0;p.inputs['Roughness'].default_value=.19;p.inputs['Specular IOR Level'].default_value=.22;p.inputs['Coat Weight'].default_value=.12
 p=paint.node_tree.nodes.get('Principled BSDF');p.inputs['Coat Weight'].default_value=.85;p.inputs['Coat Roughness'].default_value=.16
 # Consolidate four complete wheel clusters into independently pivoted static assemblies.
 bpy.context.view_layer.update()
 names=('Tyre','Rim','Dark inset','Flush wheel','Wheel hex','Machined wheel','Ventilated brake','Rotor cooling','Brake caliper','Wheel side','Split spoke','Centre cap','Lug','Brake rotor','Hub')
 wheelobs=[o for o in list(bpy.context.scene.objects) if o.type=='MESH' and o.name.startswith(names)]
 for si,side in enumerate([-1,1]):
  for ai,y in enumerate(axles):
   part=[o for o in list(bpy.context.scene.objects) if o.type=='MESH' and not o.get('part') and o.name.startswith(names) and (o.matrix_world.translation.x*side)>0 and abs(o.matrix_world.translation.y-y)<r*1.15]
   if part:
    o=join(part,f'Wheel_{"L" if side<0 else "R"}_{"Front" if ai==0 else "Rear"}');o['part']='wheel';bpy.context.scene.cursor.location=(side*w,y,r);active(o);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
 # Physical dimensions. Original generator uses small city units.
 scale=(1,1,1) if hero else (6.4,7.5,6.3) if idx==5 else (6.1,7.0,6.0) if idx==2 else (6.0,6.5,5.05)
 for o in list(bpy.context.scene.objects):
  if o.type=='MESH':
   world=o.matrix_world.copy();o.data.transform(world);o.matrix_world=Matrix.Identity(4)
   for v in o.data.vertices:
    for k in range(3):v.co[k]*=scale[k]
   if o.get('part')=='wheel':
    side=-1 if '_L_' in o.name else 1;y=axles[0 if 'Front' in o.name else 1];active(o);bpy.context.scene.cursor.location=(side*w*scale[0],y*scale[1],r*scale[2]);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
 return {'changes':['96-point interpolated body shell' if not hero else 'Metre-scale coupe master retained','Clearcoat paint and micro-surface PBR maps','Four independent wheel pivots; tread, sidewalls, rim lips, brake rotors, calipers and fasteners','Layered lamps, grille and model-specific exterior fittings'],'rig':'four static wheel assemblies; no Chaos vehicle setup'}

def person(g):
 v=g['variant'];skin=g['skin'];hair=g['hair'];shirt=g['shirt'];pants=g['pants'];shoe=g['shoe'];white=g['white'];metal=g['buckle'];female=g['female'];shoulder=g['shoulder']
 def detail(o,joint='body'):o['joint']=joint;return o
 # Anatomical landmarks are retained from the original stylized cast.
 iris=mat('Iris',(.14,.21,.16),0,.26);eye=mat('Eye sclera',(.8,.82,.73),0,.2)
 for x in [-.039,.039]:
  detail(sphere('Eye sclera',(x,-.0945,1.625),(.0105,.005,.0068),eye));detail(sphere('Iris',(x,-.0988,1.625),(.0041,.0014,.0045),iris));detail(sphere('Pupil',(x,-.1000,1.625),(.0018,.0007,.0027),g['black']))
 for side in [-1,1]:
  x=side*(shoulder+.025);joint='arm_l' if side<0 else 'arm_r'
  detail(sphere('Tailored shoulder cap',(x-side*.022,0,1.354),(.073,.062,.074),shirt),joint)
  # Rounded palms plus separated fingers, including an opposed thumb.
  for f in range(4):detail(sphere('Finger',(x+(f-1.5)*.013,-.031,.885-(.008 if f in [0,3] else 0)),(.0078,.018,.030 if f in [1,2] else .025),skin),joint)
  detail(sphere('Thumb',(x-side*.034,-.05,.925),(.013,.018,.027),skin),joint)
  if v in [0,1,2,4]:
   for z in [1.153,1.174]:detail(line('Cuff seam',[(x+.047*math.cos(i*math.tau/48),-.005+.048*math.sin(i*math.tau/48),z) for i in range(49)],.0015,white if v==4 else shirt),joint)
  # Contoured shoe toe, sole welt and separate laces.
  lx=side*.085;j='knee_l' if side<0 else 'knee_r'
  for yy in [-.099,-.067,-.035,-.003]:detail(line('Shoe lace',[(lx-.032,yy,.099),(lx,yy+.012,.107),(lx+.032,yy,.099)],.002,white if v==3 else shoe),j)
  detail(line('Shoe welt',[(lx+.053*math.cos(a),-.04+.104*math.sin(a),.032) for a in [i*math.tau/64 for i in range(65)]],.0025,shoe),j)
  if v not in [4,5]:
   detail(line('Trouser side seam',[(lx+side*.065,0,.81),(lx+side*.048,0,.58),(lx+side*.048,0,.44)],.0014,pants),'leg_l' if side<0 else 'leg_r')
   detail(line('Trouser lower seam',[(lx+side*.047,0,.42),(lx+side*.046,0,.12)],.0014,pants),j)
 # A wristwatch, shirt seams, pocket construction and restrained hair strands.
 x=-(shoulder+.025);detail(cube('Watch case',(x,-.065,1.013),(.034,.012,.042),metal,.005),'arm_l');detail(cube('Watch dial',(x,-.072,1.013),(.026,.002,.031),g['black'],.002),'arm_l')
 for side in [-1,1]:
  detail(line('Shoulder stitching',[(side*.07,-.065,1.433),(side*.14,-.08,1.401),(side*(shoulder-.01),-.05,1.36)],.0013,shirt))
 if v in [0,1]:
  for side in [-1,1]:
   detail(cube('Chest pocket',(side*.10,-.117,1.286),(.074,.010,.088),shirt,.006));detail(line('Pocket stitch',[(side*.10-.032,-.123,1.326),(side*.10-.032,-.123,1.25),(side*.10+.032,-.123,1.25),(side*.10+.032,-.123,1.326)],.0013,white))
 for k in range(0):
  t=(k-7)/8
  pts=[(t*.085+.017*math.sin(a),-.01-.062*math.cos(a),1.704+.061*math.sin(a)) for a in [i*math.pi/24 for i in range(25)]]
  detail(line('Groomed hair ridge',pts,.0015,hair))
 if v==2:detail(line('Tie',[(0,-.098,1.407),(0,-.124,1.31),(0,-.126,1.16)],.014,mat('Silk tie',(.25,.022,.032),.05,.34)))
 # Join each source joint's geometry; preserve named PBR slots, no vertex palette dependency.
 parts=[]
 for name in ['body','arm_l','arm_r','leg_l','leg_r','knee_l','knee_r']:
  obs=[o for o in list(bpy.context.scene.objects) if o.type=='MESH' and (o.get('joint')==name or o.name==name)]
  for o in obs:
   world=o.matrix_world.copy();o.parent=None;o.matrix_world=world
  ob=join(obs,name);ob['joint']=name;parts.append(ob)
 # Custom 22-bone deform rig. Explicitly not an Epic mannequin skeleton.
 bpy.ops.object.armature_add(enter_editmode=True,location=(0,0,0));rig=bpy.context.object;rig.name='SK_Neuron';a=rig.data;a.name='SKEL_Neuron_Custom';a.edit_bones.remove(a.edit_bones[0]);bones={}
 def bone(name,head,tail,parent=None):
  b=a.edit_bones.new(name);b.head=head;b.tail=tail
  if parent:b.parent=bones[parent]
  bones[name]=b
 bone('root',(0,0,0),(0,0,.2));bone('pelvis',(0,0,.84),(0,0,1.0),'root');bone('spine_01',(0,0,1.0),(0,0,1.2),'pelvis');bone('spine_02',(0,0,1.2),(0,0,1.40),'spine_01');bone('neck',(0,0,1.40),(0,0,1.49),'spine_02');bone('head',(0,0,1.49),(0,0,1.76),'neck')
 for side,sign in [('l',-1),('r',1)]:
  x=sign*(shoulder+.025);bone('clavicle_'+side,(0,0,1.37),(x,0,1.36),'spine_02');bone('upperarm_'+side,(x,0,1.36),(x,0,1.15),'clavicle_'+side);bone('lowerarm_'+side,(x,0,1.15),(x,-.025,.97),'upperarm_'+side);bone('hand_'+side,(x,-.025,.97),(x,-.03,.86),'lowerarm_'+side)
  x=sign*.085;bone('thigh_'+side,(x,0,.84),(x,0,.43),'pelvis');bone('calf_'+side,(x,0,.43),(x,0,.10),'thigh_'+side);bone('foot_'+side,(x,0,.10),(x,-.11,.055),'calf_'+side);bone('toe_'+side,(x,-.11,.055),(x,-.155,.055),'foot_'+side)
 bpy.ops.object.mode_set(mode='OBJECT')
 for o in parts:
  joint=o['joint'];groups={n:o.vertex_groups.new(name=n) for n in bones};world=o.matrix_world.copy();o.data.transform(world);o.matrix_world=Matrix.Identity(4)
  for vert in o.data.vertices:
   z=vert.co.z
   if joint=='body':
    n='head' if z>1.48 else 'neck' if z>1.4 else 'spine_02' if z>1.23 else 'spine_01' if z>1.01 else 'pelvis';groups[n].add([vert.index],1,'REPLACE')
   elif joint.startswith('arm'):
    side=joint[-1];t=max(0,min(1,(z-1.11)/.09))
    if z<.975:groups['hand_'+side].add([vert.index],1,'REPLACE')
    else:
     if t:groups['upperarm_'+side].add([vert.index],t,'REPLACE')
     if t<1:groups['lowerarm_'+side].add([vert.index],1-t,'REPLACE')
   else:
    side=joint[-1];n=('thigh_' if joint.startswith('leg') else 'foot_' if z<.13 else 'calf_')+side;groups[n].add([vert.index],1,'REPLACE')
  o.parent=rig;mod=o.modifiers.new('Skeleton deform','ARMATURE');mod.object=rig
 # Weld coincident trouser knee rings and retain the deform weight layer.
 for side in ['l','r']:
  legparts=[o for o in list(bpy.context.scene.objects) if o.type=='MESH' and o.get('joint') in ['leg_'+side,'knee_'+side]]
  if legparts:
   leg=join(legparts,'Leg_'+side);bm=bmesh.new();bm.from_mesh(leg.data);bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=.0001);bm.to_mesh(leg.data);bm.free()
 # Basic walk cycle for verification, not a finished animation library.
 for frame in range(1,26):
  phase=(frame-1)/24*math.tau
  for side,sign in [('l',1),('r',-1)]:
   for name,angle in [('thigh_',.28*math.sin(phase)*sign),('calf_',max(0,-math.sin(phase)*sign)*.42),('upperarm_',-.23*math.sin(phase)*sign),('lowerarm_',-.10)]:
    p=rig.pose.bones[name+side];p.rotation_mode='XYZ';p.rotation_euler=(angle,0,0);p.keyframe_insert('rotation_euler',frame=frame,group=p.name)
 if rig.animation_data and rig.animation_data.action:rig.animation_data.action.name=f'AN_person_{v+1}_walk_preview'
 bpy.context.scene.frame_end=25;bpy.context.scene.render.fps=24;bpy.context.scene.frame_set(1)
 return {'changes':['32-section clothing and 32x20 organic surfaces rebuilt from source proportions','Eyes with iris and pupil, separated fingers, garment seams, pockets, watch and shoe fittings','Separate fabric, skin, hair and leather PBR slots','22-bone custom deform skeleton with 25-frame walk preview'],'rig':'Custom Neuron, not Epic mannequin; no facial rig or finger articulation; animation needs production polish'}

def rack(key):
 bpy.ops.wm.read_factory_settings(use_empty=True)
 tier=['rack-basic','rack-cooled','rack-enterprise'].index(key);H=[1.25,1.85,2.2][tier];W=.8;D=1.0
 shellm=mat('Powder coated rack shell',(.038,.06,.075),.65,.35);dark=mat('Rubber and grille recess',(.008,.012,.016),0,.7);alloy=mat('Brushed aluminium',(.33,.39,.43),.85,.3);led=mat('chip_indicator',(.03,.7,.5),.1,.25,3);copper=mat('Copper coolant',(.45,.15,.045),.8,.27);blue=mat('Network cable blue',(.015,.12,.36),0,.55);amber=mat('Link amber',(.95,.24,.01),.1,.3,2)
 for x in [-.33,.33]:
  for y in [-.40,.40]:
   cylinder('Adjustable levelling foot',(x,y,.045),.055,.065,dark);cylinder('Threaded foot stem',(x,y,.10),.020,.10,alloy)
 cube('Rack plinth',(0,0,.14),(W,D,.10),shellm,.015);cube('Rack roof',(0,0,H),(W,D,.075),shellm,.012)
 for x in [-W/2+.03,W/2-.03]:
  cube('Removable side panel',(x,0,H/2+.07),(.045,D-.02,H-.14),shellm,.008)
  for y in [-.48,.48]:cube('Extruded corner post',(x,y,H/2),(.065,.055,H-.05),alloy,.004)
  for z in [.25,H-.22]:
   for y in [-.39,.39]:cylinder('Captive side panel screw',(x+(.027 if x>0 else -.027),y,z),.008,.005,alloy,12,(0,math.pi/2,0))
 for x in [-.305,.305]:
  cube('19 inch mounting rail',(x,-.455,H/2),(.035,.027,H-.24),alloy,.002)
  for i in range(int((H-.30)/.032)):cube('Square cage nut opening',(x,-.471,.24+i*.032),(.012,.002,.012),dark,.001)
 chassis=join([o for o in bpy.context.scene.objects if o.type=='MESH'],'SM_Rack_Chassis');chassis['role']='rack_chassis'
 slots=[4,7,9][tier];dz=(H-.42)/slots
 for i in range(slots):
  before=set(bpy.context.scene.objects);z=.24+i*dz
  cube('Compute module case',(0,-.03,z+dz*.42),(.57,.79,dz*.80),shellm,.004)
  cube('Anodized module fascia',(0,-.47,z+dz*.42),(.586,.025,dz*.77),alloy,.003)
  for c in range(4):
   x=-.208+c*.138;cube('Hot swap drive caddy',(x,-.489,z+dz*.45),(.129,.022,dz*.52),dark,.003)
   cube('Drive tray brushed face',(x,-.503,z+dz*.45),(.118,.010,dz*.45),shellm,.002)
   for row in range(3):
    for col in range(7):cube('Caddy ventilation slot',(x-.045+col*.014,-.51,z+dz*.33+row*dz*.11),(.009,.003,.004),dark,.001)
   cube('Caddy release latch',(x+.045,-.517,z+dz*.43),(.014,.015,dz*.26),alloy,.002)
   cube('Drive activity LED',(x-.042,-.515,z+dz*.62),(.012,.003,.003),led)
  for x in [-.279,.279]:
   for zz in [z+dz*.15,z+dz*.68]:cylinder('Rack unit captive screw',(x,-.49,zz),.007,.007,alloy,12,(math.pi/2,0,0))
   line('Folded module handle',[(x,-.49,z+dz*.25),(x,-.535,z+dz*.25),(x,-.535,z+dz*.60),(x,-.49,z+dz*.60)],.005,alloy)
  # Rear I/O with actual sockets, fan grilles and managed network patch cords.
  for x in [-.20,-.10]:
   cube('RJ45 metal socket',(x,.378,z+dz*.4),(.034,.017,.023),alloy,.002);cube('RJ45 socket recess',(x,.389,z+dz*.4),(.025,.002,.014),dark)
   for k in range(8):cube('RJ45 contact',(x-.01+k*.0028,.391,z+dz*.4),(.001,.001,.009),copper)
   line('Network patch cable',[(x,.393,z+dz*.4),(x,.445,z+dz*.4),(x+.03,.46,z+dz*.55),(.28,.46,z+dz*.7),(.31,.40,z+dz*.7+.05)],.004,blue)
  for x in [.05,.18]:
   cylinder('Rear fan recess',(x,.379,z+dz*.43),dz*.31,.012,dark,48,(math.pi/2,0,0))
   for k in range(7):
    a=k*math.tau/7;line('Fan protective grille',[(x,.39,z+dz*.43),(x+dz*.29*math.cos(a),.39,z+dz*.43+dz*.29*math.sin(a))],.0015,alloy)
   torus('Fan ring',(x,.39,z+dz*.43),dz*.29,.0025,alloy,(math.pi/2,0,0))
  ob=join([o for o in bpy.context.scene.objects if o not in before and o.type=='MESH'],f'SM_Compute_Module_{i+1:02d}');ob['role']='compute_module'
 # Network switch in the header.
 before=set(bpy.context.scene.objects);cube('Management switch',(0,-.40,H-.13),(.59,.18,.10),shellm,.006)
 for k in range(16):
  x=-.26+k*.034;cube('Network switch port',(x,-.494,H-.125),(.024,.006,.021),dark,.002);cube('Port link indicator',(x,-.498,H-.10),(.008,.002,.003),led if k%3 else amber)
 text('Rack model designation',['NEURON / EDGE','NEURON / FLOW','NEURON / CORE'][tier],(0,-.500,H-.075),.024,alloy)
 join([o for o in bpy.context.scene.objects if o not in before and o.type=='MESH'],'SM_Rack_Management')
 if tier:
  before=set(bpy.context.scene.objects)
  for y in [-.25,.25]:
   cylinder('Roof exhaust well',(0,y,H+.045),.18,.035,dark,96)
   for k in range(9):
    a=k*math.tau/9;o=cube('Fan swept blade',(.075*math.cos(a),y+.075*math.sin(a),H+.063),(.14,.044,.010),alloy,.008);o.rotation_euler.z=a+.5
   cylinder('Fan motor',(0,y,H+.072),.038,.021,shellm,48)
   for rr in [.08,.12,.16,.18]:torus('Roof fan safety ring',(0,y,H+.087),rr,.0025,alloy)
  if tier==2:
   for x in [-.437,.437]:
    pts=[(x,.35,.20),(x,.35,H-.10),(x,.23,H+.09),(x,-.25,H+.09),(x,-.34,H-.12)]
    line('External coolant manifold',pts,.018,copper)
    for z in [.3,.7,1.1,1.5,1.9]:
     cube('Pipe stand-off',(x*.94,.35,z),(.07,.03,.035),alloy,.005);torus('Pipe retaining collar',(x,.35,z),.021,.005,alloy)
  join([o for o in bpy.context.scene.objects if o not in before and o.type=='MESH'],'SM_Rack_Cooling')
 return {'changes':['Physical rack chassis with mounting rails, levelling feet and captive screws','Removable hot swap compute modules; layered drive caddies, ventilation, latches and LEDs','Rear network sockets, fan grilles and patch cables','Tier-specific exhaust fans and coolant manifolds'],'dimensions_note':'New equipment proportions: 0.8 x 1.0 m footprint, chassis heights 1.25 / 1.85 / 2.20 m'}

def interior(g):
 key=g['key'];w=g['w'];d=g['d'];back=d/2;left=-w/2;edge=g['edge'];dark=g['graphite'];white=g['white'];wood=g['wood'];red=g['red'];teal=g['teal'];amber=g['amber']
 original=list(bpy.context.scene.objects)
 # Existing furniture remains at its source coordinates, now with recognisable construction.
 for o in original:
  if o.type!='MESH':continue
  x,y,z=o.location
  if o.name.startswith('Keyboard'):
   for row in range(4):
    for col in range(12):cube('Individual keyboard key',(x-.20+col*.034,y-.052+row*.032,z+.023),(.029,.026,.011),edge,.002)
  if o.name.startswith('Upholstered seat'):
   for k in range(5):
    a=k*math.tau/5;line('Chair five star leg',[(x,y,.17),(x+.28*math.cos(a),y+.28*math.sin(a),.09)],.018,dark)
    cylinder('Chair caster',(x+.28*math.cos(a),y+.28*math.sin(a),.063),.045,.035,dark,32,(math.pi/2,0,a))
   for sign in [-1,1]:
    line('Chair armrest bracket',[(x+sign*.19,y,.45),(x+sign*.29,y+.06,.65),(x+sign*.29,y-.08,.65)],.014,edge);cube('Chair arm pad',(x+sign*.29,y-.05,.68),(.065,.26,.043),dark,.018)
   for sign in [-1,1]:line('Seat stitched seam',[(x+sign*.21,y-.18,.535),(x+sign*.21,y+.15,.535)],.002,white)
  if o.name.startswith('Monitor casing'):
   for k in range(14):cube('Monitor rear vent',(x-.24+k*.036,y+.03,z+.09),(.012,.007,.09),dark,.002)
   line('Monitor power cable',[(x,y+.04,z-.14),(x+.03,y+.08,.86),(x+.3,y+.1,.75),(x+.4,y+.1,.1)],.006,dark)
  if o.name.startswith('Fire extinguisher'):
   cylinder('Extinguisher neck',(x,y,z+.28),.03,.08,edge);cube('Extinguisher trigger',(x+.04,y,z+.32),(.12,.025,.02),dark,.008)
   line('Extinguisher hose',[(x-.025,y,z+.3),(x-.13,y,z+.30),(x-.16,y,z+.15),(x-.13,y,z-.12)],.012,dark)
   cube('Extinguisher label',(x,y-.096,z),(.10,.004,.20),white,.005)
  if o.name.startswith(('Tool cabinet','Electrical cabinet')):
   for dx in [-.4,.4]:
    for dz in [-.5,.5]:cylinder('Cabinet captive screw',(x+dx,y-.19,z+dz),.009,.007,edge,12,(math.pi/2,0,0))
 # Wall electrical service: conduit brackets, power outlets, sockets, breakers and cable loops.
 for x in [left+.6,-left-.65]:
  for z in [.35,1.10,2.30]:
   cube('Wall socket plate',(x,back-.125,z),(.15,.025,.115),white,.012)
   for dx in [-.027,.027]:cylinder('Socket pin recess',(x+dx,back-.141,z),.012,.003,dark,24,(math.pi/2,0,0))
   for dx in [-.06,.06]:cylinder('Socket plate screw',(x+dx,back-.142,z),.0035,.003,edge,12,(math.pi/2,0,0))
 for y in [back-.7-i*1.2 for i in range(int(d/1.2))]:
  for x in [left+.18,-left-.18]:
   cube('Raceway wall clip',(x,y,.20),(.055,.065,.02),edge,.003)
 # Detailed suspended fittings along utility strips keep the playable centre clear.
 for side in [-1,1]:
  x=side*(w/2-.45)
  for i in range(max(2,int(d/3))):
   y=-d/2+1.3+i*2.6
   cube('Suspended LED housing',(x,y,3.12),(.17,1.6,.09),dark,.02);cube('Prismatic diffuser',(x,y,3.073),(.125,1.49,.018),amber,.006)
   for yy in [y-.63,y+.63]:line('Luminaire suspension',[(x,yy,3.165),(x,yy,3.30)],.004,edge)
   for k in range(16):cube('Diffuser prism',(x,y-.70+k*.09,3.063),(.12,.006,.004),white,.001)
 # A believable perimeter support bench with vice / instruments for starter locations.
 if key in ['garage','workshop']:
  x=left+.5;y=back-3.4
  cube('Utility steel cabinet',(x,y,.53),(.74,1.25,1.04),dark,.025);cube('Solid utility worktop',(x,y,1.10),(.84,1.36,.09),wood,.025)
  for zz in [.21,.42,.63,.84]:
   cube('Tool drawer face',(x+.379,y,zz),(.025,1.11,.18),red,.01);line('Drawer steel handle',[(x+.40,y-.32,zz),(x+.45,y-.32,zz),(x+.45,y+.32,zz),(x+.40,y+.32,zz)],.009,edge)
  cylinder('Bench vice swivel',(x,y+.23,1.18),.12,.10,edge);cube('Vice jaw',(x,y+.18,1.28),(.20,.16,.16),dark,.01);cube('Vice sliding jaw',(x,y-.05,1.28),(.20,.06,.16),edge,.008);cylinder('Vice lead screw',(x,y-.16,1.24),.018,.29,edge,32,(math.pi/2,0,0));line('Vice tommy bar',[(x-.12,y-.30,1.24),(x+.12,y-.30,1.24)],.008,edge)
  for yy in [y-.48,y-.27]:
   cube('Instrument case',(x,yy,1.23),(.30,.18,.18),teal,.025);cube('Instrument display',(x+.151,yy,1.24),(.003,.11,.08),g['screen'],.002)
 else:
  # Wall mounted telemetry board / office notice board with individual frames and readable bars.
  x=left+1.2;z=2.0
  cube('Telemetry frame',(x,back-.13,z),(1.55,.09,.85),dark,.025);cube('Telemetry glass',(x,back-.181,z),(1.44,.012,.74),teal,.006)
  text('Telemetry title','NEURON / SYSTEMS',(x,back-.191,z+.23),.07,white)
  for j in range(6):cube('Telemetry graph',(x-.53+j*.21,back-.191,z-.10),(.13,.003,.07+.06*(j%3)),g['screen'],.003)
  for j in range(3):cube('Telemetry text',(x+.25,back-.192,z+.10-j*.055),(.39-j*.06,.003,.009),white)
 # Structural wall panel reveals and fine tile joints (actual geometry, not flat texture tricks).
 for x in [left+.8+i*.6 for i in range(max(1,int((w-1.6)/.6)))]:cube('Back wall panel reveal',(x,back-.093,1.6),(.006,.005,3.0),dark)
 for x in [left+.6+i*.6 for i in range(max(1,int((w-1.2)/.6)))]:cube('Floor tile joint',(x,0,.020),(.005,d-.4,.002),dark)
 # Enclosure is provided as a separate modular export. Cutaway preview remains unobstructed.
 enclosure=[]
 for o in bpy.context.scene.objects:
  if o.type=='MESH' and o.name.startswith(('Suspended LED','Prismatic diffuser','Luminaire suspension','Diffuser prism')):o['enclosure']=True;o['skip_collision']=True;enclosure.append(o)
 def enc(o):o['enclosure']=True;enclosure.append(o);return o
 enc(cube('Enclosure_Left_Upper',(left,0,2.27),(.18,d,2.06),g['walls'],.008))
 enc(cube('Enclosure_Right_Upper',(-left,0,1.92),(.18,d,2.76),g['walls'],.008))
 # 1.6 m wide, 2.4 m high front doorway; the collision follows this opening.
 for sign in [-1,1]:enc(cube('Enclosure_Front_Pier',(sign*(w/4+.40),-d/2,1.65),(w/2-.80,.18,3.3),g['walls'],.008))
 enc(cube('Enclosure_Door_Lintel',(0,-d/2,2.85),(1.6,.18,.9),g['walls'],.008))
 enc(cube('Enclosure_Ceiling',(0,0,3.37),(w,d,.14),g['walls'],.008))
 return {'changes':['Source floor plan and equipment bay grid retained','Furniture hardware: individual keys, monitor vents and leads, chair arms, casters and stitching','Electrical sockets, extinguisher fittings, utility bench or telemetry board','Suspended lights, wall reveals and floor joints','Separate enclosure FBX: full-height walls, ceiling and open entrance'],'interior_dimensions':[w,d,3.3],'enclosure_note':'Cutaway mesh is the authoring preview. Add the separate enclosure FBX for a closed room. No collision generated for furniture.'}

def detailed_plants(positions,leaf):
 soil=mat('Planter soil',(.023,.016,.009),0,.95);pot=next((m for m in bpy.data.materials if 'ivory' in m.name),None)
 built=[]
 for x,y,s in positions:
  before=set(bpy.context.scene.objects)
  cylinder('Planter soil',(x,y,.461*s),.224*s,.014*s,soil,64)
  if pot:torus('Planter rim',(x,y,.46*s),.245*s,.012*s,pot)
  for k in range(11):
   a=k*2.4;radius=(.12+(k%3)*.04)*s;z=(.68+(k%4)*.09)*s;start=Vector((x+radius*math.cos(a),y+radius*math.sin(a),z));end=start+Vector((math.cos(a)*.32*s,math.sin(a)*.32*s,(.17 if k%2 else .31)*s));line('Leaf stem',[(x,y,.45*s),(x+radius*.5*math.cos(a),y+radius*.5*math.sin(a),z*.88),start],.005*s,leaf)
   across=Vector((-math.sin(a),math.cos(a),0));verts=[];faces=[]
   for i in range(21):
    t=i/20;centre=start.lerp(end,t);centre.z+=math.sin(t*math.pi)*.095*s;width=math.sin(t*math.pi)**.8*.070*s
    for j in range(7):
     u=(j-3)/3;p=centre+across*(width*u);p.z-=abs(u)*width*.24;verts.append(tuple(p))
   for i in range(20):
    for j in range(6):a0=i*7+j;faces.append((a0,a0+1,a0+8,a0+7))
   me=bpy.data.meshes.new('Curved leaf blade');me.from_pydata(verts,[],faces);me.materials.append(leaf);ob=bpy.data.objects.new('Curved leaf blade',me);bpy.context.collection.objects.link(ob)
   for p in me.polygons:p.use_smooth=True
   line('Leaf midrib',[tuple(start.lerp(end,t)+Vector((0,0,math.sin(t*math.pi)*.095*s+.002*s))) for t in [i/20 for i in range(21)]],.0012*s,leaf)
  built.extend([o for o in bpy.context.scene.objects if o not in before and o.type=='MESH'])
 return built
