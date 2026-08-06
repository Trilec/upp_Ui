from pathlib import Path


def replace_one(path, old, new, label):
    p = Path(path)
    text = p.read_text(encoding='utf-8')
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f'{label}: expected one match, found {count}')
    p.write_text(text.replace(old, new), encoding='utf-8')

# Dialog footer is responsive and may flow when the authored width is reduced.
replace_one(
    'Utilities/UiDesigner/Services/UiDesignerSession.cpp',
    '    SetLayout(actions, "wrap", "None");\n',
    '    SetLayout(actions, "wrap", "Flow");\n',
    'dialog action wrap')

# Semantic box spacers must resolve the same current axis properties as controls.
replace_one(
    'Utilities/UiDesigner/Preview/UiDesignerPreview.cpp',
'''        const String cross_mode = node.GetProperty(
            horizontal ? "v_sizing" : "h_sizing", "Auto");
''',
'''        const String cross_mode = node.GetProperty(
            horizontal ? "height_mode" : "width_mode",
            node.GetProperty(horizontal ? "v_sizing" : "h_sizing", "Fit"));
''',
    'preview spacer cross sizing')

# Initial runtime box attachment must configure the item immediately. Previously
# the initial descriptor remained UiBoxLayout's default and only later edits used
# UpdateManagedLayoutItem(), causing document/Inspector/preview disagreement.
replace_one(
    'Utilities/UiDesigner/Preview/UiDesignerPreview.cpp',
'''    else if(auto *box = dynamic_cast<UiBoxLayout *>(&parent)) {
        layout_item_index = box->GetItemCount();
        box->Add(child);
    }
''',
'''    else if(auto *box = dynamic_cast<UiBoxLayout *>(&parent)) {
        layout_item_index = box->GetItemCount();
        UiBoxLayout::ItemRef item = box->Add(child);
        const bool horizontal = box->GetDirection() == UiDirection::H;
        const Size natural = max(child.GetMinSize(), Size(1, 1));
        const UiDesignerBoxSizing sizing = UiDesignerResolveBoxSizing(
            node, horizontal,
            horizontal ? natural.cx : natural.cy,
            horizontal ? natural.cy : natural.cx);

        if(sizing.main.mode == "Expand")
            item.Expand(max(1, sizing.weight));
        else if(sizing.main.mode == "Fixed")
            item.Fixed(max(1, sizing.main.fixed > 0
                              ? sizing.main.fixed : sizing.main.natural));
        else
            item.Fit();

        const int main_fixed = max(1, sizing.main.fixed > 0
                                      ? sizing.main.fixed : sizing.main.natural);
        const int main_min = sizing.main.mode == "Fixed"
            ? main_fixed : sizing.main.min;
        const int main_max = sizing.main.mode == "Fixed" ? main_fixed
            : (sizing.main.max > 0 ? max(sizing.main.max, main_min) : INT_MAX);
        item.MinMaxMain(main_min, main_max);

        if(sizing.cross.mode == "Expand") {
            const int cross_min = sizing.cross.min;
            const int cross_max = sizing.cross.max > 0
                ? max(sizing.cross.max, cross_min) : INT_MAX;
            item.MinMaxCross(cross_min, cross_max)
                .AlignSelf(UiCrossAlign::Stretch);
        }
        else {
            const int extent = sizing.cross.mode == "Fixed"
                ? max(1, sizing.cross.fixed > 0
                         ? sizing.cross.fixed : sizing.cross.natural)
                : sizing.cross.min;
            const int cross_max = sizing.cross.mode == "Fixed" ? extent
                : (sizing.cross.max > 0
                   ? max(sizing.cross.max, extent) : INT_MAX);
            item.MinMaxCross(extent, cross_max)
                .AlignSelf(UiDesignerResolveBoxAlign(sizing.cross_align));
        }
    }
''',
    'initial preview box attachment')

# Code-generated spacers must use the current axis model, with legacy fields only
# as a compatibility fallback for old documents.
replace_one(
    'Utilities/UiDesigner/CodeGen/UiDesignerCodeGen.cpp',
'''            const String main_mode = spacer.GetProperty(
                horizontal ? "h_sizing" : "v_sizing", "Auto");
            const String cross_mode = spacer.GetProperty(
                horizontal ? "v_sizing" : "h_sizing", "Auto");
''',
'''            const String main_mode = spacer.GetProperty(
                horizontal ? "width_mode" : "height_mode",
                spacer.GetProperty(horizontal ? "h_sizing" : "v_sizing", "Fit"));
            const String cross_mode = spacer.GetProperty(
                horizontal ? "height_mode" : "width_mode",
                spacer.GetProperty(horizontal ? "v_sizing" : "h_sizing", "Fit"));
''',
    'generated spacer sizing')

# Give the child adapter the authored parent so box code generation can resolve
# main and cross axes exactly as the preview does.
replace_one(
    'Utilities/UiDesigner/CodeGen/UiDesignerCodeGen.cpp',
'''struct UiDesignerChildAttachContext {
    String& out;
    const String& parent;
    const String& member;
    const UiDesignerNode& child;
    const String& title;
};
''',
'''struct UiDesignerChildAttachContext {
    String& out;
    const String& parent;
    const String& member;
    const UiDesignerNode& parent_node;
    const UiDesignerNode& child;
    const String& title;
};
''',
    'child attach context')

replace_one(
    'Utilities/UiDesigner/CodeGen/UiDesignerCodeGen.cpp',
'''static void AttachBox(UiDesignerChildAttachContext& c)
{
    c.out << "\\t" << c.parent << ".Add(" << c.member << ").Fit();\\n";
}
''',
'''static void AttachBox(UiDesignerChildAttachContext& c)
{
    const bool horizontal =
        AsString(c.parent_node.GetProperty("direction", "V")) == "H";
    const String main_mode = AsString(c.child.GetProperty(
        horizontal ? "width_mode" : "height_mode", "Fit"));
    const String cross_mode = AsString(c.child.GetProperty(
        horizontal ? "height_mode" : "width_mode", "Fit"));
    const int fixed_main = max(0, (int)c.child.GetProperty(
        horizontal ? "fixed_width" : "fixed_height", 0));
    const int fixed_cross = max(0, (int)c.child.GetProperty(
        horizontal ? "fixed_height" : "fixed_width", 0));
    const int min_main = max(0, (int)c.child.GetProperty(
        horizontal ? "min_width" : "min_height", 0));
    const int max_main = max(0, (int)c.child.GetProperty(
        horizontal ? "max_width" : "max_height", 0));
    const int min_cross = max(0, (int)c.child.GetProperty(
        horizontal ? "min_height" : "min_width", 0));
    const int max_cross = max(0, (int)c.child.GetProperty(
        horizontal ? "max_height" : "max_width", 0));
    const String cross_align = AsString(c.child.GetProperty(
        horizontal ? "cell_align_y" : "cell_align_x", "Center"));
    const int weight = max(1, (int)(double)c.child.GetProperty("weight", 1.0));

    String chain = c.parent + ".Add(" + c.member + ")";
    if(main_mode == "Expand")
        chain << ".Expand(" << weight << ")";
    else if(main_mode == "Fixed" && fixed_main > 0)
        chain << ".Fixed(DPI(" << fixed_main << "))";
    else
        chain << ".Fit()";

    if(main_mode == "Fixed" && fixed_main > 0)
        chain << ".MinMaxMain(DPI(" << fixed_main << "), DPI("
              << fixed_main << "))";
    else if(min_main || max_main)
        chain << ".MinMaxMain(DPI(" << min_main << "), "
              << (max_main ? "DPI(" + AsString(max_main) + ")" : "INT_MAX")
              << ")";

    if(cross_mode == "Fixed" && fixed_cross > 0)
        chain << ".MinMaxCross(DPI(" << fixed_cross << "), DPI("
              << fixed_cross << "))";
    else if(min_cross || max_cross)
        chain << ".MinMaxCross(DPI(" << min_cross << "), "
              << (max_cross ? "DPI(" + AsString(max_cross) + ")" : "INT_MAX")
              << ")";

    if(cross_mode == "Expand" || cross_align == "Stretch" ||
       cross_align == "Fill")
        chain << ".AlignSelf(UiCrossAlign::Stretch)";
    else
        chain << ".AlignSelf(" << BoxAlignExpr(cross_align) << ")";

    c.out << "\\t" << chain << ";\\n";
}
''',
    'generated box attachment')

replace_one(
    'Utilities/UiDesigner/CodeGen/UiDesignerCodeGen.cpp',
'''            UiDesignerChildAttachContext context{out, parent, member, *child, title};
''',
'''            UiDesignerChildAttachContext context{
                out, parent, member, node, *child, title};
''',
    'child attach context construction')

# Regression checks cover the document contract and generated parity. The exact
# emitted calls are the production contract that previously exposed the defect.
reg = Path('Utilities/UiDesigner/RegressionTests/main.cpp')
text = reg.read_text(encoding='utf-8')
anchor = '''    UiDesignerSession session;
    session.NewDocument("blank");
'''
if text.count(anchor) != 1:
    raise RuntimeError('regression insertion anchor changed')
block = '''    UiDesignerSession dialog_session;
    dialog_session.NewDocument("dialog");
    const UiDesignerDocument& dialog_document = dialog_session.Document();
    const UiDesignerNode *dialog_actions = nullptr;
    for(const UiDesignerNode& candidate : dialog_document.GetNodes())
        if(candidate.name == "dialog_actions") {
            dialog_actions = &candidate;
            break;
        }
    Check(dialog_actions &&
              AsString(dialog_actions->GetProperty("direction", "")) == "H" &&
              AsString(dialog_actions->GetProperty("wrap", "")) == "Flow",
          "Dialog actions author horizontal Flow layout before any Inspector toggle");
    if(dialog_actions) {
        Check(dialog_actions->children.GetCount() == 3,
              "Dialog actions retain Spacer, Cancel and OK source order");
        if(dialog_actions->children.GetCount() == 3) {
            const UiDesignerNode *spacer_node =
                dialog_document.Find(dialog_actions->children[0]);
            const UiDesignerNode *cancel_node =
                dialog_document.Find(dialog_actions->children[1]);
            const UiDesignerNode *ok_node =
                dialog_document.Find(dialog_actions->children[2]);
            Check(spacer_node && spacer_node->type == "Spacer" &&
                      AsString(spacer_node->GetProperty("width_mode", "")) == "Expand",
                  "Dialog action spacer expands on the horizontal main axis");
            Check(cancel_node && ok_node &&
                      AsString(cancel_node->GetProperty("width_mode", "")) == "Fixed" &&
                      AsString(ok_node->GetProperty("width_mode", "")) == "Fixed" &&
                      (int)cancel_node->GetProperty("fixed_width", 0) == 88 &&
                      (int)ok_node->GetProperty("fixed_width", 0) == 88,
                  "Dialog buttons retain fixed widths");
        }
    }
    UiDesignerGeneratedProject dialog_generated =
        UiDesignerCodeGenerator(dialog_session.Catalog()).Generate(
            dialog_document, "DialogParityWindow");
    Check(dialog_generated.source.Find("dialog_column_n2.Add(dialog_content_n4).Expand(1)") >= 0,
          "Generated dialog content expands instead of defaulting to Fit");
    Check(dialog_generated.source.Find("dialog_column_n2.Add(dialog_actions_n5).Fixed(DPI(40))") >= 0,
          "Generated dialog action row uses its fixed main-axis height");
    Check(dialog_generated.source.Find("dialog_actions_n5.AddSpacer().Expand(1)") >= 0,
          "Generated dialog preserves the expanding semantic spacer");
    Check(dialog_generated.source.Find("dialog_actions_n5.Add(cancel_button_n7).Fixed(DPI(88))") >= 0 &&
              dialog_generated.source.Find("dialog_actions_n5.Add(ok_button_n8).Fixed(DPI(88))") >= 0,
          "Generated dialog buttons use their authored fixed widths");

'''
reg.write_text(text.replace(anchor, block + anchor), encoding='utf-8')

# Remove the repository-side patch machinery from the resulting source commit.
for temporary in [
    '.github/uidesigner-box-parity-fix.py',
    '.github/workflows/uidesigner-box-parity-fix.yml',
]:
    p = Path(temporary)
    if p.exists():
        p.unlink()
