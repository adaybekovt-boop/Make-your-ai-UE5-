"""Run in the compiled UE Editor, never ordinary Python. Creates real content.

Required environment: MAI_CITY_MANIFEST, MAI_CONTENT_REPORT. Optional
MAI_CITY_NANITE=1 (baseline is off). Authored content is never overwritten.
A source/code/engine revision gets its own immutable generated package namespace.
"""
from __future__ import annotations
import hashlib
import json
import os
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from cityv4_format import sha256
from cityv4_plan import make_plan
import unreal

OWNER = 'MakeYourAI.CityV4.v2'
EL = unreal.EditorAssetLibrary
ML = unreal.MaterialEditingLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()


def atomic_json(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + '.partial')
    temporary.write_text(json.dumps(data, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    temporary.replace(path)


def package_file(package):
    return Path(unreal.Paths.project_content_dir()) / (package.removeprefix('/Game/') + '.uasset')


def make_material(parent_path, source):
    material = TOOLS.create_asset(parent_path.rsplit('/', 1)[1], parent_path.rsplit('/', 1)[0], unreal.Material, unreal.MaterialFactoryNew())
    if material is None:
        raise RuntimeError('Material creation failed: ' + parent_path)
    def node(cls, **values):
        expression = ML.create_material_expression(material, cls)
        if not expression:
            raise RuntimeError('Material expression not created')
        for key, value in values.items():
            expression.set_editor_property(key, value)
        return expression
    def connect(a, output, b, input_name):
        if not ML.connect_material_expressions(a, output, b, input_name):
            raise RuntimeError('Material graph connection failed: ' + input_name)
    def finish(expression, output, prop):
        if not ML.connect_material_property(expression, output, prop):
            raise RuntimeError('Material output was not connected')
    links = source.get('links', [])
    by_name = {n['name']: n for n in source['nodes']}
    def vertex_driven(socket):
        relevant = [l for l in links if l['to_socket'] == socket and by_name.get(l['to_node'], {}).get('type') == 'ShaderNodeBsdfPrincipled']
        for link in relevant:
            if by_name.get(link['from_node'], {}).get('type') != 'ShaderNodeVertexColor':
                raise RuntimeError('Unsupported authored color graph: ' + source['name'])
        return bool(relevant)
    base = node(unreal.MaterialExpressionVectorParameter, parameter_name='BaseColor', default_value=unreal.LinearColor(*source['base_color']))
    vertex = node(unreal.MaterialExpressionVertexColor)
    color = vertex if vertex_driven('Base Color') else base
    if any(word in source['name'].lower() for word in ('stone','brick','limestone','slate','terracotta','asphalt')):
        grain = node(unreal.MaterialExpressionNoise, scale=.025, quality=1, levels=1, output_min=.94, output_max=1.02)
        detail = node(unreal.MaterialExpressionMultiply)
        connect(color,'',detail,'A');connect(grain,'',detail,'B');color=detail
    finish(color, '', unreal.MaterialProperty.MP_BASE_COLOR)
    # Art-direction pass: subdued dielectric highlights for a readable city view.
    specular = node(unreal.MaterialExpressionScalarParameter, parameter_name='Specular', default_value=.18)
    finish(specular, '', unreal.MaterialProperty.MP_SPECULAR)
    for name, key, prop in [('Roughness','roughness',unreal.MaterialProperty.MP_ROUGHNESS), ('Metallic','metallic',unreal.MaterialProperty.MP_METALLIC)]:
        value = node(unreal.MaterialExpressionScalarParameter, parameter_name=name, default_value=float(source[key]))
        finish(value, '', prop)
    emission = node(unreal.MaterialExpressionVectorParameter, parameter_name='EmissionColor', default_value=unreal.LinearColor(*source['emission']))
    strength = node(unreal.MaterialExpressionScalarParameter, parameter_name='EmissionStrength', default_value=float(source['emission_strength']))
    multiply = node(unreal.MaterialExpressionMultiply)
    connect(vertex if vertex_driven('Emission Color') else emission, '', multiply, 'A')
    connect(strength, '', multiply, 'B'); finish(multiply, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    if float(source['alpha']) < .999:
        # Do not silently turn opaque authored glass into transparency.
        material.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
        alpha = node(unreal.MaterialExpressionScalarParameter, parameter_name='Opacity', default_value=float(source['alpha']))
        finish(alpha, '', unreal.MaterialProperty.MP_OPACITY)
    # Source Noise->Bump nodes are not numerically identical to UE's procedural
    # noise. Preserve their graph/value record and record this as a visual gap.
    # Baseline uses authored split normals, NOT fabricated baked textures.
    for usage in (unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES, unreal.MaterialUsage.MATUSAGE_NANITE):
        ML.set_material_usage(material, usage)
        if not ML.has_material_usage(material, usage):
            raise RuntimeError('Required material usage flag missing')
    ML.layout_material_expressions(material)
    errors = ML.recompile_material(material)
    if errors:
        raise RuntimeError('Material compiler reported: ' + str(errors))
    EL.set_metadata_tag(material, 'MAI_GeneratedOwner', OWNER)
    if not EL.save_loaded_asset(material):
        raise RuntimeError('Material save failed')
    return material


def build():
    report_file = Path(os.environ['MAI_CONTENT_REPORT']).resolve()
    manifest = Path(os.environ['MAI_CITY_MANIFEST']).resolve()
    data = json.loads(manifest.read_text(encoding='utf-8'))
    if data.get('format') != 'MAIMSH02':
        raise RuntimeError('Re-export v2: v1 omitted authored vertex colors')
    # 60 m groups retain HISM per-instance culling while avoiding thousands of
    # tiny components. Measured CityV4: 4,563 -> 1,695 groups, no instance loss.
    plan = make_plan(manifest, cell_cm=6000)
    nanite = os.environ.get('MAI_CITY_NANITE') == '1'
    engine = unreal.SystemLibrary.get_engine_version()
    import_source = Path(__file__).resolve().parents[1] / 'MakeYourAI/Source/MakeYourAIEditor/Private/MaiCityImportLibrary.cpp'
    batch_source = Path(__file__).resolve().parents[1] / 'MakeYourAI/Source/MakeYourAI/Private/World/MaiCityBatch.cpp'
    fingerprint = hashlib.sha256((sha256(manifest)+sha256(Path(__file__))+sha256(import_source)+sha256(batch_source)+sha256(Path(__file__).with_name('cityv4_plan.py'))+engine+str(nanite)).encode()).hexdigest()[:20]
    root = '/Game/Generated/CityV4/R_' + fingerprint
    world_package = root + '/L_City_DAY'
    map_file = Path(unreal.Paths.project_content_dir()) / (world_package.removeprefix('/Game/') + '.umap')
    receipt_file = Path(unreal.Paths.project_saved_dir()) / 'CityReceipts' / (fingerprint + '.json')
    receipt = dict(owner=OWNER, revision=fingerprint, sourceSHA256=data['source_sha256'], assets={}, complete=False)
    if receipt_file.is_file():
        receipt = json.loads(receipt_file.read_text(encoding='utf-8'))
        if receipt.get('owner') != OWNER:
            raise RuntimeError('Unrecognized content receipt')
        for name, expected in receipt['assets'].items():
            p = Path(unreal.Paths.project_content_dir()) / name
            if not p.is_file() or sha256(p) != expected:
                raise RuntimeError('Authored edits or missing generated file preserved: ' + name)
    def record(path):
        p = Path(path).resolve()
        receipt['assets'][str(p.relative_to(Path(unreal.Paths.project_content_dir()).resolve())).replace('\\','/')] = sha256(p)
        atomic_json(receipt_file, receipt)
    def known_asset(path):
        p = package_file(path)
        if p.exists():
            name = str(p.relative_to(Path(unreal.Paths.project_content_dir()))).replace('\\','/')
            if receipt['assets'].get(name) != sha256(p):
                raise RuntimeError('Refusing to replace unrecorded/edited package: ' + path)
            return unreal.load_asset(path)
        return None
    report = dict(owner=OWNER, revision=fingerprint, engine=engine, imported=False, renderVerified=False,
                  sourceSHA256=data['source_sha256'], manifestSHA256=sha256(manifest), map=world_package,
                  instances=plan['instance_count'], trianglesWithInstances=plan['triangles_with_instances'],
                  batches=len(plan['batches']), naniteRequested=nanite, meshAudits=[], materialGaps=[], reused=False)
    if not receipt['complete']:
        materials = {}
        for name, source in data['materials'].items():
            key = hashlib.sha256(json.dumps(source, sort_keys=True).encode()).hexdigest()[:20]
            parent_path = root+'/Materials/M_'+key
            parent = known_asset(parent_path)
            if parent is None:
                parent = make_material(parent_path, source); record(package_file(parent_path))
            instance_path = root+'/Materials/MI_'+key
            instance = known_asset(instance_path)
            if instance is None:
                instance = TOOLS.create_asset('MI_'+key,root+'/Materials',unreal.MaterialInstanceConstant,unreal.MaterialInstanceConstantFactoryNew())
                if instance is None: raise RuntimeError('Material instance creation failed')
                ML.set_material_instance_parent(instance,parent)
                for param, field in [('BaseColor','base_color'),('EmissionColor','emission')]:
                    ML.set_material_instance_vector_parameter_value(instance,param,unreal.LinearColor(*source[field]))
                for param, field in [('Roughness','roughness'),('Metallic','metallic'),('EmissionStrength','emission_strength')]:
                    value=float(source[field])
                    if param=='Roughness':
                        value=max(value,.32 if any(word in name.lower() for word in ('glass','window','river','water')) else .58)
                    ML.set_material_instance_scalar_parameter_value(instance,param,value)
                ML.update_material_instance(instance)
                if not EL.save_loaded_asset(instance):raise RuntimeError('Material instance save failed')
                record(package_file(instance_path))
            materials[name] = instance
            if any(n['type']=='ShaderNodeBump' for n in source['nodes']):
                report['materialGaps'].append(dict(material=name, gap='Procedural Blender bump retained in manifest; UE baseline uses source split normals, not equivalent micro-normal shader'))
        meshes = {}
        for key, item in data['meshes'].items():
            path = root+'/Geometry/SM_'+key; known_asset(path)
            geometry = manifest.parent/item['file']
            result = json.loads(unreal.MaiCityImportLibrary.import_city_mesh(str(geometry),path,hashlib.sha1(geometry.read_bytes()).hexdigest(),nanite))
            if not result.get('ok') or result['sourceLOD0Triangles']!=item['triangles'] or result['renderLOD0Triangles']!=item['triangles']-result.get('collapsedSourceTriangles',0):
                raise RuntimeError('Source/imported LOD0 mismatch: '+json.dumps(result))
            report['meshAudits'].append(result); meshes[key]=unreal.load_asset(path);record(package_file(path))
        level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
        actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
        if map_file.exists():
            # Incomplete map is only rebuilt when its previous bytes are in the
            # receipt, so repeated execution cannot destroy user edits.
            relative = str(map_file.relative_to(Path(unreal.Paths.project_content_dir()))).replace('\\','/')
            if receipt['assets'].get(relative)!=sha256(map_file):raise RuntimeError('Existing map is not owned by this import')
            if not level.load_level(world_package):raise RuntimeError('Cannot resume owned map')
            for actor in actors.get_all_level_actors():
                if 'MAI_GeneratedCity' in [str(t) for t in actor.tags]:actors.destroy_actor(actor)
        elif not level.new_level(world_package):raise RuntimeError('Cannot create generated CityV4 map')
        record(map_file) # Initial empty editor-created map is an intermediate, never a PASS.
        for group in plan['batches']:
            actor = actors.spawn_actor_from_class(unreal.MaiCityBatch,unreal.Vector())
            if actor is None:raise RuntimeError('Batch actor spawn failed')
            actor.tags=['MAI_GeneratedCity','MAI_'+group['category']]
            actor.set_actor_label(group['category']+' / '+group['id'])
            location = '' if group['game_id'] in ('dc-north','dc-south') else group['game_id']
            if not actor.configure(meshes[group['mesh']],[materials[n] for n in group['materials']],json.dumps(group['matrices']),location):
                raise RuntimeError('Instance transform/material/count failure: '+group['id'])
        # Dedicated exurban campuses replace semantic pins inside residential
        # source buildings. Source scenery remains intact; additions are explicit.
        cube=unreal.load_asset('/Engine/BasicShapes/Cube')
        if cube is None:raise RuntimeError('Engine cube required for campus construction')
        extra_instances=0
        for identity,cx,cy in [('dc-north',-23000,-34000),('dc-south',23000,12000)]:
            pieces=[]
            def box(x,y,z,sx,sy,sz,material):pieces.append((x,y,z,sx,sy,sz,material))
            box(cx,cy,0,3400,2600,160,'Land / warm earth')
            box(cx,cy,90,3000,2200,30,'Asphalt')
            for row in (-1,1):
                box(cx,cy+row*550,310,1900,650,400,'Facade / ivory stone')
                box(cx,cy+row*550,520,1950,700,35,'Roof / slate')
                for col in range(-3,4):
                    box(cx+col*230,cy+row*550,590,130,210,110,'Facade / silver mullions')
                    box(cx+col*230,cy+row*550,652,110,170,16,'Facade / blue glass')
            box(cx-1250,cy,260,300,450,300,'Facade / brick')
            for col in range(6):box(cx-900+col*320,cy,118,180,12,8,'Lane paint')
            for side in (-1,1):
                box(cx+side*1530,cy,180,15,2250,180,'Facade / silver mullions')
                box(cx,cy+side*1130,180,3050,15,180,'Facade / silver mullions')
            for index,(x,y,z,sx,sy,sz,material) in enumerate(pieces):
                actor=actors.spawn_actor_from_class(unreal.MaiCityBatch,unreal.Vector())
                actor.tags=['MAI_GeneratedCity','MAI_infrastructure','MAI_ExurbanCampus']
                actor.set_actor_label(identity+' / '+str(index))
                matrix=[sx/100,0,0,x,0,sy/100,0,y,0,0,sz/100,z,0,0,0,1]
                if not actor.configure(cube,[materials[material]],json.dumps([matrix]),identity if index==0 else ''):raise RuntimeError('Campus construction failed')
                extra_instances+=1
        report['exurbanCampusInstances']=extra_instances
        camera = plan['camera']
        rotation = unreal.MathLibrary.make_rot_from_xz(unreal.Vector(*camera['forward']),unreal.Vector(*camera['up']))
        view = actors.spawn_actor_from_class(unreal.CameraActor,unreal.Vector(*camera['position_cm']),rotation)
        view.tags=['MAI_GeneratedCity','MAI_CityCamera'];view.set_actor_label('CityV4 / '+camera['name'])
        cc=view.get_component_by_class(unreal.CameraComponent);cc.set_field_of_view(camera['fov_x_degrees']);cc.set_aspect_ratio(2200/1200)
        cc.set_editor_property('constrain_aspect_ratio',False)
        if camera['type']=='ORTHO':cc.set_projection_mode(unreal.CameraProjectionMode.ORTHOGRAPHIC);cc.set_ortho_width(camera['ortho_width_cm'])
        sun=actors.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,30000));sun.tags=['MAI_GeneratedCity','MAI_CitySun']
        sky=actors.spawn_actor_from_class(unreal.SkyLight,unreal.Vector(0,0,1000));sky.tags=['MAI_GeneratedCity','MAI_CitySky']
        sun.get_component_by_class(unreal.DirectionalLightComponent).set_mobility(unreal.ComponentMobility.MOVABLE)
        sun.get_component_by_class(unreal.DirectionalLightComponent).set_editor_property('atmosphere_sun_light',True)
        sky.get_component_by_class(unreal.SkyLightComponent).set_mobility(unreal.ComponentMobility.MOVABLE)
        sky.get_component_by_class(unreal.SkyLightComponent).set_editor_property('real_time_capture',True)
        atmosphere=actors.spawn_actor_from_class(unreal.SkyAtmosphere,unreal.Vector());atmosphere.tags=['MAI_GeneratedCity']
        atmosphere.get_component_by_class(unreal.SkyAtmosphereComponent).set_editor_property('aerial_pespective_view_distance_scale',.15)
        lighting=actors.spawn_actor_from_class(unreal.MaiCityLighting,unreal.Vector());lighting.tags=['MAI_GeneratedCity'];lighting.sun=sun;lighting.sky=sky;lighting.apply_preview(False)
        # Exposure is a baseline, not a measured visual match. Auto exposure avoids
        # a fixed daytime EV blacking out the night preview.
        exposure=actors.spawn_actor_from_class(unreal.PostProcessVolume,unreal.Vector());exposure.tags=['MAI_GeneratedCity'];exposure.set_editor_property('unbound',True)
        if not level.save_current_level():raise RuntimeError('City map save failed')
        record(map_file)
        actual = [a for a in actors.get_all_level_actors() if isinstance(a,unreal.MaiCityBatch)]
        if sum(a.instances.get_instance_count() for a in actual)!=plan['instance_count']+extra_instances:raise RuntimeError('World lost instances')
        if sum(isinstance(a,unreal.DirectionalLight) for a in actors.get_all_level_actors())!=1 or sum(isinstance(a,unreal.SkyLight) for a in actors.get_all_level_actors())!=1:
            raise RuntimeError('Lighting duplicated')
        receipt['complete']=True;receipt['report']=report;atomic_json(receipt_file,receipt)
    else:
        report=receipt['report'];report['reused']=True
    descriptor_path=Path(unreal.Paths.project_content_dir())/'Rules/city-content.json'
    if descriptor_path.exists():
        old=json.loads(descriptor_path.read_text(encoding='utf-8'))
        old_receipt=receipt_file.parent/(str(old.get('revision',''))+'.json')
        if not old_receipt.is_file() or json.loads(old_receipt.read_text(encoding='utf-8')).get('descriptorSHA256')!=sha256(descriptor_path):
            raise RuntimeError('City descriptor was edited outside this pipeline; preserving it')
    descriptor=dict(owner=OWNER,revision=fingerprint,cityMap=world_package,sourceSHA256=data['source_sha256'])
    atomic_json(descriptor_path,descriptor);receipt['descriptorSHA256']=sha256(descriptor_path);atomic_json(receipt_file,receipt)
    report['imported']=True;report['renderVerified']=False;atomic_json(report_file,report)
    unreal.log('MAI_CITY_CONTENT_COMPLETE '+str(report_file))


if __name__=='__main__':
    try:build()
    except Exception as error:
        if os.environ.get('MAI_CONTENT_REPORT'):
            atomic_json(Path(os.environ['MAI_CONTENT_REPORT']),dict(imported=False,renderVerified=False,error=str(error)))
        unreal.log_error(str(error));raise
