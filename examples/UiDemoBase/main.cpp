#include <CtrlLib/CtrlLib.h>   // full GUI stack
#include <Ui/UiStyle.h>        // our new styling engine (doesn't have to be used yet)

using namespace Upp;

class UiDemoBaseWindow : public TopWindow {
public:
    typedef UiDemoBaseWindow CLASSNAME;

    UiDemoBaseWindow()
    {
        Title("Ui – Generic UI Demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(900), DPI(600));
    }

    virtual void Paint(Draw& w) override
    {
        Rect r = GetSize();

        // --------------------------------------------------------------------
        // 1) Background
        // --------------------------------------------------------------------
        w.DrawRect(r, SColorPaper()); // neutral background

        // --------------------------------------------------------------------
        // 2) Header band
        // --------------------------------------------------------------------
        Rect header = r;
        header.bottom = header.top + DPI(96);

        // Use Face color for header band
        w.DrawRect(header, SColorFace());

        // Title and subtitle fonts
        Font title    = SansSerifZ(32).Bold();
        Font subtitle = SansSerifZ(14);
        Font body     = SansSerifZ(11);

        int x = header.left + DPI(32);
        int y = header.top  + DPI(20);

        // Title
        w.DrawText(x, y, "Ui – Generic UI Demo", title, SColorText());
        y += title.GetHeight() + DPI(4);

        // Subtitle
        w.DrawText(x, y,
                   "A sandbox window for developing and showcasing Ui* controls.",
                   subtitle,
                   SColorText());

        // --------------------------------------------------------------------
        // 3) Body copy / description
        // --------------------------------------------------------------------
        int body_x = header.left + DPI(32);
        int body_y = header.bottom + DPI(16);

        w.DrawText(body_x, body_y,
                   "This demo window is intentionally minimal:",
                   body, SColorText());
        body_y += body.GetHeight() + DPI(2);

        w.DrawText(body_x, body_y,
                   "- No legacy CtrlLib widgets are used.",
                   body, SColorText());
        body_y += body.GetHeight() + DPI(2);

        w.DrawText(body_x, body_y,
                   "- The top band draws the title, subtitle, and explanation text.",
                   body, SColorText());
        body_y += body.GetHeight() + DPI(2);

        w.DrawText(body_x, body_y,
                   "- The area on the right is reserved for future Ui* control examples.",
                   body, SColorText());
        body_y += body.GetHeight() + DPI(8);

        // --------------------------------------------------------------------
        // 4) "Controls area" placeholder
        // --------------------------------------------------------------------
        Rect controls_area = r;
        controls_area.top  = header.bottom + DPI(8);
        controls_area.left = r.CenterPoint().x + DPI(16);
        controls_area.Deflate(DPI(24), DPI(24));

        // Draw placeholder frame
        Color frame = SColorShadow();
        w.DrawRect(controls_area, SColorPaper());
        // top
        w.DrawRect(controls_area.left, controls_area.top,
                   controls_area.GetWidth(), 1, frame);
        // bottom
        w.DrawRect(controls_area.left, controls_area.bottom - 1,
                   controls_area.GetWidth(), 1, frame);
        // left
        w.DrawRect(controls_area.left, controls_area.top,
                   1, controls_area.GetHeight(), frame);
        // right
        w.DrawRect(controls_area.right - 1, controls_area.top,
                   1, controls_area.GetHeight(), frame);

        // Placeholder caption centered
        String placeholder = "Ui* Controls Area\n"
                             "(UiButton, UiLabel, UiCheckBox, UiSlider, etc.)";

        Size text_sz = GetTextSize(placeholder, body);
        int  tx = controls_area.CenterPoint().x - text_sz.cx / 2;
        int  ty = controls_area.CenterPoint().y - text_sz.cy / 2;

        w.DrawText(tx, ty, placeholder, body, SColorDisabled());
    }
};

GUI_APP_MAIN
{
    UiDemoBaseWindow().Run();
}
