/*
    UiTheme public facade

    The historical resolver implementation remains in UiThemeLegacy.h. It is
    compiled as UiThemeRaw; this facade preserves the public UiTheme API while
    enforcing one ownership boundary: a theme may style a control, but it may
    not create/remove/move structural elements or change their authored mode.

    Non-default structural choices are applied by the control/designer after
    theme resolution. This keeps theme changes visual and predictable.
*/
#ifndef _Ui_UiThemeFacade_h_
#define _Ui_UiThemeFacade_h_

#define UiTheme UiThemeRaw
#define UiThemeDefaults UiThemeRawDefaults
#include <Ui/UiThemeLegacy.h>
#undef UiThemeDefaults
#undef UiTheme

#include <Ui/UiThemeStructureContract.h>

namespace Upp {

class UiTheme : public UiThemeRaw {
public:
    static UiButton::Style ResolveButton(UiRole role)
    {
        return UiThemeStructureContract::Preserved(UiThemeRaw::ResolveButton(role));
    }
    static UiButton::Style ResolveButton(UiButtonRole role = UiButtonRole::Standard)
    {
        return UiThemeStructureContract::Preserved(UiThemeRaw::ResolveButton(role));
    }
    static UiButton::Style ResolveButton(const UiThemeContext& ctx, UiRole role)
    {
        return UiThemeStructureContract::Preserved(UiThemeRaw::ResolveButton(ctx, role));
    }
    static UiButton::Style ResolveButton(const UiThemeContext& ctx,
                                         UiButtonRole role = UiButtonRole::Standard)
    {
        return UiThemeStructureContract::Preserved(UiThemeRaw::ResolveButton(ctx, role));
    }
    static UiButton::Style ResolveButton(UiThemePreset preset, UiThemeMode mode,
                                         UiButtonRole role = UiButtonRole::Standard)
    {
        return UiThemeStructureContract::Preserved(UiThemeRaw::ResolveButton(preset, mode, role));
    }

    static UiToolButton::Style ResolveToolButton(UiRole role)
    {
        return UiThemeStructureContract::ToolButtonPreserved(UiThemeRaw::ResolveToolButton(role));
    }
    static UiToolButton::Style ResolveToolButton(
        UiToolButtonRole role = UiToolButtonRole::Standard)
    {
        return UiThemeStructureContract::ToolButtonPreserved(UiThemeRaw::ResolveToolButton(role));
    }
    static UiToolButton::Style ResolveToolButton(const UiThemeContext& ctx,
                                                 UiRole role)
    {
        return UiThemeStructureContract::ToolButtonPreserved(UiThemeRaw::ResolveToolButton(ctx, role));
    }
    static UiToolButton::Style ResolveToolButton(
        const UiThemeContext& ctx,
        UiToolButtonRole role = UiToolButtonRole::Standard)
    {
        return UiThemeStructureContract::ToolButtonPreserved(UiThemeRaw::ResolveToolButton(ctx, role));
    }
    static UiToolButton::Style ResolveToolButton(
        UiThemePreset preset, UiThemeMode mode,
        UiToolButtonRole role = UiToolButtonRole::Standard)
    {
        return UiThemeStructureContract::ToolButtonPreserved(UiThemeRaw::ResolveToolButton(preset, mode, role));
    }
    static UiToolButton::Style ResolveToolButton(UiThemePreset preset,
                                                 UiThemeMode mode,
                                                 UiRole role)
    {
        return UiThemeStructureContract::ToolButtonPreserved(UiThemeRaw::ResolveToolButton(preset, mode, role));
    }

    static UiToggle::Style ResolveToggle(UiRole role)
    {
        return UiThemeStructureContract::Preserved(UiThemeRaw::ResolveToggle(role));
    }
    static UiToggle::Style ResolveToggle()
    {
        return UiThemeStructureContract::Preserved(UiThemeRaw::ResolveToggle());
    }
    static UiToggle::Style ResolveToggle(const UiThemeContext& ctx)
    {
        return UiThemeStructureContract::Preserved(UiThemeRaw::ResolveToggle(ctx));
    }
    static UiToggle::Style ResolveToggle(const UiThemeContext& ctx, UiRole role)
    {
        return UiThemeStructureContract::Preserved(UiThemeRaw::ResolveToggle(ctx, role));
    }
    static UiToggle::Style ResolveToggle(UiThemePreset preset, UiThemeMode mode)
    {
        return UiThemeStructureContract::Preserved(UiThemeRaw::ResolveToggle(preset, mode));
    }

    static UiScrollBar::Style ResolveScrollBar()
    {
        return UiThemeStructureContract::Preserved(UiThemeRaw::ResolveScrollBar());
    }
    static UiScrollBar::Style ResolveScrollBar(const UiThemeContext& ctx)
    {
        return UiThemeStructureContract::Preserved(UiThemeRaw::ResolveScrollBar(ctx));
    }
    static UiScrollBar::Style ResolveScrollBar(UiThemePreset preset, UiThemeMode mode)
    {
        return UiThemeStructureContract::Preserved(UiThemeRaw::ResolveScrollBar(preset, mode));
    }

    static UiGroupPanel::Style ResolveGroupPanel(
        const UiThemeContext& ctx, UiRole role = UiRole::Standard)
    {
        return UiThemeStructureContract::Preserved(UiThemeRaw::ResolveGroupPanel(ctx, role));
    }
    static UiGroupPanel::Style ResolveGroupPanel(UiRole role = UiRole::Standard)
    {
        return UiThemeStructureContract::Preserved(UiThemeRaw::ResolveGroupPanel(role));
    }
    static UiGroupPanel::Style ResolveGroupPanel(
        UiThemePreset preset, UiThemeMode mode, UiRole role = UiRole::Standard)
    {
        return UiThemeStructureContract::Preserved(UiThemeRaw::ResolveGroupPanel(preset, mode, role));
    }

    static UiDropdown::Style ResolveDropdown(UiRole role)
    {
        return UiThemeStructureContract::Preserved(UiThemeRaw::ResolveDropdown(role));
    }
    static UiDropdown::Style ResolveDropdown()
    {
        return UiThemeStructureContract::Preserved(UiThemeRaw::ResolveDropdown());
    }
    static UiDropdown::Style ResolveDropdown(const UiThemeContext& ctx, UiRole role)
    {
        return UiThemeStructureContract::Preserved(UiThemeRaw::ResolveDropdown(ctx, role));
    }
    static UiDropdown::Style ResolveDropdown(const UiThemeContext& ctx)
    {
        return UiThemeStructureContract::Preserved(UiThemeRaw::ResolveDropdown(ctx));
    }
    static UiDropdown::Style ResolveDropdown(UiThemePreset preset, UiThemeMode mode)
    {
        return UiThemeStructureContract::Preserved(UiThemeRaw::ResolveDropdown(preset, mode));
    }
    static UiDropdown::Style ResolveDropdown(UiThemePreset preset, UiThemeMode mode,
                                             UiRole role)
    {
        return UiThemeStructureContract::Preserved(UiThemeRaw::ResolveDropdown(preset, mode, role));
    }

    static UiTab::Style ResolveTab(UiRole role,
                                   UiTabVisual visual = UITAB_CLASSIC)
    {
        return UiThemeStructureContract::Preserved(UiThemeRaw::ResolveTab(role, visual), visual);
    }
    static UiTab::Style ResolveTab(UiTabVisual visual = UITAB_CLASSIC)
    {
        return UiThemeStructureContract::Preserved(UiThemeRaw::ResolveTab(visual), visual);
    }
    static UiTab::Style ResolveTab(const UiThemeContext& ctx, UiRole role,
                                   UiTabVisual visual = UITAB_CLASSIC)
    {
        return UiThemeStructureContract::Preserved(UiThemeRaw::ResolveTab(ctx, role, visual), visual);
    }
    static UiTab::Style ResolveTab(const UiThemeContext& ctx,
                                   UiTabVisual visual = UITAB_CLASSIC)
    {
        return UiThemeStructureContract::Preserved(UiThemeRaw::ResolveTab(ctx, visual), visual);
    }
    static UiTab::Style ResolveTab(UiThemePreset preset, UiThemeMode mode,
                                   UiTabVisual visual = UITAB_CLASSIC)
    {
        return UiThemeStructureContract::Preserved(UiThemeRaw::ResolveTab(preset, mode, visual), visual);
    }
    static UiTab::Style ResolveTab(UiThemePreset preset, UiThemeMode mode,
                                   UiRole role,
                                   UiTabVisual visual = UITAB_CLASSIC)
    {
        return UiThemeStructureContract::Preserved(UiThemeRaw::ResolveTab(preset, mode, role, visual), visual);
    }

    static UiTitleCard::Style ResolveTitleCard(UiRole role)
    {
        return UiThemeStructureContract::Preserved(UiThemeRaw::ResolveTitleCard(role));
    }
    static UiTitleCard::Style ResolveTitleCard()
    {
        return UiThemeStructureContract::Preserved(UiThemeRaw::ResolveTitleCard());
    }
    static UiTitleCard::Style ResolveTitleCard(const UiThemeContext& ctx)
    {
        return UiThemeStructureContract::Preserved(UiThemeRaw::ResolveTitleCard(ctx));
    }
    static UiTitleCard::Style ResolveTitleCard(const UiThemeContext& ctx,
                                               UiRole role)
    {
        return UiThemeStructureContract::Preserved(UiThemeRaw::ResolveTitleCard(ctx, role));
    }
    static UiTitleCard::Style ResolveTitleCard(UiThemePreset preset,
                                               UiThemeMode mode)
    {
        return UiThemeStructureContract::Preserved(UiThemeRaw::ResolveTitleCard(preset, mode));
    }

    static UiList::Style ResolveList()
    {
        return UiThemeStructureContract::Preserved(UiThemeRaw::ResolveList());
    }
    static UiList::Style ResolveList(UiRole role)
    {
        return UiThemeStructureContract::Preserved(UiThemeRaw::ResolveList(role));
    }
    static UiList::Style ResolveList(const UiThemeContext& ctx)
    {
        return UiThemeStructureContract::Preserved(UiThemeRaw::ResolveList(ctx));
    }
    static UiList::Style ResolveList(const UiThemeContext& ctx, UiRole role)
    {
        return UiThemeStructureContract::Preserved(UiThemeRaw::ResolveList(ctx, role));
    }
    static UiList::Style ResolveList(UiThemePreset preset, UiThemeMode mode)
    {
        return UiThemeStructureContract::Preserved(UiThemeRaw::ResolveList(preset, mode));
    }
};

namespace UiThemeDefaults {
inline UiButton::Style MakeButton(UiThemePreset preset, UiThemeMode mode,
                                  UiButtonRole role = UiButtonRole::Standard)
{ return UiTheme::ResolveButton(preset, mode, role); }
inline UiToolButton::Style MakeToolButton(
    UiThemePreset preset, UiThemeMode mode,
    UiToolButtonRole role = UiToolButtonRole::Standard)
{ return UiTheme::ResolveToolButton(preset, mode, role); }
inline UiBaseEdit::Style MakeEdit(UiThemePreset preset, UiThemeMode mode,
                                  UiEditRole role = UiEditRole::Field)
{ return UiTheme::ResolveEdit(preset, mode, role); }
inline UiCheckBox::Style MakeCheckBox(
    UiThemePreset preset, UiThemeMode mode,
    UiCheckVisual visual = UICHECKVIS_CLASSIC)
{ return UiTheme::ResolveCheckBox(preset, mode, visual); }
inline UiToggle::Style MakeToggle(UiThemePreset preset, UiThemeMode mode)
{ return UiTheme::ResolveToggle(preset, mode); }
inline UiRadioButton::Style MakeRadioButton(
    UiThemePreset preset, UiThemeMode mode,
    UiRadioVisual visual = UIRADIOVIS_CLASSIC)
{ return UiTheme::ResolveRadioButton(preset, mode, visual); }
inline UiProgressBar::Style MakeProgressBar(
    UiThemePreset preset, UiThemeMode mode, UiRole role = UiRole::Standard)
{ return UiTheme::ResolveProgressBar(preset, mode, role); }
inline UiSlider::Style MakeSlider(UiThemePreset preset, UiThemeMode mode)
{ return UiTheme::ResolveSlider(preset, mode); }
inline UiScrollBar::Style MakeScrollBar(UiThemePreset preset, UiThemeMode mode)
{ return UiTheme::ResolveScrollBar(preset, mode); }
inline UiSplitter::Style MakeSplitter(UiThemePreset preset, UiThemeMode mode)
{ return UiTheme::ResolveSplitter(preset, mode); }
inline UiPanel::Style MakePanel(
    UiThemePreset preset, UiThemeMode mode,
    UiPanelRole role = UiPanelRole::Surface)
{ return UiTheme::ResolvePanel(preset, mode, role); }
inline UiGroupPanel::Style MakeGroupPanel(
    UiThemePreset preset, UiThemeMode mode, UiRole role = UiRole::Standard)
{ return UiTheme::ResolveGroupPanel(preset, mode, role); }
inline UiDropdown::Style MakeDropdown(UiThemePreset preset, UiThemeMode mode)
{ return UiTheme::ResolveDropdown(preset, mode); }
inline UiTab::Style MakeTab(UiThemePreset preset, UiThemeMode mode,
                            UiTabVisual visual = UITAB_CLASSIC)
{ return UiTheme::ResolveTab(preset, mode, visual); }
inline UiTitleCard::Style MakeTitleCard(UiThemePreset preset, UiThemeMode mode)
{ return UiTheme::ResolveTitleCard(preset, mode); }
inline UiTree::Style MakeTree(UiThemePreset preset, UiThemeMode mode)
{ return UiTheme::ResolveTree(preset, mode); }
inline UiMenu::Style MakeMenu(UiThemePreset preset, UiThemeMode mode)
{ return UiTheme::ResolveMenu(preset, mode); }
inline UiList::Style MakeList(UiThemePreset preset, UiThemeMode mode)
{ return UiTheme::ResolveList(preset, mode); }
inline UiLabel::Style MakeLabel(
    UiThemePreset preset, UiThemeMode mode,
    UiLabelRole role = UiLabelRole::Body)
{ return UiTheme::ResolveLabel(preset, mode, role); }
}

} // namespace Upp

#endif
