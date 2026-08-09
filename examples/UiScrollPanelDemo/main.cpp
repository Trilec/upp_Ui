/*
    UiScrollPanelDemo
    ------------

    Purpose
    - Active Ui control demo used as a build smoke test and visual styling reference.

    Demo hygiene header
    - Keep this package compiling in the active demo sweep.
    - Prefer BuilderDemoSupport/shared shell and UiComposite inspector rows where practical.
    - Prefer UiTheme defaults; add local styling only when the demo intentionally showcases that variation.

    Changelog
    - 2026-05: active demo sweep verified; header added during demo cleanup pass.
*/
#include "../BuilderDemoSupport.h"

using namespace Upp;
using namespace BuilderDemoSupport;

namespace {

class UiScrollPanelBuilder : public BuilderWindowBase {
public:
    typedef UiScrollPanelBuilder CLASSNAME;

    UiScrollPanelBuilder()
        : BuilderWindowBase("UiScrollPanelDemo", "U++ UiScrollPanel Builder",
                            "Compare automatic, vertical, horizontal, and disabled scrolling with live content sizing.")
    {
        for(int i = 0; i < 4; i++) {
            Preview().Add(caption_[i]);
            Preview().Add(panel_[i]);
        }

        AddStateRow(StateBox(), state_mode_row_, state_mode_label_, state_mode_value_, "Mode");
        AddStateRow(StateBox(), state_rows_row_, state_rows_label_, state_rows_value_, "Rows");
        AddStateRow(StateBox(), state_width_row_, state_width_label_, state_width_value_, "Content W");

        AddSliderRow(PropsBox(), rows_row_, "Rows", "18");
        AddSliderRow(PropsBox(), content_width_row_, "Content W", "620px");
        AddToggleRow(PropsBox(), auto_panel_row_, "Show auto");
        AddToggleRow(PropsBox(), vertical_panel_row_, "Show vertical");
        AddToggleRow(PropsBox(), horizontal_panel_row_, "Show horizontal");
        AddToggleRow(PropsBox(), none_panel_row_, "Show none");

        rows_row_.Slider().SetRange(4, 40).SetStep(1).SetValue(row_count_);
        content_width_row_.Slider().SetRange(240, 1200).SetStep(20).SetValue(content_width_);
        rows_row_.WhenAction = [=] { row_count_ = (int)rows_row_.Slider().GetValue(); RefreshFromConfig(); };
        content_width_row_.WhenAction = [=] { content_width_ = (int)content_width_row_.Slider().GetValue(); RefreshFromConfig(); };
        auto_panel_row_.Toggle().WhenAction = [=] { show_[0] = auto_panel_row_.Toggle().IsOn(); RefreshFromConfig(); };
        vertical_panel_row_.Toggle().WhenAction = [=] { show_[1] = vertical_panel_row_.Toggle().IsOn(); RefreshFromConfig(); };
        horizontal_panel_row_.Toggle().WhenAction = [=] { show_[2] = horizontal_panel_row_.Toggle().IsOn(); RefreshFromConfig(); };
        none_panel_row_.Toggle().WhenAction = [=] { show_[3] = none_panel_row_.Toggle().IsOn(); RefreshFromConfig(); };

        FinishInit();
        RefreshFromConfig();
    }

protected:
    virtual void ApplyDemoTheme() override
    {
        for(int i = 0; i < 4; i++) {
            caption_[i].SetCustomStyle(UiTheme::ResolveLabel(UiRole::Subtle));
            panel_[i].SetCustomStyle(UiScrollPanel::StyleDefault());
            for(int j = 0; j < rows_[i].GetCount(); j++)
                rows_[i][j].SetCustomStyle(UiTheme::ResolveButton(j % 4 == 0 ? UiRole::Accent : UiRole::Subtle));
        }
    }

    virtual void LayoutPreviewContent() override
    {
        Rect canvas = Preview().GetCanvasRect().Deflated(DPI(8));
        int gap = DPI(12);
        int label_h = DPI(20);
        int col_w = max(DPI(180), (canvas.GetWidth() - gap) / 2);
        int row_h = max(DPI(150), (canvas.GetHeight() - gap - label_h * 2) / 2);

        for(int i = 0; i < 4; i++) {
            int col = i % 2;
            int row = i / 2;
            int x = canvas.left + col * (col_w + gap);
            int y = canvas.top + row * (row_h + label_h + gap);
            caption_[i].SetRect(x, y, col_w, label_h);
            panel_[i].SetRect(x, y + label_h, col_w, row_h);
            LayoutPanelContent(i);
        }
    }

private:
    void RebuildRows()
    {
        const char *prefix[] = { "Auto", "Vertical", "Horizontal", "No-scroll" };
        for(int p = 0; p < 4; p++) {
            ParentCtrl& content = panel_[p].Content();
            while(rows_[p].GetCount() < row_count_) {
                UiButton& b = rows_[p].Add();
                content.Add(b);
            }
            while(rows_[p].GetCount() > row_count_)
                rows_[p].Remove(rows_[p].GetCount() - 1);
            for(int i = 0; i < rows_[p].GetCount(); i++) {
                rows_[p][i].SetText(Format("%s item %02d", prefix[p], i + 1));
                rows_[p][i].Show(show_[p]);
            }
        }
    }

    void LayoutPanelContent(int p)
    {
        if(p < 0 || p >= 4)
            return;
        int x = DPI(8);
        int y = DPI(8);
        if(p == 2) {
            for(int i = 0; i < rows_[p].GetCount(); i++) {
                rows_[p][i].SetRect(x, y, DPI(128), DPI(32));
                x += DPI(136);
            }
            return;
        }
        int w = max(DPI(180), min(content_width_, max(content_width_, panel_[p].GetSize().cx - DPI(36))));
        for(int i = 0; i < rows_[p].GetCount(); i++) {
            rows_[p][i].SetRect(x, y, w, DPI(30));
            y += DPI(36);
        }
    }

    void RefreshFromConfig()
    {
        caption_[0].SetText("AUTO");
        caption_[1].SetText("VERTICAL");
        caption_[2].SetText("HORIZONTAL");
        caption_[3].SetText("NONE");

        panel_[0].SetScrollMode(UIPANELSCROLL_AUTO);
        panel_[1].SetScrollMode(UIPANELSCROLL_VERTICAL);
        panel_[2].SetScrollMode(UIPANELSCROLL_HORIZONTAL);
        panel_[3].SetScrollMode(UIPANELSCROLL_NONE);

        for(int i = 0; i < 4; i++) {
            caption_[i].Show(show_[i]);
            panel_[i].Show(show_[i]);
        }

        RebuildRows();
        ApplyDemoTheme();

        rows_row_.Slider().SetValue(row_count_);
        content_width_row_.Slider().SetValue(content_width_);
        rows_row_.SetValueText(AsString(row_count_));
        content_width_row_.SetValueText(AsString(content_width_) + "px");
        auto_panel_row_.Toggle().SetOn(show_[0]);
        vertical_panel_row_.Toggle().SetOn(show_[1]);
        horizontal_panel_row_.Toggle().SetOn(show_[2]);
        none_panel_row_.Toggle().SetOn(show_[3]);

        state_mode_value_.SetText("Auto / Vertical / Horizontal / None");
        state_rows_value_.SetText(AsString(row_count_));
        state_width_value_.SetText(AsString(content_width_) + "px");

        SetUsageCode(BuildUsageCode());
        RefreshLayout();
        Preview().Refresh();
    }

    String BuildUsageCode() const
    {
        String code;
        code << "UiScrollPanel scroll;\n";
        code << "scroll.SetScrollMode(UIPANELSCROLL_AUTO);\n";
        code << "scroll.SetCustomStyle(UiScrollPanel::StyleDefault());\n";
        code << "ParentCtrl& content = scroll.Content();\n";
        code << "// Add child controls to content and size them to the real content bounds.\n";
        return code;
    }

    int row_count_ = 18;
    int content_width_ = DPI(620);
    bool show_[4] = { true, true, true, true };

    UiLabel caption_[4];
    UiScrollPanel panel_[4];
    Array<UiButton> rows_[4];

    UiBoxLayout state_mode_row_ { UiBoxLayout::Direction::H }, state_rows_row_ { UiBoxLayout::Direction::H }, state_width_row_ { UiBoxLayout::Direction::H };
    UiLabel state_mode_label_, state_mode_value_, state_rows_label_, state_rows_value_, state_width_label_, state_width_value_;

    DemoSliderRow rows_row_, content_width_row_;
    DemoToggleRow auto_panel_row_, vertical_panel_row_, horizontal_panel_row_, none_panel_row_;
};

}

GUI_APP_MAIN
{
    UiScrollPanelBuilder demo;
    demo.Run();
}
