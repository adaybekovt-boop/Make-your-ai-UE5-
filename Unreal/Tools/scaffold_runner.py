#!/usr/bin/env python3
"""Run actual UE scene/Automation work and retain per-run evidence. Never awards the visual gate."""
from __future__ import annotations
import argparse
import json
import os
from pathlib import Path
import platform
import re
import subprocess
import sys
import uuid
import ue5


def expected_tests(repo: Path) -> set[str]:
    files = (repo / 'Unreal/MakeYourAI/Source/MakeYourAI/Private/Tests').glob('*.cpp')
    return {name for path in files for name in re.findall(r'"(MakeYourAI\.[A-Za-z0-9_.]+)"', path.read_text(encoding='utf-8'))}


def check_automation(index: Path, expected: set[str]) -> dict:
    data = json.loads(index.read_text(encoding='utf-8'))
    tests = data.get('tests', [])
    actual = {test.get('fullTestPath'): test.get('state') for test in tests if str(test.get('fullTestPath', '')).startswith('MakeYourAI.')}
    missing = sorted(expected - actual.keys())
    failed = {name: state for name, state in actual.items() if state != 'Success'}
    return {'ok': bool(expected) and not missing and not failed, 'expected': sorted(expected),
            'executed': actual, 'missing': missing, 'failed': failed}


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action', choices=['build-scene', 'build-campaign', 'automation', 'inspect-scene', 'materials', 'lighting', 'world-partition'])
    parser.add_argument('--engine-root', default=os.environ.get('UE_ROOT'))
    parser.add_argument('--repo-root', type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument('--output', type=Path)
    parser.add_argument('--null-rhi', action='store_true')
    parser.add_argument('--timeout', type=int, default=1800)
    args = parser.parse_args(argv)
    if args.timeout <= 0:
        parser.error('Timeout must be positive')
    repo = args.repo_root.resolve()
    project = repo / 'Unreal/MakeYourAI/MakeYourAI.uproject'
    out = args.output or project.parent / 'Saved/Verification' / ('scaffold-' + uuid.uuid4().hex)
    out.mkdir(parents=True, exist_ok=False)
    result = {'schema': 1, 'action': args.action, 'status': 'blocked', 'process_exit_code': None,
              'visual_verified': False, 'gameplay_verified': False, 'null_rhi': args.null_rhi,
              'reference_gate': 'UE5 reference gate: NOT VERIFIED'}
    code = 2
    try:
        result['git'] = ue5.run_command(['git', 'rev-parse', 'HEAD'], repo, out / 'git.log', 15)
        if not args.engine_root:
            raise ValueError('UE_ROOT is not set; Editor and Automation were not started')
        root = Path(args.engine_root).expanduser().resolve()
        receipt = ue5.read_engine(root, platform.system())
        ue5.require_pin(project, receipt)
        result['engine'] = receipt
        if args.action == 'build-scene' and not (project.parent / 'Saved/ScaffoldSource/city-markers.json').is_file():
            raise ValueError('Run extract_city_markers.py in Blender first; authored marker bounds are required')
        binary = ue5.engine_paths(root, platform.system())['editor_cmd']
        command = [str(binary), str(project), '-unattended', '-nop4', '-nosplash', '-stdout', '-FullStdOutLogOutput']
        if args.null_rhi:
            command.append('-NullRHI')
        script_report = out / 'editor-result.json'
        if args.action == 'automation':
            command += ['-ExecCmds=Automation RunTest MakeYourAI;Quit', '-ReportExportPath=' + str(out / 'Automation')]
        else:
            scripts = {
                'build-scene': 'build_scaffold.py',
                'build-campaign': 'build_campaign.py',
                'inspect-scene': 'inspect_scaffold.py',
                'materials': 'run_editor_visual.py',
                'lighting': 'run_editor_visual.py',
                'world-partition': 'run_editor_visual.py',
            }
            command.append('-ExecutePythonScript=' + str(repo / 'Unreal/Tools' / scripts[args.action]))
        result['command'] = command
        environment = dict(os.environ, MAI_SCRIPT_REPORT=str(script_report))
        with (out / 'editor.log').open('w', encoding='utf-8') as log:
            process = subprocess.run(command, cwd=repo, env=environment, stdout=log,
                                     stderr=subprocess.STDOUT, timeout=args.timeout, check=False)
        result['process_exit_code'] = process.returncode
        if process.returncode:
            raise ValueError(f'Editor process failed with exit code {process.returncode}')
        if args.action == 'automation':
            result['automation'] = check_automation(out / 'Automation/index.json', expected_tests(repo))
            if not result['automation']['ok']:
                raise ValueError('Unreal Automation report has missing or unsuccessful tests')
            result['automation_verified'] = True
        else:
            result['editor'] = json.loads(script_report.read_text(encoding='utf-8'))
            if result['editor'].get('ok') is not True:
                raise ValueError('Editor script did not confirm successful execution')
        result['status'] = 'executed'; code = 0
    except subprocess.TimeoutExpired:
        result['blocker'] = 'Editor process timed out; no verification granted'; code = 124
    except (OSError, ValueError, TypeError) as error:
        result['blocker'] = str(error)
    result['wrapper_exit_code'] = code
    (out / 'result.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(result, indent=2))
    return code


if __name__ == '__main__':
    sys.exit(main())
