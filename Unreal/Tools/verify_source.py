#!/usr/bin/env python3
"""Run the full source verification on Windows/Linux; never imply UE verification."""
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
import time

ROOT = Path(__file__).resolve().parents[2]


def utc() -> str:
    return datetime.now(timezone.utc).isoformat()


def suites() -> list[tuple[str, list[list[str]]]]:
    native = [sys.executable, 'Unreal/Tests/native_runner.py']
    node = ['node']
    return [
        ('native-core', [native + ['core']]),
        ('native-campaign', [native + ['campaign']]),
        ('persistence-recovery', [native + ['durable']]),
        ('quickjs-parity', [native + ['rules']]),
        ('python-tools', [[sys.executable, '-m', 'unittest', 'discover', '-s', 'Unreal/Tools/tests', '-v']]),
        ('python-source', [[sys.executable, '-m', 'unittest', 'discover', '-s', 'Unreal/Tests', '-p', 'test_*.py', '-v']]),
        ('browser-regression', [node + ['node_modules/vitest/vitest.mjs', 'run']]),
        ('typescript-typecheck', [node + ['node_modules/typescript/bin/tsc', '-b', '--pretty', 'false']]),
        ('production-build', [node + ['node_modules/typescript/bin/tsc', '-b'], node + ['node_modules/vite/bin/vite.js', 'build']]),
        ('cross-language', [node + ['node_modules/vitest/vitest.mjs', 'run', '--config', 'Unreal/Tests/vitest.config.ts']]),
    ]


def snapshot() -> dict[str, str]:
    hashes = {}
    for folder in ('Unreal/MakeYourAI/Source', 'Unreal/MakeYourAI/Config', 'Unreal/Rules', 'Unreal/Tests', 'Unreal/Tools', 'src'):
        for p in sorted((ROOT / folder).rglob('*')):
            if p.is_file() and not any(part in p.parts for part in ('.out', '__pycache__')):
                hashes[p.relative_to(ROOT).as_posix()] = hashlib.sha256(p.read_bytes()).hexdigest()
    return hashes


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--timeout', type=int, default=900)
    args = parser.parse_args()
    if args.timeout <= 0:
        parser.error('--timeout must be positive')
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    head = subprocess.run(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True, capture_output=True, check=True).stdout.strip()
    status = subprocess.run(['git', 'status', '--porcelain=v1'], cwd=ROOT, text=True, capture_output=True, check=True).stdout
    (out / 'git-status-before.log').write_text(status, encoding='utf-8')
    before = snapshot()
    report = {'commit': head, 'startedAt': utc(), 'host': platform.platform(), 'status': 'NOT_RUN',
              'scope': 'source verification only; no Unreal Editor, physics, rendering or player input',
              'ueBuild': 'NOT_RUN', 'shipping': 'NOT_RUN', 'humanPlaytest': False, 'tests': [], 'sourceSHA256': before}
    for name, commands in suites():
        entry = {'name': name, 'startedAt': utc(), 'status': 'NOT_RUN', 'log': name + '.log', 'commands': []}
        started = time.monotonic()
        with (out / (name + '.log')).open('w', encoding='utf-8') as log:
            for command in commands:
                log.write('COMMAND ' + json.dumps(command) + '\n')
                log.flush()
                item = {'command': command, 'startedAt': utc(), 'exitCode': None, 'status': 'NOT_RUN'}
                try:
                    code = subprocess.run(command, cwd=ROOT, stdout=log, stderr=subprocess.STDOUT, timeout=args.timeout).returncode
                    item.update(exitCode=code, status='PASS' if code == 0 else 'FAIL')
                except OSError as error:
                    log.write(str(error) + '\n')
                    item.update(status='BLOCKED', error=str(error))
                except subprocess.TimeoutExpired as error:
                    log.write(str(error) + '\n')
                    item.update(status='FAIL', error=str(error))
                item['finishedAt'] = utc()
                entry['commands'].append(item)
                entry['status'] = item['status']
                if item['status'] != 'PASS':
                    break
        entry['seconds'] = round(time.monotonic() - started, 3)
        report['tests'].append(entry)
        print(f'{name}: {entry["status"]}', flush=True)
        (out / 'results.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    after = snapshot()
    report['sourceUnchangedDuringTests'] = before == after
    report['finishedAt'] = utc()
    report['status'] = 'PASS' if before == after and all(t['status'] == 'PASS' for t in report['tests']) else 'FAIL'
    if before != after:
        report['sourceChangesDuringTests'] = sorted(p for p in before.keys() | after.keys() if before.get(p) != after.get(p))
    (out / 'results.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    if os.environ.get('GITHUB_STEP_SUMMARY'):
        with open(os.environ['GITHUB_STEP_SUMMARY'], 'a', encoding='utf-8') as stream:
            stream.write(f'## Source checks: {head}\n\nUE compilation / Shipping / playthrough: NOT_RUN.\n\n')
            for t in report['tests']:
                stream.write(f'{t["name"]}: {t["status"]}\n\n')
    print('SOURCE_VERIFICATION ' + report['status'], flush=True)
    return 0 if report['status'] == 'PASS' else 1


if __name__ == '__main__':
    raise SystemExit(main())
