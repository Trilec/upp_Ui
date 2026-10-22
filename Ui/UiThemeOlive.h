/*
    Olive token and recipe authority, included by UiThemeResolverImpl.h after
    the public theme enums and concrete style types. Do not include directly.

    Opt-in, application-independent Light/Dark preset. Sizes are logical CSS
    pixels converted once by DPI. Geometry belonging to an authored control
    (tab visual, range direction/card layout, ports, child structure) stays
    with that control. No application profile or native host-theme bridge.
*/
#ifndef _Ui_UiThemeOlive_h_
#define _Ui_UiThemeOlive_h_

namespace Upp {

struct UiOliveTokens {
    Color background, rail, panel, panel2, panel3, field;
    Color line, line_soft, text, muted, subtle, accent, accent_ink, alert, focus;
    Color button, hover, active, alert_face, disabled_face, disabled_ink;
    Font body, caption, title;
    int radius, border, gap, control_height, row_height, tree_row_height;
};

namespace UiOliveDetail {

inline Font FontPx(int pixels, int weight = 400)
{
    int face = Font::FindFaceNameIndex("Segoe UI");
    if(face <= 0) face = Font::FindFaceNameIndex("Arial");
    if(face <= 0) face = Font::SANSSERIF;
    // Preserve the approved recipe's legacy binary weight mapping. Verified
    // numeric/static font setup is a separate UiFonts API, never invoked here.
    return Font(face, DPI(pixels)).Bold(weight >= 600);
}

inline UiOliveTokens Tokens(UiThemeMode mode)
{
    const bool dark = mode == UiThemeMode::Dark;
    UiOliveTokens t;
    t.background = dark ? Color(23,26,30) : Color(241,243,241);
    t.rail = dark ? Color(25,28,32) : Color(231,235,231);
    t.panel = dark ? Color(32,36,41) : Color(250,251,249);
    t.panel2 = dark ? Color(41,46,52) : Color(233,238,232);
    t.panel3 = dark ? Color(16,19,22) : Color(223,229,223);
    t.field = dark ? Color(25,29,33) : Color(240,243,238);
    t.line = dark ? Color(52,58,64) : Color(202,210,200);
    t.line_soft = dark ? Color(43,48,54) : Color(225,230,222);
    t.text = dark ? Color(232,236,235) : Color(36,48,43);
    t.muted = dark ? Color(163,174,169) : Color(94,111,100);
    t.subtle = dark ? Color(121,135,129) : Color(113,130,118);
    t.accent = dark ? Color(181,215,165) : Color(69,109,58);
    t.accent_ink = dark ? Color(23,38,27) : White();
    t.alert = dark ? Color(231,186,126) : Color(139,89,35);
    t.focus = dark ? Color(201,230,188) : Color(69,109,58);
    t.button = t.panel2; t.hover = t.line;
    // Solid PANEL-host approximations of the approved translucent washes.
    t.active = Blend(t.panel, t.accent, dark ? 20 : 16);
    t.alert_face = Blend(t.panel, t.alert, dark ? 18 : 16);
    t.disabled_face = Blend(t.panel, t.button, 115);
    t.disabled_ink = Blend(t.panel, t.text, 115);
    t.body = FontPx(13); t.caption = FontPx(10); t.title = FontPx(23,600);
    t.radius = DPI(7); t.border = DPI(1); t.gap = DPI(8);
    t.control_height = DPI(36); t.row_height = DPI(33); t.tree_row_height = DPI(30);
    return t;
}

inline Color Brightness(Color c, double factor)
{
    return Color(clamp(int(c.GetR()*factor+.5),0,255),
                 clamp(int(c.GetG()*factor+.5),0,255),
                 clamp(int(c.GetB()*factor+.5),0,255));
}

inline Color Ink(const UiOliveTokens& t, UiRole role)
{
    return role == UiRole::Subtle ? t.muted : role == UiRole::Accent ? t.accent :
           role == UiRole::Alert ? t.alert : t.text;
}

inline Color Edge(const UiOliveTokens& t, UiRole role)
{
    return role == UiRole::Accent ? t.accent : role == UiRole::Alert ? t.alert : t.line;
}

inline void Metrics(StyledMetrics& m, const UiOliveTokens& t, bool field = false)
{
    m.radius = DPI(field ? 6 : 7); m.frame_width = t.border;
    m.face_enabled = m.frame_enabled = m.focus_enabled = true;
    // Fields must fit both the 36px default and compact 30px layouts.
    m.content_margin = Rect(DPI(field ? 10 : 12),DPI(field ? 3 : 8),DPI(field ? 10 : 12),DPI(field ? 3 : 8));
    m.focus_color = t.focus; m.focus_margin = DPI(field ? 0 : 3); m.focus_alpha = 255;
    m.shadow.enabled = m.highlight.enabled = false;
    m.text_font = t.body; m.use_text_font = false;
}

inline void Palette(StyledPalette& p, const UiOliveTokens& t, UiRole role,
                    Color face, bool selected = false)
{
    Color ink = t.text, edge = Null;
    if(role == UiRole::Subtle) { ink = t.muted; edge = t.line; }
    if(role == UiRole::Accent) { face = t.accent; ink = t.accent_ink; }
    if(role == UiRole::Alert) { face = t.alert_face; ink = edge = t.alert; }
    if(selected) { face = t.active; ink = edge = t.accent; }
    for(int i = 0; i < 4; ++i) {
        p.face[i] = role == UiRole::Subtle && !selected ? UiFill::None() : UiFill::Solid(face);
        p.frame[i] = edge; p.ink[i] = p.icon[i] = ink;
    }
    p.face[ST_HOT] = p.face[ST_PRESSED] = selected ? face :
        role == UiRole::Accent ? Brightness(t.accent,1.06) : t.hover;
    if(role == UiRole::Accent && !selected)
        p.ink[ST_HOT] = p.icon[ST_HOT] =
        p.ink[ST_PRESSED] = p.icon[ST_PRESSED] = Brightness(ink,1.06);
    p.face[ST_DISABLED] = role == UiRole::Subtle && !selected ? UiFill::None() :
        UiFill::Solid(Blend(t.panel,face,115));
    p.frame[ST_DISABLED] = IsNull(edge) ? Color(Null) : Blend(t.panel,edge,115);
    p.ink[ST_DISABLED] = p.icon[ST_DISABLED] = Blend(t.panel,ink,115);
}

template<class S> inline void Base(S& s, const UiOliveTokens& t, UiRole role, Color face)
{
    Palette(s.palette,t,role,face); Metrics(s.metrics,t);
}

inline UiButton::Style Button(UiButton::Style s, const UiOliveTokens& t, UiRole role)
{
    Base(s,t,role,t.button);
    s.font = FontPx(role == UiRole::Subtle ? 12 : 13,role == UiRole::Accent ? 650 : 400);
    s.transparent = false; s.content_gap = DPI(6);
    return UiApplyTypography(s);
}

inline UiToolButton::Style ToolButton(UiToolButton::Style s, const UiOliveTokens& t, UiRole role)
{
    Base(s,t,role,t.panel); s.font = FontPx(11); s.content_gap = DPI(5); s.transparent = false;
    s.metrics.content_margin = Rect(DPI(5),DPI(5),DPI(5),DPI(5));
    for(int i = 0; i < 4; ++i) {
        s.palette.face[i] = UiFill::None(); s.palette.frame[i] = Null;
        Color ink = i == ST_NORMAL ? t.subtle : i == ST_DISABLED ? Blend(t.panel,t.subtle,115) : t.text;
        if(role == UiRole::Accent) ink = i == ST_DISABLED ? Blend(t.panel,t.accent,115) : t.accent;
        if(role == UiRole::Alert) ink = i == ST_DISABLED ? Blend(t.panel,t.alert,115) : t.alert;
        s.palette.ink[i] = s.palette.icon[i] = ink;
    }
    return UiApplyTypography(s);
}

inline UiLabel::Style Label(UiLabel::Style s, const UiOliveTokens& t, UiRole role)
{
    Base(s,t,role,t.panel); s.font = t.body; s.transparent = true;
    s.metrics.face_enabled = s.metrics.frame_enabled = s.metrics.focus_enabled = false;
    s.metrics.content_margin = Rect(0,0,0,0);
    for(int i = 0; i < 4; ++i)
        s.palette.ink[i] = s.palette.icon[i] = i == ST_DISABLED ? t.disabled_ink : Ink(t,role);
    return UiApplyTypography(s);
}

inline UiBaseEdit::Style Edit(UiBaseEdit::Style s, const UiOliveTokens& t, UiRole role)
{
    Base(s,t,UiRole::Standard,t.field); Metrics(s.metrics,t,true); s.font = t.body;
    s.caret_color = t.text; s.selection_color = t.active; s.selection_ink = t.text;
    s.placeholder_ink = t.muted; s.whitespace_color = s.tab_char_color = t.subtle;
    for(int i = 0; i < 4; ++i) {
        s.palette.face[i] = i == ST_DISABLED ? Blend(t.panel,t.field,115) : t.field;
        s.palette.frame[i] = i == ST_DISABLED ? Blend(t.panel,t.line,115) :
            role == UiRole::Alert ? t.alert : t.line;
        s.underline[i] = t.line;
    }
    return UiApplyTypography(s);
}

inline UiDropdown::Style Dropdown(UiDropdown::Style s, const UiOliveTokens& t, UiRole role)
{
    Base(s,t,UiRole::Standard,t.field); Metrics(s.metrics,t,true); s.font = t.body; s.transparent = false;
    for(int i = 0; i < 4; ++i) {
        s.palette.face[i] = i == ST_DISABLED ? Blend(t.panel,t.field,115) : t.field;
        s.palette.frame[i] = i == ST_DISABLED ? Blend(t.panel,t.line,115) : role == UiRole::Alert ? t.alert : t.line;
    }
    s.popup_item_style = Label(s.popup_item_style,t,UiRole::Standard);
    s.popup_item_style.transparent = false; s.popup_item_style.metrics.face_enabled = true;
    Palette(s.popup_item_style.palette,t,UiRole::Standard,t.panel);
    s.popup_item_style.palette.face[ST_PRESSED] = t.active;
    s.popup_item_style.palette.ink[ST_PRESSED] = t.accent;
    s.popup_item_height = t.row_height; s.item_spacing = 0;
    s.popup_frame_width = t.border; s.popup_radius = DPI(6);
    s.popup_frame_color = t.line; s.popup_background_color = t.panel;
    s.selection_badge_face = t.active; s.selection_badge_ink = t.accent;
    s.selection_badge_radius = DPI(4); s.drag_marker = t.focus;
    return UiApplyTypography(s);
}

inline UiCheckBox::Style CheckBox(UiCheckBox::Style s, const UiOliveTokens& t, UiRole role)
{
    Base(s,t,UiRole::Standard,t.panel); s.font = t.body;
    s.metrics.face_enabled = s.metrics.frame_enabled = false;
    Palette(s.indicator_palette,t,UiRole::Standard,t.field); Metrics(s.indicator_metrics,t);
    s.indicator_metrics.radius = DPI(3); s.indicator_metrics.content_margin = Rect(0,0,0,0);
    s.indicator_palette.frame[ST_NORMAL] = role == UiRole::Alert ? t.alert : t.line;
    s.indicator_palette.face[ST_PRESSED] = t.accent; s.indicator_palette.frame[ST_PRESSED] = t.accent;
    s.indicator_palette.ink[ST_PRESSED] = s.indicator_palette.icon[ST_PRESSED] = t.accent_ink;
    return UiApplyTypography(s);
}

inline UiRadioButton::Style RadioButton(UiRadioButton::Style s, const UiOliveTokens& t, UiRole role)
{
    Base(s,t,UiRole::Standard,t.panel); s.font = t.body;
    s.metrics.face_enabled = s.metrics.frame_enabled = false;
    Palette(s.indicator_palette,t,UiRole::Standard,t.field); Metrics(s.indicator_metrics,t);
    s.indicator_metrics.radius = DPI(9); s.indicator_metrics.content_margin = Rect(0,0,0,0);
    s.indicator_palette.frame[ST_NORMAL] = role == UiRole::Alert ? t.alert : t.line;
    s.indicator_palette.face[ST_PRESSED] = t.accent; s.indicator_palette.frame[ST_PRESSED] = t.accent;
    s.indicator_palette.ink[ST_PRESSED] = s.indicator_palette.icon[ST_PRESSED] = t.accent_ink;
    return UiApplyTypography(s);
}

inline UiToggle::Style Toggle(UiToggle::Style s, const UiOliveTokens& t, UiRole role)
{
    Base(s,t,role,t.panel); s.metrics.face_enabled = s.metrics.frame_enabled = false;
    Palette(s.track_palette,t,UiRole::Standard,t.field); Metrics(s.track_metrics,t);
    s.track_metrics.radius = DPI(10);
    s.track_palette.frame[ST_NORMAL] = Edge(t,role);
    s.track_palette.face[ST_PRESSED] = t.active; s.track_palette.frame[ST_PRESSED] = Edge(t,role == UiRole::Alert ? role : UiRole::Accent);
    Palette(s.thumb_palette,t,UiRole::Standard,t.muted); Metrics(s.thumb_metrics,t);
    s.thumb_palette.face[ST_PRESSED] = t.accent;
    s.thumb_metrics.radius = DPI(7); s.thumb_metrics.frame_enabled = false;
    return UiApplyTypography(s);
}

inline UiPanel::Style Panel(UiPanel::Style s, const UiOliveTokens& t, UiRole role)
{
    Base(s,t,UiRole::Standard,t.panel); s.transparent = false; s.metrics.focus_enabled = false;
    for(int i = 0; i < 3; ++i) s.palette.frame[i] = Edge(t,role);
    return UiApplyTypography(s);
}

inline UiGroupPanel::Style GroupPanel(UiGroupPanel::Style s, const UiOliveTokens& t, UiRole role)
{
    Base(s,t,UiRole::Standard,t.panel);
    for(int i = 0; i < 3; ++i) s.palette.frame[i] = Edge(t,role);
    s.title_font = t.body; s.subtitle_font = t.caption;
    s.title_color = Ink(t,role); s.subtitle_color = t.muted;
    return UiApplyTypography(s);
}

inline UiSlider::Style Slider(UiSlider::Style s, const UiOliveTokens& t, UiRole role)
{
    Palette(s.track_palette,t,UiRole::Standard,t.line); Metrics(s.track_metrics,t);
    Palette(s.thumb_palette,t,UiRole::Standard,t.accent); Metrics(s.thumb_metrics,t);
    s.track_metrics.frame_enabled = s.thumb_metrics.frame_enabled = false;
    s.track_metrics.content_margin = s.thumb_metrics.content_margin = Rect(0,0,0,0);
    s.track_metrics.radius = DPI(2); s.thumb_metrics.radius = DPI(8);
    s.tick_color = t.subtle; s.thumb_inner_ring_color = t.panel;
    return UiApplyTypography(s);
}

inline UiProgressBar::Style ProgressBar(UiProgressBar::Style s, const UiOliveTokens& t, UiRole role)
{
    Palette(s.track_palette,t,UiRole::Standard,t.field); Metrics(s.track_metrics,t);
    Palette(s.fill_palette,t,UiRole::Standard,role == UiRole::Alert ? t.alert : t.accent); Metrics(s.fill_metrics,t);
    s.track_metrics.content_margin = s.fill_metrics.content_margin = Rect(0,0,0,0);
    s.track_metrics.frame_enabled = s.fill_metrics.frame_enabled = false;
    s.font = t.caption; s.empty_text = t.muted; s.filled_text = t.accent_ink;
    return UiApplyTypography(s);
}

inline UiScrollBar::Style ScrollBar(UiScrollBar::Style s, const UiOliveTokens& t)
{
    Metrics(s.track_metrics,t); Metrics(s.thumb_metrics,t); Metrics(s.arrow_metrics,t);
    Palette(s.track_palette,t,UiRole::Standard,t.panel);
    Palette(s.thumb_palette,t,UiRole::Standard,t.line);
    Palette(s.arrow_palette,t,UiRole::Standard,t.panel);
    for(int i = 0; i < 4; ++i) {
        s.track_palette.face[i] = UiFill::None();
        s.track_palette.frame[i] = s.thumb_palette.frame[i] = Null;
    }
    s.track_metrics.frame_enabled = s.thumb_metrics.frame_enabled = false;
    s.track_metrics.content_margin = s.thumb_metrics.content_margin = Rect(0,0,0,0);
    s.thumb_metrics.radius = DPI(4); s.thumb_palette.face[ST_HOT] = t.muted;
    s.thumb_palette.face[ST_PRESSED] = t.accent; s.grip_color = t.muted;
    return UiApplyTypography(s);
}

inline UiSplitter::Style Splitter(UiSplitter::Style s, const UiOliveTokens& t, UiRole role)
{
    Palette(s.track_palette,t,UiRole::Standard,t.background); Metrics(s.track_metrics,t);
    Palette(s.thumb_palette,t,UiRole::Standard,role == UiRole::Standard ? t.line : Ink(t,role)); Metrics(s.thumb_metrics,t);
    Palette(s.background_palette,t,UiRole::Standard,t.background); Metrics(s.background_metrics,t);
    s.grip_color = t.muted; s.label_color = t.text; s.label_font = t.caption;
    return UiApplyTypography(s);
}

inline UiTab::Style Tab(UiTab::Style s, const UiOliveTokens& t, UiRole role)
{
    Base(s,t,UiRole::Standard,t.panel); Metrics(s.tab_metrics,t);
    Palette(s.tab_palette,t,UiRole::Standard,t.panel);
    s.active_tab_uses_body_face = false; s.tab_font = FontPx(11);
    s.tab_extent = DPI(51); s.item_spacing = DPI(14);
    s.tab_padding = Rect(0,DPI(17),0,DPI(15));
    s.strip_inset = Rect(DPI(12),0,DPI(12),0);
    s.indicator_thickness = DPI(2); s.active_frame_color = Edge(t,role == UiRole::Alert ? role : UiRole::Accent);
    s.tab_metrics.face_enabled = s.tab_metrics.frame_enabled = false;
    for(int i = 0; i < 4; ++i) {
        s.tab_palette.face[i] = UiFill::None(); s.tab_palette.frame[i] = Null;
        s.tab_palette.ink[i] = s.tab_palette.icon[i] =
            i == ST_PRESSED || i == ST_HOT ? t.text : i == ST_DISABLED ? Blend(t.panel,t.subtle,115) : t.subtle;
    }
    if(s.visual == UITAB_SEGMENTED) {
        s.tab_font = FontPx(10); s.tab_extent = DPI(28); s.item_spacing = DPI(2);
        s.tab_padding = Rect(DPI(10),DPI(5),DPI(10),DPI(5));
        s.strip_inset = Rect(DPI(3),DPI(3),DPI(3),DPI(3));
        s.metrics.radius = DPI(6); s.tab_metrics.radius = DPI(4);
        s.tab_metrics.face_enabled = true; s.tab_palette.face[ST_PRESSED] = t.panel2;
        s.tab_palette.ink[ST_NORMAL] = t.muted; s.palette.face[ST_NORMAL] = t.field;
    }
    return UiApplyTypography(s);
}

inline UiList::Style List(UiList::Style s, const UiOliveTokens& t, UiRole role)
{
    Base(s,t,UiRole::Standard,t.panel); s.font = FontPx(11); s.row_height = t.row_height;
    s.row_radius = DPI(5); s.ink = t.text; s.hot_ink = t.text; s.selected_ink = t.accent;
    s.disabled_ink = t.disabled_ink; s.muted_ink = t.subtle;
    s.hot_face = t.hover; s.hot_frame = t.line; s.selected_face = t.active; s.selected_frame = t.accent;
    s.separator_color = t.line_soft; s.row_even_face = s.row_odd_face = t.field;
    s.badge_face = t.panel2; s.badge_frame = t.line; s.badge_ink = t.muted;
    s.badge_radius = DPI(4); s.metadata_default = t.subtle;
    s.check_frame = t.line; s.check_fill = t.accent; s.drag_marker = t.focus;
    return UiApplyTypography(s);
}

inline UiTree::Style Tree(UiTree::Style s, const UiOliveTokens& t, UiRole role)
{
    Base(s,t,UiRole::Standard,t.panel); s.font = FontPx(11);
    s.row_height = max(DPI(32),UiGetTextSize("Ag",s.font).cy+DPI(16));
    s.v_padding = DPI(8); s.row_radius = DPI(5);
    s.ink = t.muted; s.hot_ink = t.text; s.selected_ink = t.accent; s.disabled_ink = t.disabled_ink;
    s.hot_face = t.hover; s.hot_frame = Null; s.selected_face = t.active; s.selected_frame = Null;
    s.line_color = t.line; s.glyph_color = t.subtle; s.glyph_hot_color = t.text; s.glyph_selected_color = t.accent;
    return UiApplyTypography(s);
}

inline UiTable::Style Table(UiTable::Style s, const UiOliveTokens& t, UiRole role)
{
    Base(s,t,UiRole::Standard,t.panel); s.font = FontPx(10); s.header_font = FontPx(9,500);
    s.row_height = s.header_height = DPI(32); s.cell_padding_y = DPI(9);
    s.table_bg = s.header_bg = s.row_header_bg = t.panel; s.header_hot_bg = t.hover;
    s.header_ink = t.subtle; s.cell_ink = t.text; s.muted_ink = t.muted; s.grid_color = t.line_soft;
    s.alternate_row_bg = t.panel; s.hover_bg = t.hover; s.selection_bg = s.active_bg = t.active;
    s.selection_border = t.accent; s.active_border = t.focus; s.read_only_bg = t.field;
    s.warning_bg = s.error_bg = t.alert_face; s.resize_guide = t.focus;
    return UiApplyTypography(s);
}

inline UiMenu::Style Menu(UiMenu::Style s, const UiOliveTokens& t)
{
    Base(s,t,UiRole::Standard,t.panel); s.font = s.bar_font = FontPx(11);
    s.row_height = t.row_height; s.bar_height = t.control_height;
    s.popup_bg = t.panel; s.bar_bg = t.rail; s.separator_color = t.line_soft;
    s.item_ink = t.text; s.disabled_ink = t.disabled_ink; s.right_ink = t.subtle;
    s.hot_bg = t.hover; s.hot_frame = t.line; s.pressed_bg = t.active; s.pressed_frame = t.accent;
    s.active_bar_bg = t.active; s.check_color = t.accent; s.arrow_color = t.muted; s.shadow_color = t.rail;
    return UiApplyTypography(s);
}

inline UiTitleCard::Style TitleCard(UiTitleCard::Style s, const UiOliveTokens& t, UiRole role)
{
    Base(s,t,UiRole::Standard,t.panel);
    s.title_font = t.title; s.subtitle_font = t.body; s.copy_font = t.caption;
    s.title_color = Ink(t,role); s.subtitle_color = t.muted; s.copy_color = t.subtle; s.card_line_color = t.line;
    s.transparent = true; s.metrics.face_enabled = s.metrics.frame_enabled = s.metrics.focus_enabled = false;
    s.metrics.content_margin = Rect(0,0,0,0);
    return UiApplyTypography(s);
}


inline UiRangeSegments::Style RangeSegments(UiRangeSegments::Style s, const UiOliveTokens& t)
{
    const UiSlider::Style slider = Slider(UiSlider::StyleDefault(),t,UiRole::Standard);
    s.track_palette = slider.track_palette; s.track_metrics = slider.track_metrics;
    s.thumb_palette = slider.thumb_palette; s.thumb_metrics = slider.thumb_metrics;
    Palette(s.value_palette,t,UiRole::Standard,t.panel);
    const Color series[] = {t.active,t.active,t.alert_face,t.alert_face,t.hover,t.active};
    s.series_count = 6;
    for(int i = 0; i < UiRangeSegments::MAX_SERIES_COLORS; ++i) s.series[i] = series[i % 6];
    s.label_font = s.value_font = s.right_font = t.body; s.subtitle_font = t.caption;
    s.label_color = s.value_color = t.text; s.subtitle_color = t.muted;
    s.divider_color = t.line; s.selected_frame = t.accent;
    // Only the palette/typography are prescribed. Card/compact layout, axis,
    // mirrored labels and authored series choices belong to the control/host.
    UiApplyTypography(s);
    s.value_font = UiFonts::Inherit(s.value_font);
    return s;
}

} // namespace UiOliveDetail
} // namespace Upp
#endif
