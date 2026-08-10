#include <Utilities/PropertyEditor/PropertyEditor.h>
#include <Ui/UiOsFileDialog/UiOsFileDialog.h>

using namespace Upp;

static const char *PROPERTY_EDITOR_DEMO_VERSION = "1.7.0";

static String DemoImagePath(const String& name)
{
    return NormalizePath(AppendFileName(GetFileFolder(__FILE__), "../../designs/" + name));
}

static Image ResolveDemoIcon(const Value& value, const Image& fallback)
{
    const String name = AsString(value);
    for(const UiIconCatalogEntry& entry : UiIconCatalog())
        if(entry.name == name && entry.factory)
            return entry.factory();
    return fallback;
}

class PropertyEditorDemo : public TopWindow {
public:
    typedef PropertyEditorDemo CLASSNAME;

    PropertyEditorDemo()
    {
        Title(String("PropertyEditor Demo ") + PROPERTY_EDITOR_DEMO_VERSION);
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(1120), DPI(760));
        SetMinSize(Size(DPI(760), DPI(520)));

        PropertyEditorFactory::Global().RegisterPicker("demo-image", [](Value& value, Ctrl *owner) {
            String selected = UiOsFileDialog::SelectOpenFile("Select image", String(), owner);
            if(selected.IsEmpty())
                return false;
            value = selected;
            return true;
        });
        PropertyEditorFactory::Global().RegisterThumbnailProvider(
            "demo-image", [](const Value& value) {
                String path = AsString(value);
                if(!FileExists(path) && path.StartsWith("designs/"))
                    path = DemoImagePath(GetFileName(path));
                Image image = path.IsEmpty() ? Image() : StreamRaster::LoadFileAny(path);
                return image.IsEmpty() ? ICON_DESIGN_IMAGE_48() : image;
            });

        BuildModel();
        BuildStyleModel();
        Add(editor_);
        Add(style_editor_);
        Add(status_);
        Add(version_);
        Add(system_); Add(light_); Add(dark_);
        Add(auto_label_); Add(fixed_label_); Add(ratio_label_);
        Add(expand_); Add(collapse_);

        editor_.SetModel(&model_);
        style_editor_.SetModel(&style_model_);
        editor_.SetGroupAction("Appearance", "Reset");
        editor_.SetGroupAction("Content", "Inspect");

        system_.SetText("Ui theme");
        light_.SetText("Light");
        dark_.SetText("Dark");
        auto_label_.SetText("Auto labels");
        fixed_label_.SetText("Fixed 132");
        ratio_label_.SetText("Ratio 42%");
        expand_.SetText("Expand");
        collapse_.SetText("Collapse");
        version_.SetText(String("PropertyEditor Demo ") + PROPERTY_EDITOR_DEMO_VERSION);
        UiLabel::Style version_style = UiTheme::ResolveLabel(UiRole::Standard);
        version_style.font = StdFont().Bold().Height(DPI(15));
        version_.SetCustomStyle(version_style);
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
        editor_.WhenCommit = [=](String id, Value v) {
            status_.SetText(id == "mixed_double"
                ? "Mixed values unified to " + AsString(v)
                : "Commit  " + id + " = " + AsString(v));
        };
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
        style_model_.WhenCommit = [=](String, Value) { ApplyEditorStyle(); };

        PostCallback([=] { ApplyEditorStyle(); });
        status_.SetText("Complete editor matrix: filtering is live; click rich rows to edit, drag the divider, or press Esc to cancel.");
    }

    void Layout() override
    {
        Rect r = GetSize();
        const int pad = DPI(10), gap = DPI(5), h = DPI(30), status_h = DPI(26);
        UiLayoutCursor top(RectC(pad, pad, max(0, r.GetWidth() - 2 * pad), h));
        top.SetGapX(gap);
        const auto place = [&](Ctrl& ctrl, int width) {
            ctrl.SetRect(top.TakeIncrX(width), top.Y(), width, h);
        };
        place(version_, DPI(194));
        place(system_, DPI(82));
        place(light_, DPI(64));
        place(dark_, DPI(64));
        place(auto_label_, DPI(92));
        place(fixed_label_, DPI(88));
        place(ratio_label_, DPI(88));
        place(expand_, DPI(70));
        place(collapse_, DPI(76));

        int body_top = pad + h + pad;
        int body_bottom = r.bottom - pad - status_h - pad;
        const int settings_width = min(DPI(330), max(DPI(260), r.GetWidth() / 3));
        const int body_width = max(0, r.GetWidth() - 2 * pad);
        const int editor_width = max(0, body_width - settings_width - gap);
        editor_.SetRect(pad, body_top, editor_width, max(0, body_bottom - body_top));
        style_editor_.SetRect(pad + editor_width + gap, body_top,
                              settings_width, max(0, body_bottom - body_top));
        status_.SetRect(pad, r.bottom - pad - status_h, max(0, r.GetWidth() - 2 * pad), status_h);
    }

private:
    void BuildModel()
    {
        // Every built-in kind and every first-class v1 adapter is represented.
        model_.AddText("title", "Title", "PropertyEditor v1", "Content")
              .SetDefault("Untitled").SetHelp("Standard one-line string editor.");
        model_.AddMultiline("notes", "Notes", "First note is visible here...\nSecond line\nThird line\nFourth line", "Content")
              .SetRowSpan(1).SetExpandedRowSpan(3).SetInlineEditor()
              .SetHelp("Compact summary with inline expansion or a separate multiline dialog.");
        model_.AddBoolean("visible", "Visible", true, "Content")
              .SetBooleanPresentation(PropertyBooleanPresentation::Check);
        model_.AddBoolean("enabled", "Enabled", true, "Content")
              .SetBooleanPresentation(PropertyBooleanPresentation::OnOff)
              .SetHelp("Text Boolean: click the row to toggle On/Off directly.");
        model_.AddBoolean("cache", "Use cache", false, "Content")
              .SetBooleanPresentation(PropertyBooleanPresentation::TrueFalse);
        model_.AddText("indented", "Indented child", "One level", "Content/Hierarchy").SetIndent(1);
        model_.AddInteger("nested_count", "Nested count", 3, "Content/Hierarchy/Subheading").SetIndent(2);

        model_.AddInteger("integer", "Integer", 42, "Numbers/Direct").SetUnit("px");
        model_.AddDouble("double", "Floating point", 1.25, "Numbers/Direct").SetUnit("ms");
        model_.AddDouble("mixed_double", "Mixed values (type to unify)", 0.0, "Numbers/Direct")
              .SetMixed().SetHelp("Represents several selected objects with different values. Type one number to assign that shared value to all selections.");
        model_.AddNumericInt("numeric_int", "Numeric integer", 12, 0, 100, 1, "Numbers/Toggle");
        model_.AddNumericDouble("numeric_double", "Numeric double", 0.625, 0.0, 1.0, 0.025, "Numbers/Toggle");
        model_.AddSliderInt("slider_int", "Integer slider", 35, 0, 100, 1, "Numbers/Sliders");
        model_.AddSlider("slider_double", "Double slider", 0.72, 0.0, 1.0, 0.01, "Numbers/Sliders");

        model_.AddColor("accent", "Accent colour", Color(214, 60, 55), "Appearance/Colour")
              .SetDefault(Color(214, 60, 55))
              .SetHelp("Active colour rows open on the swatch in one click and retain #RRGGBB beside it.");
        model_.AddSlider("opacity", "Opacity", 0.82, 0.0, 1.0, 0.01, "Appearance/Surface")
              .SetDefault(1.0);
        model_.AddNumericInt("radius", "Corner Radius", 12, 0, 64, 1, "Appearance/Border")
              .SetUnit("px");
        model_.AddChoice("border", "Border Style", 1, "Appearance/Border")
              .AddChoice(0, "None").AddChoice(1, "Solid").AddChoice(2, "Dashed");

        ValueArray palette;
        palette.Add(Color(31, 111, 235));
        palette.Add(Color(91, 192, 190));
        palette.Add(Color(255, 201, 60));
        palette.Add(Color(238, 99, 82));
        model_.Add("palette", "Four-colour palette", PropertyEditorKind::ColorPalette,
                   palette, "Appearance/Colour").SetColorCount(4).SetInlineEditor();
        ValueArray palette8;
        palette8.Add(Color(31, 111, 235));
        palette8.Add(Color(91, 192, 190));
        palette8.Add(Color(255, 201, 60));
        palette8.Add(Color(238, 99, 82));
        palette8.Add(Color(126, 87, 194));
        palette8.Add(Color(38, 166, 154));
        palette8.Add(Color(255, 112, 67));
        palette8.Add(Color(120, 144, 156));
        model_.Add("palette8", "Eight-colour palette", PropertyEditorKind::ColorPalette,
                   palette8, "Appearance/Colour").SetColorCount(8).SetInlineEditor();

        ValueMap solid;
        solid.Set("mode", "Solid");
        solid.Set("solid", Color(235, 239, 247));
        model_.Add("solid_fill", "Solid fill", PropertyEditorKind::FillRecipe,
                   solid, "Appearance/Fills");
        ValueMap gradient;
        gradient.Set("mode", "QuadGradient");
        gradient.Set("top_left", Color(255, 246, 198));
        gradient.Set("top_right", Color(255, 214, 165));
        gradient.Set("bottom_left", Color(184, 230, 255));
        gradient.Set("bottom_right", Color(198, 205, 255));
        model_.Add("gradient_fill", "Quad gradient", PropertyEditorKind::FillRecipe,
                   gradient, "Appearance/Fills");

        AddPropertyRange(model_, "range", "Allowed Range", 20, 80, 0, 100, 1, "Layout/Sizing")
            .SetHelp("UiRangeSliderEdit in one property line: fixed fields, explicit gaps, expanding track.");
        AddPropertyAdjustableRange(model_, "adjustable_range", "Min/max with range",
                                   0, 50, 250, 680, 900, 1000, 1, "Layout/Sizing")
            .SetHelp("The outer fields author the domain; the inner fields and track author the selected interval.");
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
        ValueArray gallery;
        gallery.Add(DemoImagePath("Generic_bg_Lavender.png"));
        gallery.Add(DemoImagePath("Generic_bg_SunSet.png"));
        AddPropertyImage(model_, "images", "Image pair", gallery, "demo-image", "Resources")
            .SetHelp("Expanded image rows divide the available preview area while preserving each aspect ratio.");
        model_.Add("file", "File path", PropertyEditorKind::FilePath,
                   "C:\\assets\\example.png", "Resources")
              .SetExpandedRowSpan(3).SetInlineEditor()
              .SetHelp("Editable path with separate expand and browse actions.");

        Vector<Pointf> curve;
        curve.Add(Pointf(0, 0));
        curve.Add(Pointf(.30, .14));
        curve.Add(Pointf(.72, .82));
        curve.Add(Pointf(1, 1));
        model_.AddCurve("curve", "Response Curve", PropertyEditorMakeCurve(curve), "Composites")
              .SetRowSpan(1).SetExpandedRowSpan(4).SetInlineEditor();
        model_.AddVector2("position", "Position X/Y", 24, 48, "Composites")
              .SetExpandedRowSpan(3).SetInlineEditor();
        model_.AddVector3("vector", "Vector X/Y/Z", 1, 2, 3, "Composites")
              .SetExpandedRowSpan(3).SetInlineEditor();
        model_.AddReadOnly("runtime", "Read-only value", "Ready", "States");
        model_.AddText("disabled", "Disabled value", "Unavailable", "States").SetEnabled(false);
        model_.AddText("inherited", "Inherited value", "Theme value", "States").SetInherited();
        PropertyEditorItem& local = model_.AddColor("override", "Local override", Color(88, 144, 219), "States/Overrides");
        local.overrideable = true;
        local.override_active = true;
        PropertyEditorItem& inherited = model_.AddColor("inactive_override", "Inactive override", Color(140, 140, 140), "States/Overrides");
        inherited.overrideable = true;
        inherited.override_active = false;

        for(int i = 0; i < 5; i++)
            model_.AddBoolean("stress" + AsString(i), "Stress Flag " + AsString(i), i & 1, "Stress")
                  .SetBooleanPresentation(PropertyBooleanPresentation::Check);

        model_.SetGroupSubtitle("Content", "text, multiline and Boolean presentations");
        model_.SetGroupSubtitle("Content/Hierarchy", "one-level property indentation");
        model_.SetGroupSubtitle("Content/Hierarchy/Subheading", "nested heading and two-level indentation");
        model_.SetGroupSubtitle("Numbers", "direct, toggle and slider numeric editors");
        model_.SetGroupSubtitle("Appearance", "colour, palette, fill recipes and Reset action");
        model_.SetGroupSubtitle("Layout", "range and matrix adapters");
        model_.SetGroupSubtitle("Composites", "compact vectors and dialog-backed curve editing");
        model_.SetGroupSubtitle("States", "read-only, disabled, inherited, mixed and override states");
        model_.StructureChanged();
    }

    void BuildStyleModel()
    {
        style_model_.AddColor("row_odd", "Odd row", Color(255, 253, 244), "Palette");
        style_model_.AddColor("row_even", "Even row", Color(248, 247, 243), "Palette");
        style_model_.AddColor("group", "Heading", Color(215, 221, 230), "Palette");
        style_model_.AddColor("subheading", "Subheading (derived)", LtColor(Color(215, 221, 230), 10), "Palette")
                    .SetReadOnly();
        style_model_.AddColor("selection", "Selection", Color(211, 236, 247), "Palette");
        style_model_.AddNumericInt("font_height", "Font height", StdFont().GetHeight(), 10, 24, 1, "Typography");
        style_model_.AddNumericInt("row_height", "Row height", 28, 24, 48, 1, "Geometry");
        style_model_.AddNumericInt("group_height", "Heading height", 30, 24, 48, 1, "Geometry");
        style_model_.AddNumericInt("filter_height", "Filter height", 36, 30, 52, 1, "Geometry");
        style_model_.AddNumericInt("label_ratio", "Label ratio", 38, 25, 60, 1, "Geometry").SetUnit("%");
        style_model_.AddNumericInt("action_icon_size", "Action icon size", 16, 12, 20, 1, "Actions").SetUnit("px");
        AddPropertyIcon(style_model_, "reset_icon", "Reset icon", "ICON_DESIGN_ARROW_CIRCLE_LEFT_48", "Actions");
        AddPropertyIcon(style_model_, "expand_icon", "Expand icon", "ICON_DESIGN_UNFOLD_MORE_48", "Actions");
        AddPropertyIcon(style_model_, "collapse_icon", "Collapse icon", "ICON_DESIGN_UNFOLD_LESS_48", "Actions");
        AddPropertyIcon(style_model_, "dialog_icon", "Dialog icon", "ICON_DESIGN_BOTTOM_PANEL_OPEN_48", "Actions");
        AddPropertyIcon(style_model_, "browse_icon", "Browse icon", "ICON_DESIGN_PENDING_48", "Actions");
        style_model_.SetGroupSubtitle("Palette", "live PropertyEditor style colours");
        style_model_.SetGroupSubtitle("Geometry", "row spans remain model-controlled");
        style_model_.SetGroupSubtitle("Actions", "central reset, expand, dialog and resource icons");
        style_model_.StructureChanged();
    }

    void ApplyEditorStyle()
    {
        PropertyEditorStyle style = editor_.GetStyle();
        style.row_odd = Color(style_model_.Find("row_odd")->value);
        style.row_even = Color(style_model_.Find("row_even")->value);
        style.group_background = Color(style_model_.Find("group")->value);
        style_model_.SetValue("subheading", LtColor(style.group_background, 10), false);
        style.row_selected = Color(style_model_.Find("selection")->value);
        const int font_height = (int)style_model_.Find("font_height")->value;
        style.label_font.Height(font_height);
        style.value_font.Height(font_height);
        style.group_font.Height(font_height).Bold();
        style.row_height = DPI((int)style_model_.Find("row_height")->value);
        style.group_height = DPI((int)style_model_.Find("group_height")->value);
        style.filter_height = DPI((int)style_model_.Find("filter_height")->value);
        style.action_icons.size = DPI((int)style_model_.Find("action_icon_size")->value);
        style.reset_icon = ResolveDemoIcon(style_model_.Find("reset_icon")->value, style.reset_icon);
        style.action_icons.expand = ResolveDemoIcon(style_model_.Find("expand_icon")->value, style.action_icons.expand);
        style.action_icons.collapse = ResolveDemoIcon(style_model_.Find("collapse_icon")->value, style.action_icons.collapse);
        style.action_icons.dialog = ResolveDemoIcon(style_model_.Find("dialog_icon")->value, style.action_icons.dialog);
        style.action_icons.browse = ResolveDemoIcon(style_model_.Find("browse_icon")->value, style.action_icons.browse);
        style.show_group_summaries = true;
        editor_.SetStyle(style);
        editor_.SetLabelRatio((int)style_model_.Find("label_ratio")->value);
    }

    PropertyEditorModel model_, style_model_;
    PropertyEditor editor_, style_editor_;
    UiLabel version_, status_;
    UiButton system_, light_, dark_;
    UiButton auto_label_, fixed_label_, ratio_label_;
    UiButton expand_, collapse_;
};

GUI_APP_MAIN
{
    PropertyEditorDemo().Run();
}
