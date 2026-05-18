/*
    UiAccordionDemo
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

struct AccordionConfig {
    bool single_open = false;
    bool enforce_one = false;
    bool show_chevron = true;
    bool animation = true;
    bool drag_reorder = false;
    bool show_drag_handle = true;
    UiAlign chevron_side = UiAlign::RIGHT;
    UiAlign drag_side = UiAlign::RIGHT;
    String drag_glyph;
    int header_height = DPI(28);
    int item_spacing = DPI(6);
    int chevron_size = DPI(10);
    int chevron_gap = DPI(8);
    int radius = DPI(8);
};

class UiAccordionBuilder : public BuilderWindowBase {
public:
    typedef UiAccordionBuilder CLASSNAME;

    UiAccordionBuilder()
        : BuilderWindowBase("UiAccordionDemo", "U++ UiAccordion Builder", "Inspect section open policy, chevron behavior, spacing, and drag reorder from one shell.", 1240, 820)
    {
        Preview().Add(acc_);
        BuildSections();

        AddStateRow(StateBox(), state_theme_row_, state_theme_label_, state_theme_value_, "Theme");
        AddStateRow(StateBox(), state_open_row_, state_open_label_, state_open_value_, "Open");
        AddStateRow(StateBox(), state_chevron_row_, state_chevron_label_, state_chevron_value_, "Chevron");
        AddStateRow(StateBox(), state_drag_row_, state_drag_label_, state_drag_value_, "Drag");

        AddSliderRow(PropsBox(), header_row_, "Header H", "28px");
        AddSliderRow(PropsBox(), gap_row_, "Item Spacing", "6px");
        AddSliderRow(PropsBox(), chevron_size_row_, "Chevron Sz", "10px");
        AddSliderRow(PropsBox(), chevron_gap_row_, "Chevron Gap", "8px");
        AddSliderRow(PropsBox(), radius_row_, "Radius", "8px");
        AddDropdownRow(PropsBox(), side_row_box_, side_label_, side_drop_, "Chevron Side");
        AddToggleRow(PropsBox(), single_row_, "Single Open");
        AddToggleRow(PropsBox(), enforce_row_, "Enforce One");
        AddToggleRow(PropsBox(), chevron_row_, "Show Chevron");
        AddToggleRow(PropsBox(), animation_row_, "Animation");
        AddToggleRow(PropsBox(), drag_row_, "Use Drag");
        AddToggleRow(PropsBox(), drag_handle_row_, "Drag Handle");
        AddDropdownRow(PropsBox(), drag_side_row_box_, drag_side_label_, drag_side_drop_, "Drag Side");
        AddDropdownRow(PropsBox(), drag_glyph_row_box_, drag_glyph_label_, drag_glyph_drop_, "Drag Glyph");
        AddButtonRow(PropsBox(), action_row_, open_all_btn_, close_all_btn_);

        open_all_btn_.SetText("Open All");
        close_all_btn_.SetText("Close All");

        const EnumOption sides[] = { { "Left", (int)UiAlign::LEFT }, { "Right", (int)UiAlign::RIGHT } };
        const EnumOption glyphs[] = {
            { "Default", 0 },
            { "Drag", 1 },
            { "Adjust", 2 },
            { "Circle", 3 }
        };
        PopulateDropdown(side_drop_, sides, 2);
        PopulateDropdown(drag_side_drop_, sides, 2);
        PopulateDropdown(drag_glyph_drop_, glyphs, 4);
        header_row_.Slider().SetRange(DPI(22), DPI(40)).SetStep(1).SetValue(cfg_.header_height);
        gap_row_.Slider().SetRange(0, DPI(14)).SetStep(1).SetValue(cfg_.item_spacing);
        chevron_size_row_.Slider().SetRange(DPI(6), DPI(24)).SetStep(1).SetValue(cfg_.chevron_size);
        chevron_gap_row_.Slider().SetRange(0, DPI(20)).SetStep(1).SetValue(cfg_.chevron_gap);
        radius_row_.Slider().SetRange(0, DPI(18)).SetStep(1).SetValue(cfg_.radius);

        header_row_.WhenAction = [=] { cfg_.header_height = (int)header_row_.Slider().GetValue(); RefreshFromConfig(); };
        gap_row_.WhenAction = [=] { cfg_.item_spacing = (int)gap_row_.Slider().GetValue(); RefreshFromConfig(); };
        chevron_size_row_.WhenAction = [=] { cfg_.chevron_size = (int)chevron_size_row_.Slider().GetValue(); RefreshFromConfig(); };
        chevron_gap_row_.WhenAction = [=] { cfg_.chevron_gap = (int)chevron_gap_row_.Slider().GetValue(); RefreshFromConfig(); };
        radius_row_.WhenAction = [=] { cfg_.radius = (int)radius_row_.Slider().GetValue(); RefreshFromConfig(); };
        side_drop_.WhenSelect = [=](int) { cfg_.chevron_side = (UiAlign)(int)side_drop_.GetSelectedData(); RefreshFromConfig(); };
        single_row_.Toggle().WhenAction = [=] { cfg_.single_open = single_row_.Toggle().IsOn(); RefreshFromConfig(); };
        enforce_row_.Toggle().WhenAction = [=] { cfg_.enforce_one = enforce_row_.Toggle().IsOn(); RefreshFromConfig(); };
        chevron_row_.Toggle().WhenAction = [=] { cfg_.show_chevron = chevron_row_.Toggle().IsOn(); RefreshFromConfig(); };
        animation_row_.Toggle().WhenAction = [=] { cfg_.animation = animation_row_.Toggle().IsOn(); RefreshFromConfig(); };
        drag_row_.Toggle().WhenAction = [=] { cfg_.drag_reorder = drag_row_.Toggle().IsOn(); RefreshFromConfig(); };
        drag_handle_row_.Toggle().WhenAction = [=] { cfg_.show_drag_handle = drag_handle_row_.Toggle().IsOn(); RefreshFromConfig(); };
        drag_side_drop_.WhenSelect = [=](int) { cfg_.drag_side = (UiAlign)(int)drag_side_drop_.GetSelectedData(); RefreshFromConfig(); };
        drag_glyph_drop_.WhenSelect = [=](int) { cfg_.drag_glyph = ResolveGlyphName((int)drag_glyph_drop_.GetSelectedData()); RefreshFromConfig(); };
        open_all_btn_.WhenAction = [=] { acc_.OpenAll(true); RefreshState(); };
        close_all_btn_.WhenAction = [=] { acc_.OpenAll(false); RefreshState(); };
        acc_.WhenSectionToggled = [=](int, bool) { RefreshState(); };
        acc_.WhenReordered = [=](int, int) { RefreshState(); };

        FinishInit();
        RefreshFromConfig();
    }

protected:
    virtual void ApplyDemoTheme() override
    {
        UiLabel::Style body = MakeBodyLabelStyle(Palette());
        UiLabel::Style value = MakeValueLabelStyle(Palette());
        UiButton::Style btn = MakeSmallButtonStyle(Palette());
        UiDropdown::Style dd = MakeDropdownStyle(Palette());

        state_theme_label_.SetCustomStyle(body); state_theme_value_.SetCustomStyle(value);
        state_open_label_.SetCustomStyle(body); state_open_value_.SetCustomStyle(value);
        state_chevron_label_.SetCustomStyle(body); state_chevron_value_.SetCustomStyle(value);
        state_drag_label_.SetCustomStyle(body); state_drag_value_.SetCustomStyle(value);
        header_row_.SetLabelStyle(body).SetValueStyle(value);
        gap_row_.SetLabelStyle(body).SetValueStyle(value);
        chevron_size_row_.SetLabelStyle(body).SetValueStyle(value);
        chevron_gap_row_.SetLabelStyle(body).SetValueStyle(value);
        radius_row_.SetLabelStyle(body).SetValueStyle(value);
        side_label_.SetCustomStyle(body);
        drag_side_label_.SetCustomStyle(body);
        drag_glyph_label_.SetCustomStyle(body);
        side_drop_.SetCustomStyle(dd);
        drag_side_drop_.SetCustomStyle(dd);
        drag_glyph_drop_.SetCustomStyle(dd);
        single_row_.SetLabelStyle(body);
        enforce_row_.SetLabelStyle(body);
        chevron_row_.SetLabelStyle(body);
        animation_row_.SetLabelStyle(body);
        drag_row_.SetLabelStyle(body);
        drag_handle_row_.SetLabelStyle(body);
        open_all_btn_.SetCustomStyle(btn);
        close_all_btn_.SetCustomStyle(btn);
        section_a_.SetCustomStyle(body);
        section_b_.SetCustomStyle(body);
        section_c_.SetCustomStyle(body);
    }

    virtual void LayoutPreviewContent() override
    {
        Rect canvas = Preview().GetCanvasRect();
        acc_.SetRect(canvas.Deflated(DPI(20), DPI(20)));
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

    String ResolveGlyphName(int id) const
    {
        switch(id) {
        case 1: return "ICON_DESIGN_DRAG_INDICATOR_48";
        case 2: return "ICON_DESIGN_ADJUST_48";
        case 3: return "ICON_DESIGN_CIRCLE_48";
        default: return String();
        }
    }

    Image ResolveGlyphImage(const String& name) const
    {
        if(name.IsEmpty())
            return Image();
        return UiIconFromName(name);
    }

    void BuildSections()
    {
        int a = acc_.AddSection("Platform", "Build policy", "Desktop + API", true);
        int b = acc_.AddSection("Navigation", "Entry points", "Header + menu + shortcuts", false);
        int c = acc_.AddSection("Release", "Checklist", "Review + QA + package", false);
        acc_.GetSectionContent(a).Add(section_a_.HSizePos().VCenterPos(DPI(20)));
        acc_.GetSectionContent(b).Add(section_b_.HSizePos().VCenterPos(DPI(20)));
        acc_.GetSectionContent(c).Add(section_c_.HSizePos().VCenterPos(DPI(20)));
        section_a_.SetText("One section can stay open while the others collapse or stack below it.");
        section_b_.SetText("Chevron side, visible drag handle, animation, and reorder should all be visible from this one preview.");
        section_c_.SetText("Use the inspector to validate the actual accordion API rather than demo-only behavior.");
    }

    void RefreshState()
    {
        int open = 0;
        for(int i = 0; i < acc_.GetCount(); i++)
            if(acc_.IsOpen(i))
                open++;
        state_theme_value_.SetText(Palette().dark ? "Dark" : "Light");
        state_open_value_.SetText(AsString(open) + " / " + AsString(acc_.GetCount()));
        state_chevron_value_.SetText(cfg_.show_chevron ? (cfg_.chevron_side == UiAlign::RIGHT ? "Right" : "Left") : "Hidden");
        if(!cfg_.drag_reorder)
            state_drag_value_.SetText("Off");
        else
            state_drag_value_.SetText(AsString(cfg_.show_drag_handle ? "Handle" : "Hidden") + " / " + (cfg_.drag_side == UiAlign::RIGHT ? "Right" : "Left"));
    }

    void RefreshFromConfig()
    {
        UiAccordion::Style style = MakeAccordionStyle(Palette());
        style.single_open = cfg_.single_open;
        style.enforce_one = cfg_.enforce_one;
        style.show_chevron = cfg_.show_chevron;
        style.chevron_side = cfg_.chevron_side;
        style.header_height = cfg_.header_height;
        style.item_spacing = cfg_.item_spacing;
        style.chevron_scale = true;
        style.chevron_size = cfg_.chevron_size;
        style.chevron_gap = cfg_.chevron_gap;
        style.animation_enabled = cfg_.animation;
        style.unified_section_radius = cfg_.radius;
        style.show_drag_handle = cfg_.show_drag_handle;
        style.drag_side = cfg_.drag_side;
        if(!cfg_.drag_glyph.IsEmpty())
            style.drag_glyph = ResolveGlyphImage(cfg_.drag_glyph);
        acc_.SetCustomStyle(style)
            .SetSingleOpen(cfg_.single_open)
            .SetEnforceOne(cfg_.enforce_one)
            .ShowChevron(cfg_.show_chevron)
            .SetChevronSide(cfg_.chevron_side)
            .SetChevronSize(cfg_.chevron_size)
            .SetChevronGap(cfg_.chevron_gap)
            .SetAnimation(cfg_.animation)
            .EnableDragReorder(cfg_.drag_reorder)
            .ShowDragHandle(cfg_.show_drag_handle)
            .SetDragSide(cfg_.drag_side)
            .SetDragGlyph(ResolveGlyphImage(cfg_.drag_glyph));

        side_drop_.SelectByData((int)cfg_.chevron_side);
        drag_side_drop_.SelectByData((int)cfg_.drag_side);
        drag_glyph_drop_.SelectByData(cfg_.drag_glyph == "ICON_DESIGN_DRAG_INDICATOR_48" ? 1 : cfg_.drag_glyph == "ICON_DESIGN_ADJUST_48" ? 2 : cfg_.drag_glyph == "ICON_DESIGN_CIRCLE_48" ? 3 : 0);
        header_row_.Slider().SetValue(cfg_.header_height);
        gap_row_.Slider().SetValue(cfg_.item_spacing);
        chevron_size_row_.Slider().SetValue(cfg_.chevron_size);
        chevron_gap_row_.Slider().SetValue(cfg_.chevron_gap);
        radius_row_.Slider().SetValue(cfg_.radius);
        single_row_.Toggle().SetOn(cfg_.single_open);
        enforce_row_.Toggle().SetOn(cfg_.enforce_one);
        chevron_row_.Toggle().SetOn(cfg_.show_chevron);
        animation_row_.Toggle().SetOn(cfg_.animation);
        drag_row_.Toggle().SetOn(cfg_.drag_reorder);
        drag_handle_row_.Toggle().SetOn(cfg_.show_drag_handle);
        header_row_.SetValueText(AsString(cfg_.header_height) + "px");
        gap_row_.SetValueText(AsString(cfg_.item_spacing) + "px");
        chevron_size_row_.SetValueText(AsString(cfg_.chevron_size) + "px");
        chevron_gap_row_.SetValueText(AsString(cfg_.chevron_gap) + "px");
        radius_row_.SetValueText(AsString(cfg_.radius) + "px");

        SetUsageCode(BuildUsageCode());
        RefreshState();
        Preview().Refresh();
    }

    String BuildUsageCode() const
    {
        String code;
        code << "UiAccordion acc;\n";
        code << "UiAccordion::Style style = UiAccordion::StyleDefault();\n";
        code << "style.single_open = " << (cfg_.single_open ? "true" : "false") << ";\n";
        code << "style.enforce_one = " << (cfg_.enforce_one ? "true" : "false") << ";\n";
        code << "style.show_chevron = " << (cfg_.show_chevron ? "true" : "false") << ";\n";
        code << "style.chevron_side = UiAlign::" << (cfg_.chevron_side == UiAlign::RIGHT ? "RIGHT" : "LEFT") << ";\n";
        code << "style.header_height = " << cfg_.header_height << ";\n";
        code << "style.item_spacing = " << cfg_.item_spacing << ";\n";
        code << "style.chevron_scale = true;\n";
        code << "style.chevron_size = " << cfg_.chevron_size << ";\n";
        code << "style.chevron_gap = " << cfg_.chevron_gap << ";\n";
        code << "style.animation_enabled = " << (cfg_.animation ? "true" : "false") << ";\n";
        code << "style.show_drag_handle = " << (cfg_.show_drag_handle ? "true" : "false") << ";\n";
        code << "style.drag_side = UiAlign::" << (cfg_.drag_side == UiAlign::RIGHT ? "RIGHT" : "LEFT") << ";\n";
        if(!cfg_.drag_glyph.IsEmpty())
            code << "style.drag_glyph = UiIconFromName(\"" << cfg_.drag_glyph << "\");\n";
        code << "acc.SetCustomStyle(style)\n";
        code << "   .EnableDragReorder(" << (cfg_.drag_reorder ? "true" : "false") << ")\n";
        code << "   .ShowDragHandle(" << (cfg_.show_drag_handle ? "true" : "false") << ")\n";
        code << "   .SetDragSide(UiAlign::" << (cfg_.drag_side == UiAlign::RIGHT ? "RIGHT" : "LEFT") << ");\n";
        return code;
    }

    AccordionConfig cfg_;
    UiAccordion acc_;
    UiLabel section_a_, section_b_, section_c_;

    UiBoxLayout state_theme_row_ { UiBoxLayout::Direction::H }, state_open_row_ { UiBoxLayout::Direction::H }, state_chevron_row_ { UiBoxLayout::Direction::H }, state_drag_row_ { UiBoxLayout::Direction::H };
    UiLabel state_theme_label_, state_theme_value_, state_open_label_, state_open_value_, state_chevron_label_, state_chevron_value_, state_drag_label_, state_drag_value_;

    UiCompositeSlider header_row_, gap_row_, chevron_size_row_, chevron_gap_row_, radius_row_;
    UiBoxLayout side_row_box_ { UiBoxLayout::Direction::H }, drag_side_row_box_ { UiBoxLayout::Direction::H }, drag_glyph_row_box_ { UiBoxLayout::Direction::H }, action_row_ { UiBoxLayout::Direction::H };
    UiLabel side_label_, drag_side_label_, drag_glyph_label_;
    UiDropdown side_drop_, drag_side_drop_, drag_glyph_drop_;
    UiCompositeToggle single_row_, enforce_row_, chevron_row_, animation_row_, drag_row_, drag_handle_row_;
    UiButton open_all_btn_, close_all_btn_;
};

}

GUI_APP_MAIN
{
    UiAccordionBuilder demo;
    demo.Run();
}

