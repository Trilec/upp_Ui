#ifndef _Ui_UiButton_h_
#define _Ui_UiButton_h_

#include <type_traits>

#include <Animation/Animation.h>
#include <CtrlCore/CtrlCore.h>
#include <CtrlLib/CtrlLib.h>
#include <Ui/UiStyle.h>
#include <Ui/UiDraw.h>

namespace Upp {

class UiButton : public Ctrl, public CtrlStyled<UiButton> {
public:
    typedef UiButton CLASSNAME;

    struct Style : ChStyle<Style> {
        StyledPalette palette;
        StyledMetrics metrics;
        StyledSkin    skin;

        Point press_offset = Point(1, 1);

        // Focus ring uses shared helper (UiPaintFocusRing).
        int   focus_margin = DPI(2);

        // Reserved for optional post-foreground overlays (tints/glass/etc).
        int   overpaint    = DPI(2);

        Font  font         = StdFont();
        bool  transparent  = false;

        // Alignment for the combined (icon+text) content block.
        UiAlign align_h = UiAlign::CENTER;
        UiAlign align_v = UiAlign::CENTER;

        // Two-block layout: support block (icon) stacked relative to main block (text).
        // Allowed: LEFT/RIGHT/TOP/BOTTOM.
        UiAlign icon_layout = UiAlign::LEFT;

        Rect icon_margin = Rect(DPI(1), DPI(1), DPI(1), DPI(1));
        Rect text_margin = Rect(DPI(2), 0, 0, 0);

        Image icon_images[4];
        bool  icon_tint_mono = true;

        bool underline        = false;
        int  underline_width  = DPI(1);
        int  underline_offset = DPI(1);

        void Serialize(Stream& s)
        {
            s % palette % metrics % skin
              % press_offset % focus_margin % overpaint
              % font % transparent
              % align_h % align_v % icon_layout
              % icon_margin % text_margin;

            for(int i = 0; i < 4; i++)
                s % icon_images[i];

            s % icon_tint_mono;

            s % underline % underline_width % underline_offset;
        }
    };

protected:
    Style  style_;
    String text_;
    wchar  accesskey_    = 0;   // explicit only (from & markup)
    bool has_access_mnemonic_ = false;
    bool   click_focus_  = true;

    // Toggle behaviour (optional)
    bool checkable_ = false;
    bool checked_   = false;

    bool        mouse_over_    = false;
    bool        pressed_       = false;
    StyledState visual_state_  = ST_NORMAL;

    Size           user_min_size_ = Size(0, 0);
    One<Animation> anim_;

    bool icon_scale_ = true;

    Vector<String> lines_;
    Vector<Size>   line_sizes_;

    mutable UiBlocksLayout layout_;
    mutable bool           minsize_dirty_ = true;
    mutable bool           layout_dirty_  = true;
    mutable Size           cached_minsize_;

    Rect layout_content_;

    void UpdateVisualState();
    void RebuildLook();

    void Activate_();

    virtual Color AdjustInk(Color base_ink, StyledState st) const { return base_ink; }

    void RebuildTextLines();
    Size GetTextBlockSize() const;

    Size GetStableIconSize() const;

    Size ComputeNaturalSize() const;
    void UpdateLayout(const Rect& content) const;

    Image ResolveIconForState(StyledState st) const;

public:
    UiButton();

    UiButton& SetText(const String& text);
    const String& GetText() const { return text_; }

    UiButton& SetIcon(const Image& img);
    UiButton& SetIconState(const Image& img, StyledState state);
    UiButton& SetIcons(const Image& normal,
                       const Image& hot      = Image(),
                       const Image& pressed  = Image(),
                       const Image& disabled = Image());
    UiButton& ClearIcon();

    UiButton& SetIconScale(bool on = true) { icon_scale_ = on; Refresh(); return *this; }
    bool      GetIconScale() const { return icon_scale_; }
    UiButton& SetIconTintMono(bool on = true) { style_.icon_tint_mono = on; Refresh(); return *this; }
    bool      GetIconTintMono() const { return style_.icon_tint_mono; }
    UiButton& SetIconColor(Color base, int hot_pct = 0, int press_pct = 0)
    {
        CtrlStyled<UiButton>::SetIconColor(base, hot_pct, press_pct);
        return *this;
    }

    UiButton& SetIconLayout(UiAlign layout);

    UiButton& SetAlign(UiAlign h, UiAlign v);
    UiButton& SetAlignH(UiAlign h);
    UiButton& SetAlignV(UiAlign v);

    UiButton& SetIconMargin(const Rect& m);
    UiButton& SetIconMargin(int l, int t, int r, int b) { return SetIconMargin(Rect(l, t, r, b)); }
    UiButton& SetIconMargin(int all) { return SetIconMargin(all, all, all, all); }
    Rect GetIconMargin() const { return style_.icon_margin; }

    UiButton& SetTextMargin(const Rect& m);
    UiButton& SetTextMargin(int l, int t, int r, int b) { return SetTextMargin(Rect(l, t, r, b)); }
    UiButton& SetTextMargin(int all) { return SetTextMargin(all, all, all, all); }
    Rect GetTextMargin() const { return style_.text_margin; }

    UiButton& ClickFocus(bool on = true);

    // Toggle (checkable) mode
    UiButton& SetCheckable(bool on = true);
    bool      IsCheckable() const { return checkable_; }

    UiButton& SetChecked(bool on = true);
    bool      IsChecked() const { return checked_; }

    UiButton& Toggle() { return SetChecked(!checked_); }

    UiButton& SetStyle(const Style& s);
    const Style& GetStyle() const { return style_; }
    static const Style& StyleDefault();

    static const Style& StyleAccent();
    static const Style& StyleSubtle();
    static const Style& StyleIcon();

    UiButton& SetAccentStyle() { return SetStyle(StyleAccent()); }
    UiButton& SetSubtleStyle() { return SetStyle(StyleSubtle()); }
    UiButton& SetIconStyle() {
        Style s = StyleIcon();
        for(int i = 0; i < 4; i++)
            s.icon_images[i] = style_.icon_images[i];
        return SetStyle(s);
    }

    UiButton& SetUnderline(bool on = true, int thickness = DPI(1), int offset = 0);

    StyledPalette& StyledPaletteRef() { return style_.palette; }
    StyledMetrics& StyledMetricsRef() { return style_.metrics; }
    StyledSkin&    StyledSkinRef()    { return style_.skin; }
    void OnStyleChanged();

    StyledPalette& StylePalette() { return style_.palette; }
    StyledMetrics& StyleMetrics() { return style_.metrics; }
    StyledSkin&    StyleSkin()    { return style_.skin; }

    template <class T>
    UiButton& Animate(const T& from, const T& to, int ms, Event<const T&> setter,
                      Easing::Fn curve = Easing::OutCubic(), Event<> on_finish = {});

    Event<> WhenPush;
    Event<> WhenAction;

    Event<Draw&, const Rect&, const StyledPalette&, const StyledMetrics&, const StyledSkin&, StyledState, bool> WhenPaintBackground;
    Event<Draw&, const Rect&, const StyledPalette&, const StyledMetrics&, const StyledSkin&, StyledState, bool> WhenPaintForeground;

    virtual void Paint(Draw& w) override;
    virtual void Layout() override;

    virtual void LeftDown(Point p, dword keyflags) override;
    virtual void LeftUp(Point p, dword keyflags) override;
    virtual void MouseEnter(Point p, dword keyflags) override;
    virtual void MouseLeave() override;
    virtual void GotFocus() override;
    virtual void LostFocus() override;
    virtual void CancelMode() override;
    virtual bool Key(dword key, int count) override;

    virtual Size GetMinSize() const override;
    virtual void SetMinSize(Size sz) override;

    virtual String GetDesc() const override;
    virtual dword GetAccessKeys() const override;
    virtual void AssignAccessKeys(dword used) override;

    virtual void SetData(const Value& v) override { SetText(AsString(v)); }
    virtual Value GetData() const override { return text_; }
};

// -----------------------------------------------------------------------------
// Template Implementation
// -----------------------------------------------------------------------------

template <class T>
UiButton& UiButton::Animate(const T& from, const T& to, int ms, Event<const T&> setter,
                            Easing::Fn curve, Event<> on_finish)
{
    if(!setter)
        return *this;

    if(anim_) {
        anim_->Cancel();
        anim_.Clear();
    }

    anim_.Create(*this);
    Animation& a = *anim_;

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
