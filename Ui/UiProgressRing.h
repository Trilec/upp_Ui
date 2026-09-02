#ifndef _Ui_UiProgressRing_h_
#define _Ui_UiProgressRing_h_

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    UiProgressRing
    ==============

    Purpose
    - Compact circular determinate/indeterminate progress indicator with
      anti-aliased ring rendering and centered value text.

    Intent
    - Mirror the useful UiProgressBar value contract while keeping circular
      geometry and ring-specific styling out of the linear progress control.
    - Keep the semantic value authoritative while optional entry animation is
      presentation-only.

    Thread context
    - GUI thread only.

    Usage
    - Set(actual, total) updates determinate progress.
    - SetIndeterminate(true) enables the animated unknown-total state.
    - SetRole(...) follows the standard Ui semantic role vocabulary.
    - SetProgressGradient(start, end) enables an along-sweep progress gradient.
    - SetCapRoundness(0..100) controls flat through fully rounded stroke ends.
    - Percentage text is shown by default; SetText() replaces it.

    Changelog
    - 2026-08: moved intro/indeterminate frame scheduling to UiFrameTicker;
      Ctrl integer callback ids are byte offsets, not arbitrary identifiers.
    - 2026-08: replaced absolute cap radius with 0..100 percent cap roundness;
      cap geometry now scales automatically with stroke thickness.
    - 2026-09: added semantic UiRole theme resolution; stable determinate ring
      rasters use UiRasterCache and shared circular-arc painting lives in UiDraw.
*/

#include <CtrlCore/CtrlCore.h>
#include <CtrlLib/CtrlLib.h>
#include <Ui/UiStyle.h>
#include <Ui/UiFrameTicker.h>

namespace Upp {

enum class UiRole : byte;

class UiProgressRing : public Ctrl {
public:
    typedef UiProgressRing CLASSNAME;

    struct Style : ChStyle<Style> {
        StyledPalette progress_palette;
        StyledPalette track_palette;
        StyledPalette text_palette;

        Color gradient_end[4];
        bool  gradient_enabled = false;

        Font font;
        int  thickness = DPI(7);
        int  cap_roundness = 100;
        int  ring_inset = DPI(2);
        int  min_text_height = DPI(7);

        bool animate_on_show = true;
        int  intro_duration_ms = 600;
        int  indeterminate_duration_ms = 1100;
        int  indeterminate_sweep_degrees = 88;

        void Serialize(Stream& s)
        {
            s % progress_palette % track_palette % text_palette;
            for(int i = 0; i < 4; i++)
                s % gradient_end[i];
            s % gradient_enabled % font
              % thickness % cap_roundness % ring_inset % min_text_height
              % animate_on_show % intro_duration_ms
              % indeterminate_duration_ms % indeterminate_sweep_degrees;
        }
    };

    struct Geometry {
        Rect    outer;
        Rect    square;
        Rect    text_rect;
        Pointf  center;
        double  radius = 0.0;
        double  start_angle = 0.0;
        double  sweep_angle = 0.0;
        double  target_ratio = 0.0;
        double  display_ratio = 0.0;
        int     thickness = 0;
        int     cap_roundness = 100;
        int     text_font_height = 0;
        bool    text_visible = false;
        bool    indeterminate = false;
    };

    static const Style& StyleDefault();

    UiProgressRing();
    virtual ~UiProgressRing();

    UiProgressRing& SetCustomStyle(const Style& s);
    UiProgressRing& ClearCustomStyle();
    bool            HasCustomStyle() const { return has_custom_style_; }
    const Style&    GetStyle() const { return GetEffectiveStyle(); }
    const Style&    GetCustomStyle() const { return style_; }

    UiProgressRing& SetRole(UiRole role);
    UiRole          GetRole() const { return role_; }

    UiProgressRing& Set(int actual, int total);
    UiProgressRing& Set(int actual) { return Set(actual, total_); }
    UiProgressRing& SetTotal(int total) { return Set(actual_, total); }

    void operator=(int value) { Set(value); }
    int  operator++();
    int  operator++(int);
    int  operator+=(int amount);
    operator int() const { return actual_; }

    int    Get() const { return actual_; }
    int    GetTotal() const { return total_; }
    double GetRatio() const;
    int    GetPercent() const;
    double GetDisplayRatio() const;

    UiProgressRing& Percent(bool on = true);
    UiProgressRing& NoPercent() { return Percent(false); }
    bool            IsPercentShown() const { return show_percent_; }

    UiProgressRing& SetText(const String& text);
    UiProgressRing& ClearText();
    String          GetText() const { return custom_text_; }

    UiProgressRing& SetIndeterminate(bool on = true);
    bool            IsIndeterminate() const { return total_ <= 0; }

    UiProgressRing& SetProgressColor(Color c);
    UiProgressRing& SetProgressGradient(Color start, Color end);
    UiProgressRing& ClearProgressGradient();
    UiProgressRing& SetTrackColor(Color c);
    UiProgressRing& SetTextColor(Color c);

    UiProgressRing& SetThickness(int px);
    int             GetThickness() const { return max(1, GetEffectiveStyle().thickness); }
    UiProgressRing& SetCapRoundness(int percent);
    int             GetCapRoundness() const { return clamp(GetEffectiveStyle().cap_roundness, 0, 100); }
    UiProgressRing& SetRingInset(int px);
    int             GetRingInset() const { return max(0, GetEffectiveStyle().ring_inset); }

    UiProgressRing& SetFont(Font f);
    UiProgressRing& SetFontSize(int height);
    Font            GetFont() const { return GetEffectiveStyle().font; }

    UiProgressRing& AnimateOnShow(bool on = true);
    bool            IsAnimateOnShow() const { return GetEffectiveStyle().animate_on_show; }
    UiProgressRing& SetIntroDuration(int ms);
    UiProgressRing& SetIndeterminateDuration(int ms);
    UiProgressRing& RestartIntroAnimation();

    virtual void SetData(const Value& v) override;
    virtual Value GetData() const override;

    virtual void Paint(Draw& w) override;
    virtual Size GetMinSize() const override;
    virtual void Layout() override;
    virtual void State(int reason) override;

    Geometry GetGeometry(Size size) const;
    bool     IsAnimationRunning() const { return animation_ticker_.IsRunning(); }

private:
    enum AnimationMode : byte {
        ANIM_NONE,
        ANIM_INTRO,
        ANIM_INDETERMINATE,
    };

    void         InvalidateStyleCache();
    Style&       StyleEdit();
    void         SyncThemeStyle();
    Style        ResolveThemeStyle() const;
    const Style& GetEffectiveStyle() const;
    void         OnStyleChanged();

    String   ResolvePaintText() const;
    Geometry BuildGeometry(Size size) const;
    Font     ResolveTextFont(const String& text, const Rect& text_rect, bool& visible) const;
    Image    RenderRingRaster(const Geometry& g, bool gradient,
                              Color track, Color progress, Color gradient_end) const;

    void UpdateAnimation();
    void StartAnimation(AnimationMode mode);
    void StopAnimation();
    void AnimationStep();

    Style style_;
    mutable Style themed_style_;
    mutable uint64 theme_revision_ = 0;
    bool has_custom_style_ = false;
    UiRole role_;

    int actual_ = 0;
    int total_ = 100;
    bool show_percent_ = true;
    String custom_text_;
    bool has_custom_text_ = false;

    bool intro_complete_ = false;
    UiFrameTicker animation_ticker_;
    AnimationMode animation_mode_ = ANIM_NONE;
    dword animation_start_ms_ = 0;
};

} // namespace Upp

#endif
