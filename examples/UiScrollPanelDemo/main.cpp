#include <Ui/Ui.h>

using namespace Upp;

class UiScrollPanelDemoWindow : public TopWindow {
public:
    typedef UiScrollPanelDemoWindow CLASSNAME;

    UiScrollPanelDemoWindow()
    {
        Title("UiScrollPanel Demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(1140), DPI(760));

        Add(title);
        Add(auto_label);
        Add(vertical_label);
        Add(horizontal_label);
        Add(none_label);

        Add(auto_panel);
        Add(vertical_panel);
        Add(horizontal_panel);
        Add(none_panel);

        title.SetText("UiScrollPanel modes: Auto / Vertical / Horizontal / None")
             .SetAlign(UiAlign::LEFT, UiAlign::CENTER);

        SetupHeaderLabel(auto_label, "AUTO");
        SetupHeaderLabel(vertical_label, "VERTICAL");
        SetupHeaderLabel(horizontal_label, "HORIZONTAL");
        SetupHeaderLabel(none_label, "NONE");

        auto_panel.SetScrollMode(UIPANELSCROLL_AUTO).SetStyle(UiScrollPanel::StyleDefault());
        vertical_panel.SetScrollMode(UIPANELSCROLL_VERTICAL).SetStyle(UiScrollPanel::StyleDefault());
        horizontal_panel.SetScrollMode(UIPANELSCROLL_HORIZONTAL).SetStyle(UiScrollPanel::StyleDefault());
        none_panel.SetScrollMode(UIPANELSCROLL_NONE).SetStyle(UiScrollPanel::StyleDefault());

        BuildAutoContent();
        BuildVerticalContent();
        BuildHorizontalContent();
        BuildNoneContent();
    }

    virtual void Layout() override
    {
        Rect r = GetSize();
        int m = DPI(12);
        int g = DPI(10);
        int title_h = DPI(30);
        int label_h = DPI(22);

        title.SetRect(m, m, r.GetWidth() - 2 * m, title_h);

        int top = m + title_h + g;
        int avail_h = r.GetHeight() - top - m;
        int row_h = (avail_h - g - label_h * 2) / 2;
        int col_w = (r.GetWidth() - 2 * m - g) / 2;

        Rect a(m, top, m + col_w, top + label_h);
        Rect b(a.right + g, top, a.right + g + col_w, top + label_h);
        Rect c(m, a.bottom + g + row_h, m + col_w, a.bottom + g + row_h + label_h);
        Rect d(c.right + g, c.top, c.right + g + col_w, c.top + label_h);

        auto_label.SetRect(a);
        vertical_label.SetRect(b);
        horizontal_label.SetRect(c);
        none_label.SetRect(d);

        auto_panel.SetRect(a.left, a.bottom, col_w, row_h);
        vertical_panel.SetRect(b.left, b.bottom, col_w, row_h);
        horizontal_panel.SetRect(c.left, c.bottom, col_w, row_h);
        none_panel.SetRect(d.left, d.bottom, col_w, row_h);

        LayoutAutoContent();
        LayoutVerticalContent();
        LayoutHorizontalContent();
        LayoutNoneContent();
    }

private:
    static void SetupHeaderLabel(UiLabel& l, const String& text)
    {
        l.SetText(text).SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        l.SetInkColor(Color(96, 110, 136));
    }

    void BuildAutoContent()
    {
        ParentCtrl& c = auto_panel.Content();
        for(int i = 0; i < 22; i++) {
            UiButton& b = auto_rows.Add();
            c.Add(b);
            b.SetText(Format("Auto row %02d", i + 1)).SetStyle(UiTheme::ResolveButton(UiButtonRole::Subtle));
        }
    }

    void BuildVerticalContent()
    {
        ParentCtrl& c = vertical_panel.Content();
        for(int i = 0; i < 24; i++) {
            UiButton& b = vertical_rows.Add();
            c.Add(b);
            if(i % 3 == 0)
                b.SetText(Format("Vertical critical event %02d", i + 1)).SetStyle(UiTheme::ResolveButton(UiButtonRole::Accent));
            else
                b.SetText(Format("Vertical event %02d", i + 1)).SetStyle(UiTheme::ResolveButton(UiButtonRole::Subtle));
        }
    }

    void BuildHorizontalContent()
    {
        ParentCtrl& c = horizontal_panel.Content();
        for(int i = 0; i < 18; i++) {
            UiButton& b = horizontal_cols.Add();
            c.Add(b);
            b.SetText(Format("Col %02d", i + 1)).SetStyle(UiTheme::ResolveButton(UiButtonRole::Subtle));
        }
    }

    void BuildNoneContent()
    {
        ParentCtrl& c = none_panel.Content();
        for(int i = 0; i < 16; i++) {
            UiButton& b = none_rows.Add();
            c.Add(b);
            b.SetText(Format("No-scroll row %02d", i + 1)).SetStyle(UiTheme::ResolveButton(UiButtonRole::Subtle));
        }
    }

    void LayoutAutoContent()
    {
        int y = DPI(8);
        int x = DPI(8);
        int w = max(DPI(180), auto_panel.GetSize().cx - DPI(44));
        for(int i = 0; i < auto_rows.GetCount(); i++) {
            auto_rows[i].SetRect(x, y, w, DPI(30));
            y += DPI(36);
        }
    }

    void LayoutVerticalContent()
    {
        int y = DPI(8);
        int x = DPI(8);
        int w = max(DPI(220), vertical_panel.GetSize().cx - DPI(44));
        for(int i = 0; i < vertical_rows.GetCount(); i++) {
            vertical_rows[i].SetRect(x, y, w, DPI(30));
            y += DPI(36);
        }
    }

    void LayoutHorizontalContent()
    {
        int x = DPI(8);
        int y = DPI(10);
        for(int i = 0; i < horizontal_cols.GetCount(); i++) {
            horizontal_cols[i].SetRect(x, y, DPI(120), DPI(34));
            x += DPI(128);
        }
    }

    void LayoutNoneContent()
    {
        int y = DPI(8);
        int x = DPI(8);
        int w = max(DPI(220), none_panel.GetSize().cx - DPI(44));
        for(int i = 0; i < none_rows.GetCount(); i++) {
            none_rows[i].SetRect(x, y, w, DPI(30));
            y += DPI(36);
        }
    }

private:
    UiLabel title;
    UiLabel auto_label;
    UiLabel vertical_label;
    UiLabel horizontal_label;
    UiLabel none_label;

    UiScrollPanel auto_panel;
    UiScrollPanel vertical_panel;
    UiScrollPanel horizontal_panel;
    UiScrollPanel none_panel;

    Array<UiButton> auto_rows;
    Array<UiButton> vertical_rows;
    Array<UiButton> horizontal_cols;
    Array<UiButton> none_rows;
};

GUI_APP_MAIN
{
    UiScrollPanelDemoWindow().Run();
}
