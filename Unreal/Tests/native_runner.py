#!/usr/bin/env python3
"""Compile and execute portable native suites. This does not invoke Unreal Engine."""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'Unreal/MakeYourAI/Source/MakeYourAI'
DOMAIN = [SOURCE / 'Private/Core' / name for name in ('MaiCatalog.cpp', 'MaiSimulation.cpp', 'MaiCodec.cpp')]
DOMAIN += sorted((SOURCE / 'Private/Campaign').glob('MaiCampaign*.cpp'))


def compiler() -> str:
    name = os.environ.get('CXX', 'clang++' if os.name == 'nt' else 'g++')
    found = shutil.which(name)
    if not found:
        raise FileNotFoundError(f'Native C++ compiler not found: {name}; set CXX to a GNU-compatible compiler executable')
    return found


def compile_command(sources: list[Path], binary: Path, *, sanitize: bool = True,
                    includes: tuple[Path, ...] = (), libraries: tuple[str, ...] = ()) -> list[str]:
    command = [compiler(), '-std=c++17', '-Wall', '-Wextra', '-Werror',
               '-Wno-misleading-indentation', '-pedantic', '-g']
    if sanitize:
        # MinGW does not ship GCC libasan/libubsan. Windows uses LLVM UBSan;
        # Linux retains both ASan and UBSan, with recovery forbidden.
        mode = 'undefined' if os.name == 'nt' else 'address,undefined'
        command += [f'-fsanitize={mode}', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer']
        if sys.platform.startswith('linux'):
            command += ['-fno-pie', '-no-pie']
    for include in (SOURCE / 'Public', ROOT / 'Unreal/Tests/native', *includes):
        command += ['-I', str(include)]
    return command + [str(p) for p in sources] + list(libraries) + ['-o', str(binary)]


def execute(command: list[str], **kwargs) -> subprocess.CompletedProcess:
    print('COMMAND ' + json.dumps(command), flush=True)
    return subprocess.run(command, cwd=ROOT, check=True, timeout=420, **kwargs)


def build(name: str, sources: list[Path], out: Path, **kwargs) -> Path:
    binary = out / (name + ('.exe' if os.name == 'nt' else ''))
    execute(compile_command(sources, binary, **kwargs))
    return binary


def core(out: Path) -> None:
    sources = DOMAIN + [ROOT / 'Unreal/Tests/native/main.cpp', ROOT / 'Unreal/Tests/native/campaign_tests.cpp']
    binary = build('mai-tests', sources, out)
    execute([str(binary)])
    # Never reuse a checked-in/previous fixture. Only a successful fresh native
    # process may publish it; validate before replacing the owned generated file.
    result = execute([str(binary), '--fixtures'], stdout=subprocess.PIPE)
    data = json.loads(result.stdout)
    if data.get('schema') != 1 or len(data.get('quotes', [])) != 336:
        raise ValueError('Native fixture is incomplete or has an unsupported schema')
    with tempfile.TemporaryDirectory(dir=out, prefix='fixture-') as tmp:
        candidate = Path(tmp) / 'fixtures.json'
        candidate.write_bytes(result.stdout)
        candidate.replace(out / 'fixtures.json')
    receipt = {'scope': 'portable C++ fixture, NOT UE runtime', 'status': 'PASS',
               'binarySHA256': hashlib.sha256(binary.read_bytes()).hexdigest(),
               'fixtureSHA256': hashlib.sha256(result.stdout).hexdigest(),
               'sourceSHA256': {p.relative_to(ROOT).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest() for p in sources}}
    (out / 'fixture-receipt.json').write_text(json.dumps(receipt, indent=2) + '\n', encoding='utf-8')
    print('NATIVE_FIXTURE_PASS quotes=336', flush=True)


def campaign(out: Path) -> None:
    binary = build('campaign-tests', DOMAIN + [ROOT / 'Unreal/Tests/native/campaign_main.cpp'], out)
    execute([str(binary)])
    with tempfile.TemporaryDirectory(prefix='mai-campaign-') as temp:
        execute([str(binary), '--export', temp])
        resumed = execute([str(binary), '--resume', str(Path(temp) / 'campaign.save')], stdout=subprocess.PIPE)
        expected = (Path(temp) / 'expected.save').read_bytes()
        if resumed.stdout != expected:
            raise AssertionError('Separate native process did not restore the exact expected campaign bytes')
    print('CROSS_PROCESS_RESTART_PASS: native campaign, NOT an Unreal playthrough', flush=True)


def durable(out: Path) -> None:
    binary = build('durable-tests', [SOURCE / 'Private/Rules/MaiDurableFile.cpp', ROOT / 'Unreal/Tests/native/durable_main.cpp'], out)
    with tempfile.TemporaryDirectory(prefix='mai-durable-') as temp:
        execute([str(binary), temp])
        for mode in ('export', 'resume', 'recover'):
            execute([str(binary), str(Path(temp) / 'restart'), mode])


def rules(out: Path) -> None:
    source = Path(os.environ['MAI_QUICKJS_SOURCE']).resolve()
    library = Path(os.environ['MAI_QUICKJS_LIBRARY']).resolve()
    if not (source / 'quickjs.h').is_file() or not library.is_file():
        raise FileNotFoundError('Pinned QuickJS headers and a platform-compatible static library are required')
    execute(['node', 'Unreal/Rules/build.mjs'])
    execute(['node', 'node_modules/typescript/bin/tsc', '-p', 'Unreal/Rules/tsconfig.json'])
    libs = (str(library),) if os.name == 'nt' else (str(library), '-lm', '-ldl', '-lpthread')
    binary = build('rules_host', [SOURCE / 'Private/Rules/MaiRulesVM.cpp', ROOT / 'Unreal/Tests/rules_host.cpp'], out,
                   sanitize=False, includes=(source,), libraries=libs)
    env = dict(os.environ, MAI_RULES_HOST=str(binary))
    execute(['node', 'Unreal/Tests/run_rules_parity.mjs'], env=env)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('suite', choices=['core', 'campaign', 'durable', 'rules'])
    parser.add_argument('--output', type=Path, default=Path(os.environ.get('MAI_TEST_OUTPUT', ROOT / 'Unreal/Tests/.out')))
    args = parser.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    try:
        {'core': core, 'campaign': campaign, 'durable': durable, 'rules': rules}[args.suite](out)
    except (OSError, KeyError) as error:
        print(f'BLOCKED: {error}', file=sys.stderr)
        return 2
    except (subprocess.SubprocessError, ValueError, AssertionError) as error:
        print(f'FAIL: {error}', file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
