"""Tests of preparation tooling only. These are NOT Unreal Automation tests."""
import contextlib
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest

spec = importlib.util.spec_from_file_location("ue5", Path(__file__).resolve().parents[1] / "ue5.py")
ue5 = importlib.util.module_from_spec(spec)
spec.loader.exec_module(ue5)


class PreparationTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.project = self.root / "MakeYourAI.uproject"
        self.project.write_text('{"FileVersion":3,"EngineAssociation":""}')

    def tearDown(self):
        self.temp.cleanup()

    def fixture_engine(self, major=5):
        # Synthetic file-presence fixture. Never run or label it as an engine.
        version = self.root / "Engine/Build/Build.version"
        version.parent.mkdir(parents=True, exist_ok=True)
        version.write_text(json.dumps({"MajorVersion": major, "MinorVersion": 0, "PatchVersion": 1}))
        for path in ue5.engine_paths(self.root, "Linux").values():
            path.parent.mkdir(parents=True, exist_ok=True)
            path.touch()
        return ue5.read_engine(self.root, "Linux")

    def test_missing_engine_rejected(self):
        with self.assertRaises(FileNotFoundError):
            ue5.read_engine(self.root, "Linux")

    def test_wrong_major_rejected(self):
        with self.assertRaises(ValueError):
            self.fixture_engine(4)

    def test_boolean_version_rejected(self):
        with self.assertRaises(ValueError):
            self.fixture_engine(True)

    def test_missing_tool_rejected(self):
        self.fixture_engine()
        ue5.engine_paths(self.root, "Linux")["ubt"].unlink()
        with self.assertRaises(ValueError):
            ue5.read_engine(self.root, "Linux")

    def test_pin_has_no_absolute_install_path(self):
        receipt = self.fixture_engine()
        ue5.pin_engine(self.project, receipt)
        ue5.require_pin(self.project, receipt)
        lock = (self.root / "UE5Engine.lock.json").read_text()
        self.assertNotIn(str(self.root), lock)
        self.assertEqual(json.loads(self.project.read_text())["EngineAssociation"], "5.0")

    def test_mismatched_pin_not_overwritten(self):
        receipt = self.fixture_engine()
        ue5.pin_engine(self.project, receipt)
        changed = {**receipt, "build_version_sha256": "different"}
        with self.assertRaises(ValueError):
            ue5.pin_engine(self.project, changed)
        with self.assertRaises(ValueError):
            ue5.require_pin(self.project, changed)
        ue5.require_pin(self.project, receipt)

    def test_unpinned_build_rejected(self):
        with self.assertRaises(ValueError):
            ue5.require_pin(self.project, self.fixture_engine())

    def test_asset_missing(self):
        self.assertFalse(ue5.inspect_asset(self.root / "missing.fbx", "x")["exists"])

    def test_lfs_pointer_not_fbx(self):
        asset = self.root / "pointer.fbx"
        asset.write_text("version https://git-lfs.github.com/spec/v1\noid sha256:abc\nsize 99\n")
        result = ue5.inspect_asset(asset, "x")
        self.assertTrue(result["lfs_pointer"])
        self.assertFalse(result["fbx_header_only"])
        self.assertFalse(result["import_verified"])

    def test_header_does_not_prove_import(self):
        asset = self.root / "header-only.fbx"
        data = b"Kaydara FBX Binary  \x00\x1a\x00"
        asset.write_bytes(data)
        expected = hashlib.sha1(f"blob {len(data)}\0".encode() + data).hexdigest()
        result = ue5.inspect_asset(asset, expected)
        self.assertTrue(result["matches_source"])
        self.assertTrue(result["fbx_header_only"])
        self.assertFalse(result["import_verified"])
        self.assertFalse(result["skeletal_verified"])

    def test_linux_editor_development_target(self):
        command = ue5.build_command(self.root, self.project, "Linux", "build")
        self.assertEqual(command[2:5], ["MakeYourAIEditor", "Linux", "Development"])
        self.assertIn(f"-Project={self.project}", command)

    def test_windows_generation_is_not_build(self):
        command = ue5.build_command(self.root, self.project, "Windows", "generate")
        self.assertIn("-ProjectFiles", command)
        self.assertNotIn("Development", command)

    def test_unsupported_platform_rejected(self):
        with self.assertRaises(ValueError):
            ue5.engine_paths(self.root, "Darwin")

    def test_real_process_failure_preserved(self):
        record = ue5.run_command([sys.executable, "-c", "print('tool fixture'); raise SystemExit(7)"],
                                  self.root, self.root / "command.log", 10)
        self.assertEqual(record["process_exit_code"], 7)
        self.assertEqual(record["wrapper_exit_code"], 7)
        self.assertIn("tool fixture", (self.root / "command.log").read_text())

    def test_missing_executable_is_not_a_compile(self):
        record = ue5.run_command([str(self.root / "missing")], self.root, self.root / "missing.log", 10)
        self.assertIsNone(record["process_exit_code"])
        self.assertEqual(record["status"], "not_started")
        self.assertEqual(record["wrapper_exit_code"], 127)

    def test_blocked_cli_does_not_grant_gate(self):
        with contextlib.redirect_stdout(io.StringIO()):
            code = ue5.main(["build", "--repo-root", str(self.root), "--engine-root", str(self.root / "absent"),
                             "--output", str(self.root / "receipt")])
        report = json.loads((self.root / "receipt/result.json").read_text())
        self.assertEqual(code, 2)
        self.assertIsNone(report["process_exit_code"])
        self.assertEqual(report["reference_gate"], "UE5 reference gate: NOT VERIFIED")
        self.assertNotIn("execution", report)


if __name__ == "__main__":
    unittest.main()
