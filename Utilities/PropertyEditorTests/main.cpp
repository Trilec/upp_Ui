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

    PropertyEditorFactory::Global().RegisterCustom(
        "test-custom",
        [] { return One<PropertyValueEditor>(new PropertyEditorTestCustomEditor); });
    PropertyEditorItem& custom =
        model.Add("custom", "Custom", PropertyEditorKind::Custom, "seed", "Advanced");
    custom.custom_editor = "test-custom";

    model.StructureChanged();

    Check(model.GetCount() == 10, "model item count");
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

    PropertyEditorItem missing_custom;
    missing_custom.kind = PropertyEditorKind::Custom;
    missing_custom.custom_editor = "missing";
    Check(!PropertyEditorFactory::Global().Create(missing_custom),
          "missing custom editor returns null");

    Cout() << "PropertyEditorTests: Checks: " << checks
           << " Fails: " << fails << "\n";

    SetExitCode(fails ? 1 : 0);
}
