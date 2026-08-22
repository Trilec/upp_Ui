#include <CtrlLib/CtrlLib.h>
#include <Ui/Ui.h>
#include <Utilities/PropertyEditor/PropertyEditor.h>

using namespace Upp;

namespace {

UiDirection ParseDirection(const String& value)
{
    return value == "Vertical" ? UiDirection::V : UiDirection::H;
}

UiAlign ParseTickSide(const String& value)
{
    const String v = ToLower(value);
    if(v == "left") return UiAlign::LEFT;
    if(v == "right") return UiAlign::RIGHT;
    if(v == "top") return UiAlign::TOP;
    return UiAlign::BOTTOM;
}

class UiSliderDemoWindow : public TopWindow {
public:
    typedef UiSliderDemoWindow CLASSNAME;

    UiSliderDemoWindow()
    {
        Title("UiSlider Demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(1180), DPI(760));

        UiThemeContext context = UiTheme::GetContext();
        context.preset = UiThemePreset::Minimal;
        context.mode = UiThemeMode::Light;
        UiTheme::Set(context);

        RegisterPropertyEditorV1Editors(factory_);

        Add(header_);
        Add(preview_panel_);
        Add(inspector_panel_);

        header_.SetTitle("UiSlider")
               .SetSubTitle("Live PropertyEditor reference for range, ticks, geometry and track/thumb styling")
               .SetMedia(ICON_DESIGN_TUNE_48())
               .SetMediaSide(UiAlign::LEFT)
               .SetMediaAlign(UiAlign::CENTER, UiAlign::CENTER)
               .SetMediaAutoFit(true)
               .ShowTitleLine(false)
               .SetContentInset(DPI(8))
               .SetContentCell(header_actions_);
        header_actions_.SetGap(DPI(4)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        header_actions_.AddSpacer(1).Expand(1);
        theme_button_.SetIcon(ICON_ACTION_DARK_MODE_48()).SetIconSize(DPI(16), DPI(16))
                     .Tip("Toggle light/dark theme");
        exit_button_.SetIcon(ICON_DESIGN_MODE_OFF_ON_48()).SetIconSize(DPI(16), DPI(16))
                    .Tip("Close demo");
        header_actions_.Add(theme_button_).Fixed(DPI(34));
        header_actions_.Add(exit_button_).Fixed(DPI(34));

        preview_panel_.Add(slider_);
        preview_panel_.Add(caption_);
        preview_panel_.Add(status_);
        caption_.SetText("Centered live UiSlider preview")
                .SetAlign(UiAlign::CENTER, UiAlign::CENTER);
        status_.SetAlign(UiAlign::CENTER, UiAlign::CENTER);

        inspector_panel_.Add(properties_.SizePos());
        properties_.SetFactory(&factory_);
        properties_.SetModel(&model_);
        properties_.SetLabelRatio(38);
        PropertyEditorStyle pe_style = PropertyEditorStyle::System();
        pe_style.show_group_summaries = true;
        properties_.SetStyle(pe_style);

        BuildModel();
        Connect();
        ApplyTheme();
        ApplyProjection();
    }

    virtual void Layout() override
    {
        Rect client = GetSize();
        const int pad = DPI(12);
        const int gap = DPI(10);
        const int header_h = DPI(72);
        const int right_w = min(DPI(440), max(DPI(350), client.GetWidth() * 37 / 100));

        header_.SetRect(pad, pad, max(0, client.GetWidth() - pad * 2), header_h);
        const int top = pad + header_h + gap;
        const int body_h = max(0, client.GetHeight() - top - pad);
        const int preview_w = max(0, client.GetWidth() - pad * 3 - right_w);
        preview_panel_.SetRect(pad, top, preview_w, body_h);
        inspector_panel_.SetRect(pad + preview_w + gap, top, right_w, body_h);

        Rect pr = preview_panel_.GetSize();
        const bool horizontal = slider_.GetDirection() == UiDirection::H;
        if(horizontal) {
            const int cx = min(DPI(420), max(DPI(220), pr.GetWidth() - DPI(80)));
            const int cy = max(DPI(42), slider_.GetMinSize().cy);
            slider_.SetRect(max(0, (pr.GetWidth() - cx) / 2),
                            max(DPI(30), (pr.GetHeight() - cy) / 2 - DPI(20)),
                            cx, cy);
        }
        else {
            const int cx = max(DPI(48), slider_.GetMinSize().cx);
            const int cy = min(DPI(420), max(DPI(220), pr.GetHeight() - DPI(150)));
            slider_.SetRect(max(0, (pr.GetWidth() - cx) / 2),
                            max(DPI(30), (pr.GetHeight() - cy) / 2 - DPI(20)),
                            cx, cy);
        }
        caption_.SetRect(DPI(18), max(0, pr.bottom - DPI(78)),
                         max(0, pr.GetWidth() - DPI(36)), DPI(26));
        status_.SetRect(DPI(18), max(0, pr.bottom - DPI(48)),
                        max(0, pr.GetWidth() - DPI(36)), DPI(26));
    }

private:
    Value Get(const char *id) const
    {
        const PropertyEditorItem *item = model_.Find(id);
        return item ? item->value : Value();
    }

    PropertyEditorItem& Resettable(PropertyEditorItem& item)
    {
        item.SetDefault(item.value);
        return item;
    }

    void BuildModel()
    {
        Resettable(model_.AddChoice("direction", "Direction", "Horizontal", "Value")
            .AddChoice("Horizontal", "Horizontal").AddChoice("Vertical", "Vertical"));
        Resettable(model_.AddNumericDouble("minimum", "Minimum", 0.0, -1000.0, 1000.0, 1.0, "Value"));
        Resettable(model_.AddNumericDouble("maximum", "Maximum", 100.0, -1000.0, 1000.0, 1.0, "Value"));
        Resettable(model_.AddNumericDouble("value", "Value", 35.0, -1000.0, 1000.0, 0.1, "Value"));
        Resettable(model_.AddNumericDouble("step", "Step", 1.0, 0.01, 100.0, 0.01, "Value"));
        Resettable(model_.AddBoolean("enabled", "Enabled", true, "Value"));

        Resettable(model_.AddBoolean("show_ticks", "Show ticks", true, "Ticks"));
        Resettable(model_.AddNumericInt("major_ticks", "Major ticks", 11, 0, 50, 1, "Ticks"));
        Resettable(model_.AddNumericInt("minor_ticks", "Minor / major", 4, 0, 10, 1, "Ticks"));
        AddPropertyMatrix(model_, "tick_side", "Tick side", "bottom", "Cardinal4", "Ticks");
        Resettable(*model_.Find("tick_side"));
        Resettable(model_.AddNumericInt("tick_major_len", "Major length", 6, 0, 24, 1, "Ticks").SetUnit("px"));
        Resettable(model_.AddNumericInt("tick_minor_len", "Minor length", 3, 0, 16, 1, "Ticks").SetUnit("px"));
        Resettable(model_.AddNumericInt("tick_gap", "Gap", 3, 0, 20, 1, "Ticks").SetUnit("px"));
        Resettable(model_.AddColor("tick_color", "Colour", Color(148, 163, 184), "Ticks"));

        Resettable(model_.AddNumericInt("track_width", "Track width", 120, 20, 600, 1, "Geometry").SetUnit("px"));
        Resettable(model_.AddNumericInt("track_height", "Track height", 4, 1, 64, 1, "Geometry").SetUnit("px"));
        Resettable(model_.AddNumericInt("thumb_width", "Thumb width", 20, 4, 96, 1, "Geometry").SetUnit("px"));
        Resettable(model_.AddNumericInt("thumb_height", "Thumb height", 20, 4, 96, 1, "Geometry").SetUnit("px"));
        Resettable(model_.AddBoolean("expand_track", "Expand track", true, "Geometry"));

        Resettable(model_.AddBoolean("track_face_enabled", "Face enabled", true, "Track"));
        Resettable(model_.AddBoolean("track_frame_enabled", "Frame enabled", false, "Track"));
        Resettable(model_.AddNumericInt("track_radius", "Radius", 999, 0, 999, 1, "Track").SetUnit("px"));
        Resettable(model_.AddNumericInt("track_frame_width", "Frame width", 0, 0, 12, 1, "Track").SetUnit("px"));
        Resettable(model_.AddColor("track_color", "Track colour", Color(148, 163, 184), "Track"));
        Resettable(model_.AddColor("active_color", "Active colour", Color(37, 99, 235), "Track"));

        Resettable(model_.AddBoolean("thumb_face_enabled", "Face enabled", true, "Thumb"));
        Resettable(model_.AddBoolean("thumb_frame_enabled", "Frame enabled", true, "Thumb"));
        Resettable(model_.AddNumericInt("thumb_radius", "Radius", 999, 0, 999, 1, "Thumb").SetUnit("px"));
        Resettable(model_.AddNumericInt("thumb_frame_width", "Frame width", 2, 0, 12, 1, "Thumb").SetUnit("px"));
        Resettable(model_.AddColor("thumb_color", "Face colour", Color(37, 99, 235), "Thumb"));
        Resettable(model_.AddColor("thumb_frame_color", "Frame colour", Color(214, 223, 235), "Thumb"));

        model_.SetGroupSubtitle("Value", "range and live value interaction");
        model_.SetGroupSubtitle("Ticks", "shared tick geometry");
        model_.SetGroupSubtitle("Geometry", "real track and thumb sizing");
        model_.SetGroupSubtitle("Track", "track surface and active ink");
        model_.SetGroupSubtitle("Thumb", "thumb surface");
        model_.StructureChanged();
    }

    void Connect()
    {
        theme_button_.WhenAction = [=] { ToggleTheme(); };
        exit_button_.WhenAction = [=] { Close(); };
        properties_.WhenPreview = [=](String, Value) { ApplyProjection(); };
        properties_.WhenCommit = [=](String, Value) { ApplyProjection(); };
        properties_.WhenReset = [=](String id) {
            PropertyEditorItem *item = model_.Find(id);
            if(item && item->resettable) {
                model_.SetValue(id, item->default_value);
                ApplyProjection();
                properties_.RefreshModel();
            }
        };
        slider_.WhenChanging = [=] { SyncLiveValue(); };
        slider_.WhenAction = [=] { SyncLiveValue(); };
    }

    void SyncLiveValue()
    {
        model_.SetValue("value", slider_.GetValue(), false);
        properties_.RefreshValue("value");
        UpdateStatus();
    }

    void ApplyProjection()
    {
        double minimum = (double)Get("minimum");
        double maximum = (double)Get("maximum");
        if(maximum <= minimum) {
            maximum = minimum + max(0.01, (double)Get("step"));
            model_.SetValue("maximum", maximum, false);
            properties_.RefreshValue("maximum");
        }
        const double step = max(0.01, (double)Get("step"));
        const double value = minmax((double)Get("value"), minimum, maximum);
        if(value != (double)Get("value")) {
            model_.SetValue("value", value, false);
            properties_.RefreshValue("value");
        }

        UiSlider::Style style = UiTheme::ResolveSlider();
        style.show_ticks = (bool)Get("show_ticks");
        style.major_ticks = (int)Get("major_ticks");
        style.minor_ticks_per_major = (int)Get("minor_ticks");
        style.tick_side = ParseTickSide(AsString(Get("tick_side")));
        style.tick_len_major = DPI((int)Get("tick_major_len"));
        style.tick_len_minor = DPI((int)Get("tick_minor_len"));
        style.tick_gap = DPI((int)Get("tick_gap"));
        style.tick_color = Color(Get("tick_color"));
        style.track_size = Size(DPI((int)Get("track_width")), DPI((int)Get("track_height")));
        style.thumb_size = Size(DPI((int)Get("thumb_width")), DPI((int)Get("thumb_height")));

        style.track_metrics.face_enabled = (bool)Get("track_face_enabled");
        style.track_metrics.frame_enabled = (bool)Get("track_frame_enabled");
        style.track_metrics.radius = DPI((int)Get("track_radius"));
        style.track_metrics.frame_width = DPI((int)Get("track_frame_width"));
        style.thumb_metrics.face_enabled = (bool)Get("thumb_face_enabled");
        style.thumb_metrics.frame_enabled = (bool)Get("thumb_frame_enabled");
        style.thumb_metrics.radius = DPI((int)Get("thumb_radius"));
        style.thumb_metrics.frame_width = DPI((int)Get("thumb_frame_width"));
        for(int i = 0; i < 4; i++) {
            style.track_palette.face[i] = UiFill::Solid(Color(Get("track_color")));
            style.track_palette.frame[i] = Color(Get("track_color"));
            style.track_palette.ink[i] = Color(Get("active_color"));
            style.thumb_palette.face[i] = UiFill::Solid(Color(Get("thumb_color")));
            style.thumb_palette.frame[i] = Color(Get("thumb_frame_color"));
        }

        slider_.SetDirection(ParseDirection(AsString(Get("direction"))))
               .SetRange(minimum, maximum)
               .SetStep(step)
               .SetCustomStyle(style)
               .SetTicks(style.show_ticks, style.major_ticks, style.minor_ticks_per_major)
               .SetTickSide(style.tick_side)
               .SetTrackSize(style.track_size)
               .SetThumbSize(style.thumb_size)
               .ExpandTrack((bool)Get("expand_track"))
               .SetValue(value);
        slider_.Enable((bool)Get("enabled"));
        UpdateStatus();
        RefreshLayout();
        Refresh();
    }

    void UpdateStatus()
    {
        status_.SetText(Format("%s · %.2f  [%g … %g]  step %g",
                               AsString(Get("direction")),
                               slider_.GetValue(), slider_.GetMin(), slider_.GetMax(),
                               slider_.GetStep()));
    }

    void ToggleTheme()
    {
        UiThemeContext context = UiTheme::GetContext();
        context.mode = context.mode == UiThemeMode::Dark ? UiThemeMode::Light
                                                          : UiThemeMode::Dark;
        UiTheme::Set(context);
        Ctrl::SwapDarkLight();
        ApplyTheme();
        ApplyProjection();
    }

    void ApplyTheme()
    {
        UiTitleCard::Style header_style = UiTheme::ResolveTitleCard(UiRole::Accent);
        header_style.title_line = false;
        header_.SetCustomStyle(header_style);
        preview_panel_.SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Surface));
        inspector_panel_.SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Subtle));
        caption_.SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Caption));
        status_.SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Caption));
        exit_button_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Alert));
        theme_button_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Standard));
        properties_.SetPaletteMode(UiTheme::GetContext().mode == UiThemeMode::Dark
            ? PropertyEditorPaletteMode::Dark : PropertyEditorPaletteMode::Light);
    }

private:
    UiTitleCard header_;
    UiBoxLayout header_actions_ { UiDirection::H };
    UiToolButton theme_button_, exit_button_;
    UiPanel preview_panel_, inspector_panel_;
    UiSlider slider_;
    UiLabel caption_, status_;
    PropertyEditor properties_;
    PropertyEditorFactory factory_;
    PropertyEditorModel model_;
};

} // namespace

GUI_APP_MAIN
{
    UiSliderDemoWindow().Run();
}
