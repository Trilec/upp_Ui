#include "../BuilderDemoSupport.h"

using namespace Upp;
using namespace BuilderDemoSupport;

namespace {

struct TitleCardConfig {
    String title = "Release Notes";
    String subtitle = "Sprint 12";
    String copy = "A compact summary card that can carry media, title, and copy in a single styled surface.";
    UiAlign media_side = UiAlign::LEFT;
    int media_share = 28;
    int media_size = DPI(28);
    int radius = DPI(8);
    bool show_rule = true;
    bool show_bottom = false;
    bool hover = false;
    bool selectable = false;
};

class UiTitleCardBuilder : public BuilderWindowBase {
public:
    typedef UiTitleCardBuilder CLASSNAME;

    UiTitleCardBuilder()
        : BuilderWindowBase("UiTitleCardDemo", "U++ UiTitleCard Builder", "Inspect header card media placement, rule display, and title/copy layout from one shell.")
    {
        Preview().Add(card_);

        AddStateRow(StateBox(), state_theme_row_, state_theme_label_, state_theme_value_, "Theme");
        AddStateRow(StateBox(), state_title_row_, state_title_label_, state_title_value_, "Title");
        AddStateRow(StateBox(), state_side_row_, state_side_label_, state_side_value_, "Media Side");
        AddStateRow(StateBox(), state_rule_row_, state_rule_label_, state_rule_value_, "Rule");

        AddEditRow(PropsBox(), title_row_box_, title_label_, title_edit_, "Title");
        AddEditRow(PropsBox(), subtitle_row_box_, subtitle_label_, subtitle_edit_, "Subtitle");
        AddEditRow(PropsBox(), copy_row_box_, copy_label2_, copy_edit_, "Copy");
        AddDropdownRow(PropsBox(), side_row_box_, side_label_, side_drop_, "Media Side");
        AddSliderRow(PropsBox(), share_row_, "Media %", "28%");
        AddSliderRow(PropsBox(), size_row_, "Media Sz", "28px");
        AddSliderRow(PropsBox(), radius_row_, "Radius", "8px");
        AddToggleRow(PropsBox(), rule_row_, "Show Rule");
        AddToggleRow(PropsBox(), bottom_row_, "Bottom Line");
        AddToggleRow(PropsBox(), hover_row_, "Hover");
        AddToggleRow(PropsBox(), selectable_row_, "Selectable");

        const EnumOption sides[] = { { "Left", (int)UiAlign::LEFT }, { "Right", (int)UiAlign::RIGHT }, { "Top", (int)UiAlign::TOP }, { "Bottom", (int)UiAlign::BOTTOM } };
        PopulateDropdown(side_drop_, sides, 4);

        title_edit_.SetData(cfg_.title);
        subtitle_edit_.SetData(cfg_.subtitle);
        copy_edit_.SetData(cfg_.copy);
        share_row_.Slider().SetRange(20, 60).SetStep(1).SetValue(cfg_.media_share);
        size_row_.Slider().SetRange(DPI(18), DPI(48)).SetStep(1).SetValue(cfg_.media_size);
        radius_row_.Slider().SetRange(0, DPI(18)).SetStep(1).SetValue(cfg_.radius);

        title_edit_.WhenChange = [=] { cfg_.title = title_edit_.GetData().ToString(); RefreshFromConfig(); };
        subtitle_edit_.WhenChange = [=] { cfg_.subtitle = subtitle_edit_.GetData().ToString(); RefreshFromConfig(); };
        copy_edit_.WhenChange = [=] { cfg_.copy = copy_edit_.GetData().ToString(); RefreshFromConfig(); };
        side_drop_.WhenSelect = [=](int) { cfg_.media_side = (UiAlign)(int)side_drop_.GetSelectedData(); RefreshFromConfig(); };
        share_row_.WhenAction = [=] { cfg_.media_share = (int)share_row_.Slider().GetValue(); RefreshFromConfig(); };
        size_row_.WhenAction = [=] { cfg_.media_size = (int)size_row_.Slider().GetValue(); RefreshFromConfig(); };
        radius_row_.WhenAction = [=] { cfg_.radius = (int)radius_row_.Slider().GetValue(); RefreshFromConfig(); };
        rule_row_.Toggle().WhenAction = [=] { cfg_.show_rule = rule_row_.Toggle().IsOn(); RefreshFromConfig(); };
        bottom_row_.Toggle().WhenAction = [=] { cfg_.show_bottom = bottom_row_.Toggle().IsOn(); RefreshFromConfig(); };
        hover_row_.Toggle().WhenAction = [=] { cfg_.hover = hover_row_.Toggle().IsOn(); RefreshFromConfig(); };
        selectable_row_.Toggle().WhenAction = [=] { cfg_.selectable = selectable_row_.Toggle().IsOn(); RefreshFromConfig(); };

        FinishInit();
        RefreshFromConfig();
    }

protected:
    virtual void ApplyDemoTheme() override
    {
        UiLabel::Style body = MakeBodyLabelStyle(Palette());
        UiLabel::Style value = MakeValueLabelStyle(Palette());
        UiBaseEdit::Style edit = MakeEditStyle(Palette());
        UiDropdown::Style dd = MakeDropdownStyle(Palette());

        state_theme_label_.SetStyle(body); state_theme_value_.SetStyle(value);
        state_title_label_.SetStyle(body); state_title_value_.SetStyle(value);
        state_side_label_.SetStyle(body); state_side_value_.SetStyle(value);
        state_rule_label_.SetStyle(body); state_rule_value_.SetStyle(value);
        title_label_.SetStyle(body); subtitle_label_.SetStyle(body); copy_label2_.SetStyle(body); side_label_.SetStyle(body);
        title_edit_.SetStyle(edit); subtitle_edit_.SetStyle(edit); copy_edit_.SetStyle(edit); side_drop_.SetStyle(dd);
        share_row_.SetLabelStyle(body).SetValueStyle(value);
        size_row_.SetLabelStyle(body).SetValueStyle(value);
        radius_row_.SetLabelStyle(body).SetValueStyle(value);
        rule_row_.SetLabelStyle(body);
        bottom_row_.SetLabelStyle(body);
        hover_row_.SetLabelStyle(body);
        selectable_row_.SetLabelStyle(body);
    }

    virtual void LayoutPreviewContent() override
    {
        Rect canvas = Preview().GetCanvasRect();
        int w = min(DPI(420), canvas.GetWidth() - DPI(30));
        int h = min(DPI(180), canvas.GetHeight() - DPI(30));
        int x = canvas.left + (canvas.GetWidth() - w) / 2;
        int y = canvas.top + (canvas.GetHeight() - h) / 2;
        card_.SetRect(x, y, w, h);
    }

private:
    struct EnumOption { const char* label; int value; };

    void PopulateDropdown(UiDropdown& drop, const EnumOption* opts, int count)
    {
        drop.UseInternalModel();
        drop.Clear();
        for(int i = 0; i < count; i++)
            drop.Add(opts[i].label, opts[i].value);
    }

    String SideLabel() const
    {
        if(cfg_.media_side == UiAlign::RIGHT) return "Right";
        if(cfg_.media_side == UiAlign::TOP) return "Top";
        if(cfg_.media_side == UiAlign::BOTTOM) return "Bottom";
        return "Left";
    }

    void RefreshFromConfig()
    {
        UiTitleCard::Style style = UiTheme::ResolveTitleCard();
        style.metrics.radius = cfg_.radius;
        style.metrics.frame_enabled = true;
        style.metrics.face_enabled = true;
        style.metrics.frame_width = DPI(1);
        for(int i = 0; i < 4; i++) {
            style.palette.face[i] = UiFill::Solid(Palette().segment_face);
            style.palette.frame[i] = Palette().segment_frame;
            style.palette.ink[i] = Palette().ink;
        }
        style.show_rule = cfg_.show_rule;
        style.show_bottom_line = cfg_.show_bottom;
        style.hover_enabled = cfg_.hover;

        card_.SetStyle(style)
             .SetTitle(cfg_.title)
             .SetSubTitle(cfg_.subtitle)
             .SetCopyText(cfg_.copy)
             .SetMedia(ICON_EDITOR_NOTES_48(), Size(cfg_.media_size, cfg_.media_size))
             .SetMediaSide(cfg_.media_side)
             .SetMediaSharePercent(cfg_.media_share)
             .EnableHover(cfg_.hover)
             .SetSelectable(cfg_.selectable);

        side_drop_.SelectByData((int)cfg_.media_side);
        share_row_.Slider().SetValue(cfg_.media_share);
        size_row_.Slider().SetValue(cfg_.media_size);
        radius_row_.Slider().SetValue(cfg_.radius);
        rule_row_.Toggle().SetOn(cfg_.show_rule);
        bottom_row_.Toggle().SetOn(cfg_.show_bottom);
        hover_row_.Toggle().SetOn(cfg_.hover);
        selectable_row_.Toggle().SetOn(cfg_.selectable);
        share_row_.SetValueText(AsString(cfg_.media_share) + "%");
        size_row_.SetValueText(AsString(cfg_.media_size) + "px");
        radius_row_.SetValueText(AsString(cfg_.radius) + "px");

        state_theme_value_.SetText(Palette().dark ? "Dark" : "Light");
        state_title_value_.SetText(cfg_.title);
        state_side_value_.SetText(SideLabel());
        state_rule_value_.SetText(cfg_.show_rule ? "Shown" : "Hidden");

        SetUsageCode(BuildUsageCode());
        Preview().Refresh();
    }

    String BuildUsageCode() const
    {
        String code;
        code << "UiTitleCard card;\n";
        code << "UiTitleCard::Style style = UiTheme::ResolveTitleCard();\n";
        code << "style.metrics.radius = " << cfg_.radius << ";\n";
        code << "style.show_rule = " << (cfg_.show_rule ? "true" : "false") << ";\n";
        code << "style.show_bottom_line = " << (cfg_.show_bottom ? "true" : "false") << ";\n";
        code << "card.SetStyle(style)\n";
        code << "    .SetTitle(" << QuoteCpp(cfg_.title) << ")\n";
        code << "    .SetSubTitle(" << QuoteCpp(cfg_.subtitle) << ")\n";
        code << "    .SetCopyText(" << QuoteCpp(cfg_.copy) << ")\n";
        code << "    .SetMedia(ICON_EDITOR_NOTES_48(), Size(" << cfg_.media_size << ", " << cfg_.media_size << "))\n";
        code << "    .SetMediaSide(UiAlign::" << (cfg_.media_side == UiAlign::RIGHT ? "RIGHT" : cfg_.media_side == UiAlign::TOP ? "TOP" : cfg_.media_side == UiAlign::BOTTOM ? "BOTTOM" : "LEFT") << ")\n";
        code << "    .SetMediaSharePercent(" << cfg_.media_share << ");\n";
        return code;
    }

    TitleCardConfig cfg_;
    UiTitleCard card_;

    UiBoxLayout state_theme_row_ { UiBoxLayout::Direction::H }, state_title_row_ { UiBoxLayout::Direction::H }, state_side_row_ { UiBoxLayout::Direction::H }, state_rule_row_ { UiBoxLayout::Direction::H };
    UiLabel state_theme_label_, state_theme_value_, state_title_label_, state_title_value_, state_side_label_, state_side_value_, state_rule_label_, state_rule_value_;

    UiBoxLayout title_row_box_ { UiBoxLayout::Direction::H }, subtitle_row_box_ { UiBoxLayout::Direction::H }, copy_row_box_ { UiBoxLayout::Direction::H }, side_row_box_ { UiBoxLayout::Direction::H };
    UiLabel title_label_, subtitle_label_, copy_label2_, side_label_;
    UiLineEdit title_edit_, subtitle_edit_, copy_edit_;
    UiDropdown side_drop_;
    UiCompositeSlider share_row_, size_row_, radius_row_;
    UiCompositeToggle rule_row_, bottom_row_, hover_row_, selectable_row_;
};

}

GUI_APP_MAIN
{
    UiTitleCardBuilder demo;
    demo.Run();
}

