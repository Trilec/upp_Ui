#include <CtrlLib/CtrlLib.h>
#include <Ui/Ui.h>
#include <Utilities/PropertyEditor/PropertyEditor.h>

using namespace Upp;

namespace {

enum SliderSample : int {
    SAMPLE_SLIDER = 0,
    SAMPLE_RANGE,
    SAMPLE_COUNT,
};

String CppBool(bool value) { return value ? "true" : "false"; }
String CppColor(Color c)
{
    return IsNull(c) ? String("Null")
                     : Format("Color(%d, %d, %d)", c.GetR(), c.GetG(), c.GetB());
}

UiDirection ParseDirection(const String& value)
{
    return value == "Vertical" ? UiDirection::V : UiDirection::H;
}

UiAlign ParseTickSide(const String& value)
{
    if(value == "Left") return UiAlign::LEFT;
    if(value == "Right") return UiAlign::RIGHT;
    if(value == "Top") return UiAlign::TOP;
    return UiAlign::BOTTOM;
}

String TickSideCode(UiAlign side)
{
    if(side == UiAlign::LEFT) return "UiAlign::LEFT";
    if(side == UiAlign::RIGHT) return "UiAlign::RIGHT";
    if(side == UiAlign::TOP) return "UiAlign::TOP";
    return "UiAlign::BOTTOM";
}

struct SliderConfig {
    String direction = "Horizontal";
    double minimum = 0.0;
    double maximum = 100.0;
    double step = 1.0;
    double value = 35.0;
    double bound_lower = 10.0;
    double lower = 25.0;
    double upper = 75.0;
    double bound_upper = 90.0;
    bool adjustable_bounds = true;
    bool endpoint_markers = true;
    bool enabled = true;

    bool show_ticks = true;
    int major_ticks = 11;
    int minor_ticks = 4;
    String tick_side = "Bottom";
    int tick_major_len = 6;
    int tick_minor_len = 3;
    int tick_gap = 3;
    Color tick_color = Color(148, 163, 184);

    int track_width = 120;
    int track_height = 4;
    int thumb_width = 20;
    int thumb_height = 20;
    bool expand_track = true;

    bool track_face_enabled = true;
    bool track_frame_enabled = false;
    int track_radius = 999;
    int track_frame_width = 0;
    Color track_color = Color(148, 163, 184);
    Color active_color = Color(37, 99, 235);

    bool thumb_face_enabled = true;
    bool thumb_frame_enabled = true;
    int thumb_radius = 999;
    int thumb_frame_width = 2;
    Color thumb_color = Color(37, 99, 235);
    Color thumb_frame_color = Color(214, 223, 235);
};

class UiSliderDemoWindow : public TopWindow {
public:
    typedef UiSliderDemoWindow CLASSNAME;

    UiSliderDemoWindow()
    {
        Title("UiSlider / UiRangeSlider Demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(1260), DPI(810));

        UiThemeContext context = UiTheme::GetContext();
        context.preset = UiThemePreset::Minimal;
        context.mode = UiThemeMode::Light;
        UiTheme::Set(context);
        RegisterPropertyEditorV1Editors(factory_);

        cfg_[SAMPLE_RANGE].value = 50.0;

        Add(header_);
        Add(preview_panel_);
        Add(rail_panel_);

        header_.SetTitle("Slider family")
               .SetSubTitle("UiSlider and UiRangeSlider share one style contract but keep their own value semantics")
               .SetMedia(ICON_DESIGN_TUNE_48())
               .SetMediaAutoFit(true)
               .ShowTitleLine(false)
               .SetContentInset(DPI(8))
               .SetContentCell(header_actions_);
        header_actions_.SetGap(DPI(4)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        header_actions_.AddSpacer(1).Expand(1);
        theme_button_.SetIcon(ICON_ACTION_DARK_MODE_48()).SetIconSize(DPI(16), DPI(16)).Tip("Toggle light/dark");
        exit_button_.SetIcon(ICON_NAVIGATION_EXIT_TO_APP_48()).SetIconSize(DPI(16), DPI(16)).Tip("Close demo");
        header_actions_.Add(theme_button_).Fixed(DPI(34));
        header_actions_.Add(exit_button_).Fixed(DPI(34));

        preview_panel_.Add(sample_bar_);
        sample_bar_.SetGap(DPI(6)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        slider_button_.SetText("UiSlider").SetCheckable().SetChecked(true);
        range_button_.SetText("UiRangeSlider").SetCheckable();
        sample_bar_.Add(slider_button_).Expand(1);
        sample_bar_.Add(range_button_).Expand(1);
        preview_panel_.Add(slider_label_);
        preview_panel_.Add(range_label_);
        preview_panel_.Add(slider_);
        preview_panel_.Add(range_);
        preview_panel_.Add(status_);
        slider_label_.SetText("UiSlider");
        range_label_.SetText("UiRangeSlider");
        status_.SetAlign(UiAlign::CENTER, UiAlign::CENTER);

        rail_panel_.Add(view_bar_);
        rail_panel_.Add(properties_);
        rail_panel_.Add(code_mode_);
        rail_panel_.Add(code_);
        view_bar_.SetGap(DPI(5)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        props_button_.SetText("Properties").SetCheckable().SetChecked(true);
        code_button_.SetText("Code").SetCheckable();
        view_bar_.Add(props_button_).Expand(1);
        view_bar_.Add(code_button_).Expand(1);

        code_mode_.UseInternalModel().Clear()
                  .Add("Usage", "usage")
                  .Add("Current changes", "changes")
                  .Add("Full explicit", "explicit");
        code_mode_.SelectByData("changes");
        code_.SetEditable(false).SetAcceptsTabs(true);

        properties_.SetFactory(&factory_);
        properties_.SetModel(&model_);
        properties_.SetLabelRatio(38);
        PropertyEditorStyle pe_style = PropertyEditorStyle::System();
        pe_style.show_group_summaries = true;
        properties_.SetStyle(pe_style);

        Connect();
        SelectSample(SAMPLE_SLIDER);
        ApplyTheme();
        ApplyAll();
        SetCodeView(false);
    }

    virtual void Layout() override
    {
        Rect client = GetSize();
        const int pad = DPI(12), gap = DPI(10), header_h = DPI(72);
        const int rail_w = min(DPI(465), max(DPI(365), client.GetWidth() * 38 / 100));
        header_.SetRect(pad, pad, max(0, client.GetWidth() - pad * 2), header_h);
        const int top = pad + header_h + gap;
        const int body_h = max(0, client.GetHeight() - top - pad);
        const int preview_w = max(0, client.GetWidth() - pad * 3 - rail_w);
        preview_panel_.SetRect(pad, top, preview_w, body_h);
        rail_panel_.SetRect(pad + preview_w + gap, top, rail_w, body_h);

        Rect pr = preview_panel_.GetSize();
        sample_bar_.SetRect(DPI(24), DPI(18), max(0, pr.GetWidth() - DPI(48)), DPI(34));
        const int side = DPI(50);
        const int major = min(DPI(430), max(DPI(220), pr.GetWidth() - side * 2));
        slider_label_.SetRect(side, DPI(92), major, DPI(24));
        range_label_.SetRect(side, DPI(260), major, DPI(24));

        if(ParseDirection(cfg_[SAMPLE_SLIDER].direction) == UiDirection::H)
            slider_.SetRect(side, DPI(126), major, DPI(70));
        else
            slider_.SetRect(max(0, pr.GetWidth() / 2 - DPI(35)), DPI(118), DPI(70), DPI(130));

        if(ParseDirection(cfg_[SAMPLE_RANGE].direction) == UiDirection::H)
            range_.SetRect(side, DPI(294), major, DPI(70));
        else
            range_.SetRect(max(0, pr.GetWidth() / 2 - DPI(35)), DPI(286), DPI(70), DPI(150));

        status_.SetRect(DPI(24), max(0, pr.bottom - DPI(52)), max(0, pr.GetWidth() - DPI(48)), DPI(26));

        Rect rr = rail_panel_.GetSize();
        view_bar_.SetRect(DPI(8), DPI(8), max(0, rr.GetWidth() - DPI(16)), DPI(32));
        const int y = DPI(48);
        properties_.SetRect(DPI(8), y, max(0, rr.GetWidth() - DPI(16)), max(0, rr.GetHeight() - y - DPI(8)));
        code_mode_.SetRect(DPI(8), y, max(0, rr.GetWidth() - DPI(16)), DPI(32));
        code_.SetRect(DPI(8), y + DPI(40), max(0, rr.GetWidth() - DPI(16)), max(0, rr.GetHeight() - y - DPI(48)));
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

    void BuildModel(SliderSample sample)
    {
        SliderConfig& cfg = cfg_[sample];
        model_.Clear(false);
        Resettable(model_.AddChoice("direction", "Direction", cfg.direction, "Value")
            .AddChoice("Horizontal", "Horizontal").AddChoice("Vertical", "Vertical"));
        Resettable(model_.AddNumericDouble("minimum", "Minimum", cfg.minimum, -1000.0, 1000.0, 1.0, "Value"));
        Resettable(model_.AddNumericDouble("maximum", "Maximum", cfg.maximum, -1000.0, 1000.0, 1.0, "Value"));
        Resettable(model_.AddNumericDouble("step", "Step", cfg.step, 0.01, 100.0, 0.01, "Value"));
        Resettable(model_.AddBoolean("enabled", "Enabled", cfg.enabled, "Value"));
        if(sample == SAMPLE_SLIDER)
            Resettable(model_.AddNumericDouble("value", "Value", cfg.value, cfg.minimum, cfg.maximum, cfg.step, "Value"));
        else {
            PropertyEditorItem& range = AddPropertyAdjustableRange(model_, "range_values", "Bounds / selection",
                cfg.minimum, cfg.bound_lower, cfg.lower, cfg.upper, cfg.bound_upper, cfg.maximum, cfg.step, "Value");
            Resettable(range);
            Resettable(model_.AddBoolean("adjustable_bounds", "Adjustable bounds", cfg.adjustable_bounds, "Value"));
            Resettable(model_.AddBoolean("endpoint_markers", "Endpoint markers", cfg.endpoint_markers, "Value"));
        }

        Resettable(model_.AddBoolean("show_ticks", "Show ticks", cfg.show_ticks, "Ticks"));
        Resettable(model_.AddNumericInt("major_ticks", "Major ticks", cfg.major_ticks, 0, 50, 1, "Ticks"));
        Resettable(model_.AddNumericInt("minor_ticks", "Minor / major", cfg.minor_ticks, 0, 10, 1, "Ticks"));
        PropertyEditorItem& tick_side = Resettable(model_.AddChoice("tick_side", "Tick side", cfg.tick_side, "Ticks")
            .AddChoice("Left", "Left").AddChoice("Right", "Right")
            .AddChoice("Top", "Top").AddChoice("Bottom", "Bottom"));
        tick_side.kind = PropertyEditorKind::Custom;
        tick_side.custom_editor = PropertyEditorMatrixId();
        tick_side.editor_variant = "Cardinal4";
        Resettable(model_.AddNumericInt("tick_major_len", "Major length", cfg.tick_major_len, 0, 24, 1, "Ticks").SetUnit("px"));
        Resettable(model_.AddNumericInt("tick_minor_len", "Minor length", cfg.tick_minor_len, 0, 16, 1, "Ticks").SetUnit("px"));
        Resettable(model_.AddNumericInt("tick_gap", "Gap", cfg.tick_gap, 0, 20, 1, "Ticks").SetUnit("px"));
        Resettable(model_.AddColor("tick_color", "Colour", cfg.tick_color, "Ticks"));

        Resettable(model_.AddNumericInt("track_width", "Track width", cfg.track_width, 20, 600, 1, "Geometry").SetUnit("px"));
        Resettable(model_.AddNumericInt("track_height", "Track height", cfg.track_height, 1, 64, 1, "Geometry").SetUnit("px"));
        Resettable(model_.AddNumericInt("thumb_width", "Thumb width", cfg.thumb_width, 4, 96, 1, "Geometry").SetUnit("px"));
        Resettable(model_.AddNumericInt("thumb_height", "Thumb height", cfg.thumb_height, 4, 96, 1, "Geometry").SetUnit("px"));
        if(sample == SAMPLE_SLIDER)
            Resettable(model_.AddBoolean("expand_track", "Expand track", cfg.expand_track, "Geometry"));

        Resettable(model_.AddBoolean("track_face_enabled", "Face enabled", cfg.track_face_enabled, "Track"));
        Resettable(model_.AddBoolean("track_frame_enabled", "Frame enabled", cfg.track_frame_enabled, "Track"));
        Resettable(model_.AddNumericInt("track_radius", "Radius", cfg.track_radius, 0, 999, 1, "Track").SetUnit("px"));
        Resettable(model_.AddNumericInt("track_frame_width", "Frame width", cfg.track_frame_width, 0, 12, 1, "Track").SetUnit("px"));
        Resettable(model_.AddColor("track_color", "Track colour", cfg.track_color, "Track"));
        Resettable(model_.AddColor("active_color", "Active colour", cfg.active_color, "Track"));

        Resettable(model_.AddBoolean("thumb_face_enabled", "Face enabled", cfg.thumb_face_enabled, "Thumb"));
        Resettable(model_.AddBoolean("thumb_frame_enabled", "Frame enabled", cfg.thumb_frame_enabled, "Thumb"));
        Resettable(model_.AddNumericInt("thumb_radius", "Radius", cfg.thumb_radius, 0, 999, 1, "Thumb").SetUnit("px"));
        Resettable(model_.AddNumericInt("thumb_frame_width", "Frame width", cfg.thumb_frame_width, 0, 12, 1, "Thumb").SetUnit("px"));
        Resettable(model_.AddColor("thumb_color", "Face colour", cfg.thumb_color, "Thumb"));
        Resettable(model_.AddColor("thumb_frame_color", "Frame colour", cfg.thumb_frame_color, "Thumb"));

        model_.SetGroupSubtitle("Value", sample == SAMPLE_RANGE ? "two-handle interval and adjustable bounds" : "single scalar value");
        model_.SetGroupSubtitle("Ticks", "shared tick presentation");
        model_.SetGroupSubtitle("Track", "shared UiSlider::Style track domain");
        model_.SetGroupSubtitle("Thumb", "shared UiSlider::Style thumb domain");
        model_.StructureChanged();
        properties_.RefreshModel();
    }

    void PullConfig(SliderSample sample)
    {
        SliderConfig& cfg = cfg_[sample];
        cfg.direction = AsString(Get("direction"));
        cfg.minimum = (double)Get("minimum");
        cfg.maximum = (double)Get("maximum");
        cfg.step = max(0.01, (double)Get("step"));
        cfg.enabled = (bool)Get("enabled");
        if(cfg.maximum <= cfg.minimum) cfg.maximum = cfg.minimum + cfg.step;
        if(sample == SAMPLE_SLIDER)
            cfg.value = minmax((double)Get("value"), cfg.minimum, cfg.maximum);
        else {
            Vector<double> v = PropertyEditorReadVector(Get("range_values"), 4, 0.0);
            if(v.GetCount() == 4) {
                cfg.bound_lower = v[0]; cfg.lower = v[1]; cfg.upper = v[2]; cfg.bound_upper = v[3];
            }
            cfg.adjustable_bounds = (bool)Get("adjustable_bounds");
            cfg.endpoint_markers = (bool)Get("endpoint_markers");
        }
        cfg.show_ticks = (bool)Get("show_ticks");
        cfg.major_ticks = (int)Get("major_ticks");
        cfg.minor_ticks = (int)Get("minor_ticks");
        cfg.tick_side = AsString(Get("tick_side"));
        cfg.tick_major_len = (int)Get("tick_major_len");
        cfg.tick_minor_len = (int)Get("tick_minor_len");
        cfg.tick_gap = (int)Get("tick_gap");
        cfg.tick_color = Color(Get("tick_color"));
        cfg.track_width = (int)Get("track_width");
        cfg.track_height = (int)Get("track_height");
        cfg.thumb_width = (int)Get("thumb_width");
        cfg.thumb_height = (int)Get("thumb_height");
        if(sample == SAMPLE_SLIDER) cfg.expand_track = (bool)Get("expand_track");
        cfg.track_face_enabled = (bool)Get("track_face_enabled");
        cfg.track_frame_enabled = (bool)Get("track_frame_enabled");
        cfg.track_radius = (int)Get("track_radius");
        cfg.track_frame_width = (int)Get("track_frame_width");
        cfg.track_color = Color(Get("track_color"));
        cfg.active_color = Color(Get("active_color"));
        cfg.thumb_face_enabled = (bool)Get("thumb_face_enabled");
        cfg.thumb_frame_enabled = (bool)Get("thumb_frame_enabled");
        cfg.thumb_radius = (int)Get("thumb_radius");
        cfg.thumb_frame_width = (int)Get("thumb_frame_width");
        cfg.thumb_color = Color(Get("thumb_color"));
        cfg.thumb_frame_color = Color(Get("thumb_frame_color"));
    }

    UiSlider::Style MakeStyle(const SliderConfig& cfg) const
    {
        UiSlider::Style style = UiTheme::ResolveSlider();
        style.show_ticks = cfg.show_ticks;
        style.major_ticks = cfg.major_ticks;
        style.minor_ticks_per_major = cfg.minor_ticks;
        style.tick_side = ParseTickSide(cfg.tick_side);
        style.tick_len_major = DPI(cfg.tick_major_len);
        style.tick_len_minor = DPI(cfg.tick_minor_len);
        style.tick_gap = DPI(cfg.tick_gap);
        style.tick_color = cfg.tick_color;
        style.track_size = Size(DPI(cfg.track_width), DPI(cfg.track_height));
        style.thumb_size = Size(DPI(cfg.thumb_width), DPI(cfg.thumb_height));
        style.track_metrics.face_enabled = cfg.track_face_enabled;
        style.track_metrics.frame_enabled = cfg.track_frame_enabled;
        style.track_metrics.radius = DPI(cfg.track_radius);
        style.track_metrics.frame_width = DPI(cfg.track_frame_width);
        style.thumb_metrics.face_enabled = cfg.thumb_face_enabled;
        style.thumb_metrics.frame_enabled = cfg.thumb_frame_enabled;
        style.thumb_metrics.radius = DPI(cfg.thumb_radius);
        style.thumb_metrics.frame_width = DPI(cfg.thumb_frame_width);
        for(int i = 0; i < 4; i++) {
            style.track_palette.face[i] = UiFill::Solid(cfg.track_color);
            style.track_palette.frame[i] = cfg.track_color;
            style.track_palette.ink[i] = cfg.active_color;
            style.thumb_palette.face[i] = UiFill::Solid(cfg.thumb_color);
            style.thumb_palette.frame[i] = cfg.thumb_frame_color;
        }
        return style;
    }

    void ApplySample(SliderSample sample)
    {
        SliderConfig& cfg = cfg_[sample];
        if(sample == SAMPLE_SLIDER) {
            slider_.SetDirection(ParseDirection(cfg.direction))
                   .SetRange(cfg.minimum, cfg.maximum)
                   .SetStep(cfg.step)
                   .SetCustomStyle(MakeStyle(cfg))
                   .SetTicks(cfg.show_ticks, cfg.major_ticks, cfg.minor_ticks)
                   .SetTickSide(ParseTickSide(cfg.tick_side))
                   .SetTrackSize(Size(DPI(cfg.track_width), DPI(cfg.track_height)))
                   .SetThumbSize(Size(DPI(cfg.thumb_width), DPI(cfg.thumb_height)))
                   .ExpandTrack(cfg.expand_track)
                   .SetValue(cfg.value);
            slider_.Enable(cfg.enabled);
        }
        else {
            range_.SetDirection(ParseDirection(cfg.direction))
                  .SetRange(cfg.minimum, cfg.maximum)
                  .SetStep(cfg.step)
                  .SetCustomStyle(MakeStyle(cfg))
                  .SetTicks(cfg.show_ticks, cfg.major_ticks, cfg.minor_ticks)
                  .SetTickSide(ParseTickSide(cfg.tick_side))
                  .SetTrackSize(Size(DPI(cfg.track_width), DPI(cfg.track_height)))
                  .SetThumbSize(Size(DPI(cfg.thumb_width), DPI(cfg.thumb_height)))
                  .EnableAdjustableBounds(cfg.adjustable_bounds)
                  .SetBounds(cfg.bound_lower, cfg.bound_upper)
                  .SetValues(cfg.lower, cfg.upper)
                  .ShowEndpointMarkers(cfg.endpoint_markers);
            range_.Enable(cfg.enabled);
        }
    }

    void ApplyAll()
    {
        ApplySample(SAMPLE_SLIDER);
        ApplySample(SAMPLE_RANGE);
        UpdateStatus();
        UpdateCode();
        RefreshLayout();
        Refresh();
    }

    void Connect()
    {
        slider_button_.WhenAction = [=] { SelectSample(SAMPLE_SLIDER); };
        range_button_.WhenAction = [=] { SelectSample(SAMPLE_RANGE); };
        props_button_.WhenAction = [=] { SetCodeView(false); };
        code_button_.WhenAction = [=] { SetCodeView(true); };
        code_mode_.WhenAction = [=] { UpdateCode(); };
        theme_button_.WhenAction = [=] { ToggleTheme(); };
        exit_button_.WhenAction = [=] { Close(); };

        properties_.WhenPreview = [=](String, Value) { PullConfig(selected_); ApplyAll(); };
        properties_.WhenCommit = [=](String, Value) { PullConfig(selected_); ApplyAll(); };
        properties_.WhenReset = [=](String id) {
            PropertyEditorItem *item = model_.Find(id);
            if(item && item->resettable) {
                model_.SetValue(id, item->default_value);
                PullConfig(selected_);
                properties_.RefreshModel();
                ApplyAll();
            }
        };

        slider_.WhenChanging = [=] {
            cfg_[SAMPLE_SLIDER].value = slider_.GetValue();
            if(selected_ == SAMPLE_SLIDER && model_.Find("value")) {
                model_.SetValue("value", cfg_[SAMPLE_SLIDER].value, false);
                properties_.RefreshValue("value");
            }
            UpdateStatus(); UpdateCode();
        };
        slider_.WhenAction = slider_.WhenChanging;

        range_.WhenChanging = [=] {
            SliderConfig& cfg = cfg_[SAMPLE_RANGE];
            cfg.bound_lower = range_.GetLowerBound();
            cfg.lower = range_.GetLowerValue();
            cfg.upper = range_.GetUpperValue();
            cfg.bound_upper = range_.GetUpperBound();
            if(selected_ == SAMPLE_RANGE && model_.Find("range_values")) {
                ValueArray v; v.Add(cfg.bound_lower); v.Add(cfg.lower); v.Add(cfg.upper); v.Add(cfg.bound_upper);
                model_.SetValue("range_values", v, false);
                properties_.RefreshValue("range_values");
            }
            UpdateStatus(); UpdateCode();
        };
        range_.WhenAction = range_.WhenChanging;
    }

    void SelectSample(SliderSample sample)
    {
        selected_ = sample;
        slider_button_.SetChecked(sample == SAMPLE_SLIDER);
        range_button_.SetChecked(sample == SAMPLE_RANGE);
        BuildModel(sample);
        ApplyTheme();
        UpdateStatus();
        UpdateCode();
    }

    void UpdateStatus()
    {
        if(selected_ == SAMPLE_RANGE)
            status_.SetText(Format("Selected UiRangeSlider · %.2f … %.2f inside %.2f … %.2f",
                                   range_.GetLowerValue(), range_.GetUpperValue(), range_.GetLowerBound(), range_.GetUpperBound()));
        else
            status_.SetText(Format("Selected UiSlider · value %.2f", slider_.GetValue()));
    }

    void SetCodeView(bool on)
    {
        code_view_ = on;
        props_button_.SetChecked(!on);
        code_button_.SetChecked(on);
        properties_.Show(!on);
        code_mode_.Show(on);
        code_.Show(on);
        ApplyTheme();
        if(on) UpdateCode();
    }

    void EmitStyle(String& out, const SliderConfig& cfg, bool explicit_style) const
    {
        out << "\n// Slider and RangeSlider deliberately share UiSlider::Style.\n";
        out << "UiSlider::Style style = UiTheme::ResolveSlider();\n";
        out << "style.show_ticks = " << CppBool(cfg.show_ticks) << ";\n"
            << "style.major_ticks = " << cfg.major_ticks << ";\n"
            << "style.minor_ticks_per_major = " << cfg.minor_ticks << ";\n"
            << "style.tick_side = " << TickSideCode(ParseTickSide(cfg.tick_side)) << ";\n"
            << "style.tick_len_major = DPI(" << cfg.tick_major_len << ");\n"
            << "style.tick_len_minor = DPI(" << cfg.tick_minor_len << ");\n"
            << "style.tick_gap = DPI(" << cfg.tick_gap << ");\n"
            << "style.tick_color = " << CppColor(cfg.tick_color) << ";\n"
            << "style.track_size = Size(DPI(" << cfg.track_width << "), DPI(" << cfg.track_height << "));\n"
            << "style.thumb_size = Size(DPI(" << cfg.thumb_width << "), DPI(" << cfg.thumb_height << "));\n";
        if(explicit_style) {
            out << "style.track_metrics.face_enabled = " << CppBool(cfg.track_face_enabled) << ";\n"
                << "style.track_metrics.frame_enabled = " << CppBool(cfg.track_frame_enabled) << ";\n"
                << "style.track_metrics.radius = DPI(" << cfg.track_radius << ");\n"
                << "style.track_metrics.frame_width = DPI(" << cfg.track_frame_width << ");\n"
                << "style.thumb_metrics.face_enabled = " << CppBool(cfg.thumb_face_enabled) << ";\n"
                << "style.thumb_metrics.frame_enabled = " << CppBool(cfg.thumb_frame_enabled) << ";\n"
                << "style.thumb_metrics.radius = DPI(" << cfg.thumb_radius << ");\n"
                << "style.thumb_metrics.frame_width = DPI(" << cfg.thumb_frame_width << ");\n"
                << "for(int state = 0; state < 4; ++state) {\n"
                << "    style.track_palette.face[state] = UiFill::Solid(" << CppColor(cfg.track_color) << ");\n"
                << "    style.track_palette.frame[state] = " << CppColor(cfg.track_color) << ";\n"
                << "    style.track_palette.ink[state] = " << CppColor(cfg.active_color) << ";\n"
                << "    style.thumb_palette.face[state] = UiFill::Solid(" << CppColor(cfg.thumb_color) << ");\n"
                << "    style.thumb_palette.frame[state] = " << CppColor(cfg.thumb_frame_color) << ";\n"
                << "}\n";
        }
    }

    void UpdateCode()
    {
        const SliderConfig& cfg = cfg_[selected_];
        String mode = AsString(code_mode_.GetSelectedData());
        String out = "#include <Ui/Ui.h>\n\nusing namespace Upp;\n\n";
        if(selected_ == SAMPLE_SLIDER) {
            out << "UiSlider slider;\n\n"
                << "slider.SetDirection(UiDirection::" << (ParseDirection(cfg.direction) == UiDirection::V ? "V" : "H") << ")\n"
                << "      .SetRange(" << cfg.minimum << ", " << cfg.maximum << ")\n"
                << "      .SetStep(" << cfg.step << ")\n"
                << "      .SetValue(" << cfg.value << ")\n"
                << "      .ExpandTrack(" << CppBool(cfg.expand_track) << ");\n"
                << "slider.Enable(" << CppBool(cfg.enabled) << ");\n";
            if(mode != "usage") {
                EmitStyle(out, cfg, mode == "explicit");
                out << "slider.SetCustomStyle(style);\n";
            }
            out << "\nslider.WhenChanging = [&] { double live = slider.GetValue(); };\n"
                << "slider.WhenAction = [&] { double committed = slider.GetValue(); };\n";
        }
        else {
            out << "UiRangeSlider range;\n\n"
                << "range.SetDirection(UiDirection::" << (ParseDirection(cfg.direction) == UiDirection::V ? "V" : "H") << ")\n"
                << "     .SetRange(" << cfg.minimum << ", " << cfg.maximum << ")\n"
                << "     .SetStep(" << cfg.step << ")\n"
                << "     .EnableAdjustableBounds(" << CppBool(cfg.adjustable_bounds) << ")\n"
                << "     .SetBounds(" << cfg.bound_lower << ", " << cfg.bound_upper << ")\n"
                << "     .SetValues(" << cfg.lower << ", " << cfg.upper << ")\n"
                << "     .ShowEndpointMarkers(" << CppBool(cfg.endpoint_markers) << ");\n"
                << "range.Enable(" << CppBool(cfg.enabled) << ");\n";
            if(mode != "usage") {
                EmitStyle(out, cfg, mode == "explicit");
                out << "range.SetCustomStyle(style);\n";
            }
            out << "\nrange.WhenChanging = [&] { double lo = range.GetLowerValue(); double hi = range.GetUpperValue(); };\n"
                << "range.WhenAction = [&] { /* committed interval */ };\n";
        }
        if(mode == "usage")
            out << "\n// Usage mode relies on the active UiTheme for visual styling.\n";
        code_.SetTextUtf8(out);
    }

    void ToggleTheme()
    {
        UiThemeContext context = UiTheme::GetContext();
        context.mode = context.mode == UiThemeMode::Dark ? UiThemeMode::Light : UiThemeMode::Dark;
        UiTheme::Set(context);
        Ctrl::SwapDarkLight();
        ApplyTheme();
        ApplyAll();
    }

    void ApplyTheme()
    {
        UiTitleCard::Style hs = UiTheme::ResolveTitleCard(UiRole::Accent);
        hs.title_line = false;
        header_.SetCustomStyle(hs);
        preview_panel_.SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Surface));
        rail_panel_.SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Subtle));
        slider_label_.SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Caption));
        range_label_.SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Caption));
        status_.SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Caption));
        slider_button_.SetCustomStyle(UiTheme::ResolveButton(selected_ == SAMPLE_SLIDER ? UiRole::Accent : UiRole::Subtle));
        range_button_.SetCustomStyle(UiTheme::ResolveButton(selected_ == SAMPLE_RANGE ? UiRole::Accent : UiRole::Subtle));
        props_button_.SetCustomStyle(UiTheme::ResolveButton(code_view_ ? UiRole::Subtle : UiRole::Accent));
        code_button_.SetCustomStyle(UiTheme::ResolveButton(code_view_ ? UiRole::Accent : UiRole::Subtle));
        theme_button_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Standard));
        exit_button_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Alert));
        code_mode_.SetCustomStyle(UiTheme::ResolveDropdown(UiRole::Standard));
        properties_.SetPaletteMode(UiTheme::GetContext().mode == UiThemeMode::Dark ? PropertyEditorPaletteMode::Dark : PropertyEditorPaletteMode::Light);
    }

private:
    SliderConfig cfg_[SAMPLE_COUNT];
    SliderSample selected_ = SAMPLE_SLIDER;

    UiTitleCard header_;
    UiBoxLayout header_actions_ { UiDirection::H };
    UiToolButton theme_button_, exit_button_;
    UiPanel preview_panel_, rail_panel_;
    UiBoxLayout sample_bar_ { UiDirection::H };
    UiButton slider_button_, range_button_;
    UiLabel slider_label_, range_label_, status_;
    UiSlider slider_;
    UiRangeSlider range_;

    UiBoxLayout view_bar_ { UiDirection::H };
    UiButton props_button_, code_button_;
    PropertyEditor properties_;
    PropertyEditorFactory factory_;
    PropertyEditorModel model_;
    UiDropdown code_mode_;
    UiMultiEdit code_;
    bool code_view_ = false;
};

} // namespace

GUI_APP_MAIN
{
    UiSliderDemoWindow().Run();
}
