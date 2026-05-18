#include <Ui/UiPasswordEdit.h>
#include <Ui/UiTheme.h>

namespace Upp {

// ============================================================================
//  UiPasswordEdit
// ============================================================================

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

UiPasswordEdit::UiPasswordEdit()
{
    // UiLineEdit already enforces single-line semantics.
    // Here we only configure the password mask behaviour and the eye button
    // that lives on the right flank.

    // Configure the eye button look:
    //
    //  - Icon only
    //  - Transparent / frameless
    //  - No click-focus (so focus stays in the edit while you click the eye)
    //
    UiButton::Style bs = UiTheme::ResolveButton(UiButtonRole::Icon);

    // Strip frame/face so it looks like a pure glyph inside the edit.
    bs.metrics.frame_width   = 0;
    bs.metrics.frame_enabled = false;
    bs.metrics.face_enabled  = false;

    // Keep a small inset around the glyph inside the edit chrome.
    bs.metrics.content_margin = Rect(DPI(2), DPI(2), DPI(4), DPI(2));
    bs.content_gap = 0;

    // Transparent background so the edit's face shows through.
    bs.transparent = true;

    eye_button_.SetCustomStyle(bs);
    eye_button_.SetText(String());   // icon-only
    eye_button_.SetIcon(Image());     // start with null icon
    eye_button_.ClickFocus(false);    // don't steal focus from the edit
    eye_button_.SetMinSize(Size(DPI(18), DPI(18)));

    // Behaviour: clicking the eye toggles plain-text visibility.
    eye_button_.WhenAction = [this] {
        SetPlainTextVisible(!plain_visible_);
    };

    SyncEyeButtonIconColor_();

    // Icon is opt-in; user calls EnableVisibilityIcon(true).
    eye_button_.Hide();
}

// ---------------------------------------------------------------------------
// Password visibility toggle
// ---------------------------------------------------------------------------

UiPasswordEdit& UiPasswordEdit::SetPlainTextVisible(bool b)
{
    if(plain_visible_ == b)
        return *this;

    plain_visible_ = b;
    InvalidateTextMetricsCache();
    SyncSb();
    Refresh();

    // If eye icon is enabled, keep icon in sync with the new state.
    if(visibility_icon_enabled_ && eye_flank_id_ >= 0) {
        Image icon = plain_visible_ ? visible_icon_ : hidden_icon_;
        if(!IsNull(icon))
            eye_button_.SetIcon(icon);
    }

    if(WhenToggleVisible)
        WhenToggleVisible(plain_visible_);

    return *this;
}

// ---------------------------------------------------------------------------
// Eye icon API (flanks)
// ---------------------------------------------------------------------------

UiPasswordEdit& UiPasswordEdit::EnableVisibilityIcon(bool on)
{
    // If state and flank presence match the request, just show/hide.
    if(visibility_icon_enabled_ == on && eye_flank_id_ >= 0) {
        if(on)
            eye_button_.Show();
        else
            eye_button_.Hide();
        return *this;
    }

    visibility_icon_enabled_ = on;

    if(on) {
        SyncEyeButtonIconColor_();

        // Ensure we have icons ??? use built-in defaults if none supplied.
        if(IsNull(visible_icon_))
            visible_icon_ = EyeVisibleIcon();
        if(IsNull(hidden_icon_))
            hidden_icon_ = EyeHiddenIcon();

        // Reflect current visibility state in the icon.
        eye_button_.SetIcon(plain_visible_ ? visible_icon_ : hidden_icon_);

        // Lazily add the eye button to the right flank the first time.
        if(eye_flank_id_ < 0) {
            SideHandle sh = AddToSide(eye_button_, UiAlign::RIGHT, Size(DPI(26), 0), UiDirection::H);
            eye_flank_id_ = sh.GetId();
        }

        eye_button_.Show();
    }
    else {
        // Turn off icon by simply hiding the button. The flank remains
        // registered so we can re-enable cheaply.
        if(eye_flank_id_ >= 0)
            eye_button_.Hide();
    }

    return *this;
}

UiPasswordEdit& UiPasswordEdit::SetVisibilityIcons(const Image& visible,
                                                   const Image& hidden)
{
    visible_icon_ = visible;
    hidden_icon_  = hidden;

    // Immediately update the displayed icon if enabled and added.
    if(visibility_icon_enabled_ && eye_flank_id_ >= 0) {
        Image icon = plain_visible_ ? visible_icon_ : hidden_icon_;
        if(!IsNull(icon))
            eye_button_.SetIcon(icon);
    }

    return *this;
}

UiPasswordEdit& UiPasswordEdit::SetCustomStyle(const UiBaseEdit::Style& s)
{
    UiLineEdit::SetCustomStyle(s);
    SyncEyeButtonIconColor_();
    return *this;
}

void UiPasswordEdit::SyncEyeButtonIconColor_()
{
    UiButton::Style bs = eye_button_.GetStyle();
    for(int i = 0; i < 4; i++) {
        Color c = UiResolveIconColor(GetEffectiveStyle().palette, (StyledState)i);
        bs.palette.icon[i] = c;
        bs.palette.ink[i] = c;
    }
    eye_button_.SetCustomStyle(bs);
}

// Built-in eye icons (48px) ??? wrappers over UiIcons helpers.

Image UiPasswordEdit::EyeVisibleIcon()
{
    return ICON_ACTION_OUTLINED_VISIBILITY_48();
}

Image UiPasswordEdit::EyeHiddenIcon()
{
    return ICON_ACTION_OUTLINED_VISIBILITY_OFF_48();
}

// ---------------------------------------------------------------------------
// UiBaseEdit Hook: Display Line
// ---------------------------------------------------------------------------

WString UiPasswordEdit::GetDisplayLine(int i) const
{
    // When plain text is visible, delegate to the base implementation.
    if(plain_visible_)
        return UiBaseEdit::GetDisplayLine(i);

    // Mask per-line text so multi-line input (if ever present) is still
    // rendered safely; UiLineEdit is logically single-line, but this keeps
    // the implementation robust.
    WString raw = UiBaseEdit::GetDisplayLine(i);
    if(raw.IsEmpty())
        return raw;

    return WString(password_char_, raw.GetCount());
}

} // namespace Upp
