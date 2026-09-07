"""Compile and execute extra native arithmetic boundary checks, without Unreal stubs."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

REPO = Path(__file__).resolve().parents[2]


@unittest.skipUnless(shutil.which('g++'), 'Native g++ is required for boundary checks')
class NativeBounds(unittest.TestCase):
    def test_extreme_tunables_and_npc_duration_are_rejected(self):
        code = r'''
#include "Core/MaiDomain.h"
#include <limits>
#include <iostream>
int main() {
    auto catalog = mai::Catalog::Defaults();
    std::string error;
    if (!catalog.Valid(error)) return 1;
    catalog.chips[3].computeMilli = 1000000;
    catalog.chassis[2].computeBps = 20000;
    catalog.locations[0].rows = 10;
    catalog.locations[0].cols = 10;
    catalog.locations[0].powerWatts = 1000000;
    if (catalog.Valid(error)) return 2;
    if (error.find("arithmetic budget") == std::string::npos) return 3;
    mai::NpcState npc;
    const auto limit = std::numeric_limits<mai::Tick>::max();
    if (mai::UpdateProximity(npc, true, 0, limit, limit)) return 4;
    if (npc.eventCount != 0) return 5;
    if (!mai::UpdateProximity(npc, true, 0, 10, 40)) return 6;
    std::cout << "NATIVE_BOUNDS_PASS catalog_overflow_guard npc_duration_guard\n";
    return 0;
}
'''
        with tempfile.TemporaryDirectory() as directory:
            folder = Path(directory)
            cpp = folder / 'bounds.cpp'
            binary = folder / 'bounds'
            cpp.write_text(code)
            command = ['g++', '-std=c++17', '-fsanitize=undefined', '-fno-sanitize-recover=all',
                       '-I', str(REPO / 'Unreal/MakeYourAI/Source/MakeYourAI/Public'),
                       str(REPO / 'Unreal/MakeYourAI/Source/MakeYourAI/Private/Core/MaiCatalog.cpp'),
                       str(cpp), '-o', str(binary)]
            compiled = subprocess.run(command, capture_output=True, text=True, timeout=60)
            self.assertEqual(compiled.returncode, 0, compiled.stderr)
            ran = subprocess.run([str(binary)], capture_output=True, text=True, timeout=10)
            self.assertEqual(ran.returncode, 0, ran.stdout + ran.stderr)
            self.assertIn('NATIVE_BOUNDS_PASS', ran.stdout)
            print(ran.stdout.strip())


if __name__ == '__main__':
    unittest.main()
