"""Refresh portable Ui skill references and deterministic upload ZIPs."""
import hashlib
import json
from pathlib import Path
import zipfile

ROOT = Path(__file__).resolve().parents[1]
SKILLS = ROOT / 'skills'
GUIDES = [
    '00_UPP_CODING_GUIDE.md', '01_UI_CONTROLS_GUIDE.md', '02_UI_THEME_GUIDE.md',
    '03_UI_MODEL_GUIDE.md', '04_UI_DEMO_GUIDE.md', '05_UI_PROPERTY_EDITOR_GUIDE.md',
    '07_UI_DRAWING_GUIDE.md', '08_UIGRAPH_GUIDE.md', '09_UIGRAPH_DEVELOPMENT.md',
]


def package(name, guides):
    source = SKILLS / name
    upstream = source / 'references/upstream'
    paths = ['docs/' + guide for guide in guides] + ['LICENSE', 'GETTING_STARTED.md']
    hashes = {}
    for relative in paths:
        data = (ROOT / relative).read_bytes()
        destination = upstream / relative
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_bytes(data)
        hashes[relative] = hashlib.sha256(data).hexdigest()
    (upstream / 'provenance.json').write_text(json.dumps({
        'source_repository': 'upp_Ui',
        'version_header': (ROOT / 'Ui/UiVersion.h').read_text(encoding='utf-8'),
        'sha256': hashes,
        'note': 'Working-tree snapshots; headers and examples are resolved in the target checkout.',
    }, indent=2) + '\n', encoding='utf-8')
    files = sorted(p for p in source.rglob('*') if p.is_file()
                   and '__pycache__' not in p.parts and p.suffix not in ('.zip', '.pyc'))
    target = SKILLS / (name + '.zip')
    with zipfile.ZipFile(target, 'w', zipfile.ZIP_DEFLATED) as archive:
        for path in files:
            info = zipfile.ZipInfo(path.relative_to(SKILLS).as_posix())
            info.compress_type = zipfile.ZIP_DEFLATED
            archive.writestr(info, path.read_bytes())
    with zipfile.ZipFile(target) as archive:
        assert archive.testzip() is None
        assert len(archive.namelist()) == len(files)
        for path in files:
            assert archive.read(path.relative_to(SKILLS).as_posix()) == path.read_bytes()
    print(f'{target}: {len(files)} verified files')


if __name__ == '__main__':
    package('upp-ui-development', GUIDES)
    package('upp-ui-html-mockup', GUIDES[1:3])
