#include <Ui/UiPasswordEdit.h>
#include <Ui/UiTheme.h>

namespace Upp {

UiPasswordEdit::UiPasswordEdit()
{
    // UiLineEdit already enforces single-line semantics. The password control
    // only adds masked display and the optional visibility flank.
    UiButton::Style bs = UiTheme::ResolveButton(UiButtonRole::Icon);
    bs.metrics.frame_width = 0;
    bs.metrics.frame_enabled = false;
    bs.metrics.face_enabled = false;
    bs.metrics.content_margin = Rect(DPI(2), DPI(2), DPI(4), DPI(2));
    bs.content_gap = 0;
    bs.transparent = true;

    eye_button_.SetCustomStyle(bs);
    eye_button_.SetText(String());
    eye_button_.SetIcon(Image());
    eye_button_.ClickFocus(false);
    eye_button_.SetMinSize(Size(DPI(18), DPI(18)));
    eye_button_.WhenAction = [this] {
        SetPlainTextVisible(!plain_visible_);
    };

    SyncEyeButtonIconColor_();
    eye_button_.Hide();
}

UiPasswordEdit& UiPasswordEdit::SetPlainTextVisible(bool b)
{
    if(plain_visible_ == b)
        return *this;

    plain_visible_ = b;
    InvalidateTextMetricsCache();
    SyncSb();
    Refresh();

    if(visibility_icon_enabled_ && eye_flank_id_ >= 0) {
        Image icon = plain_visible_ ? visible_icon_ : hidden_icon_;
        if(!IsNull(icon))
            eye_button_.SetIcon(icon);
    }

    if(WhenToggleVisible)
        WhenToggleVisible(plain_visible_);

    return *this;
}

UiPasswordEdit& UiPasswordEdit::EnableVisibilityIcon(bool on)
{
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

        // Use built-in visibility glyphs until the caller supplies its own.
        if(IsNull(visible_icon_))
            visible_icon_ = EyeVisibleIcon();
        if(IsNull(hidden_icon_))
            hidden_icon_ = EyeHiddenIcon();

        eye_button_.SetIcon(plain_visible_ ? visible_icon_ : hidden_icon_);

        if(eye_flank_id_ < 0) {
            SideHandle sh = AddToSide(eye_button_, UiAlign::RIGHT,
                                      Size(DPI(26), 0), UiDirection::H);
            eye_flank_id_ = sh.GetId();
        }

        eye_button_.Show();
    }
    else if(eye_flank_id_ >= 0)
        eye_button_.Hide();

    return *this;
}

UiPasswordEdit& UiPasswordEdit::SetVisibilityIcons(const Image& visible,
                                                   const Image& hidden)
{
    visible_icon_ = visible;
    hidden_icon_ = hidden;

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

Image UiPasswordEdit::EyeVisibleIcon()
{
    return ICON_ACTION_OUTLINED_VISIBILITY_48();
}

Image UiPasswordEdit::EyeHiddenIcon()
{
    return ICON_ACTION_OUTLINED_VISIBILITY_OFF_48();
}

WString UiPasswordEdit::GetDisplayLine(int i) const
{
    if(plain_visible_)
        return UiBaseEdit::GetDisplayLine(i);

    WString raw = UiBaseEdit::GetDisplayLine(i);
    if(raw.IsEmpty())
        return raw;

    return WString(password_char_, raw.GetCount());
}

} // namespace Upp
