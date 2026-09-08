"""Synthetic binary-format tests, not Blender import or Unreal rendering tests."""
from pathlib import Path
import copy
import hashlib
import json
import math
import sys
import tempfile
import unittest
from types import SimpleNamespace

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'Tools'))
import cityv4_format as fmt
from cityv4_plan import make_plan
from export_cityv4_instances import category

class CityV4FormatTests(unittest.TestCase):
    def test_unused_water_palette_slot_does_not_classify_a_building_as_water(self):
        obj=SimpleNamespace(name='Residential building',data=SimpleNamespace(polygons=[SimpleNamespace(material_index=0)]))
        self.assertEqual(category(obj,['Facade / brick','River Solara / blue green']), 'architecture')
    def test_used_water_material_is_classified_as_water(self):
        obj=SimpleNamespace(name='Surface',data=SimpleNamespace(polygons=[SimpleNamespace(material_index=1)]))
        self.assertEqual(category(obj,['Facade / brick','River Solara / blue green']), 'water')
    def setUp(self):
        self.folder = tempfile.TemporaryDirectory()
        self.addCleanup(self.folder.cleanup)
        self.root = Path(self.folder.name)
        self.raw = bytearray(fmt.HEADER.pack(fmt.MAGIC, 3, 1, 1))
        for p in ((0,0,0),(100,0,0),(0,100,0)):
            self.raw.extend(fmt.VERTEX.pack(*p))
        self.raw.extend(fmt.U32.pack(0))
        for i, color in enumerate(((1,0,0,1),(0,1,0,1),(0,0,1,1))):
            self.raw.extend(fmt.CORNER.pack(i,0,0,1,0,0,*color))
        self.key = hashlib.sha256(self.raw).hexdigest()[:24]
        self.mesh = self.root/(self.key+'.maimesh')
        self.mesh.write_bytes(self.raw)
        self.data = {'schema':1,'coordinates':'UE_X_NEGY_Z_CM','materials':{'glass':{}},
            'meshes':{self.key:{'file':self.mesh.name,**fmt.audit_mesh(self.mesh)}},
            'instances':[], 'scenes':[{'name':'DAY','visible_mesh_objects':3,
                'evaluated_triangles_with_instances':3,'cameras':[{'name':'authored','active':True}]}]}
        for i,x in enumerate((0, 100, 5000)):
            self.data['instances'].append({'id':str(i),'mesh':self.key,'materials':['glass'],
                'scenes':['DAY'],'category':'architecture','game_id':'',
                'matrix_cm':[1,0,0,x,0,1,0,0,0,0,1,0,0,0,0,1]})
        self.manifest = self.root/'manifest.json'
    def save(self):
        self.manifest.write_text(json.dumps(self.data))
        return self.manifest
    def test_color_and_topology_round_trip(self):
        info=fmt.audit_mesh(self.mesh)
        self.assertEqual(info['triangles'],1);self.assertEqual(info['vertices'],3)
        self.assertEqual(info['degenerate_triangles'],0)
        start=fmt.HEADER.size+3*fmt.VERTEX.size+4
        self.assertEqual(fmt.CORNER.unpack_from(self.raw,start)[-4:],(1,0,0,1))
        self.assertTrue(info['vertex_colors'])
    def test_truncation_and_invalid_index_fail(self):
        self.mesh.write_bytes(self.raw[:-1])
        with self.assertRaises(ValueError):fmt.audit_mesh(self.mesh)
        raw=bytearray(self.raw);offset=fmt.HEADER.size+3*fmt.VERTEX.size+4
        raw[offset:offset+4]=fmt.U32.pack(4);self.mesh.write_bytes(raw)
        with self.assertRaisesRegex(ValueError,'vertex'):fmt.audit_mesh(self.mesh)
    def test_nonfinite_color_fails(self):
        raw=bytearray(self.raw);offset=fmt.HEADER.size+3*fmt.VERTEX.size+4
        raw[offset:offset+fmt.CORNER.size]=fmt.CORNER.pack(0,0,0,1,0,0,math.nan,0,0,1)
        self.mesh.write_bytes(raw)
        with self.assertRaisesRegex(ValueError,'Non-finite'):fmt.audit_mesh(self.mesh)
    def test_spatial_batches_preserve_instances_materials_camera(self):
        plan=make_plan(self.save())
        self.assertEqual(len(plan['batches']),2)
        self.assertEqual(plan['instance_count'],3)
        self.assertEqual(plan['triangles_with_instances'],3)
        self.assertEqual(plan['camera']['name'],'authored')
        self.assertEqual(sorted(i for g in plan['batches'] for i in g['ids']),['0','1','2'])
    def test_gameplay_target_is_not_merged_with_scenery(self):
        self.data['instances'][1]['game_id']='garage'
        plan=make_plan(self.save())
        self.assertEqual(len(plan['batches']),3)
        self.assertEqual(sum(len(g['ids']) for g in plan['batches'] if g['game_id']=='garage'),1)
    def test_missing_instance_and_unknown_material_fail(self):
        bad=copy.deepcopy(self.data);self.data['instances'].pop()
        with self.assertRaisesRegex(ValueError,'count'):fmt.validate_manifest(self.save())
        self.data=bad;self.data['instances'][0]['materials']=['not-real']
        with self.assertRaisesRegex(ValueError,'material'):fmt.validate_manifest(self.save())
    def test_shear_not_silently_dropped(self):
        self.data['instances'][0]['matrix_cm'][1]=.4
        with self.assertRaisesRegex(ValueError,'Shear'):fmt.validate_manifest(self.save())
    def test_duplicate_identity_and_geometry_path_escape_fail(self):
        self.data['instances'][1]['id']='0'
        with self.assertRaisesRegex(ValueError,'Duplicate'):fmt.validate_manifest(self.save())
        self.data['instances'][1]['id']='1';self.data['meshes'][self.key]['file']='../foreign.maimesh'
        with self.assertRaisesRegex(ValueError,'Unsafe'):fmt.validate_manifest(self.save())
    def test_owned_derivative_rerun_never_overwrites_different_bytes(self):
        path=self.root/'result.bin';fmt.write_owned(path,b'original');fmt.write_owned(path,b'original')
        with self.assertRaises(ValueError):fmt.write_owned(path,b'replaced')
        self.assertEqual(path.read_bytes(),b'original')

if __name__=='__main__':unittest.main()
