#include <Ui/UiToolButton.h>
#include <Ui/UiTheme.h>

namespace Upp {

const UiToolButton::Style& UiToolButton::StyleDefault()
{
    static UiToolButton::Style s;
    ONCELOCK {
        const Color text_primary   = Color(48, 57, 71);
        const Color text_muted     = Color(107, 114, 128);
        const Color face_hot       = Color(242, 244, 247);
        const Color face_pressed   = Color(232, 236, 241);
        const Color accent         = Color(112, 122, 138);

        for(int i = 0; i < 4; i++) {
            s.palette.face[i] = UiFill::None();
            s.palette.frame[i] = Null;
            s.palette.ink[i] = text_primary;
            s.palette.icon[i] = Null;
        }

        s.palette.face[ST_HOT]      = UiFill::Solid(face_hot);
        s.palette.face[ST_PRESSED]  = UiFill::Solid(face_pressed);
        s.palette.face[ST_DISABLED] = UiFill::None();

        s.palette.frame[ST_HOT]      = Null;
        s.palette.frame[ST_PRESSED]  = Null;
        s.palette.frame[ST_DISABLED] = Null;

        s.palette.ink[ST_HOT]      = text_primary;
        s.palette.ink[ST_PRESSED]  = text_primary;
        s.palette.ink[ST_DISABLED] = text_muted;

        s.metrics.text_font = StdFont();
        s.metrics.use_text_font = false;
        s.metrics.content_margin = Rect(DPI(8), DPI(8), DPI(8), DPI(8));
        s.metrics.radius = DPI(4);
        s.metrics.frame_width = DPI(1);
        s.metrics.frame_enabled = false;
        s.metrics.face_enabled = true;
        s.metrics.dashed = false;
        s.metrics.high_contrast = false;
        s.metrics.shadow = StyledShadow();
        s.metrics.highlight = StyledHighlight();

        s.skin = StyledSkin();

        s.press_offset = Point(0, 0);
        s.metrics.focus_margin = DPI(2);
        s.overpaint = 0;
        s.font = StdFont();
        s.transparent = false;

        s.align_h = UiAlign::CENTER;
        s.align_v = UiAlign::CENTER;
        s.icon_side = UiAlign::CENTER;
        s.content_gap = 0;
        s.icon_render_mode = UiIconRenderMode::MonoTint;

        for(int i = 0; i < 4; i++)
            s.icon_images[i] = Image();

        s.underline = false;
        s.underline_width = DPI(1);
        s.underline_offset = DPI(2);

        s.palette.ink[ST_DISABLED] = Blend(text_muted, SColorPaper(), 40);
        s.palette.icon[ST_NORMAL] = accent;
        s.palette.icon[ST_HOT] = accent;
        s.palette.icon[ST_PRESSED] = DkColor(accent, 10);
        s.palette.icon[ST_DISABLED] = Blend(accent, SColorPaper(), 180);
    }
    return s;
}

UiToolButton::UiToolButton()
{
    user_min_size_ = Size(DPI(28), DPI(28));
    SyncThemeStyle();
    RebuildTextLinesFromStyle(GetEffectiveStyle());
    minsize_dirty_ = true;
    layout_dirty_ = true;
    layout_content_ = Rect(0, 0, 0, 0);
}

UiToolButton::Style UiToolButton::ResolveThemeStyle() const
{
    return UiTheme::ResolveToolButton();
}

String UiToolButton::GetDesc() const
{
    if(!GetText().IsEmpty())
        return GetText();
    return t_("Tool button");
}

} // namespace Upp
