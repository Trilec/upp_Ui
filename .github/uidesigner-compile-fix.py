from pathlib import Path

path = Path('Utilities/UiDesigner/UiDesigner/UiDesignerHierarchyView.cpp')
text = path.read_text(encoding='utf-8')
old = '''    if(WhenDropStatus)
        WhenDropStatus(ok ? (type_id.StartsWith("preset:") ? "Preset inserted" : "Control added")
                          : error);
'''
new = '''    if(WhenDropStatus) {
        const String status = ok
            ? String(type_id.StartsWith("preset:") ? "Preset inserted" : "Control added")
            : error;
        WhenDropStatus(status);
    }
'''
if text.count(old) != 1:
    raise RuntimeError(f'expected one ambiguous status expression, found {text.count(old)}')
path.write_text(text.replace(old, new), encoding='utf-8')

for temporary in ['.github/uidesigner-compile-fix.py',
                  '.github/workflows/uidesigner-compile-fix.yml']:
    p = Path(temporary)
    if p.exists():
        p.unlink()
