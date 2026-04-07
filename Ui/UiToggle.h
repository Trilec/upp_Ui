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
        Font    font = StdFont();
        UiAlign align_h = UiAlign::LEFT;
        UiAlign align_v = UiAlign::CENTER;
        UiAlign track_side = UiAlign::LEFT;        Size track_extent = Size(DPI(36), DPI(20));
        int  label_gap = DPI(10);
        int  thumb_inset = DPI(3);

        bool animate = true;
        int  animation_ms = 120;
        void Serialize(Stream& s)
        {
            s % palette % metrics % skin
              % track_palette % track_metrics % track_skin
              % thumb_palette % thumb_metrics % thumb_skin
              % font % align_h % align_v % track_side
              % track_extent % label_gap % thumb_inset
              % animate % animation_ms;
        }
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
    UiToggle& SetText(const String& s);
    const String& GetText() const { return text_; }
    UiToggle& SetOn(bool on = true);
    bool      IsOn() const { return on_; }
    UiToggle& Toggle();
    UiToggle& SetTrackSide(UiAlign side);
    UiToggle& SetPadding(const Rect& pad);
    UiToggle& SetPadding(int l, int t, int r, int b) { return SetPadding(Rect(l, t, r, b)); }
    UiToggle& SetPadding(int all) { return SetPadding(Rect(all, all, all, all)); }
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
private:
    void InvalidateStyleCache();
    Style& StyleEdit();
    void SyncThemeStyle();
    const Style& GetEffectiveStyle() const;
    Size GetTextSizeCached() const;
    Rect GetShellRect() const;
    Rect GetContentRect() const;
    Rect GetTrackRect(const Rect& content) const;
    Rect GetTextRect(const Rect& content, const Rect& track) const;
    Rect GetThumbRect(const Rect& track) const;
    void StartThumbAnimation(double target);
    UiToggle& SetOnInternal(bool on, bool fire_action);
private:
    Style style_;
    mutable Style themed_style_;
    mutable uint64 theme_revision_ = 0;
    bool has_style_override_ = false;
    String text_;
    bool on_ = false;
    bool hover_ = false;
    bool pressed_ = false;
    mutable bool text_size_dirty_ = true;
    mutable Size text_size_cache_ = Size(0, 0);
    Size user_min_size_ = Size(0, 0);
    double thumb_pos_ = 0.0;
    One<Animation> anim_;
};
}
#endif











