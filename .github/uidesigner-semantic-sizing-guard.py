from pathlib import Path

path = Path('Utilities/UiDesigner/Services/UiDesignerSession.cpp')
text = path.read_text(encoding='utf-8')
old = '''    const String property = height ? "height_mode" : "width_mode";
    const UiDesignerControlSpec *spec = catalog_.Find(node->type);
    if(!spec || !spec->FindProperty(property)) {
'''
new = '''    const String property = height ? "height_mode" : "width_mode";
    const UiDesignerControlSpec *spec = catalog_.Find(node->type);
    if((node->flags & UiDesignerNodeSemanticItem) ||
       !spec || !spec->FindProperty(property)) {
'''
if text.count(old) != 1:
    raise RuntimeError('CycleSizingMode semantic guard insertion point changed')
path.write_text(text.replace(old, new), encoding='utf-8')

for temporary in [
    '.github/uidesigner-semantic-sizing-guard.py',
    '.github/workflows/uidesigner-semantic-sizing-guard.yml',
]:
    candidate = Path(temporary)
    if candidate.exists():
        candidate.unlink()
