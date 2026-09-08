"""Preparation/source checks only. These are NOT UHT, UBT, Editor or rendering tests."""
from __future__ import annotations
import ast
import importlib.util
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

REPO = Path(__file__).resolve().parents[2]
PROJECT = REPO / 'Unreal/MakeYourAI'
TOOLS = REPO / 'Unreal/Tools'
sys.path.insert(0, str(TOOLS))
import scaffold_runner
import extract_city_markers


class ScaffoldChecks(unittest.TestCase):
    def test_editor_and_blender_scripts_have_valid_python_syntax(self):
        for name in ('build_scaffold.py', 'editor_assets.py', 'editor_scene.py', 'inspect_scaffold.py', 'prepare_skeletal_lods.py', 'extract_city_markers.py', 'scaffold_runner.py', 'editor_materials.py', 'editor_lighting.py', 'editor_world_partition.py', 'run_editor_visual.py'):
            with self.subTest(script=name):
                ast.parse((TOOLS / name).read_text(encoding='utf-8'), filename=name)

    def test_real_runtime_module_and_editor_only_plugins(self):
        descriptor = json.loads((PROJECT / 'MakeYourAI.uproject').read_text())
        self.assertEqual(descriptor['Modules'], [
            {'Name': 'MakeYourAI', 'Type': 'Runtime', 'LoadingPhase': 'Default'},
            {'Name': 'MakeYourAIEditor', 'Type': 'Editor', 'LoadingPhase': 'Default'},
        ])
        runtime_rules = (PROJECT / 'Source/MakeYourAI/MakeYourAI.Build.cs').read_text()
        self.assertNotIn('UnrealEd', runtime_rules)
        self.assertNotIn('AssetTools', runtime_rules)
        self.assertIn('QuickJS', runtime_rules)
        editor_target = (PROJECT / 'Source/MakeYourAIEditor.Target.cs').read_text()
        self.assertIn('MakeYourAIEditor', editor_target)
        for plugin in descriptor['Plugins']:
            self.assertEqual(plugin['TargetAllowList'], ['Editor'])

    def test_native_game_instance_and_mode_are_registered(self):
        ini = (PROJECT / 'Config/DefaultEngine.ini').read_text()
        self.assertIn('GameInstanceClass=/Script/MakeYourAI.MaiGameInstance', ini)
        self.assertIn('GlobalDefaultGameMode=/Script/MakeYourAI.MaiGameMode', ini)
        self.assertNotIn('GameDefaultMap=/Game/Scaffold', ini)

    def test_generated_headers_are_last_include(self):
        headers = list((PROJECT / 'Source/MakeYourAI/Public').rglob('*.h'))
        self.assertGreater(len(headers), 15)
        for path in headers:
            includes = re.findall(r'^#include\s+[<"]([^">]+)', path.read_text(), re.MULTILINE)
            generated = [i for i in includes if i.endswith('.generated.h')]
            if generated:
                self.assertEqual(includes[-1], generated[0], str(path))

    def test_required_runtime_areas_contain_cpp_implementation(self):
        source = PROJECT / 'Source/MakeYourAI/Private'
        for area in ('Core', 'Gameplay', 'Economy', 'Persistence', 'World', 'UI', 'NPC', 'Campaign'):
            self.assertTrue(list((source / area).glob('*.cpp')), area)
        self.assertTrue((PROJECT / 'Source/MakeYourAI/Public/Interaction/MaiInteractable.h').is_file())

    def test_editor_builder_never_writes_fake_asset_bytes(self):
        for name in ('build_scaffold.py', 'editor_assets.py', 'editor_scene.py', 'inspect_scaffold.py', 'editor_materials.py', 'editor_lighting.py', 'editor_world_partition.py', 'run_editor_visual.py'):
            tree = ast.parse((TOOLS / name).read_text())
            for node in ast.walk(tree):
                if isinstance(node, ast.Call) and isinstance(node.func, ast.Attribute):
                    self.assertNotEqual(node.func.attr, 'write_bytes', name)
        self.assertIn('levels.save_current_level()', (TOOLS / 'editor_scene.py').read_text())

    def test_no_original_blend_save_in_marker_or_lod_scripts(self):
        for name in ('extract_city_markers.py', 'prepare_skeletal_lods.py'):
            text = (TOOLS / name).read_text()
            self.assertNotIn('save_as_mainfile', text)
            self.assertNotIn('save_mainfile', text)
            self.assertIn('Saved/ScaffoldSource', text)

    def test_source_marker_normalization(self):
        self.assertEqual(extract_city_markers.normalized('Location_Server-Hall'), 'location_server_hall')
        self.assertEqual(len(extract_city_markers.REQUIRED), 8)
        self.assertIn('greenhaven', extract_city_markers.PREFIXES)

    def test_automation_expected_list_is_nonempty_and_source_derived(self):
        expected = scaffold_runner.expected_tests(REPO)
        self.assertGreaterEqual(len(expected), 4)
        self.assertIn('MakeYourAI.Persistence.RealUSaveGameRoundTrip', expected)

    def check_report(self, data, expected):
        with tempfile.TemporaryDirectory() as directory:
            file = Path(directory) / 'index.json'
            file.write_text(json.dumps(data))
            return scaffold_runner.check_automation(file, expected)

    def test_empty_automation_report_cannot_pass(self):
        self.assertFalse(self.check_report({'tests': []}, {'MakeYourAI.A'})['ok'])
        self.assertFalse(self.check_report({'tests': []}, set())['ok'])

    def test_automation_failure_and_not_run_are_not_success(self):
        for state in ('Fail', 'NotRun', 'InProcess', None):
            self.assertFalse(self.check_report({'tests': [{'fullTestPath': 'MakeYourAI.A', 'state': state}]}, {'MakeYourAI.A'})['ok'])

    def test_missing_automation_test_is_not_success(self):
        data = {'tests': [{'fullTestPath': 'MakeYourAI.A', 'state': 'Success'}]}
        self.assertFalse(self.check_report(data, {'MakeYourAI.A', 'MakeYourAI.B'})['ok'])

    def test_complete_automation_report_is_recognized(self):
        data = {'tests': [{'fullTestPath': 'MakeYourAI.A', 'state': 'Success'}]}
        self.assertTrue(self.check_report(data, {'MakeYourAI.A'})['ok'])

    def test_missing_engine_is_blocked_without_launch(self):
        with tempfile.TemporaryDirectory() as directory, patch.dict('os.environ', {}, clear=True):
            out = Path(directory) / 'receipt'
            code = scaffold_runner.main(['automation', '--repo-root', str(REPO), '--output', str(out)])
            result = json.loads((out / 'result.json').read_text())
            self.assertEqual(code, 2)
            self.assertIsNone(result['process_exit_code'])
            self.assertFalse(result['visual_verified'])
            self.assertNotIn('automation_verified', result)

    def test_generated_directories_remain_ignored(self):
        ignore = (PROJECT / '.gitignore').read_text()
        for name in ('Binaries', 'Intermediate', 'Saved', 'DerivedDataCache', '.vs'):
            self.assertIn(name, ignore)


if __name__ == '__main__':
    unittest.main()
