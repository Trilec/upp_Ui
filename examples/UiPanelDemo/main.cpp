#include <CtrlCore/CtrlCore.h>
#include <Ui/Ui.h>
#include <Painter/Painter.h>

using namespace Upp;

// ---------------------------------------------------------------------------
// Soft-UI skin helper (9-slice card)
// ---------------------------------------------------------------------------

static const int SOFT_SKIN_SIZE   = DPI(30);  // total image size
static const int SOFT_SKIN_FACE   = DPI(22);  // inner face square
static const int SOFT_SKIN_MARGIN = 10;       // For 30 / 22 this ends up ~4px.

// Simple "soft-UI" 9-slice skin – white card with subtle shadow + border.
static Image MakeNineSliceSkin()
{
    const int size = SOFT_SKIN_SIZE;
    ImageBuffer ib(size, size);
    Fill(~ib, RGBAZero(), ib.GetLength());

    double r       = DPI(4);
    double sz_face = SOFT_SKIN_FACE;

    double face_x = DPI(1);
    double face_y = DPI(1);

    double shadow_off_x = DPI(1.0);
    double shadow_off_y = DPI(3.0);

    // 1. Draw shadow base
    {
        BufferPainter p(ib, MODE_ANTIALIASED);
        p.Begin();
        p.RoundedRectangle(face_x + shadow_off_x,
                           face_y + shadow_off_y,
                           sz_face, sz_face, r);
        p.Fill(Color(140, 140, 140));
        p.End();
    }

    // 2. Apply Fast Blur (from UiStyle.h)
    FastBlur(ib, 4);
    FastBlur(ib, 4);

    // 3. Draw face + borders
    {
        BufferPainter p(ib, MODE_ANTIALIASED);
        p.Begin();

        // Face fill
        p.RoundedRectangle(face_x, face_y, sz_face, sz_face, r);
        p.Fill(Color(240, 240, 240));

        // Outer border
        p.RoundedRectangle(face_x, face_y, sz_face, sz_face, r);
        p.Stroke(1.0, Color(180, 180, 180));

        // Inner bezel (highlight)
        p.RoundedRectangle(face_x + 1.0, face_y + 1.0,
                           sz_face - 2.0, sz_face - 2.0,
                           max(0.0, r - 1.0));
        p.Stroke(2.5, Color(255, 255, 255));
        p.End();
    }

    return ib;
}

// ---------------------------------------------------------------------------
// Demo window showing 4 panel variants
// ---------------------------------------------------------------------------

class UiPanelDemo : public TopWindow {
public:
    typedef UiPanelDemo CLASSNAME;

    UiPanelDemo()
    {
        Title("UiPanel Demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(640), DPI(480));

        Add(grad_panel);
        Add(dark_panel);
        Add(flat_panel);
        Add(skin_panel);

        // Gradient panel: light card with quad gradient.
        grad_panel.SetStyle(UiTheme::ResolvePanel(UiPanelRole::Surface))
                  .SetSizeMin(DPI(120), DPI(80));
        grad_panel.SetFaceQuadGradient(
            Color(243, 247, 255),  // TL
            Color(230, 240, 255),  // TR
            Color(220, 230, 250),  // BL
            Color(210, 220, 250),  // BR
            32, 3);
        grad_panel.SetRadius(DPI(12));

        // Dark panel: uses StyleDark().
        dark_panel.SetStyle(UiTheme::ResolvePanel(UiThemePreset::Minimal, UiThemeMode::Dark, UiPanelRole::Surface))
                  .SetSizeMin(DPI(120), DPI(80));

        // Flat square panel: explicit light surface with a clear lower-right shadow.
        UiPanel::Style flat_style = UiTheme::ResolvePanel(UiThemePreset::Linear, UiThemeMode::Light, UiPanelRole::Surface);
        flat_style.metrics.face_enabled = true;
        flat_style.metrics.frame_enabled = true;
        flat_style.metrics.radius = 0;
        for(int i = 0; i < 4; i++) {
            flat_style.palette.face[i] = UiFill::Solid(Color(255, 255, 255));
            flat_style.palette.frame[i] = Color(214, 223, 235);
        }
        flat_style.metrics.shadow.enabled = true;
        flat_style.metrics.shadow.inset = false;
        flat_style.metrics.shadow.distance = DPI(6);
        flat_style.metrics.shadow.angle = 45;
        flat_style.metrics.shadow.alpha = 120;
        flat_style.metrics.shadow.blur = DPI(18);
        flat_style.metrics.shadow.color = Color(48, 62, 84);
        flat_panel.SetStyle(flat_style)
                  .SetSizeMin(DPI(120), DPI(80));

        // Nine-slice panel with soft-UI card skin.
        Image skin_img = MakeNineSliceSkin();
        skin_panel.SetStyle(UiTheme::ResolvePanel(UiThemePreset::Linear, UiThemeMode::Light, UiPanelRole::Surface))
                  .SetSizeMin(DPI(120), DPI(80));
        skin_panel.SetFill9Slice(skin_img, SOFT_SKIN_MARGIN, true);
        skin_panel.EnableFace(false).EnableFrame(false); // skin owns the look
    }

    // Header paint (same pattern as UiLabel demo)
    virtual void Paint(Draw& w) override
    {
        Rect r = GetSize();
        w.DrawRect(r, SColorPaper());

        int head_h = DPI(80);
        w.DrawRect(0, 0, r.GetWidth(), head_h, SColorFace());

        Font title = SansSerifZ(24).Bold();
        Font desc  = SansSerifZ(12);

		w.DrawText(DPI(32), DPI(10), "UiPanel Demo", title, SColorText());
		
		String line1 = "Styled background / container control built on UiStyle / UiDraw.";
		String line2 = "Shows gradient, dark, flat and 9-slice \"soft\" panels.";
		
		w.DrawText(DPI(32), DPI(40), line1, desc, SColorText());
		
		int  line_gap = DPI(2);
		int  y2 = DPI(40) + max(1, desc.GetCy()) + line_gap;
		
		w.DrawText(DPI(32), y2, line2, desc, SColorText());

    }

    virtual void Layout() override
    {
        Rect r = GetSize();
        int head_h = DPI(80);
        int gap    = DPI(24);

        // Area below the header
        Rect area = r;
        area.top += head_h + gap;
        area.Deflate(gap, gap);

        int mid_x = (area.left + area.right) / 2;
        int mid_y = (area.top  + area.bottom) / 2;

        Rect tl(area.left,        area.top,
                mid_x - gap / 2,  mid_y - gap / 2);
        Rect tr(mid_x + gap / 2,  area.top,
                area.right,       mid_y - gap / 2);
        Rect bl(area.left,        mid_y + gap / 2,
                mid_x - gap / 2,  area.bottom);
        Rect br(mid_x + gap / 2,  mid_y + gap / 2,
                area.right,       area.bottom);

        grad_panel.SetRect(tl);
        dark_panel.SetRect(tr);
        flat_panel.SetRect(bl);
        skin_panel.SetRect(br);
    }

private:
    UiPanel grad_panel;
    UiPanel dark_panel;
    UiPanel flat_panel;
    UiPanel skin_panel;
};

GUI_APP_MAIN
{
    UiPanelDemo().Run();
}







