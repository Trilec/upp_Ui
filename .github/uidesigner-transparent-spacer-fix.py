from pathlib import Path

path = Path('Utilities/UiDesigner/Preview/UiDesignerPreview.cpp')
text = path.read_text(encoding='utf-8')
old = '''    const bool selected = selection_ && selection_->Contains(node.id);
    const Color frame = selected ? accent_ : Blend(SColorText(), SColorPaper(), 150);
    w.DrawRect(r, Blend(SColorPaper(), frame, 235));
    w.DrawRect(r.left, r.top, r.Width(), 1, frame);
    w.DrawRect(r.left, r.bottom - 1, r.Width(), 1, frame);
    w.DrawRect(r.left, r.top, 1, r.Height(), frame);
    w.DrawRect(r.right - 1, r.top, 1, r.Height(), frame);
    const String label = node.GetProperty("layout_break", false) ? "Break" : "Spacer";
    w.DrawText(r.left + DPI(5), r.top + DPI(3), label, StdFont().Height(DPI(11)), frame);
    if(node.GetProperty("line_enabled", false)) {
        const Color line = node.GetProperty("line_color_enabled", false)
            ? (Color)node.GetProperty("line_color", frame) : frame;
'''
new = '''    // Semantic spacers have no runtime face, frame or label. The Designer's
    // geometry/selection layer already supplies the orange or blue outline;
    // painting another filled semantic surface here misrepresents generated
    // output and obscures the optional authored separator line.
    if(node.GetProperty("line_enabled", false)) {
        const Color fallback = Blend(SColorText(), SColorPaper(), 150);
        const Color line = node.GetProperty("line_color_enabled", false)
            ? (Color)node.GetProperty("line_color", fallback) : fallback;
'''
if text.count(old) != 1:
    raise RuntimeError(f'expected one PaintSemantic block, found {text.count(old)}')
path.write_text(text.replace(old, new), encoding='utf-8')

for temporary in [
    '.github/uidesigner-transparent-spacer-fix.py',
    '.github/workflows/uidesigner-transparent-spacer-fix.yml',
]:
    p = Path(temporary)
    if p.exists():
        p.unlink()
