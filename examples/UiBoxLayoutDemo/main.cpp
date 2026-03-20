#include <Ui/Ui.h>

using namespace Upp;

class UiBoxLayoutDemoWindow : public TopWindow {
public:
    typedef UiBoxLayoutDemoWindow CLASSNAME;

    UiBoxLayoutDemoWindow()
    {
        Title("UiBoxLayout Demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(980), DPI(640));

        Add(root);
        root.SetDirection(UiDirection::V).SetGap(DPI(10)).SetInset(DPI(12));

        row_top.SetDirection(UiDirection::H).SetGap(DPI(8)).SetWrap(true).SetInset(DPI(8));
        row_bottom.SetDirection(UiDirection::H).SetGap(DPI(8)).SetInset(DPI(8));

        root.Add(row_top).Expand(2);
        root.Add(row_bottom).Expand(1);

        for(int i = 0; i < 12; i++) {
            cards[i].SetFaceColor(Blend(SColorFace(), SColorPaper(), 220)).SetFrameColor(SColorShadow()).SetRadius(DPI(6));
            labels[i].SetText(Format("Card %d", i + 1));
            labels[i].SetAlign(UiAlign::CENTER, UiAlign::CENTER);
            cards[i].Add(labels[i].HCenterPos(80).VCenterPos(24));
            row_top.Add(cards[i]).Fit().MinMaxWidth(DPI(130), DPI(170)).MinHeight(DPI(80));
        }

        info.SetText("Resize window to test wrapping, spacing and expand behavior.")
            .SetAlign(UiAlign::LEFT, UiAlign::CENTER)
            .SetInkColor(SColorDisabled());
        action.SetText("Action").SetStyle(UiTheme::ResolveButton(UiButtonRole::Accent));
        cancel.SetText("Cancel").SetStyle(UiTheme::ResolveButton(UiButtonRole::Subtle));

        row_bottom.Add(info).Expand(1).MinHeight(DPI(34));
        row_bottom.Add(action).Fixed(DPI(120)).MinHeight(DPI(34));
        row_bottom.Add(cancel).Fixed(DPI(120)).MinHeight(DPI(34));
    }

    virtual void Layout() override
    {
        root.SetRect(GetSize());
    }

private:
    UiBoxLayout root { UiDirection::V };
    UiBoxLayout row_top { UiDirection::H };
    UiBoxLayout row_bottom { UiDirection::H };

    UiPanel cards[12];
    UiLabel labels[12];
    UiLabel info;
    UiButton action;
    UiButton cancel;
};

GUI_APP_MAIN
{
    UiBoxLayoutDemoWindow().Run();
}


