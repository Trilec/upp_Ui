#include <CtrlLib/CtrlLib.h>
#include <Ui/Ui.h>

using namespace Upp;

// -----------------------------------------------------------------------------
// UiBaseEdit / Flank demo
// -----------------------------------------------------------------------------

class UiBaseEditDemoWindow : public TopWindow {
public:
    typedef UiBaseEditDemoWindow CLASSNAME;

    UiBaseEditDemoWindow()
    {
        Title("Ui Base Edit / Flank Showcase");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(1100), DPI(720));

        // ---------------------------------------------------------------------
        // Header labels
        // ---------------------------------------------------------------------
        Add(header_title);
        header_title.SetText("Ui Base Edit / Flank Showcase");
        header_title.SetAlign(UiAlign::LEFT, UiAlign::TOP);
        header_title.SetInkColor(SColorText());

        Add(header_subtitle);
        header_subtitle.SetText(
            "Examples of UiLineEdit, UiPasswordEdit and UiMultiEdit using left/right flanks\n"
            "and simple per-row \"themes\" (corporate / pill / dark glass / tool).");
        header_subtitle.SetAlign(UiAlign::LEFT, UiAlign::TOP);
        header_subtitle.SetInkColor(SColorDisabled());

        // ---------------------------------------------------------------------
        // Row captions
        // ---------------------------------------------------------------------
        Add(row1_label);
        row1_label.SetText("1. Modern corporate search + password");
        row1_label.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        row1_label.SetInkColor(SColorText());

        Add(row2_label);
        row2_label.SetText("2. Soft pill search field with flanks");
        row2_label.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        row2_label.SetInkColor(SColorText());

        Add(row3_label);
        row3_label.SetText("3. Dark glass login");
        row3_label.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        row3_label.SetInkColor(SColorText());

        Add(row4_label);
        row4_label.SetText("4. Technical tool command + log");
        row4_label.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        row4_label.SetInkColor(SColorText());

        // ---------------------------------------------------------------------
        // ROW 1: Modern corporate – search + password, light theme
        // ---------------------------------------------------------------------
        Add(r1_search);
        r1_search.SetPlaceholder("Search clients…");
        r1_search.SetRadius(DPI(4));
        r1_search.SetFrameWidth(DPI(1));
        r1_search.Tip("UiLineEdit: standard framed search field");

        Add(r1_pass);
        r1_pass.SetPlaceholder("Password");
        r1_pass.SetRadius(DPI(4));
        r1_pass.SetFrameWidth(DPI(1));
        r1_pass.EnableVisibilityIcon(true);
        r1_pass.Tip("UiPasswordEdit: built-in eye icon via EnableVisibilityIcon(true)");

        Add(r1_primary);
        r1_primary.SetText("Search");
        r1_primary.SetMinSize(Size(DPI(110), DPI(30)));
        r1_primary.SetBaseColors(SColorHighlight(),
                                 SColorShadow(),
                                 SColorHighlightText());
        r1_primary.SetRadius(DPI(4));
        r1_primary.Tip("UiButton primary action");

        Add(r1_cancel);
        r1_cancel.SetText("Cancel");
        r1_cancel.SetMinSize(Size(DPI(100), DPI(30)));
        r1_cancel.SetStyle(UiButton::StyleSubtle());
        r1_cancel.Tip("UiButton subtle secondary action");

        // ---------------------------------------------------------------------
        // ROW 2: Soft & friendly – pill search with left/right flanks
        // ---------------------------------------------------------------------
        Add(r2_search);
        r2_search.SetPlaceholder("Search products…");
        r2_search.SetRadius(DPI(100));       // pill
        r2_search.EnableFrame(false);
        r2_search.SetFaceColor(Blend(SColorPaper(), SColorHighlight(), 235));
        r2_search.Tip("UiLineEdit: pill search field built from UiBaseEdit");

        // Left search icon button (flank)
        r2_search_icon.SetStyle(UiButton::StyleIcon());
        r2_search_icon.SetIcon(CtrlImg::go_forward());
        r2_search_icon.SetMinSize(Size(DPI(24), DPI(24)));
        r2_search_icon.ClickFocus(false);
        r2_search_icon.Tip("Left flank: search icon (UiButton)");

        // Right clear button (flank)
        r2_search_clear.SetStyle(UiButton::StyleIcon());
        r2_search_clear.SetIcon(CtrlImg::remove());
        r2_search_clear.SetMinSize(Size(DPI(24), DPI(24)));
        r2_search_clear.ClickFocus(false);
        r2_search_clear.Tip("Right flank: clear text");

        r2_search.AddToSide(r2_search_icon, UiAlign::LEFT, Size(DPI(24), DPI(24)));
        r2_search.AddToSide(r2_search_clear, UiAlign::RIGHT, Size(DPI(24), DPI(24)));

        r2_search_clear.WhenAction = [&] {
            r2_search.SetText(WString());    // clear content (UiBaseEdit expects WString)
        };

        Add(r2_notes);
        r2_notes.SetPlaceholder("Multi-line notes...");
        r2_notes.SetRadius(DPI(12));
        r2_notes.EnableFrame(false);
        r2_notes.SetFaceColor(Blend(SColorPaper(), SColorHighlight(), 240));
        r2_notes.Tip("UiMultiEdit with soft background / rounded corners");

        // ---------------------------------------------------------------------
        // ROW 3: Dark glass login – dark background, white ink, password eye
        // ---------------------------------------------------------------------
        Add(r3_user);
        r3_user.SetPlaceholder("Email or username");
        r3_user.SetRadius(DPI(6));
        r3_user.SetFrameWidth(0);
        r3_user.SetFaceColor(Color(30, 30, 30));
        r3_user.SetInkColor(White(), 0, 0);
        r3_user.Tip("Dark \"glass\" user field");

        Add(r3_pass);
        r3_pass.SetPlaceholder("Password");
        r3_pass.SetRadius(DPI(6));
        r3_pass.SetFrameWidth(0);
        r3_pass.SetFaceColor(Color(30, 30, 30));
        r3_pass.SetInkColor(White(), 0, 0);
        r3_pass.EnableVisibilityIcon(true);
        r3_pass.Tip("Dark \"glass\" password field with eye toggle");

        Add(r3_login);
        r3_login.SetText("Sign In");
        r3_login.SetBaseColors(Color(70, 130, 180),  // steel-ish blue
                               Color(90, 150, 200),
                               White());
        r3_login.SetRadius(DPI(6));
        r3_login.Tip("Login button arranged next to password field");

        // ---------------------------------------------------------------------
        // ROW 4: Technical tool – sharp corners, thick frames, log window
        // ---------------------------------------------------------------------
        Add(r4_cmd);
        r4_cmd.SetPlaceholder("Command palette…");
        r4_cmd.SetRadius(0);
        r4_cmd.SetFrameWidth(DPI(2));
        r4_cmd.SetFrameColor(Color(20, 20, 20));
        r4_cmd.SetFaceColor(Color(250, 250, 250));
        r4_cmd.Tip("Command entry (UiLineEdit) with thick technical frame");

        Add(r4_log);
        r4_log.SetPlaceholder("Log output…");
        r4_log.SetRadius(0);
        r4_log.SetFrameWidth(DPI(2));
        r4_log.SetFrameColor(Color(30, 30, 30));
        r4_log.SetFaceColor(Color(245, 245, 245));
        r4_log.Tip("UiMultiEdit used as a scrolling log pane");

        Add(r4_tool);
        r4_tool.SetText("Tool");
        r4_tool.SetMinSize(Size(DPI(70), DPI(70)));
        r4_tool.SetStyle(UiButton::StyleIcon());
        r4_tool.SetIcon(CtrlImg::go_forward());
        r4_tool.SetIconLayout(UiAlign::TOP);
        r4_tool.SetRadius(0);
        r4_tool.SetFrameWidth(DPI(2));
        r4_tool.SetFrameColor(Color(40, 40, 40));
        r4_tool.SetFaceColor(Color(230, 230, 230));
        r4_tool.Tip("Icon TOP – toolbar style button");
    }

    // Simple header band; everything else is done by child controls.
    virtual void Paint(Draw& w) override
    {
        Rect r = GetSize();
        w.DrawRect(r, SColorPaper());

        int header_h = DPI(80);
        w.DrawRect(0, 0, r.GetWidth(), header_h, SColorFace());

        Font title = SansSerifZ(20).Bold();
        Font desc  = SansSerifZ(10);

        w.DrawText(DPI(32), DPI(12),
                   "Ui Base Edit / Flank Showcase",
                   title, SColorText());
        w.DrawText(DPI(32), DPI(40),
                   "UiLineEdit / UiPasswordEdit / UiMultiEdit with left/right flanks",
                   desc, SColorText());
    }

    virtual void Layout() override
    {
        Rect r = GetSize();

        int header_h  = DPI(90);
        int margin_x  = DPI(32);
        int margin_y  = DPI(20);
        int row_gap   = DPI(16);
        int label_w   = DPI(260);
        int content_h = r.GetHeight() - header_h - margin_y * 2;
        int row_h     = content_h / 4;

        // Header labels
        header_title.SetRect(margin_x, DPI(10),
                             r.GetWidth() - 2 * margin_x, DPI(24));
        header_subtitle.SetRect(margin_x, DPI(40),
                                r.GetWidth() - 2 * margin_x, DPI(40));

        auto LayoutRow = [&](int idx,
                             UiLabel& caption,
                             auto&& layout_row_controls) {
            int row_top = header_h + margin_y + idx * (row_h + row_gap);
            Rect row_rect(margin_x,
                          row_top,
                          r.GetWidth() - margin_x,
                          row_top + row_h);

            // Caption on the left
            int cap_h = DPI(24);
            caption.SetRect(row_rect.left,
                            row_rect.top + (row_h - cap_h) / 2,
                            label_w,
                            cap_h);

            Rect controls_rect(row_rect.left + label_w + DPI(16),
                               row_rect.top,
                               row_rect.right,
                               row_rect.bottom);

            layout_row_controls(controls_rect);
        };

        // Row 1 layout
        LayoutRow(0, row1_label, [&](const Rect& rr) {
            int cols  = 4;
            int gap   = DPI(10);
            int col_w = (rr.GetWidth() - gap * (cols - 1)) / cols;

            auto place = [&](Ctrl& c, int col, int min_h = DPI(30)) {
                Size ms = c.GetMinSize();
                int w   = max(ms.cx, col_w);
                int h   = max(ms.cy, min_h);
                int x   = rr.left + col * (col_w + gap);
                int y   = rr.top + (rr.GetHeight() - h) / 2;
                c.SetRect(x, y, w, h);
            };

            place(r1_search, 0);
            place(r1_pass,   1);
            place(r1_primary,2);
            place(r1_cancel, 3);
        });

        // Row 2 layout
        LayoutRow(1, row2_label, [&](const Rect& rr) {
            int cols  = 2;
            int gap   = DPI(10);
            int col_w = (rr.GetWidth() - gap * (cols - 1)) / cols;

            // Search pill on the left
            {
                Size ms = r2_search.GetMinSize();
                int w   = max(ms.cx, col_w);
                int h   = max(ms.cy, DPI(34));
                int x   = rr.left;
                int y   = rr.top + (rr.GetHeight() - h) / 2;
                r2_search.SetRect(x, y, w, h);
            }

            // Multi-edit on the right (taller)
            {
                int x = rr.left + col_w + gap;
                int y = rr.top + DPI(4);
                int w = col_w;
                int h = rr.GetHeight() - DPI(8);
                r2_notes.SetRect(x, y, w, h);
            }
        });

        // Row 3 layout
        LayoutRow(2, row3_label, [&](const Rect& rr) {
            int cols  = 3;
            int gap   = DPI(10);
            int col_w = (rr.GetWidth() - gap * (cols - 1)) / cols;

            auto place = [&](Ctrl& c, int col, int min_h = DPI(32)) {
                Size ms = c.GetMinSize();
                int w   = max(ms.cx, col_w);
                int h   = max(ms.cy, min_h);
                int x   = rr.left + col * (col_w + gap);
                int y   = rr.top + (rr.GetHeight() - h) / 2;
                c.SetRect(x, y, w, h);
            };

            place(r3_user,  0);
            place(r3_pass,  1);
            place(r3_login, 2);
        });

        // Row 4 layout
        LayoutRow(3, row4_label, [&](const Rect& rr) {
            int cols  = 3;
            int gap   = DPI(10);
            int col_w = (rr.GetWidth() - gap * (cols - 1)) / cols;

            // Left: tool button, vertically centred
            {
                Size ms = r4_tool.GetMinSize();
                int w   = max(ms.cx, col_w);
                int h   = max(ms.cy, DPI(64));
                int x   = rr.left;
                int y   = rr.top + (rr.GetHeight() - h) / 2;
                r4_tool.SetRect(x, y, w, h);
            }

            // Right side: command line + log stacked (2 columns width)
            {
                int x = rr.left + col_w + gap;
                int w = col_w * 2 - gap; // last two columns
                int h_row = rr.GetHeight();

                int h_cmd = DPI(26);
                r4_cmd.SetRect(x, rr.top, w, h_cmd);

                int h_log = h_row - h_cmd - DPI(8);
                r4_log.SetRect(x, rr.top + h_cmd + DPI(4), w, h_log);
            }
        });
    }

private:
    // Header
    UiLabel header_title;
    UiLabel header_subtitle;

    // Row captions
    UiLabel row1_label;
    UiLabel row2_label;
    UiLabel row3_label;
    UiLabel row4_label;

    // Row 1
    UiLineEdit    r1_search;
    UiPasswordEdit r1_pass;
    UiButton      r1_primary;
    UiButton      r1_cancel;

    // Row 2
    UiLineEdit   r2_search;
    UiButton     r2_search_icon;
    UiButton     r2_search_clear;
    UiMultiEdit  r2_notes;

    // Row 3
    UiLineEdit    r3_user;
    UiPasswordEdit r3_pass;
    UiButton      r3_login;

    // Row 4
    UiLineEdit   r4_cmd;
    UiMultiEdit  r4_log;
    UiButton     r4_tool;
};

GUI_APP_MAIN
{
    UiBaseEditDemoWindow().Run();
}
