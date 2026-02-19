#include <CtrlLib/CtrlLib.h>
#include <Ui/UiMaskEdit.h>

using namespace Upp;

class UiMaskEditDemo : public TopWindow {
public:
    typedef UiMaskEditDemo CLASSNAME;

    UiMaskEditDemo()
    {
        Title("UiMaskEdit Demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(600), DPI(600));

        // Add controls
        Add(lbl_phone);    Add(edit_phone);
        Add(lbl_date);     Add(edit_date);
        Add(lbl_zip);      Add(edit_zip);
        Add(lbl_plate);    Add(edit_plate);
        Add(lbl_username); Add(edit_username);

        // 1. Phone Number (classic mask)
        lbl_phone.SetText("Phone Number:");
        edit_phone.SetMask("(###) ###-####");
        edit_phone.SetTip("Format: (123) 456-7890");
        SetupValidation(edit_phone);

        // 2. Date (mask + semantic validator)
        lbl_date.SetText("Date (MM/DD/YYYY):");
        edit_date.SetMask("##/##/####");
        edit_date.SetValidator(UiMaskEdit::DateValidator()); // Built-in check
        edit_date.SetTip("Format: 12/31/2024 (validates month/day/year)");
        SetupValidation(edit_date);

        // 3. ZIP Code (simple numeric mask)
        lbl_zip.SetText("ZIP Code:");
        edit_zip.SetMask("#####");
        edit_zip.SetTip("5-digit ZIP code");
        // Mask already restricts to digits; validator just piggybacks on completeness.
        SetupValidation(edit_zip);

        // 4. License Plate (Auto-Caps via 'U')
        lbl_plate.SetText("License Plate:");
        edit_plate.SetMask("UUU-####");
        edit_plate.SetTip("Format: ABC-1234 (auto-capitalizes letters)");
        SetupValidation(edit_plate);
        
        // 5. Username (Styled, *formatter-only*, NO mask)
        lbl_username.SetText("Username (Formatter only):");

        // NOTE: No SetMask() here – we want unlimited-length, free typing,
        // with on-the-fly formatting only.
        edit_username.SetTip("Live formatting: spaces/punctuation → '_', "
                             "each word's first letter becomes uppercase.");

        // Formatter: username-style (spaces/punct → '_', title-cased words)
        edit_username.SetFormatter(UiMaskEdit::UsernameFormatter());

        // Validator: final value must be [A-Za-z0-9_]* (allow empty),
        // so after formatting it should always be valid.
        edit_username.SetValidator(UiMaskEdit::AlnumUnderscoreValidator(true));

        // Custom pill style for username
        {
            UiBaseEdit::Style s = UiBaseEdit::StyleDefault();

            s.metrics.radius        = DPI(999);   // effectively pill (clamped by height)
            s.metrics.frame_width   = 0;
            s.metrics.frame_enabled = false;
            s.metrics.face_enabled  = true;

            Color base = Color(45, 45, 48);

            s.palette.face[ST_NORMAL]   = base;
            s.palette.face[ST_HOT]      = Blend(base, White(), 15);
            s.palette.face[ST_PRESSED]  = Blend(base, Black(), 15);
            s.palette.face[ST_DISABLED] = Blend(base, SColorPaper(), 70);

            s.palette.ink[ST_NORMAL]   = White();
            s.palette.ink[ST_HOT]      = White();
            s.palette.ink[ST_PRESSED]  = White();
            s.palette.ink[ST_DISABLED] = GrayColor(170);

            // Slightly bolder / larger to highlight the demo
            s.font = StdFont().Bold().Height(StdFont().GetHeight() + DPI(1));

            edit_username.SetStyle(s);
        }

        // For username we still use SetupValidation, but since the
        // validator always passes (or empty), it will not flash red –
        // it just shows the green flash on "good" actions.
        SetupValidation(edit_username);
    }

    void SetupValidation(UiMaskEdit& edit)
    {
        // Live error state while typing
        edit.WhenChange = [=, &edit] {
            edit.ShowError(!edit.IsValid());
        };

        // On "action" (Enter/lose focus), flash success or error
        edit.WhenAction = [=, &edit] {
            if(edit.IsValid()) {
                edit.ShowError(false);
                edit.FlashSuccess();
            }
            else {
                edit.ShowError(true);
                edit.FlashError();
            }
        };
    }

    virtual void Paint(Draw& w) override
    {
        Rect r = GetSize();
        w.DrawRect(r, SColorPaper());

        // Header
        int head_h = DPI(80);
        Rect header = r;
        header.bottom = header.top + head_h;
        w.DrawRect(header, SColorFace());

        Font title = SansSerifZ(20).Bold();
        w.DrawText(DPI(32), DPI(25), "UiMaskEdit Demo", title, SColorText());
    }

    virtual void Layout() override
    {
        Rect r = GetSize();
        int header_h = DPI(80);
        int margin   = DPI(32);
        int label_w  = DPI(200);
        int edit_w   = DPI(260);
        int h        = DPI(32);
        int vgap     = DPI(20);

        int x_label = margin;
        int x_edit  = x_label + label_w + DPI(10);
        int y       = header_h + DPI(30);

        auto PlaceRow = [&](Label& lbl, UiMaskEdit& edit) {
            lbl.SetRect(x_label, y, label_w, h);
            edit.SetRect(x_edit,  y, edit_w,  h);
            y += h + vgap;
        };

        PlaceRow(lbl_phone,    edit_phone);
        PlaceRow(lbl_date,     edit_date);
        PlaceRow(lbl_zip,      edit_zip);
        PlaceRow(lbl_plate,    edit_plate);
        PlaceRow(lbl_username, edit_username);
    }

private:
    Label     lbl_phone, lbl_date, lbl_zip, lbl_plate, lbl_username;
    UiMaskEdit edit_phone;
    UiMaskEdit edit_date;
    UiMaskEdit edit_zip;
    UiMaskEdit edit_plate;
    UiMaskEdit edit_username;
};

GUI_APP_MAIN
{
    UiMaskEditDemo().Run();
}
