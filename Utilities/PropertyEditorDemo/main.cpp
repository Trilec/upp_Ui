#include <Utilities/PropertyEditor/PropertyEditor.h>

using namespace Upp;

class PropertyEditorDemo : public TopWindow {
public:
    typedef PropertyEditorDemo CLASSNAME;

    PropertyEditorDemo()
    {
        Title("PropertyEditor v1 demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(1120), DPI(760));
        SetMinSize(Size(DPI(760), DPI(520)));

        PropertyEditorFactory::Global().RegisterPicker("demo-image", [](Value& value, Ctrl*) {
            value = "selected-image.png";
            return true;
        });

        BuildModel();
        Add(editor_);
        Add(status_);
        Add(system_); Add(light_); Add(dark_);
        Add(auto_label_); Add(fixed_label_); Add(ratio_label_);
        Add(expand_); Add(collapse_);

        editor_.SetModel(&model_);
        editor_.SetGroupAction("Appearance", "Reset");

        system_.SetText("Ui theme");
        light_.SetText("Light");
        dark_.SetText("Dark");
        auto_label_.SetText("Auto labels");
        fixed_label_.SetText("Fixed 132");
        ratio_label_.SetText("Ratio 42%");
        expand_.SetText("Expand");
        collapse_.SetText("Collapse");
        status_.SetCustomStyle(UiTheme::ResolveLabel(UiRole::Subtle));

        system_.WhenAction = [=] { editor_.SetPaletteMode(PropertyEditorPaletteMode::FollowUiTheme); };
        light_.WhenAction = [=] { editor_.SetPaletteMode(PropertyEditorPaletteMode::Light); };
        dark_.WhenAction = [=] { editor_.SetPaletteMode(PropertyEditorPaletteMode::Dark); };
        auto_label_.WhenAction = [=] { editor_.SetLabelAuto(); };
        fixed_label_.WhenAction = [=] { editor_.SetLabelWidth(DPI(132)); };
        ratio_label_.WhenAction = [=] { editor_.SetLabelRatio(42); };
        expand_.WhenAction = [=] { editor_.ExpandAll(); };
        collapse_.WhenAction = [=] { editor_.CollapseAll(); };

        editor_.WhenBeginEdit = [=](String id, Value) { status_.SetText("Begin  " + id); };
        editor_.WhenPreview = [=](String id, Value v) { status_.SetText("Preview  " + id + " = " + AsString(v)); };
        editor_.WhenCommit = [=](String id, Value v) { status_.SetText("Commit  " + id + " = " + AsString(v)); };
        editor_.WhenCancel = [=](String id, Value) { status_.SetText("Cancelled  " + id); };
        editor_.WhenUndoRequest = [=](String id) { status_.SetText("Undo requested for " + id); };
        editor_.WhenHelp = [=](String help) { if(!help.IsEmpty()) status_.SetText(help); };
        editor_.WhenGroupAction = [=](String group) {
            if(group == "Appearance") {
                String error;
                model_.Reset("accent", &error);
                model_.Reset("opacity", &error);
                status_.SetText("Reset Appearance defaults");
            }
        };

        status_.SetText("V1: type in Filter, drag the label divider, double-click it for Auto, or press Esc while editing.");
    }

    void Layout() override
    {
        Rect r = GetSize();
        const int pad = DPI(10), gap = DPI(5), h = DPI(30), status_h = DPI(26);
        UiLayoutCursor top(RectC(pad, pad, max(0, r.GetWidth() - 2 * pad), h));
        top.SetGapX(gap);
        system_.SetRect(top.TakeIncrX(DPI(82)));
        light_.SetRect(top.TakeIncrX(DPI(64)));
        dark_.SetRect(top.TakeIncrX(DPI(64)));
        auto_label_.SetRect(top.TakeIncrX(DPI(92)));
        fixed_label_.SetRect(top.TakeIncrX(DPI(88)));
        ratio_label_.SetRect(top.TakeIncrX(DPI(88)));
        expand_.SetRect(top.TakeIncrX(DPI(70)));
        collapse_.SetRect(top.TakeIncrX(DPI(76)));

        int body_top = pad + h + pad;
        int body_bottom = r.bottom - pad - status_h - pad;
        editor_.SetRect(pad, body_top, max(0, r.GetWidth() - 2 * pad), max(0, body_bottom - body_top));
        status_.SetRect(pad, r.bottom - pad - status_h, max(0, r.GetWidth() - 2 * pad), status_h);
    }

private:
    void BuildModel()
    {
        model_.AddText("title", "Title", "PropertyEditor v1", "Content")
              .SetDefault("Untitled").SetHelp("Standard one-line string editor.");
        model_.AddMultiline("notes", "Notes", "Line one\nLine two", "Content")
              .SetRowSpan(2).SetHelp("Explicit two-line row span.");
        model_.AddBoolean("visible", "Visible", true, "Content")
              .SetBooleanPresentation(PropertyBooleanPresentation::Check);
        model_.AddBoolean("enabled", "Enabled", true, "Content")
              .SetBooleanPresentation(PropertyBooleanPresentation::OnOff)
              .SetHelp("Text Boolean: click the row to toggle On/Off directly.");
        model_.AddBoolean("cache", "Use cache", false, "Content")
              .SetBooleanPresentation(PropertyBooleanPresentation::TrueFalse);

        model_.AddColor("accent", "Accent colour", Color(214, 60, 55), "Appearance/Colour")
              .SetDefault(Color(214, 60, 55))
              .SetHelp("Active colour rows open on the swatch in one click and retain #RRGGBB beside it.");
        model_.AddSlider("opacity", "Opacity", 0.82, 0.0, 1.0, 0.01, "Appearance/Surface")
              .SetDefault(1.0);
        model_.AddNumericInt("radius", "Corner Radius", 12, 0, 64, 1, "Appearance/Border")
              .SetUnit("px");
        model_.AddChoice("border", "Border Style", 1, "Appearance/Border")
              .AddChoice(0, "None").AddChoice(1, "Solid").AddChoice(2, "Dashed");

        AddPropertyRange(model_, "range", "Allowed Range", 20, 80, 0, 100, 1, "Layout/Sizing")
            .SetHelp("UiRangeSliderEdit in one property line: fixed fields, explicit gaps, expanding track.");
        AddPropertyMatrix(model_, "anchor", "Anchor", "center", "Position9", "Layout/Position")
            .SetDefault("center").SetHelp("Two-line UiMatrixSelector property row.");
        AddPropertyMatrix(model_, "direction", "Direction", "east", "Compass8", "Layout/Position")
            .SetHelp("Compass preset uses the same matrix adapter.");

        AddPropertyIcon(model_, "icon", "Icon", "ICON_DESIGN_HOME_48", "Resources")
            .SetHelp("Icon choices come directly from the Ui icon catalog.");
        String face = Font::GetFaceCount() ? Font::GetFaceName(0) : String();
        AddPropertyFont(model_, "font", "Font Face", face, "Resources")
            .SetHelp("Font faces are enumerated lazily by the visual editor.");
        AddPropertyImage(model_, "image", "Image", "hero.png", "demo-image", "Resources")
            .SetHelp("Image selection is provider-driven; PropertyEditor has no SymbolPicker dependency.");

        Vector<Pointf> curve;
        curve.Add(Pointf(0, 0)); curve.Add(Pointf(.35, .15)); curve.Add(Pointf(1, 1));
        model_.AddCurve("curve", "Response Curve", PropertyEditorMakeCurve(curve), "Advanced")
              .SetRowSpan(3);
        model_.AddVector3("vector", "Vector", 1, 2, 3, "Advanced").SetRowSpan(2);
        model_.AddReadOnly("runtime", "Runtime", "Ready", "Advanced");

        for(int i = 0; i < 24; i++)
            model_.AddBoolean("stress" + AsString(i), "Stress Flag " + AsString(i), i & 1, "Stress")
                  .SetBooleanPresentation(PropertyBooleanPresentation::Check);

        model_.SetGroupSubtitle("Appearance", "nested groups + Reset action");
        model_.SetGroupSubtitle("Layout", "range + matrix");
        model_.StructureChanged();
    }

    PropertyEditorModel model_;
    PropertyEditor editor_;
    UiLabel status_;
    UiButton system_, light_, dark_;
    UiButton auto_label_, fixed_label_, ratio_label_;
    UiButton expand_, collapse_;
};

GUI_APP_MAIN
{
    PropertyEditorDemo().Run();
}
