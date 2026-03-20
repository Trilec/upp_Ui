#ifndef _Ui_UiRadioButton_h_
#define _Ui_UiRadioButton_h_

/*
    UiRadioButton
    =============

    Purpose
    - Styled radio button control for mutually exclusive selection.

    Intent
    - Keep radio semantics explicit while sharing the same style vocabulary and
      action/event conventions used by the rest of the Ui control family.

    Thread context
    - GUI thread only.

    Usage
    - Use in groups for exclusive choice; observe committed user toggles with
      WhenAction.

    Changelog
    - 2026-03: added release-standard header documentation.
*/

#include <CtrlCore/CtrlCore.h>
#include <CtrlLib/CtrlLib.h>
#include <Ui/UiStyle.h>
#include <Ui/UiDraw.h>

namespace Upp {

enum UiRadioVisual : byte {
    UIRADIOVIS_CLASSIC = 0,
    UIRADIOVIS_PILLS,
    UIRADIOVIS_LIST,
};

class UiRadioButton : public Ctrl, public CtrlStyled<UiRadioButton> {
public:
    typedef UiRadioButton CLASSNAME;

    struct Style : ChStyle<Style> {
        StyledPalette palette;
        StyledMetrics metrics;
        StyledSkin    skin;

        StyledPalette indicator_palette;
        StyledMetrics indicator_metrics;
        StyledSkin    indicator_skin;

        Font    font = StdFont();
        UiAlign indicator_side = UiAlign::LEFT;
        int indicator_size = DPI(18);
        int indicator_gap = DPI(8);

        void Serialize(Stream& s)
        {
            s % palette % metrics % skin
              % indicator_palette % indicator_metrics % indicator_skin
              % font % indicator_side % indicator_size % indicator_gap;
        }
    };

    static const Style& StyleDefault();

    UiRadioButton();

    UiRadioButton& SetStyle(const Style& s);
    UiRadioButton& ClearStyleOverride();
    bool HasStyleOverride() const { return has_style_override_; }
    const Style& GetStyle() const { return GetEffectiveStyle(); }

    StyledPalette& StyledPaletteRef() { return StyleEdit().palette; }
    StyledMetrics& StyledMetricsRef() { return StyleEdit().metrics; }
    StyledSkin&    StyledSkinRef()    { return StyleEdit().skin; }
    void OnStyleChanged();

    UiRadioButton& SetText(const String& s);
    const String& GetText() const { return text_; }

    UiRadioButton& SetChecked(bool on = true);
    bool IsChecked() const { return checked_; }

    UiRadioButton& SetGroup(int g) { group_ = g; return *this; }
    int GetGroup() const { return group_; }

    UiRadioButton& SetVisual(UiRadioVisual vis);
    UiRadioButton& SetIndicatorSide(UiAlign side);
    UiRadioButton& SetIndicatorRadius(int px);
    UiRadioButton& SetIndicatorRoundness(int percent);

    UiRadioButton& SetSizeMin(Size sz)        { SetMinSize(sz); return *this; }
    UiRadioButton& SetSizeMin(int cx, int cy) { return SetSizeMin(Size(cx, cy)); }
    UiRadioButton& SetSizeFixed(Size sz)      { return SetSizeMin(sz); }
    UiRadioButton& SetSizeFixed(int cx, int cy) { return SetSizeMin(Size(cx, cy)); }

    Event<> WhenAction;

    Event<Draw&, const Rect&,
          const StyledPalette&, const StyledMetrics&, const StyledSkin&,
          StyledState, bool> WhenPaintBackground;
    Event<Draw&, const Rect&,
          const StyledPalette&, const StyledMetrics&, const StyledSkin&,
          StyledState, bool> WhenPaintForeground;

    virtual void Paint(Draw& w) override;
    virtual void Layout() override;
    virtual Size GetMinSize() const override;
    virtual void SetMinSize(Size sz) override;

    virtual void LeftDown(Point p, dword flags) override;
    virtual bool Key(dword key, int count) override;
    virtual void GotFocus() override;
    virtual void LostFocus() override;
    virtual void MouseEnter(Point p, dword flags) override;
    virtual void MouseLeave() override;

    virtual void SetData(const Value& v) override;
    virtual Value GetData() const override { return checked_; }

private:
    void InvalidateStyleCache();
    Style& StyleEdit();
    void SyncThemeStyle();
    const Style& GetEffectiveStyle() const;
    Size GetTextSizeCached() const;
    void RebuildLayoutCache(const Rect& content) const;
    UiRadioButton& SetCheckedInternal(bool on, bool fire_action);

    void UncheckSiblings_();
    Rect GetIndicatorRect(const Rect& r) const;
    Rect GetTextRect(const Rect& r, const Rect& ind) const;

private:
    Style style_;
    mutable Style themed_style_;
    mutable uint64 theme_revision_ = 0;
    bool has_style_override_ = false;
    UiRadioVisual visual_ = UIRADIOVIS_CLASSIC;
    String text_;
    bool checked_ = false;
    int  group_ = 0;

    bool has_focus_ = false;
    bool hover_ = false;
    bool pressed_ = false;

    Size user_min_size_ = Size(0, 0);
    mutable bool text_size_dirty_ = true;
    mutable Size text_size_cache_ = Size(0, 0);
    mutable UiBlocksLayout layout_cache_;
    mutable Rect           layout_content_cache_ = Rect(0, 0, 0, 0);
    mutable bool           layout_dirty_ = true;
};

}

#endif


