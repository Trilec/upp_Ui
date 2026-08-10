#include <Utilities/PropertyEditor/PropertyEditor.h>

using namespace Upp;

struct TestCtx {
    int checks = 0;
    int fails = 0;
    void Expect(bool ok, const char *text) {
        checks++;
        if(!ok) { fails++; Cout() << "[FAIL] " << text << "\n"; }
    }
};

CONSOLE_APP_MAIN
{
    TestCtx t;

    PropertyEditorModel model;
    PropertyEditorItem& radius = model.AddInteger("radius", "Corner Radius", 8, "Appearance/Border");
    radius.SetDomain(PropertyEditorDomain::Theme).SetRowSpan(1);
    PropertyEditorItem& enabled = model.AddBoolean("enabled", "Enabled", true, "Appearance");
    enabled.SetBooleanPresentation(PropertyBooleanPresentation::OnOff);
    PropertyEditorItem& visible = model.AddBoolean("visible", "Visible", false, "Appearance");
    visible.SetBooleanPresentation(PropertyBooleanPresentation::TrueFalse);
    model.AddColor("accent", "Accent", Color(220, 54, 52), "Appearance/Colour");
    model.AddDouble("mixed", "Mixed", 0.0, "Appearance").SetMixed();
    ValueArray eight_colours;
    for(int i = 0; i < 8; i++)
        eight_colours.Add(Color(i * 20, i * 15, i * 10));
    model.Add("palette8", "Palette", PropertyEditorKind::ColorPalette,
              eight_colours, "Appearance/Colour").SetColorCount(8).SetInlineEditor();
    model.AddMultiline("notes", "Notes", "First\nSecond", "Content")
         .SetExpandedRowSpan(3).SetInlineEditor();
    AddPropertyRange(model, "range", "Allowed Range", 20, 80, 0, 100, 1, "Layout");
    AddPropertyAdjustableRange(model, "adjustable_range", "Adjustable Range",
                               0, 10, 20, 80, 90, 100, 1, "Layout");
    AddPropertyMatrix(model, "anchor", "Anchor", "center", "Position9", "Layout/Position");
    AddPropertyIcon(model, "icon", "Icon", "ICON_DESIGN_HOME_48", "Content");
    AddPropertyFont(model, "font", "Font", Font::GetFaceCount() ? Font::GetFaceName(0) : String(), "Content");
    AddPropertyImage(model, "image", "Image", "hero.png", "test-image", "Content");
    t.Expect(model.Find("image")->expanded_row_span == 3,
             "image adapter defaults to a three-row expanded preview");

    for(int i = 0; i < 40; i++)
        model.AddBoolean("flag" + AsString(i), "Flag " + AsString(i), (i & 1) != 0, "Stress");
    model.StructureChanged();

    t.Expect(radius.kind == PropertyEditorKind::Integer, "theme integer starts as Integer");
    t.Expect(IsNull(radius.minimum) && IsNull(radius.maximum), "theme integer has no injected range");
    t.Expect(radius.row_span == 1, "row span metadata retained");
    t.Expect(enabled.boolean_presentation == PropertyBooleanPresentation::OnOff,
             "boolean presentation metadata retained");
    radius.SetIndent(2);
    t.Expect(radius.indent == 2, "property indentation has a public metadata API");

    PropertyEditorFactory& factory = PropertyEditorFactory::Global();
    RegisterPropertyEditorV1Editors(factory);
    t.Expect(factory.HasCustom(PropertyEditorRangeDoubleId()), "range editor registered");
    t.Expect(factory.HasCustom(PropertyEditorAdjustableRangeId()),
             "adjustable-domain range editor registered");
    t.Expect(factory.HasCustom(PropertyEditorMatrixId()), "matrix editor registered");
    t.Expect(factory.HasCustom(PropertyEditorIconId()), "icon editor registered");
    t.Expect(factory.HasCustom(PropertyEditorFontId()), "font editor registered");
    t.Expect(factory.HasCustom(PropertyEditorImageId()), "image editor registered");
    t.Expect(factory.Create(*model.Find("range")), "range editor creates");
    t.Expect(factory.Create(*model.Find("anchor")), "matrix editor creates");
    t.Expect(model.Find("anchor")->row_span == 1 &&
             model.Find("anchor")->expanded_row_span == 3,
             "matrix adapter declares compact and expanded row spans");
    t.Expect(factory.Create(*model.Find("icon")), "icon editor creates");
    t.Expect(factory.Create(*model.Find("font")), "font editor creates");
    t.Expect(factory.Create(*model.Find("image")), "image editor creates");

    for(int kind = (int)PropertyEditorKind::Text;
        kind <= (int)PropertyEditorKind::ReadOnly; kind++) {
        PropertyEditorItem item;
        item.kind = (PropertyEditorKind)kind;
        item.value = kind == (int)PropertyEditorKind::Color ? Value(Color(1, 2, 3)) : Value(0);
        if(item.kind == PropertyEditorKind::ColorPalette)
            item.value = Value(ValueArray());
        if(item.kind == PropertyEditorKind::FillRecipe)
            item.value = Value(ValueMap());
        t.Expect(factory.Create(item), "every built-in property kind has a visual editor");
    }

    UiFloatEdit float_edit;
    float_edit.SetData(Null);
    t.Expect(float_edit.IsEmpty() && IsNull(float_edit.GetData()),
             "null floating-point data remains empty instead of formatting DBL_NULL");
    float_edit.SetTextUtf8("-1.25e3");
    t.Expect(fabs((double)float_edit.GetData() + 1250.0) < 0.0001,
             "floating-point editor accepts scientific notation");
    double parsed = 0;
    float_edit.SetTextUtf8("7e");
    t.Expect(!float_edit.TryGetValue(parsed) && !float_edit.IsInputComplete(),
             "partial scientific notation remains an incomplete edit");
    float_edit.SetTextUtf8("7e5");
    t.Expect(float_edit.TryGetValue(parsed) && fabs(parsed - 700000.0) < 0.0001,
             "completed scientific notation parses without model validation churn");
    float_edit.SetTextUtf8("+2.5e-2");
    t.Expect(float_edit.TryGetValue(parsed) && fabs(parsed - 0.025) < 0.0001,
             "signed mantissa and exponent syntax is accepted");

    t.Expect(model.Find("palette8")->color_count == 8,
             "colour palette metadata supports eight ordered slots");
    String palette_error;
    t.Expect(model.Commit("palette8", eight_colours, &palette_error),
             "eight-colour palette passes canonical model validation");
    String mixed_error;
    t.Expect(model.Commit("mixed", 12.5, &mixed_error) &&
             !model.Find("mixed")->mixed && (double)model.Find("mixed")->value == 12.5,
             "committing one value resolves a mixed multi-selection state");

    UiColorPicker picker;
    picker.EnableSessionPersistence(false);
    picker.SetRect(0, 0, 720, 520);
    picker.SetSlotCount(1);
    picker.Layout();
    t.Expect(picker.GetSlotCount() == 1,
             "caller-authored single-colour picker count is authoritative on first layout");
    picker.SetSlotCount(4);
    picker.Layout();
    t.Expect(picker.GetSlotCount() == 4,
             "caller-authored quad picker count updates immediately");
    picker.SetSlotCount(8);
    picker.Layout();
    t.Expect(picker.GetSlotCount() == 8,
             "caller-authored eight-slot picker count updates immediately");

    PropertyEditorItem mixed_double;
    mixed_double.kind = PropertyEditorKind::Double;
    mixed_double.value = 0.0;
    mixed_double.mixed = true;
    One<PropertyValueEditor> mixed_editor = factory.Create(mixed_double);
    mixed_editor->Configure(mixed_double);
    mixed_editor->SetEditorValue(mixed_double.value, true);
    t.Expect(IsNull(mixed_editor->GetEditorValue()),
             "mixed floating-point editor exposes an empty null value without sentinel text");

    factory.RegisterPicker("test-image", [](Value& v, Ctrl*) {
        v = "picked.png";
        return true;
    });
    factory.RegisterThumbnailProvider("test-image", [](const Value&) {
        return ICON_DESIGN_IMAGE_48();
    });
    Value image = "old.png";
    t.Expect(factory.PickValue("test-image", image, nullptr) && AsString(image) == "picked.png",
             "provider picker edits accepted image value");
    t.Expect(!factory.ResolveThumbnail("test-image", image).IsEmpty(),
             "provider resolves a compact image thumbnail independently of its value");

    PropertyEditor editor;
    editor.SetRect(0, 0, 360, 180);
    editor.SetModel(&model);
    editor.Layout();
    t.Expect(radius.kind == PropertyEditorKind::Integer,
             "visual rebuild never mutates theme kind");
    t.Expect(IsNull(radius.minimum) && IsNull(radius.maximum),
             "visual rebuild never injects theme bounds");
    t.Expect(editor.GetInlineEditorCount() < 20,
             "rich inline controls are bounded to the viewport");
    editor.SetPropertyExpanded("anchor", true);
    editor.Layout();
    t.Expect(editor.IsPropertyExpanded("anchor"),
             "PropertyEditor owns temporary expanded-row state");
    editor.SetPropertyExpanded("anchor", false);
    t.Expect(!editor.IsPropertyExpanded("anchor"),
             "expanded rich editor returns to one compact row");
    t.Expect(editor.SelectProperty("notes", true) && editor.IsPropertyExpanded("notes"),
             "activating a compact expandable row opens its inline editor");

    UiSlider slider;
    slider.SetRect(0, 0, 320, 28);
    int natural_track = slider.GetTrackGeometry().GetWidth();
    slider.ExpandTrack();
    int expanded_track = slider.GetTrackGeometry().GetWidth();
    t.Expect(natural_track <= DPI(120) && expanded_track > natural_track,
             "slider expanding-track mode consumes available horizontal space");

    editor.SetGroupOpen("Appearance", false);
    t.Expect(!editor.SelectProperty("radius"), "collapsed nested row is normally hidden");
    editor.SetFilter("Corner Radius");
    t.Expect(editor.SelectProperty("radius"), "filter reveals match inside collapsed nested group");
    t.Expect(editor.GetDisplayRowCount() <= 4, "filter reduces display rows to match and ancestors");
    editor.SetFilter(String());
    editor.SetGroupOpen("Appearance", true);

    editor.SetLabelAuto();
    t.Expect(editor.GetLabelMode() == PropertyEditorLabelMode::Auto, "label Auto mode available");
    editor.SetLabelWidth(118);
    t.Expect(editor.GetLabelMode() == PropertyEditorLabelMode::Fixed && editor.GetLabelWidth() == 118,
             "label Fixed mode available");
    editor.SetLabelRatio(45);
    t.Expect(editor.GetLabelMode() == PropertyEditorLabelMode::Ratio && editor.GetLabelRatio() == 45,
             "label Ratio mode available");

    int undo_requests = 0;
    editor.WhenUndoRequest = [&](String id) { if(id == "radius") undo_requests++; };
    t.Expect(editor.SelectProperty("radius", true), "numeric row activates for transaction test");
    const int original_radius = (int)model.Find("radius")->value;
    model.Preview("radius", 33);
    editor.Key(K_ESCAPE, 1);
    t.Expect((int)model.Find("radius")->value == original_radius,
             "Escape restores transaction origin");
    editor.SelectProperty("radius");
    editor.Key(K_CTRL_Z, 1);
    t.Expect(undo_requests == 1, "Ctrl+Z delegates one undo request to host");

    editor.SelectProperty("enabled");
    bool before = (bool)model.Find("enabled")->value;
    editor.Key(K_ENTER, 1);
    t.Expect((bool)model.Find("enabled")->value != before,
             "OnOff boolean toggles directly without checkbox editor");
    editor.SelectProperty("visible");
    before = (bool)model.Find("visible")->value;
    editor.Key(K_ENTER, 1);
    t.Expect((bool)model.Find("visible")->value != before,
             "TrueFalse boolean toggles directly without checkbox editor");

    Value range = model.Find("range")->value;
    ValueArray pair = range;
    t.Expect(pair.GetCount() == 2, "range value uses two-element array contract");
    ValueArray reversed;
    reversed.Add(90.0); reversed.Add(10.0);
    String error;
    t.Expect(model.Commit("range", reversed, &error), "range custom normalization commits");
    pair = model.Find("range")->value;
    t.Expect((double)pair[0] == 10.0 && (double)pair[1] == 90.0,
             "range normalization orders endpoints");
    ValueArray adjustable;
    adjustable.Add(100.0); adjustable.Add(90.0); adjustable.Add(10.0); adjustable.Add(0.0);
    t.Expect(model.Commit("adjustable_range", adjustable, &error),
             "adjustable range accepts its four-value contract");
    adjustable = model.Find("adjustable_range")->value;
    t.Expect(adjustable.GetCount() == 4 && (double)adjustable[0] == 0.0 &&
             (double)adjustable[1] == 10.0 && (double)adjustable[2] == 90.0 &&
             (double)adjustable[3] == 100.0,
             "adjustable range normalizes domain and selected interval together");

    PropertyEditorStyle style = editor.GetStyle();
    t.Expect(style.action_icons.size == DPI(16),
             "compact PropertyEditor action icons default to 16 pixels");
    t.Expect(!style.reset_icon.IsEmpty() && !style.action_icons.expand.IsEmpty() &&
             !style.action_icons.collapse.IsEmpty() && !style.action_icons.dialog.IsEmpty() &&
             !style.action_icons.browse.IsEmpty(),
             "reset, expansion, dialog and browse imagery are style-configurable");
    style.group_font = StdFont().Bold().Height(15);
    style.action_icons.size = DPI(15);
    style.label_font = StdFont().Height(13);
    style.value_font = StdFont().Height(13);
    editor.SetStyle(style);
    t.Expect(editor.GetStyle().action_icons.size == DPI(15),
             "PropertyEditor accepts application-authored action icon geometry");
    t.Expect(editor.GetStyle().group_font.GetHeight() == 15,
             "group font is style-configurable");
    t.Expect(editor.GetStyle().label_font.GetHeight() == 13 &&
             editor.GetStyle().value_font.GetHeight() == 13,
             "label and value fonts are style-configurable");

    editor.SetGroupAction("Appearance", "Reset");
    t.Expect(editor.GetGroupAction("Appearance") == "Reset",
             "group action metadata is configurable");

    Vector<Pointf> curve_points;
    curve_points.Add(Pointf(0.0, 0.0));
    curve_points.Add(Pointf(0.5, 0.25));
    curve_points.Add(Pointf(1.0, 1.0));
    const String curve_summary = PropertyEditorFormatCurve(PropertyEditorMakeCurve(curve_points));
    t.Expect(curve_summary.Find("3 points") >= 0 &&
             curve_summary.Find("(0.0000, 0.0000)") >= 0 &&
             curve_summary.Find("...") >= 0,
             "curve summary exposes count, bounded coordinates and continuation");

    PropertyEditorModel stress;
    for(int i = 0; i < 1000; i++) {
        const String id = "stress_" + AsString(i);
        const String group = "Stress/Group " + AsString(i / 100);
        if((i % 4) == 0)
            stress.AddColor(id, "Colour " + AsString(i),
                            Color(i % 255, (i * 3) % 255, (i * 7) % 255), group);
        else
            stress.AddBoolean(id, "Flag " + AsString(i), (i & 1) != 0, group);
    }
    stress.StructureChanged();
    editor.SetModel(&stress);
    editor.SetRect(0, 0, 360, 180);
    editor.Layout();
    t.Expect(editor.GetStyle().filter_height >= DPI(36),
             "default filter has unclipped 36px geometry");
    t.Expect(editor.GetInlineEditorCount() < 20,
             "one-thousand-row model keeps inline editor count viewport-bounded");
    for(int pass = 0; pass < 25; pass++) {
        editor.SetFilter((pass & 1) ? "Colour 9" : "Flag 8");
        editor.Layout();
        t.Expect(editor.GetInlineEditorCount() < 20,
                 "repeated filtering keeps inline editor count bounded");
    }
    editor.SetFilter(String());
    for(int pass = 0; pass < 25; pass++) {
        editor.MouseWheel(Point(180, 100), -120, 0);
        editor.Layout();
        t.Expect(editor.GetInlineEditorCount() < 20,
                 "repeated scrolling keeps inline editor count bounded");
    }
    for(int pass = 0; pass < 10; pass++) {
        editor.CollapseAll();
        editor.ExpandAll();
        editor.Layout();
        t.Expect(editor.GetInlineEditorCount() < 20,
                 "repeated group lifecycle keeps inline editor count bounded");
    }
    PropertyEditorModel replacement;
    replacement.AddColor("replacement", "Replacement", Color(1, 2, 3), "Current");
    replacement.StructureChanged();
    for(int pass = 0; pass < 10; pass++) {
        editor.SetModel((pass & 1) ? &stress : &replacement);
        editor.Layout();
        t.Expect(editor.GetInlineEditorCount() < 20,
                 "model replacement removes stale inline editors");
    }
    editor.SetModel(nullptr);
    editor.Layout();
    t.Expect(editor.GetInlineEditorCount() == 0,
             "detaching the model destroys all inline editors");

    Cout() << "PropertyEditorV1RunTests: Checks: " << t.checks
           << " Fails: " << t.fails << "\n";
    SetExitCode(t.fails ? 1 : 0);
}
