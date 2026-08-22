#include <Utilities/PropertyEditor/PropertyEditor.h>
#include <Ui/UiOsFileDialog/UiOsFileDialog.h>

using namespace Upp;

class PropertyEditorSemanticDemo : public TopWindow {
public:
    typedef PropertyEditorSemanticDemo CLASSNAME;

    PropertyEditorSemanticDemo()
    {
        Title("PropertyEditor Semantic Demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(920), DPI(760));
        SetMinSize(Size(DPI(700), DPI(520)));

        RegisterPropertyEditorEditors(PropertyEditorFactory::Global());
        PropertyEditorFactory::Global().RegisterPicker("semantic-reference",
            [](Value& value, Ctrl *owner) {
                String selected = UiOsFileDialog::SelectOpenFile(
                    "Choose referenced resource", String(), owner);
                if(selected.IsEmpty())
                    return false;
                value = selected;
                return true;
            });

        BuildModel();

        Add(title_); Add(editor_); Add(status_); Add(theme_); Add(close_);
        title_.SetText("Semantic PropertyEditor capability matrix")
              .SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        UiLabel::Style heading = UiTheme::ResolveLabel(UiRole::Standard);
        heading.font = StdFont().Bold().Height(DPI(16));
        title_.SetCustomStyle(heading);

        theme_.SetText("Light / Dark");
        close_.SetText("Close");
        editor_.SetModel(&model_);
        editor_.SetLabelRatio(38);

        theme_.WhenAction = [=] { ToggleTheme(); };
        close_.WhenAction = [=] { Close(); };
        editor_.WhenPreview = [=](String id, Value value) {
            status_.SetText("Preview  " + id + " = " + AsString(value));
        };
        editor_.WhenCommit = [=](String id, Value value) {
            status_.SetText("Commit  " + id + " = " + AsString(value));
        };
        editor_.WhenReset = [=](String id) {
            status_.SetText("Reset  " + id);
        };

        ApplyTheme();
        status_.SetText("Open the semantic editors: time, geometry, flags, ordered lists, gradients, shortcuts, references and nullable values.");
    }

    void Layout() override
    {
        Rect r = GetSize();
        const int pad = DPI(12), h = DPI(32), gap = DPI(8), status_h = DPI(28);
        title_.SetRect(pad, pad, max(0, r.GetWidth() - DPI(250)), h);
        close_.SetRect(r.right - pad - DPI(72), pad, DPI(72), h);
        theme_.SetRect(r.right - pad - DPI(182), pad, DPI(102), h);
        const int top = pad + h + gap;
        editor_.SetRect(pad, top, max(0, r.GetWidth() - 2 * pad),
                        max(0, r.GetHeight() - top - status_h - pad * 2));
        status_.SetRect(pad, r.bottom - pad - status_h,
                        max(0, r.GetWidth() - 2 * pad), status_h);
    }

private:
    void BuildModel()
    {
        AddPropertyDate(model_, "date", "Date", Date(2026, 8, 23), "Date & Time")
            .SetHelp("Production UiDateTime in Date mode.");
        AddPropertyTime(model_, "time", "Time", Time(1970, 1, 1, 14, 30, 0), false, "Date & Time")
            .SetHelp("Time-only value anchored to 1970-01-01 internally.");
        AddPropertyDateTime(model_, "datetime", "Date / Time", Time(2026, 8, 23, 14, 30, 45), true, "Date & Time")
            .SetHelp("Combined picker with seconds enabled.");
        AddPropertyDuration(model_, "duration", "Duration", 2.5, 0.0, 86400.0, 0.001, "Date & Time")
            .SetDefault(1.0).SetHelp("Durable unit is seconds; edit using ms, s, min or h.");

        AddPropertyPoint(model_, "point", "Point", 24, 48, "Geometry")
            .SetHelp("Semantic X/Y rather than an unnamed vector.");
        AddPropertySize(model_, "size", "Size", 640, 360, "Geometry")
            .SetHelp("Width / Height compound value.");
        AddPropertyRect(model_, "rect", "Rect", 20, 30, 320, 180, "Geometry")
            .SetHelp("X / Y / Width / Height compound value.");
        AddPropertyInsets(model_, "insets", "Insets", 12, 12, 12, 12, true, "Geometry")
            .SetHelp("Left / Top / Right / Bottom with optional linked editing.");
        AddPropertyCorners(model_, "corners", "Corner radii", 10, 10, 10, 10, true, "Geometry")
            .SetHelp("Top-left / top-right / bottom-right / bottom-left with linked editing.");

        ValueArray flags;
        flags.Add("text"); flags.Add("icon");
        AddPropertyFlags(model_, "features", "Features", flags, "Selection")
            .AddChoice("text", "Text")
            .AddChoice("icon", "Icon")
            .AddChoice("badge", "Badge")
            .AddChoice("tooltip", "Tooltip");

        ValueArray names;
        names.Add("Primary"); names.Add("Secondary"); names.Add("Fallback");
        AddPropertyStringList(model_, "ordered", "Ordered values", names, 12, "Selection")
            .SetHelp("Small property-level sequence with Add / Remove / Up / Down. Model-backed controls still use Data pages.");

        ValueArray stops;
        stops.Add(PropertyEditorMakeGradientStop(0.0, Color(47, 111, 237), 255));
        stops.Add(PropertyEditorMakeGradientStop(0.45, Color(108, 92, 231), 230));
        stops.Add(PropertyEditorMakeGradientStop(1.0, Color(236, 72, 153), 255));
        AddPropertyGradient(model_, "gradient", "Gradient",
                            PropertyEditorMakeGradient(stops, "Linear", 35.0, "Smooth"),
                            "Appearance")
            .SetHelp("Linear or radial recipe with ordered stops, alpha, angle and interpolation.");

        Vector<Pointf> curve;
        curve.Add(Pointf(0, 0)); curve.Add(Pointf(.35, .18)); curve.Add(Pointf(1, 1));
        model_.AddCurve("linear_curve", "Point curve", PropertyEditorMakeCurve(curve), "Appearance / Curves")
              .SetExpandedRowSpan(4).SetInlineEditor();
        model_.AddBezierCurve("bezier_curve", "Bezier curve",
                              PropertyEditorMakeBezierCurve(.25, .1, .25, 1.0),
                              "Appearance / Curves")
              .SetHelp("Cubic Bezier editor using UiBezierCurveEditor.");

        AddPropertyKeyChord(model_, "shortcut", "Shortcut", "Ctrl+Shift+S", "Input")
            .SetHelp("Canonical key chord string.");
        AddPropertyReference(model_, "reference", "Resource", Null,
                             "semantic-reference", "Resources")
            .SetHelp("Application-owned resource picker; PropertyEditor stores the returned reference Value.");

        AddPropertyOptional(model_, "optional_text", "Optional text", Null,
                            "Inherited label", "text", "Nullable")
            .SetHelp("Explicit Set/unset state. Null is a durable value, independent of inherited/override UI state.");
        AddPropertyOptional(model_, "optional_int", "Optional integer", Null,
                            12, "int", "Nullable");
        AddPropertyOptional(model_, "optional_double", "Optional number", 0.75,
                            1.0, "double", "Nullable");

        model_.SetGroupSubtitle("Date & Time", "semantic date/time values and unit-aware duration");
        model_.SetGroupSubtitle("Geometry", "named compound geometry with linkable four-sided values");
        model_.SetGroupSubtitle("Selection", "multi-choice and bounded ordered collection properties");
        model_.SetGroupSubtitle("Appearance", "gradient recipe and existing curve editors");
        model_.SetGroupSubtitle("Nullable", "explicit optional values; not theme inheritance");
        model_.StructureChanged();
    }

    void ToggleTheme()
    {
        UiThemeContext context = UiTheme::GetContext();
        context.mode = context.mode == UiThemeMode::Dark ? UiThemeMode::Light : UiThemeMode::Dark;
        UiTheme::Set(context);
        Ctrl::SwapDarkLight();
        ApplyTheme();
    }

    void ApplyTheme()
    {
        title_.SetCustomStyle(UiTheme::ResolveLabel(UiRole::Standard));
        status_.SetCustomStyle(UiTheme::ResolveLabel(UiRole::Subtle));
        theme_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Subtle));
        close_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Alert));
        editor_.SetPaletteMode(UiTheme::GetContext().mode == UiThemeMode::Dark
            ? PropertyEditorPaletteMode::Dark : PropertyEditorPaletteMode::Light);
    }

    PropertyEditorModel model_;
    PropertyEditor editor_;
    UiLabel title_, status_;
    UiButton theme_, close_;
};

GUI_APP_MAIN
{
    UiThemeContext context = UiTheme::GetContext();
    context.preset = UiThemePreset::Minimal;
    context.mode = UiThemeMode::Light;
    UiTheme::Set(context);

    PropertyEditorSemanticDemo().Run();
}
