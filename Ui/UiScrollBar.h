#ifndef _Ui_UiScrollBar_h_
#define _Ui_UiScrollBar_h_

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.
    UiScrollBar
    ===========

    Purpose
    - Styled scrollbar control for scrollable Ui surfaces and popups.

    Intent
    - Provide one theme-driven scrollbar implementation shared by standalone
      scroll surfaces and composite controls such as dropdown popups.

    Thread context
    - GUI thread only.

    Usage
    - Use directly for custom scroll surfaces or indirectly through controls
      that own their own viewport/content model.

    Changelog
    - 2026-03: added release-standard header documentation.
    - 2026-04: tightened default thin-idle geometry and thumb contrast so
      scrollbars reserve the full gutter but paint a more centered, inset thumb.
    - 2026-04: aligned arrow icon rendering with shared UiIconRenderMode while
      keeping arrow-specific naming scoped to the scrollbar style.
    - 2026-04: normalized part-aware paint hooks for track, thumb, and arrows.
    - 2026-05: widened the minimal default idle/hot scrollbar thumb and hot
      gutter by a small amount so scroll panels are easier to acquire.
*/

#include <CtrlCore/CtrlCore.h>
#include <CtrlLib/CtrlLib.h>
#include <Ui/UiStyle.h>
#include <Ui/UiDraw.h>
#include <Animation/Animation.h>

namespace Upp {

enum UiScrollArrowsLayout : byte {
    UIARROWS_NONE = 0,
    UIARROWS_SPLIT,
    UIARROWS_GROUP_START,
    UIARROWS_GROUP_END,
};

enum UiScrollArrowCross : byte {
    UIARROWCROSS_FILL = 0,
    UIARROWCROSS_SQUARE,
};

enum UiScrollThumbLenMode : byte {
    UITHUMB_PROPORTIONAL = 0,
    UITHUMB_FIXED,
};

enum UiScrollGrip : byte {
    UIGRIP_NONE = 0,
    UIGRIP_LINES,
    UIGRIP_DOTS,
    UIGRIP_SLOT,
    UIGRIP_IMAGE,
};

class UiScrollBar : public Ctrl, public CtrlStyled<UiScrollBar> {
public:
    typedef UiScrollBar CLASSNAME;

    struct Style : ChStyle<Style> {
        StyledPalette track_palette;
        StyledMetrics track_metrics;
        StyledSkin    track_skin;

        StyledPalette thumb_palette;
        StyledMetrics thumb_metrics;
        StyledSkin    thumb_skin;

        StyledPalette arrow_palette;
        StyledMetrics arrow_metrics;
        StyledSkin    arrow_skin;

        bool  arrow_icons      = false;
        bool  arrow_icon_scale = true;
        UiIconRenderMode arrow_icon_render_mode = UiIconRenderMode::MonoTint;
        Image arrow_icon_prev_h;
        Image arrow_icon_next_h;
        Image arrow_icon_prev_v;
        Image arrow_icon_next_v;

        bool  show_arrows    = false;
        UiScrollArrowsLayout arrows_layout = UIARROWS_SPLIT;
        UiScrollArrowCross  arrow_cross   = UIARROWCROSS_SQUARE;
        int   arrow_size     = DPI(14);
        int   thumb_min_size = DPI(20);
        UiScrollThumbLenMode thumb_len_mode = UITHUMB_PROPORTIONAL;
        int   fixed_thumb_len_px = DPI(24);
        bool  paint_track_under_arrows = false;
        bool  auto_hide      = false;

        bool thin_idle       = false;
        int  thin_px         = DPI(5);
        int  thick_px        = DPI(18);
        int  track_paint_px_idle = DPI(5);
        int  track_paint_px_hot  = DPI(18);
        int  thumb_paint_px_idle = DPI(10);
        int  thumb_paint_px_hot  = DPI(18);

        bool animate_expand  = true;
        int  expand_ms       = 180;
        Easing::Fn expand_easing = Easing::OutCubic();
        int  collapse_ms     = 1000;

        bool fade_idle       = true;
        int  fade_ms         = 300;
        int  idle_fade_pct   = 70;
        Easing::Fn fade_easing = Easing::OutCubic();

        UiScrollGrip grip = UIGRIP_NONE;
        Color grip_color = Null;
        Image grip_image;
        Rect track_inset = Rect(0, 0, 0, 0);
        Rect thumb_inset = Rect(0, 0, 0, 0);

        void Serialize(Stream& s)
        {
            int _arrows_layout = (int)arrows_layout;
            int _arrow_cross = (int)arrow_cross;
            int _thumb_len_mode = (int)thumb_len_mode;
            int _grip = (int)grip;
            s % track_palette
              % track_metrics
              % track_skin
              % thumb_palette
              % thumb_metrics
              % thumb_skin
              % arrow_palette
              % arrow_metrics
              % arrow_skin
              % arrow_icons
              % arrow_icon_scale
              % arrow_icon_render_mode
              % arrow_icon_prev_h
              % arrow_icon_next_h
              % arrow_icon_prev_v
              % arrow_icon_next_v
              % show_arrows
              % _arrows_layout
              % _arrow_cross
              % arrow_size
              % thumb_min_size
              % _thumb_len_mode
              % fixed_thumb_len_px
              % paint_track_under_arrows
              % auto_hide
              % thin_idle
              % thin_px
              % thick_px
              % track_paint_px_idle
              % track_paint_px_hot
              % thumb_paint_px_idle
              % thumb_paint_px_hot
              % animate_expand
              % expand_ms
              % collapse_ms
              % fade_idle
              % fade_ms
              % idle_fade_pct
              % _grip
              % grip_color
              % grip_image
              % track_inset
              % thumb_inset;

            if(s.IsLoading()) {
                arrows_layout = (UiScrollArrowsLayout)clamp(_arrows_layout, (int)UIARROWS_NONE, (int)UIARROWS_GROUP_END);
                arrow_cross = (UiScrollArrowCross)clamp(_arrow_cross, (int)UIARROWCROSS_FILL, (int)UIARROWCROSS_SQUARE);
                thumb_len_mode = (UiScrollThumbLenMode)clamp(_thumb_len_mode, (int)UITHUMB_PROPORTIONAL, (int)UITHUMB_FIXED);
                grip = (UiScrollGrip)clamp(_grip, (int)UIGRIP_NONE, (int)UIGRIP_IMAGE);
            }
        }
    };

    struct PaintContext {
        Rect outer;
        Rect track_hit;
        Rect track;
        Rect thumb_hit;
        Rect thumb;
        Rect arrow0;
        Rect arrow1;
        const Style* style = nullptr;
        StyledState track_state = ST_NORMAL;
        StyledState thumb_state = ST_NORMAL;
        StyledState arrow0_state = ST_NORMAL;
        StyledState arrow1_state = ST_NORMAL;
        UiDirection direction = UiDirection::V;
        int min = 0;
        int max = 0;
        int page = 0;
        int pos = 0;
    };

private:
    Style style_;
    mutable uint64 theme_revision_ = 0;
    bool has_custom_style_ = false;

    int pos_ = 0;
    int min_ = 0;
    int max_ = 100;
    int page_ = 20;

    bool dragging_ = false;
    Point drag_offset_;
    bool hover_track_ = false;
    bool hover_thumb_ = false;
    int  hover_arrow_ = -1;

    StyledState track_state_  = ST_NORMAL;
    StyledState thumb_state_  = ST_NORMAL;
    StyledState arrow0_state_ = ST_NORMAL;
    StyledState arrow1_state_ = ST_NORMAL;

    One<Animation> anim_generic_;
    One<Animation> anim_thickness_;
    One<Animation> anim_fade_;
    int paint_thickness_ = 0;
    double fade_t_ = 1.0;
    TimeCallback collapse_tc_;
    Size user_min_size_ = Size(0, 0);

    void InvalidateStyleCache();
    Style& StyleEdit();
    void SyncThemeStyle();
    const Style& GetEffectiveStyle() const;
    void UpdateVisualState();
    void RebuildLook();
    void UpdateVisibility_();
    void UpdateAnimatedVisuals_(bool hover_now);
    void AnimateThickness_(int target);
    void AnimateFade_(double target);
    void PaintCore_(Draw& w, const Rect& outer);

    // Part-aware paint hooks. Set handled = true to replace the default paint
    // for that surface while keeping geometry ownership inside the control.
    Event<Draw&, const PaintContext&, bool&> WhenPaintTrack;
    Event<Draw&, const PaintContext&, bool&> WhenPaintThumb;
    Event<Draw&, const PaintContext&, int, bool&> WhenPaintArrow;

public:
    UiScrollBar();
    UiScrollBar(UiDirection dir);

    UiScrollBar& SetDirection(UiDirection dir);
    UiDirection  GetDirection() const { return dir_; }

    UiScrollBar& SetRange(int min, int max, int page);
    UiScrollBar& SetPos(int pos);
    int          GetPos() const { return pos_; }
    int          GetMin() const { return min_; }
    int          GetMax() const { return max_; }
    int          GetPage() const { return page_; }

    virtual void  SetData(const Value& v) override { SetPos((int)v); }
    virtual Value GetData() const override         { return pos_; }

    UiScrollBar& SetCustomStyle(const Style& s);
    UiScrollBar& ClearCustomStyle();
    bool         HasCustomStyle() const { return has_custom_style_; }
    const Style& GetStyle() const { return GetEffectiveStyle(); }
    const Style& GetCustomStyle() const { return style_; }
    static const Style& StyleDefault();

    StyledPalette& StyledPaletteRef() { return StyleEdit().track_palette; }
    StyledMetrics& StyledMetricsRef() { return StyleEdit().track_metrics; }
    StyledSkin&    StyledSkinRef()    { return StyleEdit().track_skin; }
    void           OnStyleChanged();

    StyledPalette& TrackPalette() { return StyleEdit().track_palette; }
    StyledMetrics& TrackMetrics() { return StyleEdit().track_metrics; }
    StyledSkin&    TrackSkin()    { return StyleEdit().track_skin; }

    StyledPalette& ThumbPalette() { return StyleEdit().thumb_palette; }
    StyledMetrics& ThumbMetrics() { return StyleEdit().thumb_metrics; }
    StyledSkin&    ThumbSkin()    { return StyleEdit().thumb_skin; }

    StyledPalette& ArrowPalette() { return StyleEdit().arrow_palette; }
    StyledMetrics& ArrowMetrics() { return StyleEdit().arrow_metrics; }
    StyledSkin&    ArrowSkin()    { return StyleEdit().arrow_skin; }

    UiScrollBar& SetThumbColor(Color face, Color frame = Null, Color ink = Null);
    UiScrollBar& SetArrowColor(Color face, Color frame = Null, Color ink = Null);

    UiScrollBar& ShowArrows(bool on = true);
    UiScrollBar& SetArrowsLayout(UiScrollArrowsLayout l);
    UiScrollBar& SetArrowCross(UiScrollArrowCross c);
    UiScrollBar& SetThumbLenMode(UiScrollThumbLenMode m);
    UiScrollBar& SetFixedThumbLen(int px);
    UiScrollBar& SetGrip(UiScrollGrip g);
    UiScrollBar& EnableAutoHide(bool on = true);
    UiScrollBar& EnableThinIdle(bool on = true);

    template <class T>
    UiScrollBar& Animate(const T& from, const T& to,
                         int ms,
                         Event<const T&> setter,
                         Easing::Fn curve = Easing::OutCubic(),
                         Event<> on_finish = {});

    virtual Size GetMinSize() const override;
    virtual void SetMinSize(Size sz) override;

    UiScrollBar& SetSizeMin(Size sz) { SetMinSize(sz); return *this; }
    UiScrollBar& SetSizeMin(int cx, int cy) { return SetSizeMin(Size(cx, cy)); }
    UiScrollBar& SetSizeFixed(Size sz) { return SetSizeMin(sz); }
    UiScrollBar& SetSizeFixed(int cx, int cy) { return SetSizeMin(Size(cx, cy)); }

    Event<> WhenScroll;
    Event<> WhenBar;

    virtual void Paint(Draw& w) override;
    virtual void LeftDown(Point p, dword keyflags) override;
    virtual void LeftUp(Point p, dword keyflags) override;
    virtual void MouseMove(Point p, dword keyflags) override;
    virtual void MouseEnter(Point p, dword keyflags) override;
    virtual void MouseLeave() override;
    virtual void CancelMode() override;
    virtual void MouseWheel(Point p, int zdelta, dword keyflags) override;
    void SyncHoverFromMouse();

    virtual String GetDesc() const override;
    virtual dword  GetAccessKeys() const override { return 0; }
    virtual void   AssignAccessKeys(dword used) override {}

private:
    UiDirection dir_ = UiDirection::V;

    Rect GetTrackRect() const;
    Rect GetThumbLaneRect_() const;
    Rect GetThumbRect() const;
    Rect GetArrowRect(int idx) const;
    Rect GetArrowHitRect_(int idx) const;
    int  GetArrowSide_() const;
    int  ComputeThumbLength() const;
    int  ComputeThumbPosition() const;
    bool PtInThumb(Point p) const;
    bool PtInArrow(Point p, int& idx) const;
    void JumpToPosition(int new_pos);
};

template <class T>
UiScrollBar& UiScrollBar::Animate(const T& from, const T& to, int ms,
                                  Event<const T&> setter,
                                  Easing::Fn curve, Event<> on_finish)
{
    if(!setter)
        return *this;

    if(anim_generic_) {
        anim_generic_->Cancel();
        anim_generic_.Clear();
    }

    anim_generic_.Create(*this);
    Animation& a = *anim_generic_;

    bool have_finish = (bool)on_finish;

    a([ctrl_ptr = Ptr<Ctrl>(this), setter, from, to, on_finish,
       have_finish](double p) mutable -> bool {
        if(!ctrl_ptr)
            return false;

        T value;
        if constexpr(std::is_same_v<T, Color>) {
            value = Blend(from, to, int(p * 255));
        }
        else if constexpr(std::is_same_v<T, Point>) {
            value = Point(int(from.x + (to.x - from.x) * p + 0.5),
                          int(from.y + (to.y - from.y) * p + 0.5));
        }
        else if constexpr(std::is_same_v<T, Size>) {
            value = Size(int(from.cx + (to.cx - from.cx) * p + 0.5),
                         int(from.cy + (to.cy - from.cy) * p + 0.5));
        }
        else if constexpr(std::is_same_v<T, Rect>) {
            value = Rect(Point(int(from.left + (to.left - from.left) * p + 0.5),
                               int(from.top + (to.top - from.top) * p + 0.5)),
                         Size(int(from.Width() + (to.Width() - from.Width()) * p + 0.5),
                              int(from.Height() + (to.Height() - from.Height()) * p + 0.5)));
        }
        else {
            value = from + (to - from) * p;
        }

        setter(value);
        ctrl_ptr->Refresh();

        if(p >= 1.0 && have_finish) {
            have_finish = false;
            if(on_finish)
                on_finish();
            return false;
        }
        return true;
    })
    .Duration(ms)
    .Ease(curve)
    .Play();

    return *this;
}

} // namespace Upp

#endif

