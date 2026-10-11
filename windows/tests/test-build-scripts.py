#!/usr/bin/env python3
"""Host-local regression tests for native build and test wrappers."""
from __future__ import annotations
import argparse
from datetime import datetime
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[2]
RUNNER = ROOT / 'windows/tests/run_all.ps1'
BUILD = ROOT / 'windows/build/build.ps1'


def invoke(script, args, cwd=ROOT, env=None):
    result = subprocess.run(['powershell.exe', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', str(script), *map(str, args)], cwd=cwd, env=env, text=True, capture_output=True, timeout=60)
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--evidence-root', required=True, type=Path)
    args = parser.parse_args()
    args.evidence_root.mkdir(parents=True, exist_ok=True)
    records = []
    csc = Path(os.environ['WINDIR']) / 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
    if not csc.is_file():
        raise RuntimeError('Installed Microsoft C# compiler required to create development-only subprocess fixtures.')
    with tempfile.TemporaryDirectory(prefix='ASH wrapper tests ', dir=args.evidence_root) as temporary:
        workspace = Path(temporary)
        source = workspace / 'ProcessFixture.cs'
        source.write_text('''using System; using System.IO; using System.Reflection; using System.Threading; using System.Diagnostics;
class ProcessFixture { static int Main(string[] args) {
 if(args.Length>0 && args[0]=="pipe-child") { Thread.Sleep(5000); return 0; }
 string log=Environment.GetEnvironmentVariable("ASH_WRAPPER_FIXTURE_LOG");
 if(!String.IsNullOrEmpty(log)) { File.WriteAllLines(log,args); return 0; }
 string mode=File.ReadAllText(Assembly.GetExecutingAssembly().Location+".mode");
 if(mode=="timeout") Thread.Sleep(5000);
 if(mode=="silent") return 0;
 if(mode=="contradictory") { Console.WriteLine("Total: 1  Failures: 1"); return 0; }
 if(mode=="empty") { Console.WriteLine("Total: 0  Failures: 0"); return 0; }
 if(mode=="inherited-pipe") {
  ProcessStartInfo child=new ProcessStartInfo(Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location),"PipeHoldingChild.exe"),"pipe-child");
  child.UseShellExecute=false; child.CreateNoWindow=true; Process.Start(child);
  Console.WriteLine("Total: 1  Failures: 0"); return 0;
 }
 int code; if(!Int32.TryParse(mode,out code)) code=0;
 Console.WriteLine("Total: 1  Failures: "+(code==0?"0":"1")); return code;
} }''', encoding='utf-8')
        stub = workspace / 'ProcessFixture.exe'
        compilation = subprocess.run([str(csc), '/nologo', '/target:exe', '/platform:x64', '/out:' + str(stub), str(source)], capture_output=True, text=True, timeout=30)
        if compilation.returncode:
            raise RuntimeError(compilation.stdout + compilation.stderr)
        x86_stub = workspace / 'ProcessFixture-x86.exe'
        x86_compilation = subprocess.run([str(csc), '/nologo', '/target:exe', '/platform:x86', '/out:' + str(x86_stub), str(source)], capture_output=True, text=True, timeout=30)
        if x86_compilation.returncode:
            raise RuntimeError(x86_compilation.stdout + x86_compilation.stderr)
        names = json.loads((ROOT / 'windows/tests/test-inventory.json').read_text())['tests']
        binary_dir = workspace / 'native fixtures'
        binary_dir.mkdir()
        shutil.copyfile(stub, binary_dir / 'PipeHoldingChild.exe')
        inventory = workspace / 'inventory.json'
        inventory.write_text(json.dumps({'schemaVersion': '1.0', 'tests': names}), encoding='utf-8')
        for name in names:
            shutil.copyfile(stub, binary_dir / name)
            (binary_dir / (name + '.mode')).write_text('0')

        def check(name, expected_success, expected_status=None, maximum_seconds=None):
            report = args.evidence_root / (name + '.json')
            start = time.time()
            result = invoke(RUNNER, ['-BinDir', binary_dir, '-InventoryPath', inventory, '-ReportPath', report, '-TimeoutSeconds', 1])
            data = json.loads(report.read_text())
            matched = (result.returncode == 0) == expected_success
            if expected_status:
                matched = matched and any(item['status'] == expected_status for item in data['results'])
            elapsed = time.time()-start
            runner_elapsed = (datetime.fromisoformat(data['finishedAtUtc']) - datetime.fromisoformat(data['startedAtUtc'])).total_seconds()
            if maximum_seconds is not None:
                # The descendant also inherits the outer Python capture handle;
                # communicate() can outlive the runner process. Its own report
                # must finish before the child and cannot hide an output wait.
                matched = matched and runner_elapsed < maximum_seconds
            records.append({'case': name, 'pass': matched, 'exitCode': result.returncode, 'elapsedSeconds': elapsed, 'runnerElapsedSeconds': runner_elapsed, 'report': report.name})
            (args.evidence_root / (name + '.log')).write_text(result.stdout + result.stderr, encoding='utf-8')
            if not matched:
                raise AssertionError(name + ': ' + result.stdout + result.stderr)

        check('complete-inventory', True)
        for name in names:
            path = binary_dir / name
            saved = path.read_bytes()
            path.unlink()
            check('missing-' + name.removesuffix('.exe'), False, 'missing')
            path.write_bytes(saved)
        for name in names:
            (binary_dir / name).unlink()
        check('all-required-missing', False, 'missing')
        for name in names:
            shutil.copyfile(stub, binary_dir / name)
        (binary_dir / (names[0] + '.mode')).write_text('1')
        (binary_dir / (names[1] + '.mode')).write_text('-1')
        check('signed-nonzero-exits', False, 'failed')
        for name in names:
            (binary_dir / (name + '.mode')).write_text('0')
        invalid = binary_dir / names[0]
        invalid.write_bytes(b'invalid executable')
        check('launch-failure', False, 'launch_failed')
        shutil.copyfile(stub, invalid)
        (binary_dir / (names[0] + '.mode')).write_text('timeout')
        check('timeout', False, 'timeout')
        (binary_dir / (names[0] + '.mode')).write_text('inherited-pipe')
        check('inherited-pipe-timeout', False, 'output_timeout', maximum_seconds=4)
        (binary_dir / (names[0] + '.mode')).write_text('empty')
        check('zero-case-executable', False, 'empty_suite')
        (binary_dir / (names[0] + '.mode')).write_text('silent')
        check('missing-case-report', False, 'empty_suite')
        (binary_dir / (names[0] + '.mode')).write_text('contradictory')
        check('contradictory-case-report', False, 'failed')
        (binary_dir / (names[0] + '.mode')).write_text('0')
        inventory.write_text(json.dumps({'schemaVersion': '1.0', 'tests': []}), encoding='utf-8')
        check('empty-inventory', False)
        inventory.write_text(json.dumps({'schemaVersion': '1.0', 'tests': names}), encoding='utf-8')
        if os.environ.get('PROCESSOR_ARCHITECTURE') != 'ARM64':
            image = bytearray(invalid.read_bytes())
            pe = struct.unpack_from('<I', image, 0x3c)[0]
            struct.pack_into('<H', image, pe+4, 0xaa64)
            invalid.write_bytes(image)
            check('arm64-host-refusal', False, 'architecture_mismatch')
            shutil.copyfile(stub, invalid)
        shutil.copyfile(x86_stub, invalid)
        check('x86-platform-refusal', False, 'architecture_mismatch')
        shutil.copyfile(stub, invalid)
        check('restored-clean-inventory', True)
        env = os.environ.copy()
        env['ASH_WRAPPER_FIXTURE_LOG'] = str(workspace / 'msbuild-arguments.txt')
        spaces = workspace / 'working directory with spaces'
        spaces.mkdir()
        for name, cwd in [('root', ROOT), ('build', BUILD.parent), ('spaces', spaces)]:
            result = invoke(BUILD, ['-Clean', '-BuildRoot', workspace / 'output with spaces', '-MSBuildPath', stub], cwd=cwd, env=env)
            arguments = Path(env['ASH_WRAPPER_FIXTURE_LOG']).read_text().splitlines()
            passed = result.returncode == 0 and str(BUILD.parent / 'ash-windows.sln') in arguments and '/t:Clean' in arguments
            records.append({'case': 'build-path-' + name, 'pass': passed, 'exitCode': result.returncode, 'arguments': arguments})
            (args.evidence_root / ('build-path-' + name + '.log')).write_text(result.stdout + result.stderr, encoding='utf-8')
            if not passed:
                raise AssertionError(name + ': ' + result.stdout + result.stderr)
        result = invoke(BUILD, ['-Clean', '-Test', '-BuildRoot', workspace / 'output with spaces', '-MSBuildPath', stub], env=env)
        records.append({'case': 'clean-test-rejected', 'pass': result.returncode != 0, 'exitCode': result.returncode})
        if result.returncode == 0:
            raise AssertionError('Clean unexpectedly executed combined Test.')
    report = {'schemaVersion': '1.0', 'status': 'pass' if all(item['pass'] for item in records) else 'fail', 'cases': records, 'limits': 'Synthetic Microsoft process fixtures prove wrapper failure/control behavior only; native positive qualification uses separate real MSVC builds and libraries.'}
    (args.evidence_root / 'script-regression-results.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps({'status': report['status'], 'cases': len(records)}))

if __name__ == '__main__':
    main()
