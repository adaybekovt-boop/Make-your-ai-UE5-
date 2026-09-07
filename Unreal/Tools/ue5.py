#!/usr/bin/env python3
"""Record real preflight/build results. Never grants the UE5 reference gate."""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import platform
import subprocess
import sys
import uuid

ASSETS = {
    "Unreal/Assets/interiors/garage/garage.fbx": "3f291e6dd3755de290bc06d0aaa05ac0da51000a",
    "Unreal/Assets/interiors/garage/garage-enclosure.fbx": "41fa99fd34791ae204b4cb414209d38db321f3dc",
    "Unreal/CityV4/city-v4.fbx": "67105e990ad02eae231c9ab97d2630f8e6731c8c",
    "Unreal/Assets/characters/person-1/person-1.fbx": "3e333924ecf6e59b9032d05c498834d3a2b8cfdf",
}
GATE = "UE5 reference gate: NOT VERIFIED"


def inspect_asset(path: Path, expected: str) -> dict:
    if not path.is_file():
        return {"exists": False, "expected_git_blob_sha1": expected}
    size = path.stat().st_size
    sha256 = hashlib.sha256()
    git_hash = hashlib.sha1(f"blob {size}\0".encode())
    with path.open("rb") as stream:
        header = stream.read(256)
        stream.seek(0)
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            sha256.update(chunk)
            git_hash.update(chunk)
    is_pointer = header.startswith(b"version https://git-lfs.github.com/spec/v1")
    fbx_header = (header.startswith(b"Kaydara FBX Binary  \x00\x1a\x00")
                  or header.lstrip().startswith(b"; FBX"))
    return {"exists": True, "bytes": size, "sha256": sha256.hexdigest(),
            "git_blob_sha1": git_hash.hexdigest(), "expected_git_blob_sha1": expected,
            "matches_source": git_hash.hexdigest() == expected,
            "lfs_pointer": is_pointer, "fbx_header_only": fbx_header,
            "import_verified": False, "skeletal_verified": False}


def engine_paths(root: Path, host: str) -> dict[str, Path]:
    if host == "Linux":
        build = root / "Engine/Build/BatchFiles/Linux/Build.sh"
        binary = root / "Engine/Binaries/Linux/UnrealEditor"
        commandlet = root / "Engine/Binaries/Linux/UnrealEditor-Cmd"
    elif host == "Windows":
        build = root / "Engine/Build/BatchFiles/Build.bat"
        binary = root / "Engine/Binaries/Win64/UnrealEditor.exe"
        commandlet = root / "Engine/Binaries/Win64/UnrealEditor-Cmd.exe"
    else:
        raise ValueError(f"Unsupported build host: {host}; configure its commands explicitly.")
    return {"build_script": build, "editor": binary, "editor_cmd": commandlet,
            "ubt": root / "Engine/Binaries/DotNET/UnrealBuildTool/UnrealBuildTool.dll"}


def read_engine(root: Path, host: str) -> dict:
    version_path = root / "Engine/Build/Build.version"
    data = version_path.read_bytes()
    version = json.loads(data)
    for key in ("MajorVersion", "MinorVersion", "PatchVersion"):
        if type(version.get(key)) is not int or version[key] < 0:
            raise ValueError(f"Invalid engine version field: {key}")
    if version["MajorVersion"] != 5:
        raise ValueError("An actual UE5 installation is required.")
    missing = [key for key, path in engine_paths(root, host).items() if not path.is_file()]
    if missing:
        raise ValueError("Missing engine tools: " + ", ".join(missing))
    return {"build_version": version, "build_version_sha256": hashlib.sha256(data).hexdigest()}


def pin_engine(project: Path, receipt: dict) -> None:
    lock = project.parent / "UE5Engine.lock.json"
    if lock.exists() and json.loads(lock.read_text(encoding="utf-8")) != receipt:
        raise ValueError("Engine pin differs. Review an explicit engine migration; not overwriting it.")
    lock.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    # An explicit engine root is used for builds. No workstation path/GUID is stored.
    descriptor = json.loads(project.read_text(encoding="utf-8"))
    version = receipt["build_version"]
    descriptor["EngineAssociation"] = f'{version["MajorVersion"]}.{version["MinorVersion"]}'
    project.write_text(json.dumps(descriptor, indent=2) + "\n", encoding="utf-8")


def require_pin(project: Path, receipt: dict) -> None:
    lock = project.parent / "UE5Engine.lock.json"
    if not lock.is_file() or json.loads(lock.read_text(encoding="utf-8")) != receipt:
        raise ValueError("Installed engine is not pinned or differs from UE5Engine.lock.json. Run pin-engine first.")


def build_command(root: Path, project: Path, host: str, action: str) -> list[str]:
    script = engine_paths(root, host)["build_script"]
    prefix = ["bash", str(script)] if host == "Linux" else ["cmd.exe", "/d", "/c", str(script)]
    if action == "generate":
        return prefix + ["-ProjectFiles", f"-Project={project}", "-Game", "-Engine"]
    if action != "build":
        raise ValueError(f"Not a build action: {action}")
    target = "Linux" if host == "Linux" else "Win64"
    return prefix + ["MakeYourAIEditor", target, "Development", f"-Project={project}",
                     "-WaitMutex", "-NoHotReloadFromIDE"]


def run_command(command: list[str], cwd: Path, log: Path, timeout: int) -> dict:
    record = {"command": command, "cwd": str(cwd), "log": str(log), "process_exit_code": None}
    with log.open("w", encoding="utf-8") as stream:
        try:
            process = subprocess.run(command, cwd=cwd, stdout=stream, stderr=subprocess.STDOUT,
                                     timeout=timeout, check=False)
            record.update(status="executed", process_exit_code=process.returncode,
                          wrapper_exit_code=process.returncode)
        except subprocess.TimeoutExpired:
            record.update(status="timed_out", wrapper_exit_code=124)
        except OSError as exc:
            stream.write(str(exc) + "\n")
            record.update(status="not_started", wrapper_exit_code=127, error=str(exc))
    return record


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=["audit", "pin-engine", "generate", "build"])
    parser.add_argument("--engine-root", default=os.environ.get("UE_ROOT"))
    parser.add_argument("--repo-root", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--output", type=Path)
    parser.add_argument("--timeout", type=int, default=1800)
    args = parser.parse_args(argv)
    if args.timeout <= 0:
        parser.error("--timeout must be positive")
    repo = args.repo_root.resolve()
    project = repo / "Unreal/MakeYourAI/MakeYourAI.uproject"
    run_id = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ") + "-" + uuid.uuid4().hex[:8]
    output = args.output or project.parent / "Saved/Verification" / run_id
    output.mkdir(parents=True, exist_ok=False)
    report = {"utc": datetime.now(timezone.utc).isoformat(), "action": args.action,
              "host": platform.platform(), "reference_gate": GATE,
              "process_exit_code": None, "status": "blocked"}
    try:
        report["git"] = run_command(["git", "rev-parse", "HEAD"], repo, output / "git.log", 15)
        if args.action == "audit":
            report["assets"] = {name: inspect_asset(repo / name, sha) for name, sha in ASSETS.items()}
            report["display_environment"] = {key: os.environ.get(key) for key in ("DISPLAY", "WAYLAND_DISPLAY")}
            report["gpu_device_nodes"] = [str(p) for p in Path("/dev").glob("nvidia*")] + [str(p) for p in Path("/dev/dri").glob("*")]
            report["gpu_render_verified"] = False
        if not args.engine_root:
            raise ValueError("UE_ROOT / --engine-root is not set; no installed UE5 version can be selected.")
        engine = Path(args.engine_root).expanduser().resolve()
        receipt = read_engine(engine, platform.system())
        report["engine"] = receipt
        if args.action == "audit":
            report["status"] = "audited_file_presence_only"
            assets_ok = all(a.get("matches_source") and a.get("fbx_header_only") and not a.get("lfs_pointer")
                            for a in report["assets"].values())
            code = 0 if assets_ok else 2
        elif args.action == "pin-engine":
            pin_engine(project, receipt)
            report["status"] = "pinned_file_metadata_only"
            code = 0
        else:
            require_pin(project, receipt)
            report["execution"] = run_command(build_command(engine, project, platform.system(), args.action),
                                               engine, output / f"{args.action}.log", args.timeout)
            report["status"] = report["execution"]["status"]
            report["process_exit_code"] = report["execution"]["process_exit_code"]
            code = report["execution"]["wrapper_exit_code"]
    except (OSError, ValueError) as exc:
        report["blocker"] = str(exc)
        code = 2
    report["wrapper_exit_code"] = code
    (output / "result.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report, indent=2))
    return code


if __name__ == "__main__":
    sys.exit(main())
