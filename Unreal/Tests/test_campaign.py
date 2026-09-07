"""Source/runner checks only; successful tests here do NOT verify UE compilation or physics."""
import ast
import hashlib
import json
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'Unreal/MakeYourAI/Source/MakeYourAI'
TOOLS = ROOT / 'Unreal/Tools'
sys.path.insert(0, str(TOOLS))
import scaffold_runner

class CampaignSourceTests(unittest.TestCase):
    def test_published_sources_match_locally_tested_bytes(self):
        manifest = json.loads((ROOT / 'Unreal/Evidence/campaign/source-hashes.json').read_text())
        self.assertGreaterEqual(len(manifest['git_blob_sha1']), 35)
        for name, expected in manifest['git_blob_sha1'].items():
            content = (ROOT / name).read_bytes()
            actual = hashlib.sha1(b'blob ' + str(len(content)).encode() + b'\0' + content).hexdigest()
            self.assertEqual(actual, expected, name)

    def test_editor_bootstrap_is_syntax_valid_and_uses_real_editor_saves(self):
        text = (TOOLS / 'build_campaign.py').read_text()
        tree = ast.parse(text)
        self.assertIn('levels.save_current_level()', text)
        self.assertIn('Existing asset will not be overwritten', text)
        for node in ast.walk(tree):
            if isinstance(node, ast.Call) and isinstance(node.func, ast.Attribute):
                self.assertNotEqual(node.func.attr, 'write_bytes')

    def test_missing_engine_build_campaign_fails_closed(self):
        with tempfile.TemporaryDirectory() as folder, patch.dict(os.environ, {}, clear=True):
            out = Path(folder) / 'evidence'
            code = scaffold_runner.main(['build-campaign', '--repo-root', str(ROOT), '--output', str(out)])
            result = json.loads((out / 'result.json').read_text())
            self.assertEqual(code, 2)
            self.assertIsNone(result['process_exit_code'])
            self.assertFalse(result['visual_verified'])
            self.assertFalse(result['gameplay_verified'])

    def test_automation_inventory_includes_engine_save_and_real_movement(self):
        expected = scaffold_runner.expected_tests(ROOT)
        self.assertIn('MakeYourAI.Campaign.FullFlowRealSaveGame', expected)
        self.assertIn('MakeYourAI.Campaign.GarageSweptMovementAndInteraction', expected)
        self.assertIn('MakeYourAI.Campaign.EditableProfilesMatchDomain', expected)

    def test_loading_and_company_share_persistent_owner(self):
        h = (SOURCE / 'Public/Campaign/MaiLoadingSubsystem.h').read_text()
        cpp = (SOURCE / 'Private/Campaign/MaiLoadingSubsystem.cpp').read_text()
        company = (SOURCE / 'Private/Gameplay/MaiCompanySubsystem.cpp').read_text()
        self.assertIn('UGameInstanceSubsystem', h)
        self.assertIn('RequestAsyncLoad', cpp)
        self.assertIn('LoadLevelInstance', cpp)
        self.assertIn('OnTravelFailure', cpp)
        self.assertIn('Campaign unavailable', company)
        self.assertIn('Game->AdvanceReal', company)
        self.assertNotIn('Sim->AdvanceReal', company)

    def test_umg_controls_use_domain_actions_not_visual_only_values(self):
        actions = (SOURCE / 'Private/UI/MaiFlowActions.cpp').read_text()
        for name in ('ChooseDifficulty', 'PrologueAction', 'StartReview', 'ChooseReview', 'StartTraining', 'ChooseEnding'):
            self.assertIn(name, actions)
        ui = (SOURCE / 'Private/UI/MaiFlowWidget.cpp').read_text()
        self.assertIn('configuration failed', ui)
        self.assertIn('CanReturnToSave', ui)
        self.assertIn('No photographic file', ui)

    def test_walk_input_and_interaction_are_actual_engine_components(self):
        walk = (SOURCE / 'Private/World/MaiWalkCharacter.cpp').read_text()
        point = (SOURCE / 'Private/Interaction/MaiInteriorPoint.cpp').read_text()
        self.assertIn('AddMovementInput', walk)
        self.assertIn('BindAxis', walk)
        self.assertIn('LineTraceSingleByChannel', point)
        self.assertIn('CanInteract(Character)', point)

if __name__ == '__main__':
    unittest.main()
