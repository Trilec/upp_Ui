/*
    UiTitleCardDemo
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

struct TitleCardConfig {
    String title = "Release Notes";
    String subtitle = "Sprint 12";
    String copy = "A compact summary card that can carry media, title, and copy in a single styled surface.";
    UiAlign media_side = UiAlign::LEFT;
    int media_share = 28;
    int media_size = DPI(28);
    int radius = DPI(8);
    bool title_line = true;
    UiSpan title_line_length = MEDIUM;
    int title_line_thickness = 1;
    bool card_line = true;
    UiSpan card_line_length = LARGE;
    int card_line_thickness = 1;
    bool hover = false;
    bool selectable = false;
};

class UiTitleCardBuilder : public BuilderWindowBase {
public:
    typedef UiTitleCardBuilder CLASSNAME;

    UiTitleCardBuilder()
        : BuilderWindowBase("UiTitleCardDemo", "U++ UiTitleCard Builder", "Inspect header card media placement, title lines, and title/copy layout from one shell.")
    {
        Preview().Add(card_);
        Preview().Add(mirror_card_);

        card_cell_.SetDirection(UiDirection::H).SetGap(DPI(8)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        mirror_cell_.SetDirection(UiDirection::H).SetGap(DPI(8)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        card_cell_.Add(card_cell_primary_).Fit();
        card_cell_.Add(card_cell_secondary_).Fit();
        mirror_cell_.Add(mirror_cell_primary_).Fit();
        mirror_cell_.Add(mirror_cell_secondary_).Fit();

        card_cell_primary_.SetText("Primary");
        card_cell_secondary_.SetText("Secondary");
        mirror_cell_primary_.SetText("Accept");
        mirror_cell_secondary_.SetText("Cancel");

        card_.SetContentCell(card_cell_);
        mirror_card_.SetContentCell(mirror_cell_);

        AddStateRow(StateBox(), state_theme_row_, state_theme_label_, state_theme_value_, "Theme");
        AddStateRow(StateBox(), state_title_row_, state_title_label_, state_title_value_, "Title");
        AddStateRow(StateBox(), state_side_row_, state_side_label_, state_side_value_, "Media Side");
        AddStateRow(StateBox(), state_title_line_row_, state_title_line_label_, state_title_line_value_, "Title Line");

        AddEditRow(PropsBox(), title_row_box_, title_label_, title_edit_, "Title");
        AddEditRow(PropsBox(), subtitle_row_box_, subtitle_label_, subtitle_edit_, "Subtitle");
        AddEditRow(PropsBox(), copy_row_box_, copy_label2_, copy_edit_, "Copy");
        AddDropdownRow(PropsBox(), side_row_box_, side_label_, side_drop_, "Media Side");
        AddSliderRow(PropsBox(), share_row_, "Media %", "28%");
        AddSliderRow(PropsBox(), size_row_, "Media Sz", "28px");
        AddSliderRow(PropsBox(), radius_row_, "Radius", "8px");
        AddToggleRow(PropsBox(), title_line_row_, "Title Line");
        AddDropdownRow(PropsBox(), title_line_length_box_, title_line_length_label_, title_line_length_drop_, "Title Length");
        AddSliderRow(PropsBox(), title_line_thickness_row_, "Title Thick", "1px");
        AddToggleRow(PropsBox(), card_line_row_, "Card Line");
        AddDropdownRow(PropsBox(), card_line_length_box_, card_line_length_label_, card_line_length_drop_, "Card Length");
        AddSliderRow(PropsBox(), card_line_thickness_row_, "Card Thick", "1px");
        AddToggleRow(PropsBox(), hover_row_, "Hover");
        AddToggleRow(PropsBox(), selectable_row_, "Selectable");

        const EnumOption sides[] = { { "Left", (int)UiAlign::LEFT }, { "Right", (int)UiAlign::RIGHT }, { "Top", (int)UiAlign::TOP }, { "Bottom", (int)UiAlign::BOTTOM } };
        PopulateDropdown(side_drop_, sides, 4);
        const EnumOption spans[] = { { "None", (int)NONE }, { "Small", (int)SMALL }, { "Medium", (int)MEDIUM }, { "Large", (int)LARGE } };
        PopulateDropdown(title_line_length_drop_, spans, 4);
        PopulateDropdown(card_line_length_drop_, spans, 4);

        title_edit_.SetData(cfg_.title);
        subtitle_edit_.SetData(cfg_.subtitle);
        copy_edit_.SetData(cfg_.copy);
        share_row_.Slider().SetRange(20, 60).SetStep(1).SetValue(cfg_.media_share);
        size_row_.Slider().SetRange(DPI(18), DPI(48)).SetStep(1).SetValue(cfg_.media_size);
        radius_row_.Slider().SetRange(0, DPI(18)).SetStep(1).SetValue(cfg_.radius);
        title_line_thickness_row_.Slider().SetRange(1, 6).SetStep(1).SetValue(cfg_.title_line_thickness);
        card_line_thickness_row_.Slider().SetRange(1, 6).SetStep(1).SetValue(cfg_.card_line_thickness);

        title_edit_.WhenChange = [=] { cfg_.title = title_edit_.GetData().ToString(); RefreshFromConfig(); };
        subtitle_edit_.WhenChange = [=] { cfg_.subtitle = subtitle_edit_.GetData().ToString(); RefreshFromConfig(); };
        copy_edit_.WhenChange = [=] { cfg_.copy = copy_edit_.GetData().ToString(); RefreshFromConfig(); };
        side_drop_.WhenSelect = [=](int) { cfg_.media_side = (UiAlign)(int)side_drop_.GetSelectedData(); RefreshFromConfig(); };
        share_row_.WhenAction = [=] { cfg_.media_share = (int)share_row_.Slider().GetValue(); RefreshFromConfig(); };
        size_row_.WhenAction = [=] { cfg_.media_size = (int)size_row_.Slider().GetValue(); RefreshFromConfig(); };
        radius_row_.WhenAction = [=] { cfg_.radius = (int)radius_row_.Slider().GetValue(); RefreshFromConfig(); };
        title_line_row_.Toggle().WhenAction = [=] { cfg_.title_line = title_line_row_.Toggle().IsOn(); RefreshFromConfig(); };
        title_line_length_drop_.WhenSelect = [=](int) { cfg_.title_line_length = (UiSpan)(int)title_line_length_drop_.GetSelectedData(); RefreshFromConfig(); };
        title_line_thickness_row_.WhenAction = [=] { cfg_.title_line_thickness = (int)title_line_thickness_row_.Slider().GetValue(); RefreshFromConfig(); };
        card_line_row_.Toggle().WhenAction = [=] { cfg_.card_line = card_line_row_.Toggle().IsOn(); RefreshFromConfig(); };
        card_line_length_drop_.WhenSelect = [=](int) { cfg_.card_line_length = (UiSpan)(int)card_line_length_drop_.GetSelectedData(); RefreshFromConfig(); };
        card_line_thickness_row_.WhenAction = [=] { cfg_.card_line_thickness = (int)card_line_thickness_row_.Slider().GetValue(); RefreshFromConfig(); };
        hover_row_.Toggle().WhenAction = [=] { cfg_.hover = hover_row_.Toggle().IsOn(); RefreshFromConfig(); };
        selectable_row_.Toggle().WhenAction = [=] { cfg_.selectable = selectable_row_.Toggle().IsOn(); RefreshFromConfig(); };

        FinishInit();
        RefreshFromConfig();
    }

protected:
    virtual void ApplyDemoTheme() override
    {
        RefreshFromConfig();
    }

    virtual void LayoutPreviewContent() override
    {
        Rect canvas = Preview().GetCanvasRect();
        int w = min(DPI(480), canvas.GetWidth() - DPI(30));
        int h = min(DPI(150), max(DPI(110), (canvas.GetHeight() - DPI(40)) / 2));
        int x = canvas.left + (canvas.GetWidth() - w) / 2;
        int y = canvas.top + DPI(18);
        card_.SetRect(x, y, w, h);
        mirror_card_.SetRect(x, y + h + DPI(16), w, h);
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

    String SpanLabel(UiSpan span) const
    {
        switch(span) {
        case NONE: return "None";
        case SMALL: return "Small";
        case MEDIUM: return "Medium";
        case LARGE:
        default: return "Large";
        }
    }

    String SpanCode(UiSpan span) const
    {
        switch(span) {
        case NONE: return "NONE";
        case SMALL: return "SMALL";
        case MEDIUM: return "MEDIUM";
        case LARGE:
        default: return "LARGE";
        }
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
        style.title_line = cfg_.title_line;
        style.title_line_length = cfg_.title_line_length;
        style.title_line_thickness = cfg_.title_line_thickness;
        style.card_line = cfg_.card_line;
        style.card_line_length = cfg_.card_line_length;
        style.card_line_thickness = cfg_.card_line_thickness;
        style.hover_enabled = cfg_.hover;

        card_.SetCustomStyle(style)
             .SetTitle(cfg_.title)
             .SetSubTitle(cfg_.subtitle)
             .SetCopyText(cfg_.copy)
             .SetMedia(ICON_EDITOR_NOTES_48(), Size(cfg_.media_size, cfg_.media_size))
             .SetMediaSide(cfg_.media_side)
             .SetMediaSharePercent(cfg_.media_share)
             .EnableHover(cfg_.hover)
             .SetSelectable(cfg_.selectable);

        UiTitleCard::Style mirror_style = style;
        mirror_card_.SetCustomStyle(mirror_style)
             .SetTitle(cfg_.title)
             .SetSubTitle(cfg_.subtitle)
             .SetCopyText(cfg_.copy)
             .SetMedia(ICON_EDITOR_NOTES_48(), Size(cfg_.media_size, cfg_.media_size))
             .SetMediaSide(cfg_.media_side == UiAlign::LEFT ? UiAlign::RIGHT : UiAlign::LEFT)
             .SetMediaSharePercent(cfg_.media_share)
             .EnableHover(false)
             .SetSelectable(false);

        card_.SetTextAlign(UiAlign::LEFT, UiAlign::CENTER);
        card_.SetCardLineSide(UiAlign::RIGHT);
        card_.SetContentCellGap(DPI(8));
        mirror_card_.SetTextAlign(UiAlign::RIGHT, UiAlign::CENTER);
        mirror_card_.SetCardLineSide(UiAlign::LEFT);
        mirror_card_.SetContentCellGap(DPI(8));

        side_drop_.SelectByData((int)cfg_.media_side);
        share_row_.Slider().SetValue(cfg_.media_share);
        size_row_.Slider().SetValue(cfg_.media_size);
        radius_row_.Slider().SetValue(cfg_.radius);
        title_line_row_.Toggle().SetOn(cfg_.title_line);
        title_line_length_drop_.SelectByData((int)cfg_.title_line_length);
        title_line_thickness_row_.Slider().SetValue(cfg_.title_line_thickness);
        card_line_row_.Toggle().SetOn(cfg_.card_line);
        card_line_length_drop_.SelectByData((int)cfg_.card_line_length);
        card_line_thickness_row_.Slider().SetValue(cfg_.card_line_thickness);
        hover_row_.Toggle().SetOn(cfg_.hover);
        selectable_row_.Toggle().SetOn(cfg_.selectable);
        share_row_.SetValueText(AsString(cfg_.media_share) + "%");
        size_row_.SetValueText(AsString(cfg_.media_size) + "px");
        radius_row_.SetValueText(AsString(cfg_.radius) + "px");
        title_line_thickness_row_.SetValueText(AsString(cfg_.title_line_thickness) + "px");
        card_line_thickness_row_.SetValueText(AsString(cfg_.card_line_thickness) + "px");

        state_theme_value_.SetText(Palette().dark ? "Dark" : "Light");
        state_title_value_.SetText(cfg_.title);
        state_side_value_.SetText(SideLabel());
        state_title_line_value_.SetText(cfg_.title_line ? SpanLabel(cfg_.title_line_length) : "Hidden");

        SetUsageCode(BuildUsageCode());
        Preview().Refresh();
    }

    String BuildUsageCode() const
    {
        String code;
        code << "UiBoxLayout card_cell(UiDirection::H);\n";
        code << "card_cell.SetGap(DPI(8)).SetInset(0).SetAlignItems(UiCrossAlign::Center);\n";
        code << "card_cell.Add(card_primary).Fit();\n";
        code << "card_cell.Add(card_secondary).Fit();\n";
        code << "UiTitleCard card;\n";
        code << "UiTitleCard::Style style = UiTheme::ResolveTitleCard();\n";
        code << "style.metrics.radius = " << cfg_.radius << ";\n";
        code << "style.title_line = " << (cfg_.title_line ? "true" : "false") << ";\n";
        code << "style.title_line_length = " << SpanCode(cfg_.title_line_length) << ";\n";
        code << "style.title_line_thickness = " << cfg_.title_line_thickness << ";\n";
        code << "style.card_line = " << (cfg_.card_line ? "true" : "false") << ";\n";
        code << "style.card_line_length = " << SpanCode(cfg_.card_line_length) << ";\n";
        code << "style.card_line_thickness = " << cfg_.card_line_thickness << ";\n";
        code << "card.SetCustomStyle(style)\n";
        code << "    .SetTitle(" << QuoteCpp(cfg_.title) << ")\n";
        code << "    .SetSubTitle(" << QuoteCpp(cfg_.subtitle) << ")\n";
        code << "    .SetCopyText(" << QuoteCpp(cfg_.copy) << ")\n";
        code << "    .SetMedia(ICON_EDITOR_NOTES_48(), Size(" << cfg_.media_size << ", " << cfg_.media_size << "))\n";
        code << "    .SetMediaSide(UiAlign::" << (cfg_.media_side == UiAlign::RIGHT ? "RIGHT" : cfg_.media_side == UiAlign::TOP ? "TOP" : cfg_.media_side == UiAlign::BOTTOM ? "BOTTOM" : "LEFT") << ")\n";
        code << "    .SetMediaSharePercent(" << cfg_.media_share << ")\n";
        code << "    .SetContentCell(card_cell);\n";
        return code;
    }

    TitleCardConfig cfg_;
    UiTitleCard card_;
    UiTitleCard mirror_card_;
    UiBoxLayout card_cell_ { UiBoxLayout::Direction::H };
    UiBoxLayout mirror_cell_ { UiBoxLayout::Direction::H };
    UiButton card_cell_primary_;
    UiButton card_cell_secondary_;
    UiButton mirror_cell_primary_;
    UiButton mirror_cell_secondary_;

    UiBoxLayout state_theme_row_ { UiBoxLayout::Direction::H }, state_title_row_ { UiBoxLayout::Direction::H }, state_side_row_ { UiBoxLayout::Direction::H }, state_title_line_row_ { UiBoxLayout::Direction::H };
    UiLabel state_theme_label_, state_theme_value_, state_title_label_, state_title_value_, state_side_label_, state_side_value_, state_title_line_label_, state_title_line_value_;

    UiBoxLayout title_row_box_ { UiBoxLayout::Direction::H }, subtitle_row_box_ { UiBoxLayout::Direction::H }, copy_row_box_ { UiBoxLayout::Direction::H }, side_row_box_ { UiBoxLayout::Direction::H };
    UiBoxLayout title_line_length_box_ { UiBoxLayout::Direction::H }, card_line_length_box_ { UiBoxLayout::Direction::H };
    UiLabel title_label_, subtitle_label_, copy_label2_, side_label_, title_line_length_label_, card_line_length_label_;
    UiLineEdit title_edit_, subtitle_edit_, copy_edit_;
    UiDropdown side_drop_, title_line_length_drop_, card_line_length_drop_;
    UiCompositeSlider share_row_, size_row_, radius_row_, title_line_thickness_row_, card_line_thickness_row_;
    UiCompositeToggle title_line_row_, card_line_row_, hover_row_, selectable_row_;
};

}

GUI_APP_MAIN
{
    UiTitleCardBuilder demo;
    demo.Run();
}

