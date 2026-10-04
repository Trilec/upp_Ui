"""Build the maintained demo inventory; promote successful executables to bin/windows-x64.

Usage: python examples/build_demos.py --umk E:/upp-18468/umk.exe
No demo imports this script. Tests and generated-code checks are separate targets.
"""
import argparse
import json
import shutil
import subprocess
import sys
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--umk', default=shutil.which('umk'))
    parser.add_argument('--upp-root', type=Path, help='U++ installation containing uppsrc')
    parser.add_argument('--method', default='CLANGx64')
    parser.add_argument('--jobs', type=int, default=2)
    parser.add_argument('--package', action='append', help='Build only these maintained packages')
    parser.add_argument('--canonical-only', action='store_true')
    parser.add_argument('--list', action='store_true')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    inventory = json.loads((root / 'tests/ui_release_inventory.json').read_text())
    canonical = {Path(row['example']).name for row in inventory['controls'] if row.get('example')}
    specialists = {row['package'] for row in inventory['specialist_examples']}
    maintained = canonical | specialists
    targets = sorted(set(args.package) if args.package else canonical if args.canonical_only else maintained)
    if set(targets) - maintained:
        parser.error('Unmaintained packages: ' + ', '.join(sorted(set(targets) - maintained)))
    if args.list:
        print('\n'.join(targets))
        return 0
    if not args.umk:
        parser.error('Provide --umk or place umk on PATH')
    umk = Path(args.umk).resolve()
    upp_root = (args.upp_root or umk.parent).resolve()
    assembly_paths = [root / 'examples', root, root.parent / 'upp_statemachine',
                      root.parent / 'upp_animation', upp_root / 'uppsrc']
    for path in [umk, *assembly_paths]:
        if not path.exists():
            parser.error('Missing toolchain/assembly path: ' + str(path))
    if args.jobs < 1:
        parser.error('--jobs must be positive')
    build = root / 'build/demos'
    bin_dir = root / 'bin/windows-x64'
    build.mkdir(parents=True, exist_ok=True)
    bin_dir.mkdir(parents=True, exist_ok=True)
    assembly = ','.join(str(path) for path in assembly_paths)
    results = []
    for package in targets:
        manifest = root / 'examples' / package / (package + '.upp')
        if not manifest.is_file():
            results.append(dict(package=package, status='missing package'))
            print(f'{package}: missing package', flush=True)
            continue
        staging = build / package
        staging.mkdir(exist_ok=True)
        executable = staging / (package + '.exe')
        log = staging / 'build.log'
        command = [str(umk), assembly, package, args.method, '--out-dir', str(build / 'cache'),
                   '-brH' + str(args.jobs), str(executable)]
        print(f'Building {package}...', flush=True)
        with log.open('w', encoding='utf-8') as output:
            completed = subprocess.run(command, cwd=root, stdout=output, stderr=subprocess.STDOUT)
        success = completed.returncode == 0 and executable.is_file()
        status = 'failed'
        if success:
            # Only a successfully linked current executable replaces the finished copy.
            try:
                shutil.copy2(executable, bin_dir / executable.name)
                status = 'built'
            except OSError as error:
                status = 'promotion failed: ' + str(error)
        results.append(dict(package=package, status=status, exit_code=completed.returncode,
                            log=str(log.relative_to(root))))
        print(f'{package}: {status}', flush=True)
    report = build / 'build_report.json'
    report.write_text(json.dumps(dict(method=args.method, results=results), indent=2) + '\n')
    print(f'Build report: {report}')
    return 0 if all(row['status'] == 'built' for row in results) else 1


if __name__ == '__main__':
    sys.exit(main())
