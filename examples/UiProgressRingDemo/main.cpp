#include <Ui/Ui.h>
#include <Utilities/PropertyEditor/PropertyEditor.h>
#include <Utilities/PropertyEditor/PropertyValueEditors.h>

using namespace Upp;

namespace {

class UiProgressRingDemo : public TopWindow {
public:
    typedef UiProgressRingDemo CLASSNAME;

    UiProgressRingDemo()
    {
        Title("UiProgressRing Demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(1120), DPI(720));

        RegisterPropertyEditorV1Editors(pe_factory);

        BuildHeader();
        BuildPreview();
        BuildProperties();
        ConnectEvents();
        ApplyProjection();
    }

    void Layout() override
    {
        Rect r = GetSize();
        r.Deflate(DPI(16));

        tc_header.SetRect(r.left, r.top, r.GetWidth(), DPI(70));

        int top = r.top + DPI(82);
        int body_h = max(0, r.bottom - top);
        int rail_w = min(DPI(360), max(DPI(300), r.GetWidth() / 3));
        int gap = DPI(14);
        int preview_w = max(0, r.GetWidth() - rail_w - gap);

        pnl_preview.SetRect(r.left, top, preview_w, body_h);
        pnl_properties.SetRect(r.left + preview_w + gap, top, rail_w, body_h);

        Size preview = pnl_preview.GetSize();
        int caption_h = DPI(34);
        int area_h = max(0, preview.cy - caption_h);
        int requested_w = max(DPI(24), (int)ValueOf("width", 180));
        int requested_h = max(DPI(24), (int)ValueOf("height", 180));
        int rw = min(requested_w, max(0, preview.cx - DPI(24)));
        int rh = min(requested_h, max(0, area_h - DPI(24)));
        int rx = max(0, (preview.cx - rw) / 2);
        int ry = max(0, (area_h - rh) / 2);
        ring_preview.SetRect(rx, ry, rw, rh);
        lbl_caption.SetRect(0, max(0, preview.cy - caption_h), preview.cx, caption_h);
    }

private:
    void BuildHeader()
    {
        Add(tc_header);
        tc_header.SetTitle("UiProgressRing")
                 .SetSubTitle("Circular progress with gradient sweep, independent track colour and responsive center text")
                 .ShowTitleLine(false)
                 .SetContentInset(DPI(8))
                 .SetContentCell(box_header_actions);

        box_header_actions.SetGap(DPI(6)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        box_header_actions.AddSpacer(1).Expand(1);
        btn_replay.SetText("Replay");
        btn_exit.SetText("Exit");
        box_header_actions.Add(btn_replay).Fixed(DPI(82));
        box_header_actions.Add(btn_exit).Fixed(DPI(64));
    }

    void BuildPreview()
    {
        Add(pnl_preview);
        pnl_preview.Add(ring_preview);
        pnl_preview.Add(lbl_caption);
        lbl_caption.SetText("The allocated rectangle may be non-square; the ring stays circular and centers itself.")
                   .SetAlign(UiAlign::CENTER, UiAlign::CENTER);
    }

    void BuildProperties()
    {
        Add(pnl_properties);
        pnl_properties.Add(pe_properties.SizePos());

        pe_model.AddNumericInt("value", "Value", 68, 0, 1000, 1, "Progress");
        pe_model.AddNumericInt("total", "Total", 100, 1, 1000, 1, "Progress");
        pe_model.AddBoolean("show_percent", "Show percentage", true, "Progress");
        pe_model.AddBoolean("custom_text", "Use custom text", false, "Progress");
        pe_model.AddText("text", "Center text", "68%", "Progress");
        pe_model.AddBoolean("indeterminate", "Indeterminate", false, "Progress");
        pe_model.AddBoolean("animate_on_show", "Animate from zero on open", true, "Animation");
        pe_model.AddNumericInt("intro_duration", "Intro duration", 600, 120, 3000, 10, "Animation").SetUnit("ms");
        pe_model.AddNumericInt("indeterminate_duration", "Indeterminate duration", 1100, 240, 4000, 10, "Animation").SetUnit("ms");

        pe_model.AddColor("track_color", "Unused track", Color(229, 231, 235), "Ring");
        pe_model.AddColor("progress_start", "Progress start", Color(59, 130, 246), "Ring");
        pe_model.AddBoolean("gradient", "Use gradient", true, "Ring");
        pe_model.AddColor("progress_end", "Progress end", Color(37, 99, 235), "Ring");
        pe_model.AddColor("text_color", "Text colour", Color(17, 24, 39), "Ring");
        pe_model.AddNumericInt("thickness", "Thickness", 8, 1, 48, 1, "Ring").SetUnit("px");
        pe_model.AddNumericInt("cap_radius", "Cap radius", 4, 0, 24, 1, "Ring").SetUnit("px");
        pe_model.AddNumericInt("ring_inset", "Ring inset", 3, 0, 40, 1, "Ring").SetUnit("px");

        AddPropertyFont(pe_model, "font_face", "Font", StdFont().GetFaceName(), "Typography");
        pe_model.AddNumericInt("font_height", "Font size", 18, 6, 96, 1, "Typography").SetUnit("px");
        pe_model.AddBoolean("font_bold", "Bold", true, "Typography");
        pe_model.AddBoolean("font_italic", "Italic", false, "Typography");

        pe_model.AddNumericInt("width", "Rectangle width", 180, 24, 520, 1, "Layout").SetUnit("px");
        pe_model.AddNumericInt("height", "Rectangle height", 180, 24, 520, 1, "Layout").SetUnit("px");
        pe_model.AddBoolean("enabled", "Enabled", true, "Layout");

        pe_model.SetGroupSubtitle("Progress", "semantic value and center readout");
        pe_model.SetGroupSubtitle("Ring", "track, sweep, gradient and geometry");
        pe_model.SetGroupSubtitle("Typography", "preferred font; it shrinks only when needed");
        pe_model.SetGroupSubtitle("Layout", "allocated control rectangle");
        pe_model.StructureChanged();

        pe_properties.SetFactory(&pe_factory);
        pe_properties.SetModel(&pe_model);
        pe_properties.SetLabelRatio(46);
        PropertyEditorStyle style = PropertyEditorStyle::System();
        style.show_group_summaries = true;
        pe_properties.SetStyle(style);
    }

    void ConnectEvents()
    {
        auto changed = [=](String, Value) { ApplyProjection(); };
        pe_properties.WhenPreview = changed;
        pe_properties.WhenCommit = changed;
        btn_replay.WhenAction = [=] { ring_preview.RestartIntroAnimation(); };
        btn_exit.WhenAction = [=] { Close(); };
    }

    Value ValueOf(const String& id, const Value& fallback = Value()) const
    {
        const PropertyEditorItem *item = pe_model.Find(id);
        return item ? item->value : fallback;
    }

    void ApplyProjection()
    {
        UiProgressRing::Style style = UiProgressRing::StyleDefault();
        Color track = Color(ValueOf("track_color", Color(229, 231, 235)));
        Color progress_start = Color(ValueOf("progress_start", Color(59, 130, 246)));
        Color progress_end = Color(ValueOf("progress_end", Color(37, 99, 235)));
        Color text = Color(ValueOf("text_color", Color(17, 24, 39)));

        for(int st = 0; st < 4; st++) {
            style.track_palette.face[st] = UiFill::Solid(track);
            style.progress_palette.face[st] = UiFill::Solid(progress_start);
            style.gradient_end[st] = progress_end;
            style.text_palette.ink[st] = text;
        }
        style.track_palette.face[ST_DISABLED] = UiFill::Solid(Blend(track, SColorFace(), 160));
        style.progress_palette.face[ST_DISABLED] = UiFill::Solid(DisabledColor(progress_start));
        style.gradient_end[ST_DISABLED] = DisabledColor(progress_end);
        style.text_palette.ink[ST_DISABLED] = DisabledColor(text);
        style.gradient_enabled = (bool)ValueOf("gradient", true);
        style.thickness = max(1, (int)ValueOf("thickness", 8));
        style.cap_radius = max(0, (int)ValueOf("cap_radius", 4));
        style.ring_inset = max(0, (int)ValueOf("ring_inset", 3));
        style.animate_on_show = (bool)ValueOf("animate_on_show", true);
        style.intro_duration_ms = max(1, (int)ValueOf("intro_duration", 600));
        style.indeterminate_duration_ms = max(120, (int)ValueOf("indeterminate_duration", 1100));

        style.font.FaceName(AsString(ValueOf("font_face", StdFont().GetFaceName())));
        style.font.Height(max(1, (int)ValueOf("font_height", 18)));
        style.font.Bold((bool)ValueOf("font_bold", true));
        style.font.Italic((bool)ValueOf("font_italic", false));

        ring_preview.SetCustomStyle(style);
        ring_preview.Enable((bool)ValueOf("enabled", true));

        if((bool)ValueOf("custom_text", false))
            ring_preview.SetText(AsString(ValueOf("text", String())));
        else {
            ring_preview.ClearText();
            ring_preview.Percent((bool)ValueOf("show_percent", true));
        }

        if((bool)ValueOf("indeterminate", false))
            ring_preview.SetIndeterminate(true);
        else
            ring_preview.Set((int)ValueOf("value", 68), max(1, (int)ValueOf("total", 100)));

        RefreshLayout();
        Refresh();
    }

private:
    UiTitleCard tc_header;
    UiBoxLayout box_header_actions { UiDirection::H };
    UiButton btn_replay, btn_exit;

    UiPanel pnl_preview;
    UiProgressRing ring_preview;
    UiLabel lbl_caption;

    UiPanel pnl_properties;
    PropertyEditor pe_properties;
    PropertyEditorFactory pe_factory;
    PropertyEditorModel pe_model;
};

} // namespace

GUI_APP_MAIN
{
    UiThemeContext context;
    context.preset = UiThemePreset::Minimal;
    context.mode = UiThemeMode::Light;
    UiTheme::Set(context);

    UiProgressRingDemo demo;
    demo.Run();
}
