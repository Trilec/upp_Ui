"""Install the two local Ui skills to explicitly named skill roots, preserving backups."""
import argparse
from datetime import datetime
from pathlib import Path
import zipfile

ROOT = Path(__file__).resolve().parents[1]
NAMES = ('upp-ui-development', 'upp-ui-html-mockup')


def install(destination):
    destination = destination.resolve()
    backup = ROOT / 'skills/backup' / ('installed-' + datetime.now().strftime('%Y%m%d-%H%M%S-%f') + '.zip')
    backup.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(backup, 'x', zipfile.ZIP_DEFLATED) as archive:
        archive.writestr('destination.txt', str(destination))
        for name in NAMES:
            current = destination / name
            if current.is_dir():
                for path in current.rglob('*'):
                    if path.is_file():
                        archive.write(path, path.relative_to(destination).as_posix())
    for name in NAMES:
        source = ROOT / 'skills' / name
        target = destination / name
        for path in source.rglob('*'):
            if not path.is_file() or '__pycache__' in path.parts:
                continue
            output = target / path.relative_to(source)
            output.parent.mkdir(parents=True, exist_ok=True)
            temporary = output.with_name(output.name + '.install-tmp')
            temporary.write_bytes(path.read_bytes())
            temporary.replace(output)
            assert output.read_bytes() == path.read_bytes()
        # This retired reference was replaced by build.md and composition.md.
        if name == 'upp-ui-development':
            stale = target / 'references/build-and-integration.md'
            if stale.exists():
                stale.unlink()
        print('Installed and verified:', target)
    print('Previous installed files preserved:', backup)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('roots', nargs='+', type=Path)
    for root in parser.parse_args().roots:
        install(root)
