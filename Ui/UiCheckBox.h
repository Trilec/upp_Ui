#ifndef _Ui_UiCheckBox_h_
#define _Ui_UiCheckBox_h_

/*
    UiCheckBox
    ========== 

    Purpose
    - Styled checkbox control supporting boolean and tri-state interaction.

    Intent
    - Keep toggle behavior and indicator visuals unified with the rest of the
      Ui control family while allowing visual modes such as classic, switch,
      chip, and list presentation.

    Thread context
    - GUI thread only.

    Usage
    - Bind with SetData/GetData for generic boolean or tri-state use.
    - Observe committed user toggles with WhenAction.

    Changelog
    - 2026-03: added release-standard header documentation.
*/

#include <CtrlCore/CtrlCore.h>
#include <CtrlLib/CtrlLib.h>
#include <Ui/UiStyle.h>
#include <Ui/UiDraw.h>

namespace Upp {

enum UiCheckState : byte {
    UICHECK_UNCHECKED = 0,
    UICHECK_CHECKED,
    UICHECK_INDETERMINATE,
};

enum UiCheckVisual : byte {
    UICHECKVIS_CLASSIC = 0,
    UICHECKVIS_SWITCH,
    UICHECKVIS_CHIP,
    UICHECKVIS_LIST,
};

class UiCheckBox : public Ctrl, public CtrlStyled<UiCheckBox> {
public:
    typedef UiCheckBox CLASSNAME;

    struct Style : ChStyle<Style> {
        StyledPalette palette;
        StyledMetrics metrics;
        StyledSkin    skin;

        StyledPalette indicator_palette;
        StyledMetrics indicator_metrics;
        StyledSkin    indicator_skin;

        Font    font = StdFont();
        UiAlign align_h = UiAlign::LEFT;
        UiAlign align_v = UiAlign::CENTER;
        UiAlign indicator_side = UiAlign::LEFT;

        int indicator_size = DPI(18);
        Size indicator_extent = Size(0, 0);
        int indicator_gap = DPI(8);
        int mark_thickness = DPI(2);

        void Serialize(Stream& s)
        {
            s % palette % metrics % skin
              % indicator_palette % indicator_metrics % indicator_skin
              % font % align_h % align_v % indicator_side
              % indicator_size % indicator_extent % indicator_gap % mark_thickness;
        }
    };

    static const Style& StyleDefault();

    UiCheckBox();

    UiCheckBox& SetStyle(const Style& s);
    UiCheckBox& ClearStyleOverride();
    bool HasStyleOverride() const { return has_style_override_; }
    const Style& GetStyle() const { return GetEffectiveStyle(); }

    StyledPalette& StyledPaletteRef() { return StyleEdit().palette; }
    StyledMetrics& StyledMetricsRef() { return StyleEdit().metrics; }
    StyledSkin&    StyledSkinRef()    { return StyleEdit().skin; }
    void OnStyleChanged();

    UiCheckBox& SetText(const String& s);
    const String& GetText() const { return text_; }

    UiCheckBox& SetTriState(bool on = true);
    bool IsTriState() const { return tri_state_; }

    UiCheckBox& SetState(UiCheckState st);
    UiCheckState GetState() const { return state_; }

    UiCheckBox& SetChecked(bool on = true) { return SetState(on ? UICHECK_CHECKED : UICHECK_UNCHECKED); }
    bool IsChecked() const { return state_ == UICHECK_CHECKED; }

    UiCheckBox& SetVisual(UiCheckVisual vis);
    UiCheckVisual GetVisual() const { return visual_; }

    UiCheckBox& SetIndicatorSide(UiAlign side);
    UiCheckBox& SetIndicatorRadius(int px);
    UiCheckBox& SetIndicatorRoundness(int percent);

    UiCheckBox& SetSizeMin(Size sz)        { SetMinSize(sz); return *this; }
    UiCheckBox& SetSizeMin(int cx, int cy) { return SetSizeMin(Size(cx, cy)); }
    UiCheckBox& SetSizeFixed(Size sz)      { return SetSizeMin(sz); }
    UiCheckBox& SetSizeFixed(int cx, int cy) { return SetSizeMin(Size(cx, cy)); }

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
    virtual Value GetData() const override;

private:
    void InvalidateStyleCache();
    Style& StyleEdit();
    void SyncThemeStyle();
    const Style& GetEffectiveStyle() const;
    Size GetTextSizeCached() const;
    void RebuildLayoutCache(const Rect& content) const;
    UiCheckBox& SetStateInternal(UiCheckState st, bool fire_action);

    Rect GetIndicatorRect(const Rect& r) const;
    Rect GetTextRect(const Rect& r, const Rect& ind) const;
    void Toggle_();

private:
    Style style_;
    mutable Style themed_style_;
    mutable uint64 theme_revision_ = 0;
    bool has_style_override_ = false;
    UiCheckVisual visual_ = UICHECKVIS_CLASSIC;
    String text_;
    UiCheckState state_ = UICHECK_UNCHECKED;
    bool tri_state_ = false;

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





