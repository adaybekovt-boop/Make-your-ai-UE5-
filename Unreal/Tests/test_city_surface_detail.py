from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'Tools'))
from city_surface_detail import surface_profile, surface_code, NORMAL_CODE


class SurfaceDetailTests(unittest.TestCase):
    def test_only_solid_building_surfaces_receive_masonry(self):
        for name in ('Architectural glass', 'Window illumination', 'Foliage', 'River water'):
            self.assertIsNone(surface_profile(name))

    def test_profiles_have_centimetre_scale_and_bounded_relief(self):
        for name in ('brick', 'limestone', 'slate', 'asphalt'):
            profile = surface_profile(name)
            self.assertIsNotNone(profile)
            self.assertGreaterEqual(profile[3], 0)
            self.assertLessEqual(profile[3], .35)

    def test_detail_is_filtered_and_not_animated(self):
        code = surface_code(surface_profile('Facade / brick'))
        self.assertIn('fwidth(grid)', code)
        self.assertIn('smoothstep(1600.0,5000.0', code)
        self.assertNotIn('Time', code)
        self.assertIn('max(abs(det),0.00001)', NORMAL_CODE)


if __name__ == '__main__':
    unittest.main()
