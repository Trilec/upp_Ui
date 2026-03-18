#include <Ui/Ui.h>

using namespace Upp;

class UiIntFloatDemoWindow : public TopWindow {
public:
    typedef UiIntFloatDemoWindow CLASSNAME;

    UiIntFloatDemoWindow()
    {
        Title("UiIntEdit / UiFloatEdit Demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(900), DPI(560));

        Add(lbl_int);
        Add(lbl_float);
        Add(lbl_note);
        Add(int_edit);
        Add(float_edit);
        Add(sync_btn);

        lbl_int.SetText("Integer").SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        lbl_float.SetText("Float").SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        lbl_note.SetText("Mouse wheel / arrow keys / spin buttons are enabled.")
                .SetAlign(UiAlign::LEFT, UiAlign::TOP)
                .SetInkColor(SColorDisabled());

        int_edit.MinMax(-100, 100).Step(5).ShowSpin(true).SetPlaceholder("int");
        int_edit.SetValue(20);
        int_edit.SetSizeMin(DPI(180), DPI(32));

        float_edit.MinMax(-10.0, 10.0).Precision(3).Step(0.25).ShowSpin(true).SetPlaceholder("float");
        float_edit.SetValue(1.5);
        float_edit.SetSizeMin(DPI(220), DPI(32));

        sync_btn.SetText("Copy float -> int").SetStyle(UiTheme::ResolveButton(UiButtonRole::Accent));
        sync_btn.WhenAction = [=] {
            int_edit.SetValue((int)float_edit.GetValue());
        };
    }

    virtual void Layout() override
    {
        Rect r = GetSize();
        int m = DPI(24);
        int lh = DPI(24);
        int eh = DPI(32);
        int y = m;

        lbl_int.SetRect(m, y, DPI(120), lh);
        int_edit.SetRect(m + DPI(130), y - DPI(4), DPI(260), eh);
        y += DPI(52);

        lbl_float.SetRect(m, y, DPI(120), lh);
        float_edit.SetRect(m + DPI(130), y - DPI(4), DPI(320), eh);
        y += DPI(52);

        sync_btn.SetRect(m + DPI(130), y, DPI(180), DPI(30));
        y += DPI(40);

        lbl_note.SetRect(m, y, r.GetWidth() - 2 * m, DPI(50));
    }

private:
    UiLabel lbl_int;
    UiLabel lbl_float;
    UiLabel lbl_note;
    UiIntEdit int_edit;
    UiFloatEdit float_edit;
    UiButton sync_btn;
};

GUI_APP_MAIN
{
    UiIntFloatDemoWindow().Run();
}

