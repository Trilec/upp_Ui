#include "../BuilderDemoSupport.h"

using namespace Upp;
using namespace BuilderDemoSupport;

namespace {

Image MakeMediaPreview()
{
    const Size size(640, 420);
    ImageBuffer out(size);

    for(int y = 0; y < size.cy; y++) {
        RGBA *row = out[y];
        for(int x = 0; x < size.cx; x++) {
            const int r = 34 + x * 62 / size.cx + y * 18 / size.cy;
            const int g = 48 + x * 35 / size.cx + y * 52 / size.cy;
            const int b = 69 + x * 50 / size.cx + y * 40 / size.cy;
            row[x] = RGBA(Color(min(r, 255), min(g, 255), min(b, 255)));
            row[x].a = 255;
        }
    }
    return Image(out);
}

struct MediaCardConfig {
    String title = "SH030 take 03";
    String subtitle = "1536 x 864 - Flux";
    String metadata = "AURORA / SQ020";
    UiRole role = UiRole::Standard;
    UiMediaFit fit = UiMediaFit::Cover;
    UiAlign label_side = UiAlign::BOTTOM;
    int radius = DPI(10);
    bool selected = false;
    bool enabled = true;
    bool show_badges = true;
};

class UiMediaCardBuilder : public BuilderWindowBase {
public:
    typedef UiMediaCardBuilder CLASSNAME;

    UiMediaCardBuilder()
        : BuilderWindowBase(
            "UiMediaCardDemo",
            "U++ UiMediaCard Builder",
            "Media-first card presentation, semantic badges, fit modes and theme-safe interaction.")
    {
        Preview().Add(card_);
        Preview().Add(empty_card_);
        Preview().Add(side_card_);

        AddStateRow(StateBox(), state_theme_row_,
                    state_theme_label_, state_theme_value_, "Theme");
        AddStateRow(StateBox(), state_fit_row_,
                    state_fit_label_, state_fit_value_, "Fit");
        AddStateRow(StateBox(), state_side_row_,
                    state_side_label_, state_side_value_, "Labels");
        AddStateRow(StateBox(), state_badges_row_,
                    state_badges_label_, state_badges_value_, "Badges");

        AddEditRow(PropsBox(), title_row_, title_label_, title_edit_, "Title");
        AddEditRow(PropsBox(), subtitle_row_, subtitle_label_, subtitle_edit_, "Subtitle");
        AddEditRow(PropsBox(), metadata_row_, metadata_label_, metadata_edit_, "Metadata");
        AddDropdownRow(PropsBox(), role_row_, role_label_, role_drop_, "Role");
        AddDropdownRow(PropsBox(), fit_row_, fit_label_, fit_drop_, "Media Fit");
        AddDropdownRow(PropsBox(), side_row_, side_label_, side_drop_, "Label Side");
        AddSliderRow(PropsBox(), radius_row_, "Radius", "10px");
        AddToggleRow(PropsBox(), badges_row_, "Badges");
        AddToggleRow(PropsBox(), selected_row_, "Selected");
        AddToggleRow(PropsBox(), enabled_row_, "Enabled");

        PopulateRole();
        PopulateFit();
        PopulateSide();

        title_edit_.SetData(cfg_.title);
        subtitle_edit_.SetData(cfg_.subtitle);
        metadata_edit_.SetData(cfg_.metadata);
        radius_row_.Slider().SetRange(0, DPI(24)).SetStep(1).SetValue(cfg_.radius);

        title_edit_.WhenChange =
            [=] { cfg_.title = title_edit_.GetData().ToString(); RefreshFromConfig(); };
        subtitle_edit_.WhenChange =
            [=] { cfg_.subtitle = subtitle_edit_.GetData().ToString(); RefreshFromConfig(); };
        metadata_edit_.WhenChange =
            [=] { cfg_.metadata = metadata_edit_.GetData().ToString(); RefreshFromConfig(); };
        role_drop_.WhenSelect =
            [=](int) { cfg_.role = (UiRole)(int)role_drop_.GetSelectedData(); RefreshFromConfig(); };
        fit_drop_.WhenSelect =
            [=](int) { cfg_.fit = (UiMediaFit)(int)fit_drop_.GetSelectedData(); RefreshFromConfig(); };
        side_drop_.WhenSelect =
            [=](int) { cfg_.label_side = (UiAlign)(int)side_drop_.GetSelectedData(); RefreshFromConfig(); };
        radius_row_.WhenAction =
            [=] { cfg_.radius = (int)radius_row_.Slider().GetValue(); RefreshFromConfig(); };
        badges_row_.Toggle().WhenAction =
            [=] { cfg_.show_badges = badges_row_.Toggle().IsOn(); RefreshFromConfig(); };
        selected_row_.Toggle().WhenAction =
            [=] { cfg_.selected = selected_row_.Toggle().IsOn(); RefreshFromConfig(); };
        enabled_row_.Toggle().WhenAction =
            [=] { cfg_.enabled = enabled_row_.Toggle().IsOn(); RefreshFromConfig(); };

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
        const Rect canvas = Preview().GetCanvasRect();
        const int gap = DPI(18);
        const int main_w =
            min(DPI(340), max(DPI(220), canvas.GetWidth() * 46 / 100));
        const int main_h =
            min(DPI(390), max(DPI(270), canvas.GetHeight() - DPI(48)));
        const int x = canvas.left + DPI(28);
        const int y =
            canvas.top + max(0, (canvas.GetHeight() - main_h) / 2);

        card_.SetRect(x, y, main_w, main_h);

        const int small_w =
            min(DPI(220), max(DPI(160),
                canvas.right - card_.GetRect().right - gap - DPI(24)));
        const int small_h =
            min(DPI(190), max(DPI(145), (main_h - gap) / 2));
        const int sx = card_.GetRect().right + gap;

        empty_card_.SetRect(sx, y, small_w, small_h);
        side_card_.SetRect(sx, y + small_h + gap, small_w, small_h);
    }

private:
    void PopulateRole()
    {
        role_drop_.UseInternalModel();
        role_drop_.Clear();
        role_drop_.Add("Standard", (int)UiRole::Standard);
        role_drop_.Add("Subtle", (int)UiRole::Subtle);
        role_drop_.Add("Accent", (int)UiRole::Accent);
        role_drop_.Add("Alert", (int)UiRole::Alert);
    }

    void PopulateFit()
    {
        fit_drop_.UseInternalModel();
        fit_drop_.Clear();
        fit_drop_.Add("Cover", (int)UiMediaFit::Cover);
        fit_drop_.Add("Contain", (int)UiMediaFit::Contain);
    }

    void PopulateSide()
    {
        side_drop_.UseInternalModel();
        side_drop_.Clear();
        side_drop_.Add("Bottom", (int)UiAlign::BOTTOM);
        side_drop_.Add("Top", (int)UiAlign::TOP);
        side_drop_.Add("Left", (int)UiAlign::LEFT);
        side_drop_.Add("Right", (int)UiAlign::RIGHT);
    }

    String RoleName() const
    {
        switch(cfg_.role) {
        case UiRole::Subtle: return "Subtle";
        case UiRole::Accent: return "Accent";
        case UiRole::Alert: return "Alert";
        default: return "Standard";
        }
    }

    String FitName() const
    {
        return cfg_.fit == UiMediaFit::Contain ? "Contain" : "Cover";
    }

    String SideName() const
    {
        switch(cfg_.label_side) {
        case UiAlign::TOP: return "Top";
        case UiAlign::LEFT: return "Left";
        case UiAlign::RIGHT: return "Right";
        default: return "Bottom";
        }
    }

    String SideCode() const
    {
        switch(cfg_.label_side) {
        case UiAlign::TOP: return "TOP";
        case UiAlign::LEFT: return "LEFT";
        case UiAlign::RIGHT: return "RIGHT";
        default: return "BOTTOM";
        }
    }

    void AddSemanticBadges(UiMediaCard& card)
    {
        if(!cfg_.show_badges)
            return;

        UiBadgeData kind("IMAGE", UiRole::Subtle, UiBadgeVariant::Filled);
        kind.id = "kind";
        card.AddBadge(kind, UiMediaBadgeAnchor::TopLeft);

        UiBadgeData state("READY", UiRole::Accent, UiBadgeVariant::Soft);
        state.id = "state";
        card.AddBadge(state, UiMediaBadgeAnchor::TopRight);

        UiBadgeData take("take 03", UiRole::Standard, UiBadgeVariant::Soft);
        take.id = "take";
        card.AddBadge(take, UiMediaBadgeAnchor::BottomRight);
    }

    void RefreshFromConfig()
    {
        card_.ClearCustomStyle().SetRole(cfg_.role);

        UiMediaCard::Style style = card_.GetStyle();
        style.metrics.radius = cfg_.radius;
        style.media_metrics.radius = max(0, cfg_.radius - DPI(3));
        for(int i = 0; i < 4; i++)
            style.badge_style[i].metrics.radius =
                max(DPI(3), cfg_.radius / 2);

        card_.SetCustomStyle(style)
             .SetImage(MakeMediaPreview())
             .SetTitle(cfg_.title)
             .SetSubTitle(cfg_.subtitle)
             .SetMetadata(cfg_.metadata)
             .SetMediaFit(cfg_.fit)
             .SetLabelSide(cfg_.label_side)
             .SetSelected(cfg_.selected)
             .ClearBadges();

        AddSemanticBadges(card_);
        card_.Enable(cfg_.enabled);

        empty_card_.ClearCustomStyle().SetRole(UiRole::Subtle);
        UiMediaCard::Style empty_style = empty_card_.GetStyle();
        empty_style.metrics.radius = DPI(9);

        empty_card_.SetCustomStyle(empty_style)
                   .ClearImage()
                   .SetEmptyCue("+")
                   .SetTitle("Reference 1")
                   .SetSubTitle("Drop or choose media")
                   .SetMetadata("")
                   .SetLabelSide(UiAlign::BOTTOM)
                   .ClearBadges();

        UiBadgeData empty_kind(
            "IMAGE", UiRole::Subtle, UiBadgeVariant::Outline);
        empty_card_.AddBadge(
            empty_kind, UiMediaBadgeAnchor::TopLeft);

        side_card_.ClearCustomStyle().SetRole(UiRole::Standard);
        UiMediaCard::Style side_style = side_card_.GetStyle();
        side_style.metrics.radius = DPI(9);

        side_card_.SetCustomStyle(side_style)
                  .SetImage(MakeMediaPreview())
                  .SetTitle("Camera reference")
                  .SetSubTitle("50 mm")
                  .SetMetadata("frame 138")
                  .SetLabelSide(UiAlign::RIGHT)
                  .SetMediaFit(UiMediaFit::Cover)
                  .ClearBadges();

        UiBadgeData ref(
            "3D REF", UiRole::Accent, UiBadgeVariant::Outline);
        side_card_.AddBadge(
            ref, UiMediaBadgeAnchor::TopLeft);

        role_drop_.SelectByData((int)cfg_.role);
        fit_drop_.SelectByData((int)cfg_.fit);
        side_drop_.SelectByData((int)cfg_.label_side);
        radius_row_.Slider().SetValue(cfg_.radius);
        radius_row_.SetValueText(AsString(cfg_.radius) + "px");
        badges_row_.Toggle().SetOn(cfg_.show_badges);
        selected_row_.Toggle().SetOn(cfg_.selected);
        enabled_row_.Toggle().SetOn(cfg_.enabled);

        state_theme_value_.SetText(Palette().dark ? "Dark" : "Light");
        state_fit_value_.SetText(FitName());
        state_side_value_.SetText(SideName());
        state_badges_value_.SetText(
            cfg_.show_badges ? AsString(card_.GetBadgeCount()) : "None");

        String code;
        code << "UiMediaCard card;\n";
        code << "card.SetImage(image)\n";
        code << "    .SetTitle(" << QuoteCpp(cfg_.title) << ")\n";
        code << "    .SetSubTitle(" << QuoteCpp(cfg_.subtitle) << ")\n";
        code << "    .SetMetadata(" << QuoteCpp(cfg_.metadata) << ")\n";
        code << "    .SetRole(UiRole::" << RoleName() << ")\n";
        code << "    .SetMediaFit(UiMediaFit::" << FitName() << ")\n";
        code << "    .SetLabelSide(UiAlign::" << SideCode() << ");\n";

        if(cfg_.show_badges) {
            code << "UiBadgeData state(\"READY\", UiRole::Accent, UiBadgeVariant::Soft);\n";
            code << "card.AddBadge(state, UiMediaBadgeAnchor::TopRight);\n";
        }

        SetUsageCode(code);
        Preview().Refresh();
    }

    MediaCardConfig cfg_;
    UiMediaCard card_;
    UiMediaCard empty_card_;
    UiMediaCard side_card_;

    UiBoxLayout state_theme_row_ { UiDirection::H };
    UiBoxLayout state_fit_row_ { UiDirection::H };
    UiBoxLayout state_side_row_ { UiDirection::H };
    UiBoxLayout state_badges_row_ { UiDirection::H };
    UiLabel state_theme_label_, state_theme_value_;
    UiLabel state_fit_label_, state_fit_value_;
    UiLabel state_side_label_, state_side_value_;
    UiLabel state_badges_label_, state_badges_value_;

    UiBoxLayout title_row_ { UiDirection::H };
    UiBoxLayout subtitle_row_ { UiDirection::H };
    UiBoxLayout metadata_row_ { UiDirection::H };
    UiBoxLayout role_row_ { UiDirection::H };
    UiBoxLayout fit_row_ { UiDirection::H };
    UiBoxLayout side_row_ { UiDirection::H };
    UiLabel title_label_, subtitle_label_, metadata_label_;
    UiLabel role_label_, fit_label_, side_label_;
    UiLineEdit title_edit_, subtitle_edit_, metadata_edit_;
    UiDropdown role_drop_, fit_drop_, side_drop_;
    DemoSliderRow radius_row_;
    DemoToggleRow badges_row_, selected_row_, enabled_row_;
};

} // namespace

GUI_APP_MAIN
{
    UiMediaCardBuilder app;
    app.Run();
}
