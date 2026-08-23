#include <Utilities/PropertyEditor/PropertyEditor.h>

#include <cmath>

using namespace Upp;

static int checks = 0;
static int fails = 0;

class PropertyEditorTestCustomEditor : public PropertyValueEditor {
public:
    typedef PropertyEditorTestCustomEditor CLASSNAME;

    PropertyEditorTestCustomEditor()
    {
        Add(edit_.SizePos());
        edit_.WhenAction = [=] {
            if(!syncing_) {
                WhenPreview(edit_.GetData());
                WhenCommit(edit_.GetData());
            }
        };
    }

    virtual void Configure(const PropertyEditorItem& item) override
    {
        edit_.Enable(item.enabled && !item.read_only);
    }

    virtual void SetEditorValue(const Value& value, bool mixed) override
    {
        syncing_ = true;
        edit_.SetData(mixed ? Value(String()) : value);
        syncing_ = false;
    }

    virtual Value GetEditorValue() const override
    {
        return edit_.GetData();
    }

    virtual void FocusEditor() override
    {
        edit_.SetFocus();
        edit_.SetSelection();
    }

private:
    UiLineEdit edit_;
    bool syncing_ = false;
};

static void Check(bool condition, const char *message)
{
    checks++;
    if(!condition) {
        fails++;
        Cout() << "FAIL: " << message << "\n";
    }
}

CONSOLE_APP_MAIN
{
    PropertyEditorModel model;

    model.AddText("title", "Title", "PropertyEditor utility", "General")
         .SetDefault("Untitled");

    model.AddMultiline("notes", "Notes", "Line 1\nLine 2", "General");

    model.AddBoolean("enabled", "Enabled", true, "General");

    model.AddInteger("count", "Count", 4, "General")
         .SetRange(0, 10, 1)
         .SetDefault(4);

    model.AddDouble("ratio", "Ratio", 0.25, "General")
         .SetRange(0.0, 1.0, 0.01);

    model.AddChoice("mode", "Mode", 1, "General")
         .AddChoice(0, "A")
         .AddChoice(1, "B");

    model.AddSliderInt("steps", "Steps", 4, 0, 10, 2, "General");

    model.AddVector3("position", "Position", 1.0, 2.0, 3.0, "Transform");

    Vector<Pointf> curve_points;
    curve_points.Add(Pointf(1.0, 1.0));
    curve_points.Add(Pointf(0.0, 0.0));
    curve_points.Add(Pointf(0.5, 0.25));
    model.AddCurve("curve", "Curve", PropertyEditorMakeCurve(curve_points), "Advanced");
    PropertyEditorItem& bezier = model.AddBezierCurve(
        "bezier", "Bezier", PropertyEditorMakeBezierCurve(-0.2, 0.25, 0.8, 1.4),
        "Advanced");
    Check(bezier.kind == PropertyEditorKind::Curve &&
              bezier.editor_variant == "bezier" && bezier.expanded_row_span == 4,
          "Bezier curve uses the shared semantic Curve editor variant");
    ValueArray normalized_bezier = bezier.value;
    Check(normalized_bezier.GetCount() == 4 &&
              (double)normalized_bezier[0] == 0.0 &&
              (double)normalized_bezier[1] == 0.25 &&
              (double)normalized_bezier[2] == 0.8 &&
              (double)normalized_bezier[3] == 1.4,
          "Bezier curve constrains time while preserving easing overshoot");
    Value bounded_bezier;
    String bounded_error;
    bezier.SetRange(-1.0, 1.0, 0.01);
    Check(PropertyEditorNormalizeValue(bezier, bezier.value,
                                       bounded_bezier, bounded_error) &&
              (double)ValueArray(bounded_bezier)[3] == 1.0,
          "Bezier properties can explicitly bound their output axis");
    Check(PropertyEditorFormatBezierCurve(bezier.value).StartsWith("cubic-bezier("),
          "Bezier curve has a stable compact summary");

    PropertyEditorFactory::Global().RegisterCustom(
        "test-custom",
        [] { return One<PropertyValueEditor>(new PropertyEditorTestCustomEditor); });
    PropertyEditorItem& custom =
        model.Add("custom", "Custom", PropertyEditorKind::Custom, "seed", "Advanced");
    custom.custom_editor = "test-custom";

    model.StructureChanged();

    const int subtitle_revision = model.GetStructureRevision();
    model.SetGroupSubtitle("General", "10 local");
    Check(model.GetGroupSubtitle("General") == "10 local",
          "group subtitle can be set");
    model.SetGroupSubtitle("General", "8 local");
    Check(model.GetGroupSubtitle("General") == "8 local" &&
              model.GetStructureRevision() == subtitle_revision,
          "group subtitle replacement is non-structural");
    model.SetGroupSubtitle("Transform", "2 local");
    Check(model.GetGroupSubtitle("Transform") == "2 local" &&
              model.GetGroupSubtitle("General") == "8 local",
          "independent group subtitles are retained");
    model.ClearGroupSubtitle("General");
    Check(model.GetGroupSubtitle("General").IsEmpty() &&
              model.GetGroupSubtitle("Transform") == "2 local",
          "one group subtitle can be cleared independently");
    model.ClearGroupSubtitles();
    Check(model.GetGroupSubtitle("Transform").IsEmpty(),
          "ClearGroupSubtitles removes stale metadata");

    Check(model.GetCount() == 11, "model item count");
    Check(model.Find("count") != nullptr, "find property");
    Check(model.Find("notes") != nullptr, "find multiline property");
    Check(model.FindIndex("missing") < 0, "missing property");
    Check(model.Find("notes")->kind == PropertyEditorKind::Multiline, "multiline kind");
    Check(model.Find("steps")->kind == PropertyEditorKind::SliderInt, "slider int kind");
    Check(PropertyEditorFactory::Global().HasCustom("test-custom"),
          "custom editor registered");
    Check(PropertyEditorFactory::Global().Create(*model.Find("custom")),
          "custom editor created");
    Check(PropertyEditorFactory::Global().Create(*model.Find("title")),
          "text editor created");
    Check(PropertyEditorFactory::Global().Create(*model.Find("notes")),
          "multiline editor created");
    Check(PropertyEditorFactory::Global().Create(*model.Find("enabled")),
          "boolean editor created");
    Check(PropertyEditorFactory::Global().Create(*model.Find("count")),
          "integer editor created");
    Check(PropertyEditorFactory::Global().Create(*model.Find("ratio")),
          "double editor created");
    Check(PropertyEditorFactory::Global().Create(*model.Find("mode")),
          "choice editor created");
    Check(PropertyEditorFactory::Global().Create(*model.Find("steps")),
          "slider editor created");
    Check(PropertyEditorFactory::Global().Create(*model.Find("position")),
          "vector editor created");
    Check(PropertyEditorFactory::Global().Create(*model.Find("curve")),
          "curve editor created");

    String error;
    Check(model.Preview("count", "8", &error), "integer string preview");
    Check((int)model.Find("count")->value == 8, "integer normalized");
    Check(model.Preview("notes", "Line 1\nLine 3", &error), "multiline preview");
    Check(AsString(model.Find("notes")->value).Find('\n') >= 0,
          "multiline preserved");

    Check(model.Commit("count", 100, &error), "integer commit");
    Check((int)model.Find("count")->value == 10, "integer clamped to maximum");
    Check(model.Commit("steps", 5, &error), "slider int commit");
    Check((int)model.Find("steps")->value == 6, "slider int snapped to step");

    Check(!model.Preview("count", "bad", &error), "invalid integer rejected");
    Check(!error.IsEmpty(), "invalid integer error");

    Check(model.Preview("ratio", "0.75", &error), "double string preview");
    Check(fabs((double)model.Find("ratio")->value - 0.75) < 0.000001,
          "double normalized");

    Check(!model.Commit("mode", 9, &error), "invalid choice rejected");
    Check(model.Commit("mode", 0, &error), "valid choice committed");

    Check(model.Preview("position", "4, 5, 6", &error), "vector string accepted");
    Vector<double> position =
        PropertyEditorReadVector(model.Find("position")->value, 3);
    Check(position.GetCount() == 3, "vector component count");
    Check(position[0] == 4 && position[1] == 5 && position[2] == 6,
          "vector components normalized");

    Vector<Pointf> curve =
        PropertyEditorReadCurve(model.Find("curve")->value);
    Check(curve.GetCount() == 3, "curve point count");
    Check(curve[0].x == 0.0 && curve.Top().x == 1.0,
          "curve sorted and normalized");

    int preview_count = 0;
    int commit_count = 0;
    int reset_count = 0;

    model.WhenPreview = [&](String, Value) { preview_count++; };
    model.WhenCommit = [&](String, Value) { commit_count++; };
    model.WhenReset = [&](String) { reset_count++; };

    Check(model.Preview("ratio", 0.5, &error), "preview event input");
    Check(model.Commit("ratio", 0.6, &error), "commit event input");
    Check(model.Reset("count", &error), "reset property");

    Check(preview_count == 1, "preview event count");
    Check(commit_count == 2, "commit includes reset commit");
    Check(reset_count == 1, "reset event count");
    Check((int)model.Find("count")->value == 4, "reset default value");

    PropertyEditorStyle light = PropertyEditorStyle::Light();
    PropertyEditorStyle dark = PropertyEditorStyle::Dark();
    Check(RelativeLuminance(light.background) > RelativeLuminance(dark.background),
          "light and dark palettes differ");
    Check(light.row_odd != light.row_even, "light alternate rows");
    Check(dark.row_odd != dark.row_even, "dark alternate rows");

    UiStack property_host;
    PropertyEditor hosted_editor;
    property_host.Add(hosted_editor, "properties");
    property_host.SetRect(0, 0, 320, 240);
    Check(property_host.GetCount() == 1 && hosted_editor.GetSize() == Size(320, 240),
          "property editor survives stack-hosted layout");

    // Structural replacement must invalidate the active editor before U++
    // sends its LostFocus callback back through the old row model.
    PropertyEditorModel live_model;
    live_model.AddInteger("inset", "Inset", 8, "Layout");
    live_model.AddInteger("gap", "Gap", 8, "Layout");
    live_model.StructureChanged();
    PropertyEditor live_editor;
    property_host.Add(live_editor, "live-properties");
    live_editor.SetModel(&live_model);
    Check(live_editor.SelectProperty("inset", true),
          "activate inset editor before model replacement");
    live_model.Clear(false);
    live_model.AddInteger("gap", "Gap", 8, "Layout");
    live_model.StructureChanged();
    Check(live_model.Find("inset") == nullptr && live_model.Find("gap") != nullptr,
          "replacement model contains only current properties");
    Check(live_editor.SelectProperty("gap", true),
          "switch directly from replaced inset to gap");
    Check(live_model.GetCount() == 1, "replacement does not retain stale rows");
    live_model.Clear(false);
    live_model.AddInteger("inset", "Inset", 8, "Layout");
    live_model.StructureChanged();
    Check(live_model.Find("inset") != nullptr,
          "selection replacement can restore inset safely");

    PropertyEditorItem missing_custom;
    missing_custom.kind = PropertyEditorKind::Custom;
    missing_custom.custom_editor = "missing";
    Check(!PropertyEditorFactory::Global().Create(missing_custom),
          "missing custom editor returns null");

    PropertyEditorModel override_model;
    PropertyEditorItem& override_item =
        override_model.AddChoice("surface", "Surface", "UseTheme", "Surface");
    override_item.AddChoice("UseTheme", "Use theme")
                 .AddChoice("Solid", "Solid");
    override_item.overrideable = true;
    override_item.override_active = false;
    override_item.inherited = true;
    override_item.enabled = true;
    override_item.value_editable = false;
    override_model.StructureChanged();
    Check(override_item.enabled && !override_item.value_editable,
          "inherited override keeps row action enabled while value editor is locked");

    PropertyEditor override_editor;
    override_editor.SetRect(0, 0, 320, 160);
    override_editor.SetModel(&override_model);
    int override_requests = 0;
    override_editor.WhenOverride = [&](String id, bool active) {
        override_requests++;
        Check(id == "surface" && active,
              "override request identifies inherited activation");
        PropertyEditorItem* item = override_model.Find(id);
        item->override_active = true;
        item->inherited = false;
        item->value_editable = true;
    };
    Check(override_editor.SelectProperty("surface"),
          "select inherited override row");
    override_editor.Key(K_ENTER, 1);
    Check(override_requests == 1,
          "Enter requests inherited override activation exactly once");
    override_editor.Key(K_SPACE, 1);
    Check(override_requests == 1,
          "Space opens active value editing without another activation request");

    PropertyEditor mouse_override_editor;
    mouse_override_editor.SetRect(0, 0, 320, 160);
    mouse_override_editor.SetModel(&override_model);
    override_model.Find("surface")->override_active = false;
    override_model.Find("surface")->inherited = true;
    override_model.Find("surface")->value_editable = false;
    override_model.Find("surface")->enabled = false;
    int mouse_requests = 0;
    mouse_override_editor.WhenOverride = [&](String id, bool active) {
        mouse_requests++;
        Check(id == "surface" && active,
              "mouse override request identifies inherited activation");
    };
    Check(mouse_override_editor.SelectProperty("surface"),
          "select inherited row for mouse activation");

    // Derive the click from the live PropertyEditor geometry instead of a
    // stale absolute Y coordinate. The filter/group spacing changed as the
    // editor matured; the contract under test is the row interaction itself.
    mouse_override_editor.Layout();
    const PropertyEditorStyle& mouse_style = mouse_override_editor.GetStyle();
    const int mouse_y = mouse_style.frame_width + mouse_style.filter_height +
                        max(0, mouse_style.filter_gap) + mouse_style.group_height +
                        mouse_style.row_height / 2;
    const int mouse_override_x = mouse_override_editor.GetSize().cx -
                                 mouse_style.frame_width -
                                 max(1, mouse_style.override_width / 2);
    const int mouse_body_x = max(mouse_style.frame_width + DPI(8),
                                 mouse_override_x - mouse_style.override_width - DPI(24));

    mouse_override_editor.LeftDown(Point(mouse_override_x, mouse_y), 0);
    Check(mouse_requests == 1,
          "mouse circle requests inherited activation exactly once");
    mouse_override_editor.LeftDown(Point(mouse_body_x, mouse_y), 0);
    Check(mouse_requests == 2,
          "mouse row body also requests inherited activation");

    Cout() << "PropertyEditorTests: Checks: " << checks
           << " Fails: " << fails << "\n";

    SetExitCode(fails ? 1 : 0);
}
