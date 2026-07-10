#ifndef _Ui_UiProgressBar_h_
#define _Ui_UiProgressBar_h_

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.
    UiProgressBar
    =============

    Purpose
    - Styled determinate and indeterminate progress indicator for the Ui
      control set.

    Intent
    - Preserve the useful U++ ProgressIndicator value contract while using the
      Ui theme/style surface model and explicit testable geometry.

    Thread context
    - GUI thread only.

    Usage
    - Use Set(actual, total) for determinate progress.
    - Use SetTotal(0) or SetIndeterminate(true) for the animated unknown-total
      state.
    - Percent() shows centered percentage text for determinate progress.

    Changelog
    - 2026-07: introduced as the runtime progress bar control.
*/

#include <CtrlCore/CtrlCore.h>
#include <CtrlLib/CtrlLib.h>
#include <Ui/UiStyle.h>
#include <Ui/UiDraw.h>

namespace Upp {

class UiProgressBar : public Ctrl, public CtrlStyled<UiProgressBar> {
public:
    typedef UiProgressBar CLASSNAME;

    enum class Orientation : byte {
        Auto,
        Horizontal,
        Vertical
    };

    struct Style : ChStyle<Style> {
        StyledPalette track_palette;
        StyledMetrics track_metrics;
        StyledSkin    track_skin;

        StyledPalette fill_palette;
        StyledMetrics fill_metrics;
        StyledSkin    fill_skin;

        Font  font;
        Color filled_text = White();
        Color empty_text = SColorText();
        Rect  content_inset = Rect(0, 0, 0, 0);
        int   indeterminate_span = DPI(42);
        int   indeterminate_duration_ms = 1100;

        void Serialize(Stream& s)
        {
            s % track_palette % track_metrics % track_skin
              % fill_palette % fill_metrics % fill_skin
              % font % filled_text % empty_text % content_inset
              % indeterminate_span % indeterminate_duration_ms;
        }
    };

    struct Geometry {
        Rect outer;
        Rect content;
        Rect track;
        Rect fill;
        bool vertical = false;
        bool indeterminate = false;
        int actual = 0;
        int total = 0;
        int percent = 0;
    };

    static const Style& StyleDefault();

    UiProgressBar();
    virtual ~UiProgressBar();

    UiProgressBar& SetCustomStyle(const Style& s);
    UiProgressBar& ClearCustomStyle();
    bool HasCustomStyle() const { return has_custom_style_; }
    const Style& GetStyle() const { return GetEffectiveStyle(); }
    const Style& GetCustomStyle() const { return style_; }

    StyledPalette& StyledPaletteRef() { return StyleEdit().track_palette; }
    StyledMetrics& StyledMetricsRef() { return StyleEdit().track_metrics; }
    StyledSkin&    StyledSkinRef()    { return StyleEdit().track_skin; }
    void OnStyleChanged();

    UiProgressBar& Set(int actual, int total);
    UiProgressBar& Set(int actual) { return Set(actual, total_); }
    UiProgressBar& SetTotal(int total) { return Set(actual_, total); }

    void operator=(int value) { Set(value); }
    int  operator++();
    int  operator++(int);
    int  operator+=(int amount);
    operator int() const { return actual_; }

    int Get() const { return actual_; }
    int GetTotal() const { return total_; }
    double GetRatio() const;
    int GetPercent() const;

    UiProgressBar& Percent(bool on = true);
    UiProgressBar& NoPercent() { return Percent(false); }
    bool IsPercentShown() const { return show_percent_; }

    UiProgressBar& SetIndeterminate(bool on = true);
    bool IsIndeterminate() const { return total_ <= 0; }

    UiProgressBar& SetOrientation(Orientation orientation);
    Orientation GetOrientation() const { return orientation_; }

    UiProgressBar& SetText(const String& text);
    UiProgressBar& ClearText();
    String GetText() const { return custom_text_; }

    UiProgressBar& SetColor(Color c);
    UiProgressBar& SetFont(Font f);
    Font GetFont() const { return GetEffectiveStyle().font; }

    virtual void SetData(const Value& v) override;
    virtual Value GetData() const override;

    virtual void Paint(Draw& w) override;
    virtual Size GetMinSize() const override;
    virtual void Layout() override;
    virtual void State(int reason) override;

    Geometry GetGeometry(Size size) const;
    bool IsAnimationRunning() const { return animation_running_; }

private:
    enum { ANIM_CB_ID = 0x70524F47 };

    void InvalidateStyleCache();
    Style& StyleEdit();
    void SyncThemeStyle();
    const Style& GetEffectiveStyle() const;

    bool ResolveVertical(Size size) const;
    String ResolvePaintText() const;
    Geometry BuildGeometry(Size size, int phase_px = -1) const;
    void PaintText(Draw& w, const Rect& content, const Rect& fill,
                   const String& text, const Style& style) const;

    void UpdateAnimation();
    void StartAnimation();
    void StopAnimation();
    void AnimationStep();

    Style style_;
    mutable Style themed_style_;
    mutable uint64 theme_revision_ = 0;
    bool has_custom_style_ = false;

    int actual_ = 0;
    int total_ = 100;
    bool show_percent_ = false;
    Orientation orientation_ = Orientation::Auto;
    String custom_text_;
    bool has_custom_text_ = false;

    bool animation_running_ = false;
    dword animation_start_ms_ = 0;
};

}

#endif
