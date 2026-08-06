from pathlib import Path

p = Path('Utilities/UiDesigner/Services/UiDesignerPresets.cpp')
s = p.read_text()
old = '''    UiDesignerPresetBuilder& Text(UiDesignerNodeId id, const String& value)
    {
        return P(id, "text", value).P(id, "title", value);
    }
'''
new = '''    UiDesignerPresetBuilder& Text(UiDesignerNodeId id, const String& value)
    {
        UiDesignerNode *node = document_.Find(id);
        const UiDesignerControlSpec *spec = node ? catalog_.Find(node->type) : nullptr;
        if(node && spec) {
            if(spec->FindProperty("text"))
                node->properties.Set("text", value);
            if(spec->FindProperty("title"))
                node->properties.Set("title", value);
        }
        return *this;
    }
'''
if s.count(old) != 1:
    raise RuntimeError('Text helper block missing')
p.write_text(s.replace(old, new))

p = Path('Utilities/UiDesigner/Core/UiDesignerSerialization.cpp')
s = p.read_text()
old = '''            const int64 old_child = children[i];
            if(seen.FindAdd(old_child) >= 0) {
                error = "Parent contains duplicate child " + AsString(old_child);
                return false;
            }
'''
new = '''            const int64 old_child = children[i];
            if(seen.Find(old_child) >= 0) {
                error = "Parent contains duplicate child " + AsString(old_child);
                return false;
            }
            seen.Add(old_child);
'''
if s.count(old) != 1:
    raise RuntimeError('child duplicate guard missing')
p.write_text(s.replace(old, new))

# Give grid children deterministic cells rather than overlapping at 0,0.
p = Path('Utilities/UiDesigner/Services/UiDesignerPresets.cpp')
s = p.read_text()
s = s.replace('''        b.Text(card, Format("Story %d", i)).Size(card, "Expand", "Fixed", 0, 72);
''', '''        b.Text(card, Format("Story %d", i)).Size(card, "Expand", "Fixed", 0, 72)
         .P(card, "grid_row", (i - 1) / 2).P(card, "grid_column", (i - 1) % 2);
''', 1)
s = s.replace('''        b.Text(card, Format("Story %d", i)).Size(card, "Expand", "Fixed", 0, 80);
''', '''        b.Text(card, Format("Story %d", i)).Size(card, "Expand", "Fixed", 0, 80)
         .P(card, "grid_row", (i - 1) / 2).P(card, "grid_column", (i - 1) % 2);
''', 1)
s = s.replace('''        b.Text(card, Format("Card %d", i)).Size(card, "Expand", "Fixed", 0, 96)
         .P(card, "subtitle", "Reusable summary content");
''', '''        b.Text(card, Format("Card %d", i)).Size(card, "Expand", "Fixed", 0, 96)
         .P(card, "subtitle", "Reusable summary content")
         .P(card, "grid_row", (i - 1) / 3).P(card, "grid_column", (i - 1) % 3);
''', 1)
p.write_text(s)

for temporary in ['.github/preset-rc-review.py', '.github/workflows/preset-rc-review.yml']:
    q = Path(temporary)
    if q.exists():
        q.unlink()
