#include <Ui/Ui.h>
#include <Ui/UiColorPicker.h>

using namespace Upp;

namespace {

UiPanel::Style MakePickerHostStyle()
{
    UiPanel::Style s = UiTheme::ResolvePanel(UiThemePreset::Minimal, UiThemeMode::Dark, UiPanelRole::Surface);
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::Solid(Color(17, 17, 17));
        s.palette.frame[i] = Color(34, 34, 34);
        s.palette.ink[i] = Color(208, 208, 208);
    }
    s.metrics.face_enabled = true;
    s.metrics.frame_enabled = true;
    s.metrics.frame_width = DPI(1);
    s.metrics.radius = DPI(8);
    s.metrics.focus_enabled = false;
    s.metrics.content_margin = Rect(0, 0, 0, 0);
    s.metrics.shadow.enabled = false;
    return s;
}

class ColorPickerDemoWindow : public TopWindow {
public:
    typedef ColorPickerDemoWindow CLASSNAME;

    ColorPickerDemoWindow()
    {
        UiThemeContext ctx = UiTheme::GetContext();
        ctx.preset = UiThemePreset::Minimal;
        ctx.mode = UiThemeMode::Dark;
        UiTheme::SetContext(ctx);

        Title("UiColorPickerDemo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(960), DPI(720));
        SetMinSize(Size(DPI(800), DPI(620)));

        Add(host_);
        host_.Add(picker_);
        host_.SetStyle(MakePickerHostStyle());

        picker_.SetSlotCount(4)
               .SetSlotLabel(0, "C1")
               .SetSlotLabel(1, "C2")
               .SetSlotLabel(2, "C3")
               .SetSlotLabel(3, "C4")
               .SetAlphaEnabled(true)
               .SetActiveSlot(0);
    }

    virtual void Paint(Draw& w) override
    {
        w.DrawRect(GetSize(), Color(8, 8, 8));
    }

    virtual void Layout() override
    {
        Rect r = GetSize();
        Size target(DPI(720), DPI(560));
        int w = min(r.GetWidth() - DPI(24), target.cx);
        int h = min(r.GetHeight() - DPI(24), target.cy);
        Rect host = RectC((r.GetWidth() - w) / 2, (r.GetHeight() - h) / 2, w, h);
        host_.SetRect(host);
        picker_.SetRect(host_.GetSize());
    }

private:
    UiPanel       host_;
    UiColorPicker picker_;
};

}

GUI_APP_MAIN
{
    StdLogSetup(LOG_COUT|LOG_FILE);
    ColorPickerDemoWindow win;
    win.Run();
}
