#include <Ui/Ui.h>

using namespace Upp;

namespace {

class UiProgressBarDemo : public TopWindow {
public:
    typedef UiProgressBarDemo CLASSNAME;

    UiProgressBarDemo()
    {
        Title("UiProgressBarDemo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(680), DPI(420));

        UiLabel::Style title_style = UiTheme::ResolveLabel(UiLabelRole::Body);
        title_style.font = SansSerifZ(22).Bold();
        title_.SetCustomStyle(title_style).SetText("UiProgressBar");
        note_.SetText("Determinate, vertical, role-tuned, and indeterminate progress. Small bar, honest job.");

        standard_.Set(35, 100).Percent();
        subtle_.SetCustomStyle(UiTheme::ResolveProgressBar(UiRole::Subtle));
        subtle_.Set(62, 100).Percent();
        accent_.SetCustomStyle(UiTheme::ResolveProgressBar(UiRole::Accent));
        accent_.Set(78, 100).Percent();
        alert_.SetCustomStyle(UiTheme::ResolveProgressBar(UiRole::Alert));
        alert_.Set(42, 100).Percent();

        vertical_.SetOrientation(UiProgressBar::Orientation::Vertical).Set(70, 100).Percent();
        indeterminate_.SetIndeterminate(true).SetText("Working");

        Add(title_);
        Add(note_);
        Add(standard_);
        Add(subtle_);
        Add(accent_);
        Add(alert_);
        Add(indeterminate_);
        Add(vertical_);
    }

    void Layout() override
    {
        Rect r = GetSize();
        r.Deflate(DPI(24), DPI(22));

        title_.SetRect(r.left, r.top, r.GetWidth(), DPI(32));
        note_.SetRect(r.left, r.top + DPI(36), r.GetWidth(), DPI(24));

        int y = r.top + DPI(86);
        int w = min(DPI(470), r.GetWidth() - DPI(110));
        int h = DPI(24);
        standard_.SetRect(r.left, y, w, h); y += DPI(42);
        subtle_.SetRect(r.left, y, w, h); y += DPI(42);
        accent_.SetRect(r.left, y, w, h); y += DPI(42);
        alert_.SetRect(r.left, y, w, h); y += DPI(52);
        indeterminate_.SetRect(r.left, y, w, h);

        vertical_.SetRect(r.left + w + DPI(44), r.top + DPI(86), DPI(34), DPI(214));
    }

private:
    UiLabel title_;
    UiLabel note_;
    UiProgressBar standard_;
    UiProgressBar subtle_;
    UiProgressBar accent_;
    UiProgressBar alert_;
    UiProgressBar indeterminate_;
    UiProgressBar vertical_;
};

}

GUI_APP_MAIN
{
    UiThemeContext ctx;
    ctx.preset = UiThemePreset::Minimal;
    ctx.mode = UiThemeMode::Light;
    UiTheme::Set(ctx);
    UiProgressBarDemo demo;
    demo.Run();
}
