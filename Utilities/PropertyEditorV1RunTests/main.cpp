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
    model.AddColor("accent", "Accent", Color(220, 54, 52), "Appearance/Colour");
    AddPropertyRange(model, "range", "Allowed Range", 20, 80, 0, 100, 1, "Layout");
    AddPropertyMatrix(model, "anchor", "Anchor", "center", "Position9", "Layout/Position");
    AddPropertyIcon(model, "icon", "Icon", "ICON_DESIGN_HOME_48", "Content");
    AddPropertyFont(model, "font", "Font", Font::GetFaceCount() ? Font::GetFaceName(0) : String(), "Content");
    AddPropertyImage(model, "image", "Image", "hero.png", "test-image", "Content");

    for(int i = 0; i < 40; i++)
        model.AddBoolean("flag" + AsString(i), "Flag " + AsString(i), (i & 1) != 0, "Stress");
    model.StructureChanged();

    t.Expect(radius.kind == PropertyEditorKind::Integer, "theme integer starts as Integer");
    t.Expect(IsNull(radius.minimum) && IsNull(radius.maximum), "theme integer has no injected range");
    t.Expect(radius.row_span == 1, "row span metadata retained");
    t.Expect(enabled.boolean_presentation == PropertyBooleanPresentation::OnOff,
             "boolean presentation metadata retained");

    PropertyEditorFactory& factory = PropertyEditorFactory::Global();
    RegisterPropertyEditorV1Editors(factory);
    t.Expect(factory.HasCustom(PropertyEditorRangeDoubleId()), "range editor registered");
    t.Expect(factory.HasCustom(PropertyEditorMatrixId()), "matrix editor registered");
    t.Expect(factory.HasCustom(PropertyEditorIconId()), "icon editor registered");
    t.Expect(factory.HasCustom(PropertyEditorFontId()), "font editor registered");
    t.Expect(factory.HasCustom(PropertyEditorImageId()), "image editor registered");
    t.Expect(factory.Create(*model.Find("range")), "range editor creates");
    t.Expect(factory.Create(*model.Find("anchor")), "matrix editor creates");
    t.Expect(factory.Create(*model.Find("icon")), "icon editor creates");
    t.Expect(factory.Create(*model.Find("font")), "font editor creates");
    t.Expect(factory.Create(*model.Find("image")), "image editor creates");

    factory.RegisterPicker("test-image", [](Value& v, Ctrl*) {
        v = "picked.png";
        return true;
    });
    Value image = "old.png";
    t.Expect(factory.PickValue("test-image", image, nullptr) && AsString(image) == "picked.png",
             "provider picker edits accepted image value");

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

    PropertyEditorStyle style = editor.GetStyle();
    style.group_font = StdFont().Bold().Height(15);
    style.label_font = StdFont().Height(13);
    style.value_font = StdFont().Height(13);
    editor.SetStyle(style);
    t.Expect(editor.GetStyle().group_font.GetHeight() == 15,
             "group font is style-configurable");
    t.Expect(editor.GetStyle().label_font.GetHeight() == 13 &&
             editor.GetStyle().value_font.GetHeight() == 13,
             "label and value fonts are style-configurable");

    editor.SetGroupAction("Appearance", "Reset");
    t.Expect(editor.GetGroupAction("Appearance") == "Reset",
             "group action metadata is configurable");

    Cout() << "PropertyEditorV1RunTests: Checks: " << t.checks
           << " Fails: " << t.fails << "\n";
    SetExitCode(t.fails ? 1 : 0);
}
