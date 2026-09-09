import math
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'Tools'))
from city_tree_detail import build_tree
from cityv4_format import audit_mesh, HEADER, VERTEX, CORNER, U32, write_owned


class TreeDetailTests(unittest.TestCase):
    def source(self, suffix):
        return dict(source_name='template tree.'+suffix, material_slots=23,
                    bounds_min_cm=[-48., -45., -71.], bounds_max_cm=[48., 45., 71.])

    colors = {6: (.06, .19, .045, 1.), 7: (.12, .23, .065, 1.), 8: (.15, .07, .025, 1.)}

    def test_valid_bounded_geometry_for_all_three_species(self):
        hashes = set()
        for suffix in ('001', '002', '004'):
            source = self.source(suffix)
            raw = build_tree(source, self.colors)
            self.assertEqual(raw, build_tree(source, self.colors))
            with tempfile.TemporaryDirectory() as directory:
                path = Path(directory) / 'near.maimesh'
                write_owned(path, raw)
                audit = audit_mesh(path)
            self.assertEqual(audit['degenerate_triangles'], 0)
            self.assertGreater(audit['triangles'], 2000)
            self.assertLess(audit['triangles'], 30000)
            self.assertEqual(audit['material_slots'], 23)
            self.assertEqual(audit['used_slots'], [6, 7, 8])
            for axis in range(3):
                self.assertAlmostEqual(audit['bounds_min_cm'][axis], source['bounds_min_cm'][axis], places=4)
                self.assertAlmostEqual(audit['bounds_max_cm'][axis], source['bounds_max_cm'][axis], places=4)
            hashes.add(audit['sha256'])
        self.assertEqual(len(hashes), 3)

    def test_each_geometric_edge_has_two_faces(self):
        raw = build_tree(self.source('001'), self.colors)
        _, vertices, triangles, _ = HEADER.unpack_from(raw)
        offset = HEADER.size+vertices*VERTEX.size
        edges = {}
        for _ in range(triangles):
            offset += U32.size
            ids = []
            for _ in range(3):
                ids.append(CORNER.unpack_from(raw, offset)[0])
                offset += CORNER.size
            for a, b in zip(ids, ids[1:]+ids[:1]):
                edge = tuple(sorted((a, b)))
                edges[edge] = edges.get(edge, 0)+1
        self.assertTrue(edges)
        self.assertTrue(all(count == 2 for count in edges.values()))

    def test_reject_changed_bindings_or_bounds(self):
        for replacement in ({'material_slots': 3}, {'source_name': 'house'}, {'bounds_max_cm': [0, 0, math.nan]}):
            with self.assertRaises(ValueError):
                build_tree(self.source('001') | replacement, self.colors)


if __name__ == '__main__':
    unittest.main()
