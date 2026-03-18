#include <CtrlLib/CtrlLib.h>
#include <Ui/UiMultiEdit.h>
#include <Ui/UiButton.h>
#include <Ui/UiTheme.h>

using namespace Upp;

class UiMultiEditDemo : public TopWindow {
public:
    typedef UiMultiEditDemo CLASSNAME;

    UiMultiEditDemo()
    {
        Title("UiMultiEdit Demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(900), DPI(850));

        // Add controls
        Add(lbl_standard);    Add(edit_standard);
        Add(lbl_readonly);    Add(edit_readonly);
        Add(lbl_icon_left);   Add(edit_icon_left);
        Add(lbl_centered);    Add(edit_centered);
        Add(lbl_code);        Add(edit_code);
        Add(lbl_log);         Add(edit_log);

        // 1. Standard Multi-line
        lbl_standard.SetText("Standard Notes:");
        edit_standard.SetText(
            "This is a standard multi-line editor.\n"
            "Press Enter to create new lines.\n"
            "Use the scrollbars if text exceeds the view.\n\n"
            "It supports standard clipboard operations (Ctrl+C, Ctrl+V)."
        );
        edit_standard.SetPlaceholder("Enter your notes here...");
        edit_standard.SetTip("Basic UiMultiEdit usage.");

        // 2. Read-only (License/Terms example)
        lbl_readonly.SetText("Read-only (Terms):");
        edit_readonly.SetText(
            "MIT License\n\n"
            "Copyright (c) 2024 U++ User\n\n"
            "Permission is hereby granted, free of charge, to any person obtaining a copy "
            "of this software and associated documentation files (the \"Software\"), to deal "
            "in the Software without restriction..."
        );
        edit_readonly.SetReadOnly();
        edit_readonly.SetTip("Read-only mode. Text can be selected and copied, but not modified.");

        // 3. Icon Left (e.g., "Description" field)
        lbl_icon_left.SetText("Icon Left:");
        edit_icon_left.SetPlaceholder("Enter description...");
        edit_icon_left.SetText("Item description goes here.\nIt has an icon on the left.");
        icon_left_btn.SetStyle(UiTheme::ResolveButton(UiButtonRole::Icon));
        icon_left_btn.SetIcon(CtrlImg::write());
        icon_left_btn.ClickFocus(false);
        edit_icon_left.AddToSide(icon_left_btn, UiAlign::LEFT, Size(DPI(24), DPI(24)));
        edit_icon_left.SetTip("Demonstrates icon alignment in a multi-line control.");

        // 4. Centered Text Alignment
        lbl_centered.SetText("Centered Text:");
        edit_centered.SetText(
            "Title Page\n"
            "Subtitle\n"
            "Author Name"
        );
        edit_centered.SetTextAlign(UiAlign::CENTER);
        edit_centered.SetTip("Demonstrates UiAlign::CENTER for text.");

        // 5. Custom Style (Code Editor Look)
        lbl_code.SetText("Custom Style (Code):");
        {
            UiBaseEdit::Style s = UiBaseEdit::StyleDefault();
            s.font = Monospace(DPI(13)); // Monospace font
            
            // Rounded corners
            s.metrics.radius = DPI(6);

            // Use 'face' for background
            s.palette.face[ST_NORMAL] = Color(30, 30, 30);  // Dark background
            
            // Lighter green text
            s.palette.ink[ST_NORMAL]  = Color(144, 238, 144); // Light Green
            
            s.palette.frame[ST_NORMAL] = Color(50, 50, 50);
            
            s.selection_color = Color(60, 80, 110);
            s.caret_color = White();
            edit_code.SetStyle(s);
        }
        edit_code.SetText(
            "#include <iostream>\n\n"
            "int main() {\n"
            "    std::cout << \"Hello World!\" << std::endl;\n"
            "    return 0;\n"
            "}"
        );
        edit_code.SetTip("Custom dark theme with monospace font.");

        // 6. Log / Output View (Bottom)
        lbl_log.SetText("Log Output:");
        edit_log.SetReadOnly();
        edit_log.SetText(
            "[INFO] System started.\n"
            "[INFO] Loading resources...\n"
            "[WARN] Config file not found, using defaults.\n"
            "[INFO] Ready."
        );
        clear_log_btn.SetStyle(UiTheme::ResolveButton(UiButtonRole::Icon));
        clear_log_btn.SetIcon(CtrlImg::remove());
        clear_log_btn.ClickFocus(false);
        edit_log.AddToSide(clear_log_btn, UiAlign::RIGHT, Size(DPI(24), DPI(24)));
        clear_log_btn.WhenAction = [=] {
            if(PromptYesNo("Clear the log?")) {
                edit_log.Clear();
            }
        };
        edit_log.SetTip("Read-only log view. Click the 'X' icon to clear.");
    }

    virtual void Paint(Draw& w) override
    {
        Rect r = GetSize();
        w.DrawRect(r, SColorPaper());

        // Header
        int head_h = DPI(100);
        Rect header = r;
        header.bottom = header.top + head_h;
        w.DrawRect(header, SColorFace());

        Font title = SansSerifZ(24).Bold();
        Font subtitle = SansSerifZ(12);

        int x = DPI(32);
        int y = DPI(20);

        w.DrawText(x, y, "UiMultiEdit Demo", title, SColorText());
        y += title.GetHeight() + DPI(6);

        w.DrawText(x, y,
            "A multi-line text editor with scrolling support.",
            subtitle, SColorText());

        // Left label column background
        int margin = DPI(20);
        int label_panel_w = DPI(160);
        Rect panel = r;
        panel.top = header.bottom + DPI(12);
        panel.bottom = r.bottom - DPI(16);
        panel.left = margin;
        panel.right = margin + label_panel_w;

        w.DrawRect(panel, Blend(SColorFace(), SColorPaper(), 220));
    }

    virtual void Layout() override
    {
        Rect r = GetSize();
        int header_h = DPI(100);
        int margin = DPI(20);
        int label_panel_w = DPI(160);
        int label_w = label_panel_w - DPI(16);
        int vgap = DPI(16); // Larger gap for multi-line controls

        int x_label = margin + DPI(8);
        int x_edit = x_label + label_w + DPI(12);
        int edit_w = r.right - x_edit - margin;

        int y = header_h + DPI(30);

        // Helper to place rows with variable height
        auto PlaceRow = [&](Label& lbl, UiMultiEdit& edit, int height_dp) {
            int h = DPI(height_dp);
            lbl.SetRect(x_label, y, label_w, DPI(24)); // Label stays at top
            edit.SetRect(x_edit, y, edit_w, h);
            y += h + vgap;
        };

        PlaceRow(lbl_standard,   edit_standard,  100);
        PlaceRow(lbl_readonly,   edit_readonly,  80);
        PlaceRow(lbl_icon_left,  edit_icon_left, 80);
        PlaceRow(lbl_centered,   edit_centered,  80);
        PlaceRow(lbl_code,       edit_code,      120);
        PlaceRow(lbl_log,        edit_log,       100);
    }

private:
    Label lbl_standard, lbl_readonly, lbl_icon_left, lbl_centered, lbl_code, lbl_log;
    UiMultiEdit edit_standard;
    UiMultiEdit edit_readonly;
    UiMultiEdit edit_icon_left;
    UiMultiEdit edit_centered;
    UiMultiEdit edit_code;
    UiMultiEdit edit_log;

    UiButton icon_left_btn;
    UiButton clear_log_btn;
};

GUI_APP_MAIN
{

    UiMultiEditDemo().Run();
}


