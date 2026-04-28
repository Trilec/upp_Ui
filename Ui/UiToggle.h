#ifndef _Ui_UiToggle_h_
#define _Ui_UiToggle_h_

/*
    UiToggle
    ========

    Purpose
    - Styled on/off switch control.

    Intent
    - Keep boolean switch behavior explicit while staying aligned with the Ui
      family action, theme, and invalidation contracts.

    Thread context
    - GUI thread only.

    Usage
    - Use SetData/GetData for generic boolean binding and WhenAction for user
      commits.

    Changelog
    - 2026-03: added release-standard header documentation.
    - 2026-03-31: disabled controls now ignore keyboard and mouse activation paths.
    - 2026-03-31: SetData now updates state without emitting WhenAction.
    - 2026-04: added part-aware paint hooks for track and thumb.
*/
#include <Animation/Animation.h>
#include <CtrlCore/CtrlCore.h>
#include <CtrlLib/CtrlLib.h>
#include <Ui/UiStyle.h>
#include <Ui/UiDraw.h>
namespace Upp {
class UiToggle : public Ctrl, public CtrlStyled<UiToggle> {
public:
    typedef UiToggle CLASSNAME;

    struct Style : ChStyle<Style> {
        StyledPalette palette;
        StyledMetrics metrics;
        StyledSkin    skin;
        StyledPalette track_palette;
        StyledMetrics track_metrics;
        StyledSkin    track_skin;
        StyledPalette thumb_palette;
        StyledMetrics thumb_metrics;
        StyledSkin    thumb_skin;
        UiDirection direction = UiDirection::H;
        UiAlign align_h = UiAlign::LEFT;
        UiAlign align_v = UiAlign::CENTER;
        UiAlign track_side = UiAlign::LEFT;
        Size track_size = Size(DPI(36), DPI(20));
        Size thumb_size = Size(0, 0);
        int  thumb_inset = DPI(3);

        bool animate = true;
        int  animation_ms = 120;
        void Serialize(Stream& s)
        {
            s % palette % metrics % skin
              % track_palette % track_metrics % track_skin
              % thumb_palette % thumb_metrics % thumb_skin
              % direction % align_h % align_v % track_side
              % track_size % thumb_size % thumb_inset
              % animate % animation_ms;
        }
    };

    struct PaintContext {
        Rect outer;
        Rect shell;
        Rect content;
        Rect track;
        Rect thumb;
        const Style* style = nullptr;
        StyledState state = ST_NORMAL;
        bool has_focus = false;
        bool on = false;
        double thumb_pos = 0.0;
    };

    static const Style& StyleDefault();
    UiToggle();
    UiToggle& SetStyle(const Style& s);
    UiToggle& ClearStyleOverride();
    bool HasStyleOverride() const { return has_style_override_; }
    const Style& GetStyle() const { return GetEffectiveStyle(); }
    StyledPalette& StyledPaletteRef() { return StyleEdit().palette; }
    StyledMetrics& StyledMetricsRef() { return StyleEdit().metrics; }
    StyledSkin&    StyledSkinRef()    { return StyleEdit().skin; }
    void OnStyleChanged();
    UiToggle& SetOn(bool on = true);
    bool      IsOn() const { return on_; }
    UiToggle& Toggle();
    UiToggle& SetDirection(UiDirection dir);
    UiDirection GetDirection() const { return GetEffectiveStyle().direction; }
    UiToggle& SetTrackSide(UiAlign side);
    UiToggle& SetTrackSize(Size sz);
    UiToggle& SetThumbSize(Size sz);
    UiToggle& SetTrackRadius(int radius);
    UiToggle& SetThumbRadius(int radius);
    UiToggle& SetThumbInset(int inset);
    UiToggle& SetMargin(const Rect& pad);
    UiToggle& SetMargin(int l, int t, int r, int b) { return SetMargin(Rect(l, t, r, b)); }
    UiToggle& SetMargin(int all) { return SetMargin(Rect(all, all, all, all)); }
    virtual void Paint(Draw& w) override;
    virtual void Layout() override;
    virtual Size GetMinSize() const override;
    virtual void SetMinSize(Size sz) override;
    virtual void LeftDown(Point p, dword flags) override;
    virtual void LeftUp(Point p, dword flags) override;
    virtual void MouseMove(Point p, dword flags) override;
    virtual void MouseEnter(Point p, dword flags) override;
    virtual void MouseLeave() override;
    virtual void GotFocus() override;
    virtual void LostFocus() override;
    virtual bool Key(dword key, int count) override;
    virtual void CancelMode() override;
    virtual void  SetData(const Value& v) override;
    virtual Value GetData() const override;
    Event<> WhenAction;
    Event<Draw&, const Rect&, const StyledPalette&, const StyledMetrics&, const StyledSkin&, StyledState, bool> WhenPaintBackground;
    Event<Draw&, const Rect&, const StyledPalette&, const StyledMetrics&, const StyledSkin&, StyledState, bool> WhenPaintForeground;
    // Part-aware paint hooks. Set handled = true to replace the default paint
    // for that surface while keeping geometry ownership inside the control.
    Event<Draw&, const PaintContext&, bool&> WhenPaintTrack;
    Event<Draw&, const PaintContext&, bool&> WhenPaintThumb;
private:
    void InvalidateStyleCache();
    Style& StyleEdit();
    void SyncThemeStyle();
    const Style& GetEffectiveStyle() const;
    Rect GetShellRect() const;
    Rect GetContentRect() const;
    Size GetTrackExtent() const;
    Rect GetTrackShadowMargins() const;
    Size GetTrackSlotSize() const;
    Rect GetTrackSlotRect(const Rect& content) const;
    int ClampRadiusPx(int radius, Size bounds) const;
    Rect GetTrackRect(const Rect& content) const;
    Rect GetThumbRect(const Rect& track) const;
    void StartThumbAnimation(double target);
    UiToggle& SetOnInternal(bool on, bool fire_action);
private:
    Style style_;
    mutable Style themed_style_;
    mutable uint64 theme_revision_ = 0;
    bool has_style_override_ = false;
    bool on_ = false;
    bool hover_ = false;
    bool pressed_ = false;
    Size user_min_size_ = Size(0, 0);
    double thumb_pos_ = 0.0;
    One<Animation> anim_;
};
}
#endif
