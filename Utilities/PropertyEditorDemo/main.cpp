#include <Utilities/PropertyEditor/PropertyEditor.h>

using namespace Upp;

class PropertyEditorDemoCustomEditor : public PropertyValueEditor {
public:
    typedef PropertyEditorDemoCustomEditor CLASSNAME;

    PropertyEditorDemoCustomEditor()
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
        if(item.read_only || !item.enabled)
            edit_.SetReadOnly();
        else
            edit_.SetEditable(true);
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
    EditString edit_;
    bool syncing_ = false;
};

class PropertyEditorDemoPreview : public Ctrl {
public:
    void SetModel(PropertyEditorModel *model)
    {
        model_ = model;
        Refresh();
    }

    virtual void Paint(Draw& w) override
    {
        Size sz = GetSize();
        w.DrawRect(sz, SColorPaper());

        if(!model_)
            return;

        String title = ValueOf("title", "PropertyEditor utility");
        bool enabled = (bool)ValueOf("enabled", true);
        Color color = Color(ValueOf("color", Color(58, 124, 214)));
        double opacity = (double)ValueOf("opacity", 1.0);
        Vector<double> position = PropertyEditorReadVector(ValueOf("position", PropertyEditorMakeVector(0.5, 0.5)), 2);
        Vector<double> scale = PropertyEditorReadVector(ValueOf("scale", PropertyEditorMakeVector(1.0, 1.0, 1.0)), 3);

        Color paper = Blend(SColorPaper(), color, (int)minmax(opacity * 180.0, 0.0, 180.0));
        Rect r = RectC(DPI(26), DPI(54), max(DPI(80), sz.cx - DPI(52)), max(DPI(80), sz.cy - DPI(92)));
        w.DrawRect(r, paper);
        DrawFrame(w, r, enabled ? color : SColorDisabled());

        Font title_font = SansSerifZ(18).Bold();
        w.DrawText(DPI(24), DPI(18), title, title_font, enabled ? SColorText() : SColorDisabled());

        String info = Format("Position %.2f, %.2f    Scale %.2f, %.2f, %.2f",
                             position[0], position[1], scale[0], scale[1], scale[2]);
        w.DrawText(DPI(24), r.bottom + DPI(14), info, StdFont(), SColorText());
    }

private:
    Value ValueOf(const String& id, const Value& fallback) const
    {
        const PropertyEditorItem *item = model_ ? model_->Find(id) : nullptr;
        return item ? item->value : fallback;
    }

    PropertyEditorModel *model_ = nullptr;
};

class PropertyEditorDemo : public TopWindow {
public:
    typedef PropertyEditorDemo CLASSNAME;

    PropertyEditorDemo()
    {
        Title("PropertyEditor utility demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(980), DPI(680));

        BuildModel();

        Add(editor_);
        Add(preview_);
        Add(status_);
        Add(system_);
        Add(light_);
        Add(dark_);
        Add(expand_);
        Add(collapse_);

        editor_.SetModel(&model_);
        preview_.SetModel(&model_);

        system_.SetLabel("System");
        light_.SetLabel("Light");
        dark_.SetLabel("Dark");
        expand_.SetLabel("Expand all");
        collapse_.SetLabel("Collapse all");

        system_.WhenAction = [=] { editor_.SetPaletteMode(PropertyEditorPaletteMode::System); };
        light_.WhenAction = [=] { editor_.SetPaletteMode(PropertyEditorPaletteMode::Light); };
        dark_.WhenAction = [=] { editor_.SetPaletteMode(PropertyEditorPaletteMode::Dark); };
        expand_.WhenAction = [=] { editor_.ExpandAll(); };
        collapse_.WhenAction = [=] { editor_.CollapseAll(); };

        model_.WhenPreview = [=](String id, Value value) {
            preview_.Refresh();
            status_.SetLabel("Preview  " + id + " = " + AsString(value));
        };
        model_.WhenCommit = [=](String id, Value value) {
            preview_.Refresh();
            status_.SetLabel("Committed  " + id + " = " + AsString(value));
        };
        model_.WhenReset = [=](String id) {
            preview_.Refresh();
            status_.SetLabel("Reset  " + id);
        };
        editor_.WhenHelp = [=](String text) {
            if(!text.IsEmpty())
                status_.SetLabel(text);
        };

        status_.SetLabel("Select a property. Continuous controls preview while editing and commit when finished.");
    }

    virtual void Layout() override
    {
        Rect r = GetSize();
        int pad = DPI(8);
        int top_h = DPI(32);
        int status_h = DPI(28);
        int button_w = DPI(88);
        int gap = DPI(5);

        int x = pad;
        system_.SetRect(x, pad, button_w, top_h);
        x += button_w + gap;
        light_.SetRect(x, pad, button_w, top_h);
        x += button_w + gap;
        dark_.SetRect(x, pad, button_w, top_h);
        x += button_w + DPI(18);
        expand_.SetRect(x, pad, button_w + DPI(14), top_h);
        x += button_w + DPI(14) + gap;
        collapse_.SetRect(x, pad, button_w + DPI(14), top_h);

        int body_top = pad + top_h + pad;
        int body_bottom = r.bottom - pad - status_h - pad;
        int editor_cx = min(DPI(440), max(DPI(330), r.GetWidth() * 45 / 100));

        editor_.SetRect(pad, body_top, editor_cx, max(0, body_bottom - body_top));
        preview_.SetRect(pad + editor_cx + pad, body_top,
                         max(0, r.GetWidth() - editor_cx - 3 * pad),
                         max(0, body_bottom - body_top));
        status_.SetRect(pad, r.bottom - pad - status_h,
                        max(0, r.GetWidth() - 2 * pad), status_h);
    }

private:
    void BuildModel()
    {
        PropertyEditorFactory::Global().RegisterCustom(
            "demo-custom",
            [] { return One<PropertyValueEditor>(new PropertyEditorDemoCustomEditor); });

        model_.AddText("title", "Title", "PropertyEditor utility", "General")
              .SetDefault("Untitled")
              .SetHelp("A normal string property.")
              .SetDomain(PropertyEditorDomain::Content)
              .SetImpact(PropertyImpactControlState | PropertyImpactPaint);

        model_.AddMultiline("notes", "Notes", "Line 1\nLine 2", "General")
              .SetHelp("Multiline text uses a larger editor.");

        model_.AddBoolean("enabled", "Enabled", true, "General")
              .SetHelp("A discrete Boolean editor.")
              .SetImpact(PropertyImpactControlState | PropertyImpactPaint);

        model_.AddInteger("count", "Count", 8, "General")
              .SetRange(0, 100, 1)
              .SetDefault(8)
              .SetHelp("Integer parsing, range clamping and reset.");

        model_.AddChoice("mode", "Mode", 1, "General")
              .AddChoice(0, "Minimal")
              .AddChoice(1, "Balanced")
              .AddChoice(2, "Detailed")
              .SetHelp("Choice values are keys; labels are presentation.");

        model_.AddSliderInt("steps", "Steps", 4, 0, 10, 2, "General")
              .SetHelp("Integer slider snaps to its step.");

        model_.AddColor("color", "Accent colour", Color(58, 124, 214), "Appearance")
              .SetDefault(Color(58, 124, 214))
              .SetHelp("Uses the standard U++ colour pusher.")
              .SetDomain(PropertyEditorDomain::Appearance)
              .SetImpact(PropertyImpactPaint);

        model_.AddSlider("opacity", "Opacity", 0.82, 0.0, 1.0, 0.01, "Appearance")
              .SetDefault(1.0)
              .SetHelp("Slider previews continuously and commits on release.")
              .SetImpact(PropertyImpactPaint);

        model_.AddDouble("radius", "Radius", 12.0, "Appearance")
              .SetRange(0.0, 100.0, 0.5)
              .SetUnit("px")
              .SetHelp("Double property with units.");

        model_.AddVector2("position", "Position", 0.5, 0.5, "Transform")
              .SetHelp("Two-component numeric editor.")
              .SetImpact(PropertyImpactLocalLayout | PropertyImpactPaint);

        model_.AddVector3("scale", "Scale", 1.0, 1.0, 1.0, "Transform")
              .SetHelp("Three-component numeric editor.")
              .SetImpact(PropertyImpactLocalLayout);

        Vector<Pointf> curve;
        curve.Add(Pointf(0.0, 0.0));
        curve.Add(Pointf(0.25, 0.08));
        curve.Add(Pointf(0.72, 0.88));
        curve.Add(Pointf(1.0, 1.0));
        model_.AddCurve("curve", "Response curve",
                        PropertyEditorMakeCurve(curve), "Advanced")
              .SetHelp("Interactive normalized curve editor.");

        model_.AddReadOnly("runtime", "Runtime status", "Ready", "Advanced")
              .SetHelp("Read-only values use the same row model.");

        PropertyEditorItem& custom =
            model_.Add("custom", "Custom", PropertyEditorKind::Custom, "demo", "Advanced");
        custom.custom_editor = "demo-custom";
        custom.SetHelp("Registered custom editor instance.");

        PropertyEditorItem& mixed =
            model_.AddDouble("mixed-example", "Mixed value", 0.5, "Advanced");
        mixed.SetMixed().SetHelp("Demonstrates the mixed-value presentation used by multi-selection.");

        PropertyEditorItem& inherited =
            model_.AddText("inherited-example", "Inherited value", "Default", "Advanced");
        inherited.SetInherited().SetDefault("Default")
                 .SetHelp("Demonstrates inherited/reset presentation.");

        model_.StructureChanged();
    }

    PropertyEditorModel model_;
    PropertyEditor editor_;
    PropertyEditorDemoPreview preview_;

    Label status_;
    Button system_;
    Button light_;
    Button dark_;
    Button expand_;
    Button collapse_;
};

GUI_APP_MAIN
{
    PropertyEditorDemo().Run();
}
