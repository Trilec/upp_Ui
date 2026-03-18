#include <CtrlLib/CtrlLib.h>
#include <Ui/UiBaseEdit.h>
#include <Ui/UiLineEdit.h>
#include <Ui/UiButton.h>
#include <Ui/UiTheme.h>

using namespace Upp;

class UiLineEditDemo : public TopWindow {
public:
    typedef UiLineEditDemo CLASSNAME;

    UiLineEditDemo()
    {
        Title("UiLineEdit Demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(900), DPI(650)); // Increased height for extra row

        // Add controls to the window
        Add(lbl_standard);
        Add(edit_standard);
        Add(lbl_center);
        Add(edit_center);
        Add(lbl_right);
        Add(edit_right);
        Add(lbl_placeholder);
        Add(edit_placeholder);
        Add(lbl_readonly);
        Add(edit_readonly);
        Add(lbl_disabled);
        Add(edit_disabled);
        Add(lbl_custom);
        Add(edit_custom);

        // New: Icon Left
        Add(lbl_icon_left);
        Add(edit_icon_left);

        // Existing: Icon Right
        Add(lbl_icon);
        Add(edit_icon);

        // New: Side controls (left user, right search + clear)
        Add(lbl_side);
        Add(edit_side);

        // WhenAction demo
        Add(lbl_action);
        Add(edit_action);
        Add(lbl_action_info); // info label on the right

        // 1. Standard (left-aligned)
        lbl_standard.SetText("Standard:");
        edit_standard.SetData("You can edit this");
        edit_standard.SetTip("Basic UiLineEdit using UiBaseEdit's text model.");

        // 2. Center-aligned
        lbl_center.SetText("Center-aligned:");
        edit_center.SetTextAlign(UiAlign::CENTER);
        edit_center.SetData("Centered text");
        edit_center.SetTip("Text is horizontally centered using UiAlign::CENTER.");

        // 3. Right-aligned (typical numeric)
        lbl_right.SetText("Right-aligned:");
        edit_right.SetTextAlign(UiAlign::RIGHT);
        edit_right.SetData("12345.67");
        edit_right.SetTip("Right-aligned ??? useful for numeric input.");

        // 4. Placeholder
        lbl_placeholder.SetText("Placeholder:");
        edit_placeholder.SetPlaceholder("Type here...");
        edit_placeholder.SetTip("Placeholder is shown only when empty and unfocused.");

        // 5. Read-only
        lbl_readonly.SetText("Read-only:");
        edit_readonly.SetData("This text cannot be changed.");
        edit_readonly.SetReadOnly();
        edit_readonly.SetTip("Read-only: caret still visible when focused, but no edits.");

        // 6. Disabled
        lbl_disabled.SetText("Disabled:");
        edit_disabled.SetData("This control is disabled.");
        edit_disabled.Enable(false);
        edit_disabled.SetTip("Disabled: no focus, no caret, no interaction.");

        // 7. Custom style
        lbl_custom.SetText("Custom style:");
        {
            UiBaseEdit::Style s = UiBaseEdit::StyleDefault();
            s.palette.frame[ST_NORMAL]  = SColorShadow();
            s.palette.frame[ST_HOT]     = SColorHighlight();
            s.palette.frame[ST_PRESSED] = SColorHighlight();
            s.metrics.frame_width       = DPI(2);
            s.metrics.radius            = DPI(8);
            s.font                      = Arial(DPI(14)).Italic();
            edit_custom.SetStyle(s);
        }
        edit_custom.SetData("Accent styled line edit");
        edit_custom.SetTip("Demonstrates custom frame radius, width, and font.");

        // 8a. Icon Left
        lbl_icon_left.SetText("Icon Left:");
        edit_icon_left.SetPlaceholder("User Name");
        edit_icon_left.SetTip("This row will later serve as a simple icon-left example.");

        // 8b. Icon Right (Search-style)
        lbl_icon.SetText("Icon Right (Search):");
        edit_icon.SetPlaceholder("Search colors...");
        edit_icon.SetTip(
            "Hover to see this hint.\n"
            "In a real app you could add a right-side control (button or icon) here.");

        // 9. Side controls + layout demo (new Side API)
        lbl_side.SetText("Side controls:");
        edit_side.SetPlaceholder("Type to enable clear button...");

        // Use UiButton as compact icon buttons attached to the line edit.
        // NOTE: We intentionally do NOT remove frames; instead we rely on UiButton
        //       styles (Accent/Subtle/Icon) to control look.

        // Left user "avatar" icon ??? uses a generic save icon here just to be safe.
        btn_side_user.SetStyle(UiTheme::ResolveButton(UiButtonRole::Icon));
        btn_side_user.SetIcon(CtrlImg::save());

       // btn_side_user.SetMinSize(Size(DPI(20), DPI(20)));
        btn_side_user.ClickFocus(false);

        // Right search icon
        btn_side_search.SetStyle(UiTheme::ResolveButton(UiButtonRole::Icon));
        btn_side_search.SetIcon(CtrlImg::color_edit());

       // btn_side_search.SetMinSize(Size(DPI(20), DPI(20)));
        btn_side_search.ClickFocus(false);

        // Right clear icon (X)
        btn_side_clear.SetStyle(UiTheme::ResolveButton(UiButtonRole::Icon));
        btn_side_clear.SetIcon(CtrlImg::remove());
       // btn_side_clear.SetMinSize(Size(DPI(20), DPI(20)));
        btn_side_clear.ClickFocus(false);
    const Size side_btn_sz(DPI(28), DPI(28));
        // Attach buttons to the sides of edit_side
        side_user   = edit_side.AddToSide(btn_side_user,   UiAlign::LEFT ,side_btn_sz);//,  Size(DPI(24), DPI(24)) );
        side_search = edit_side.AddToSide(btn_side_search, UiAlign::RIGHT ,side_btn_sz);//, Size(DPI(24), DPI(24)));
        side_clear  = edit_side.AddToSide(btn_side_clear,  UiAlign::RIGHT,side_btn_sz);// Size(DPI(24), DPI(24)));


        // Clear button starts hidden; it appears only when there is text.
        side_clear.Hidden(true);

        // Show/hide clear button based on content
        edit_side.WhenChange << [=] {
            String s = edit_side.GetData().ToString();
            bool empty = IsNull(s) || s.IsEmpty();
            if(side_clear.IsValid())
                side_clear.Hidden(empty);
        };

        // Search button: demonstrates query-dependent behaviour
        btn_side_search.WhenAction << [=] {
            String s = edit_side.GetData().ToString();
            if(IsNull(s) || s.IsEmpty()) {
                PromptOK("Nothing to search ??? edit is empty.");
            } else {
                PromptOK(Format("Pretend we are searching for: \"%s\"", s));
            }
        };

        // Clear button: clears text and hides itself again
        btn_side_clear.WhenAction << [=] {
            edit_side.SetData(String());
            edit_side.SetCursor(0);
            if(side_clear.IsValid())
                side_clear.Hidden(true);
        };

        // 10. WhenAction demo (Enter key)
        lbl_action.SetText("WhenAction (Enter):");
        edit_action.SetPlaceholder("Type and press Enter");
        edit_action.SetTip("WhenAction is fired when the user presses Enter.");

        lbl_action_info.SetText("Length: (press Enter)");

        edit_action.WhenAction << [=] {
            String s = edit_action.GetData().ToString();
            int len = s.GetCount();
            lbl_action_info.SetText(Format("Length: %d character%s",
                                           len, len == 1 ? "" : "s"));
        };
    }

    // Painted header + label column
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

        w.DrawText(x, y, "UiLineEdit Demo", title, SColorText());
        y += title.GetHeight() + DPI(4);

        w.DrawText(x, y, "A single-line text field built on UiBaseEdit.", subtitle,
                   SColorText());
        y += subtitle.GetHeight() + DPI(2);

        w.DrawText(x, y,
                   "Showcases alignment, placeholder, read-only/disabled, custom styles,\n"
                   "side controls (left/right buttons), and WhenAction.",
                   body, SColorText());

        // Left label column background
        int margin        = DPI(20);
        int label_panel_w = DPI(160);
        Rect panel = r;
        panel.top    = header.bottom + DPI(12);
        panel.bottom = r.bottom - DPI(16);
        panel.left   = margin;
        panel.right  = margin + label_panel_w;

        w.DrawRect(panel, Blend(SColorFace(), SColorPaper(), 220));

        // Column headers
        Font  colhdr = SansSerifZ(10).Bold();
        Color hdrc   = SColorDisabled();

        int y_hdr = header.bottom + DPI(16);
        w.DrawText(margin + DPI(8), y_hdr, "Pattern / Variant", colhdr, hdrc);

        int right_hdr_x = r.right - DPI(200);
        w.DrawText(right_hdr_x, y_hdr, "Notes / Result", colhdr, hdrc);
    }

    virtual void Layout() override
    {
        Rect r = GetSize();

        int header_h      = DPI(110);
        int margin        = DPI(20);
        int label_panel_w = DPI(160);
        int label_w       = label_panel_w - DPI(16); // inside shaded panel
        int info_w        = DPI(180);                // width for the result label on the right
        int vgap          = DPI(8);
        int row_hmin      = DPI(28);

        int x_label = margin + DPI(8);
        int x_info  = r.right - margin - info_w;
        int x_edit  = x_label + label_w + DPI(12);
        int edit_w  = x_info - DPI(8) - x_edit; // leave a gap before info label

        int y = header_h + DPI(40);

        auto PlaceRow2 = [&](Label& lbl, Ctrl& ctrl) {
            Size ms = ctrl.GetMinSize();
            int  h  = max(row_hmin, ms.cy);
            lbl.SetRect(x_label, y, label_w, h);
            ctrl.SetRect(x_edit,  y, edit_w,  h);
            y += h + vgap;
        };

        auto PlaceActionRow = [&] {
            Size ms = edit_action.GetMinSize();
            int  h  = max(row_hmin, ms.cy);
            lbl_action.SetRect(x_label, y, label_w, h);
            edit_action.SetRect(x_edit, y, edit_w, h);
            lbl_action_info.SetRect(x_info, y, info_w, h);
            y += h + vgap;
        };

        PlaceRow2(lbl_standard,    edit_standard);
        PlaceRow2(lbl_center,      edit_center);
        PlaceRow2(lbl_right,       edit_right);
        PlaceRow2(lbl_placeholder, edit_placeholder);
        PlaceRow2(lbl_readonly,    edit_readonly);
        PlaceRow2(lbl_disabled,    edit_disabled);
        PlaceRow2(lbl_custom,      edit_custom);

        PlaceRow2(lbl_icon_left,   edit_icon_left);
        PlaceRow2(lbl_icon,        edit_icon);
        PlaceRow2(lbl_side,        edit_side);

        PlaceActionRow();
    }

private:
    // Labels
    Label lbl_standard;
    Label lbl_center;
    Label lbl_right;
    Label lbl_placeholder;
    Label lbl_readonly;
    Label lbl_disabled;
    Label lbl_custom;
    Label lbl_icon_left;
    Label lbl_icon;
    Label lbl_side;
    Label lbl_action;
    Label lbl_action_info;

    // Edits
    UiLineEdit edit_standard;
    UiLineEdit edit_center;
    UiLineEdit edit_right;
    UiLineEdit edit_placeholder;
    UiLineEdit edit_readonly;
    UiLineEdit edit_disabled;
    UiLineEdit edit_custom;
    UiLineEdit edit_icon_left;
    UiLineEdit edit_icon;
    UiLineEdit edit_side;
    UiLineEdit edit_action;

    // Side controls for edit_side
    UiButton  btn_side_user;
    UiButton  btn_side_search;
    UiButton  btn_side_clear;
    SideHandle side_user;
    SideHandle side_search;
    SideHandle side_clear;
};

GUI_APP_MAIN
{
    Ctrl::GlobalBackPaint();
    UiLineEditDemo().Run();
}


