#include <CtrlLib/CtrlLib.h>
#include <Ui/UiPasswordEdit.h>
#include <Ui/UiButton.h>

using namespace Upp;

class UiPasswordEditDemoWindow : public TopWindow {
public:
    typedef UiPasswordEditDemoWindow CLASSNAME;

    UiPasswordEditDemoWindow()
    {
        Title("UiPasswordEdit Demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(900), DPI(600));

        // Register controls
        Add(lbl_standard);      Add(pass_standard);
        Add(lbl_custom_char);   Add(pass_custom_char);
        Add(lbl_toggle_right);  Add(pass_toggle_right);
        Add(lbl_plain_visible); Add(pass_plain_visible);
        Add(lbl_login_style);   Add(pass_login);

        // -----------------------------------------------------------------
        // 1. Standard masked password
        // -----------------------------------------------------------------
        lbl_standard.SetText("Standard:");
        pass_standard.SetPlaceholder("Enter password");
        pass_standard.SetTip("Default masked password field using UiPasswordEdit.");

        // -----------------------------------------------------------------
        // 2. Custom mask character
        // -----------------------------------------------------------------
        lbl_custom_char.SetText("Custom mask char:");
        pass_custom_char.SetPlaceholder("Password with '*'");
        pass_custom_char.SetPasswordChar('*');
        pass_custom_char.SetTip("Mask character changed via SetPasswordChar('*').");

        // -----------------------------------------------------------------
        // 3. Toggle visibility (right-side visibility icons)
        // -----------------------------------------------------------------
        lbl_toggle_right.SetText("Toggle visibility:");
        pass_toggle_right.SetPlaceholder("Click eye icon to show / hide");
        pass_toggle_right.EnableVisibilityIcon(true);

        pass_toggle_right.SetTip(
            "Visibility toggle using built-in design visibility icons."
        );

        // -----------------------------------------------------------------
        // 4. Always-visible password (no masking)
        // -----------------------------------------------------------------
        lbl_plain_visible.SetText("Plain visible:");
        pass_plain_visible.SetPlaceholder("Visible text (no masking)");
        pass_plain_visible.SetPlainTextVisible(true);
        pass_plain_visible.SetTip(
            "Masking disabled via SetPlainTextVisible(true) – handy for debugging."
        );

        // -----------------------------------------------------------------
        // 5. Login-style composite (styled + visibility + submit arrow)
        // -----------------------------------------------------------------
        lbl_login_style.SetText("Login-style field:");

        pass_login.SetPlaceholder("Password");

        // a) Use built-in visibility icons
        pass_login
             .EnableVisibilityIcon(true);

        pass_login.SetTip(
            "Styled login field: rounded, dark background, inline visibility toggle "
            "and a submit arrow attached on the right."
        );

        // b) Style the password edit to look like a dark, slightly rounded login box
        {
            UiBaseEdit::Style s = pass_login.GetStyle();

            // Dark-ish blue/grey base
            Color face  = Color(30, 50, 80);
            Color frame = Color(90, 130, 170);
            Color ink   = Color(235, 240, 245);

            s.palette.face[ST_NORMAL]   = face;
            s.palette.face[ST_HOT]      = DkColor(face, 10);
            s.palette.face[ST_PRESSED]  = DkColor(face, 20);
            s.palette.face[ST_DISABLED] = DisabledColor(face);

            s.palette.frame[ST_NORMAL]   = frame;
            s.palette.frame[ST_HOT]      = LtColor(frame, 15);
            s.palette.frame[ST_PRESSED]  = DkColor(frame, 10);
            s.palette.frame[ST_DISABLED] = DisabledColor(frame);

            s.palette.ink[ST_NORMAL]   = ink;
            s.palette.ink[ST_HOT]      = ink;
            s.palette.ink[ST_PRESSED]  = ink;
            s.palette.ink[ST_DISABLED] = SColorDisabled();

            s.metrics.radius      = DPI(4);  // pill-ish corners
            s.metrics.frame_width = DPI(1);

            s.metrics.content_padding = Rect(DPI(8), DPI(4), DPI(8), DPI(4));

            s.caret_color     = ink;
            s.placeholder_ink = Blend(ink, face, 160);

            pass_login.SetStyle(s);
        }

        // c) Configure the submit arrow button as a right flank
        {
            // Style the arrow button so it visually merges with the edit
            UiButton::Style bs = UiButton::StyleDefault();

            // Match background + frame as closely as possible
            Color face  = Color(38, 58, 88);
            Color frame = Color(90, 130, 170);
            Color ink   = Color(235, 240, 245);

            bs.palette.face[ST_NORMAL]   = face;
            bs.palette.face[ST_HOT]      = DkColor(face, 10);
            bs.palette.face[ST_PRESSED]  = DkColor(face, 20);
            bs.palette.face[ST_DISABLED] = DisabledColor(face);

            bs.palette.frame[ST_NORMAL]   = frame;
            bs.palette.frame[ST_HOT]      = LtColor(frame, 15);
            bs.palette.frame[ST_PRESSED]  = DkColor(frame, 10);
            bs.palette.frame[ST_DISABLED] = DisabledColor(frame);

            bs.palette.ink[ST_NORMAL]   = ink;
            bs.palette.ink[ST_HOT]      = ink;
            bs.palette.ink[ST_PRESSED]  = ink;
            bs.palette.ink[ST_DISABLED] = SColorDisabled();

            bs.metrics.radius      = DPI(4);
            bs.metrics.frame_width = DPI(0);

            bs.metrics.content_padding = Rect(DPI(6), DPI(2), DPI(6), DPI(2));

            login_submit_btn.SetStyle(bs);
            login_submit_btn.SetText(String());             // icon-only
            login_submit_btn.SetIcon(CtrlImg::go_forward());
            login_submit_btn.SetIconTintMono(false);
            login_submit_btn.ClickFocus(false);             // keep focus on edit
            login_submit_btn.SetMinSize(Size(DPI(32), 0));  // width ~= height

            // Attach as right-hand flank; size auto-tracks edit height
            pass_login.AddToSide(login_submit_btn, UiAlign::RIGHT, Size(0, 0), UiDirection::H);

            // Simple behaviour: when clicked, "submit" the password
            login_submit_btn.WhenAction = [=] {
                String pwd = pass_login.GetText().ToString();
                PromptOK(Format("Submitting password: \"%s\"", pwd));
            };
        }
    }

    // ---------------------------------------------------------------------
    // Nice painted header + label column (same general vibe as other demos)
    // ---------------------------------------------------------------------
    virtual void Paint(Draw& w) override
    {
        Rect r = GetSize();
        w.DrawRect(r, SColorPaper()); // background

        // Header band
        int head_h = DPI(110);
        Rect header = r;
        header.bottom = header.top + head_h;
        w.DrawRect(header, SColorFace());

        Font title    = SansSerifZ(24).Bold();
        Font subtitle = SansSerifZ(12);
        Font body     = SansSerifZ(11);

        int x = DPI(32);
        int y = DPI(16);

        w.DrawText(x, y, "UiPasswordEdit Demo", title, SColorText());
        y += title.GetHeight() + DPI(4);

        w.DrawText(
            x, y,
            "Password entry built on UiLineEdit / UiBaseEdit.",
            subtitle, SColorText()
        );
        y += subtitle.GetHeight() + DPI(2);

        w.DrawText(
            x, y,
            "Shows masking, custom mask chars, visibility toggles, plain-visible "
            "mode, and a styled login-style composite with flanking controls.",
            body, SColorText()
        );

        // Left label column background
        int margin        = DPI(20);
        int label_panel_w = DPI(180);
        Rect panel = r;
        panel.top    = header.bottom + DPI(12);
        panel.bottom = r.bottom - DPI(16);
        panel.left   = margin;
        panel.right  = margin + label_panel_w;

        w.DrawRect(panel, Blend(SColorFace(), SColorPaper(), 220));

        // Column headers
        Font colhdr = SansSerifZ(10).Bold();
        Color hdrc  = SColorDisabled();

        int y_hdr = header.bottom + DPI(16);
        w.DrawText(margin + DPI(8), y_hdr, "Variant", colhdr, hdrc);

        int right_hdr_x = r.right - DPI(260);
        w.DrawText(right_hdr_x, y_hdr, "Notes / Behaviour", colhdr, hdrc);
    }

    virtual void Layout() override
    {
        Rect r = GetSize();

        int header_h      = DPI(110);
        int margin        = DPI(20);
        int label_panel_w = DPI(180);
        int label_w       = label_panel_w - DPI(16); // inside shaded panel
        int vgap          = DPI(10);
        int row_hmin      = DPI(32);

        int x_label  = margin + DPI(8);
        int x_edit   = x_label + label_w + DPI(20);
        int edit_w   = r.right - margin - x_edit;

        int y = header_h + DPI(40);

        auto PlaceRow = [&](Label& lbl, Ctrl& ctrl) {
            Size ms = ctrl.GetMinSize();
            int  h  = max(row_hmin, ms.cy);

            lbl.SetRect(x_label, y, label_w, h);
            ctrl.SetRect(x_edit,  y, edit_w,  h);
            y += h + vgap;
        };

        PlaceRow(lbl_standard,      pass_standard);
        PlaceRow(lbl_custom_char,   pass_custom_char);
        PlaceRow(lbl_toggle_right,  pass_toggle_right);
        PlaceRow(lbl_plain_visible, pass_plain_visible);
        PlaceRow(lbl_login_style,   pass_login);
    }

private:
    // Labels
    Label lbl_standard;
    Label lbl_custom_char;
    Label lbl_toggle_right;
    Label lbl_plain_visible;
    Label lbl_login_style;

    // Password edits
    UiPasswordEdit pass_standard;
    UiPasswordEdit pass_custom_char;
    UiPasswordEdit pass_toggle_right;
    UiPasswordEdit pass_plain_visible;

    // Login-style composite
    UiPasswordEdit pass_login;
    UiButton       login_submit_btn;
};

GUI_APP_MAIN
{
    Ctrl::GlobalBackPaint();
    UiPasswordEditDemoWindow().Run();
}
