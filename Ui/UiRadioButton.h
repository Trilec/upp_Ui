#ifndef _Ui_UiRadioButton_h_
#define _Ui_UiRadioButton_h_

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

        UiRadioVisual visual = UIRADIOVIS_CLASSIC;

        void Serialize(Stream& s)
        {
            int vis = (int)visual;
            s % palette % metrics % skin
              % indicator_palette % indicator_metrics % indicator_skin
              % font % indicator_side % indicator_size % indicator_gap
              % vis;
            visual = (UiRadioVisual)vis;
        }
    };

    static const Style& StyleDefault();
    static const Style& StyleClassic();
    static const Style& StylePills();
    static const Style& StyleList();

    UiRadioButton();

    UiRadioButton& SetStyle(const Style& s);
    const Style& GetStyle() const { return style_; }

    StyledPalette& StyledPaletteRef() { return style_.palette; }
    StyledMetrics& StyledMetricsRef() { return style_.metrics; }
    StyledSkin&    StyledSkinRef()    { return style_.skin; }
    void OnStyleChanged();

    UiRadioButton& SetText(const String& s);
    const String& GetText() const { return text_; }

    UiRadioButton& SetChecked(bool on = true);
    bool IsChecked() const { return checked_; }

    UiRadioButton& SetGroup(int g) { group_ = g; return *this; }
    int GetGroup() const { return group_; }

    UiRadioButton& SetVisual(UiRadioVisual vis);
    UiRadioButton& SetIndicatorSide(UiAlign side);

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
    Size GetTextSizeCached() const;
    void RebuildLayoutCache(const Rect& content) const;
    UiRadioButton& SetCheckedInternal(bool on, bool fire_action);

private:
    void UncheckSiblings_();
    Rect GetIndicatorRect(const Rect& r) const;
    Rect GetTextRect(const Rect& r, const Rect& ind) const;

private:
    Style style_;
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
