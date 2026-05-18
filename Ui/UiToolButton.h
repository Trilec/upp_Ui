#ifndef _Ui_UiToolButton_h_
#define _Ui_UiToolButton_h_

/*
    UiToolButton
    ============

    Purpose
    - Tool-button specialization for icon-heavy command surfaces that should
      style independently from regular UiButton instances.

    Intent
    - Reuse the shared UiButton behavior pipeline while keeping a separate
      public control type, separate theme resolver, and tighter default metrics.

    Thread context
    - GUI thread only.

    Usage
    - Configure exactly like UiButton, but prefer UiTheme::ResolveToolButton()
      for default styling.
    - Use when the control belongs to toolbar, chrome, or utility-button
      surfaces rather than general command buttons.

    Changelog
    - 2026-03: reduced duplicated implementation by specializing UiButton
      instead of carrying a second full button copy.
    - 2026-03: kept fluent wrapper forwards intentionally so inherited
      mutators continue to chain as UiToolButton& while behavior stays shared.
    - 2026-04: aligned icon rendering configuration with UiButton via the
      shared UiIconRenderMode contract.
    - 2026-04: aligned inherited spacing vocabulary with UiButton around
      content_margin, content_gap, and icon_side.
*/

#include <Ui/UiButton.h>

namespace Upp {

class UiToolButton : public UiButton {
public:
    typedef UiToolButton CLASSNAME;
    using Style = UiButton::Style;

    UiToolButton();

    // These forwards are intentionally narrow wrappers around UiButton's
    // implementation so fluent chains stay on UiToolButton.
    UiToolButton& SetText(const String& text) { UiButton::SetText(text); return *this; }
    UiToolButton& SetIcon(const Image& img) { UiButton::SetIcon(img); return *this; }
    UiToolButton& SetIconState(const Image& img, StyledState state) { UiButton::SetIconState(img, state); return *this; }
    UiToolButton& SetIcons(const Image& normal,
                           const Image& hot      = Image(),
                           const Image& pressed  = Image(),
                           const Image& disabled = Image())
    {
        UiButton::SetIcons(normal, hot, pressed, disabled);
        return *this;
    }
    UiToolButton& ClearIcon() { UiButton::ClearIcon(); return *this; }
    UiToolButton& SetIconRenderMode(UiIconRenderMode mode) { UiButton::SetIconRenderMode(mode); return *this; }
    UiToolButton& SetIconColor(Color base, int hot_pct = 0, int press_pct = 0) { UiButton::SetIconColor(base, hot_pct, press_pct); return *this; }
    UiToolButton& SetIconSide(UiAlign side) { UiButton::SetIconSide(side); return *this; }
    UiToolButton& SetAlign(UiAlign h, UiAlign v) { UiButton::SetAlign(h, v); return *this; }
    UiToolButton& SetAlignH(UiAlign h) { UiButton::SetAlignH(h); return *this; }
    UiToolButton& SetAlignV(UiAlign v) { UiButton::SetAlignV(v); return *this; }
    UiToolButton& SetContentGap(int gap) { UiButton::SetContentGap(gap); return *this; }
    UiToolButton& ClickFocus(bool on = true) { UiButton::ClickFocus(on); return *this; }
    UiToolButton& SetCheckable(bool on = true) { UiButton::SetCheckable(on); return *this; }
    UiToolButton& SetChecked(bool on = true) { UiButton::SetChecked(on); return *this; }
    UiToolButton& Toggle() { UiButton::Toggle(); return *this; }
    UiToolButton& SetCustomStyle(const Style& s) { UiButton::SetCustomStyle(s); return *this; }
    UiToolButton& ClearCustomStyle() { UiButton::ClearCustomStyle(); return *this; }
    UiToolButton& SetUnderline(bool on = true, int thickness = DPI(1), int offset = 0) { UiButton::SetUnderline(on, thickness, offset); return *this; }

    static const Style& StyleDefault();

protected:
    virtual Style ResolveThemeStyle() const override;

public:
    virtual String GetDesc() const override;
};

} // namespace Upp

#endif
