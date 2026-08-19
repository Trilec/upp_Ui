#ifndef _Ui_UiButton_h_
#define _Ui_UiButton_h_

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    UiButton
    ========

    Purpose
    - General-purpose clickable command control with shared styled button behavior.

    Intent
    - Own the canonical button interaction, cache, and paint pipeline used by
      button-like controls.
    - Keep theme resolution overridable so specializations such as UiToolButton
      can reuse behavior without duplicating the implementation.

    Thread context
    - GUI thread only.

    Usage
    - Use SetText(), SetIcon(), SetIconRenderMode(), SetContentInset(), SetContentGap(), SetCheckable(), and
      SetCustomStyle() to configure button behavior and appearance.
    - SetPressedContentOffset() controls paint-only displacement of icon/text
      while pressed; it does not change layout, control bounds, or hit geometry.
    - Pointer interaction defaults to the styled surface rather than the full
      outer paint bounds, so procedural outer shadows are decorative only.
      SetInteractionInset() can exclude additional baked image/9-slice decoration.
    - Setters and style/theme changes drive cache invalidation and layout
      refresh; Paint() also repairs stale layout before drawing.

    Changelog
    - 2026-03: hardened as the shared button behavior base for UiButton and
      UiToolButton while preserving per-control theme resolution.
    - 2026-04: adopted shared UiIconRenderMode instead of button-local mono
      tint state so icon rendering policy matches the wider Ui style system.
    - 2026-04: added explicit icon size control and removed button-local icon
      scale toggling so icon rendering uses the target icon size directly.
    - 2026-04: simplified public content spacing to outer content_margin plus
      one content_gap so buttons no longer expose separate icon/text margins.
    - 2026-05: exposed explicit content inset setters so split-button and
      other button-like controls can tune body padding without style hacks.
    - 2026-08: documented press_offset as paint-only pressed-content feedback
      and exposed SetPressedContentOffset()/GetPressedContentOffset() so callers
      need not infer its semantics from the Style field name.
    - 2026-08: hardened pointer interaction to follow UiStyledSurfaceRect(),
      excluding procedural outer shadows by default and allowing an explicit
      additional interaction inset for baked decorative pixels.
*/

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

        // Paint-only feedback applied to the icon/text block while the button
        // is in its pressed visual state. This does NOT move the styled face,
        // participate in layout/min-size calculation, or alter pointer hit
        // geometry. The storage name is retained for style/source compatibility;
        // new control code should prefer SetPressedContentOffset().
        Point press_offset = Point(0, 0);

        int   overpaint    = 0;

        Font  font         = StdFont();
        bool  transparent  = false;

        UiAlign align_h = UiAlign::CENTER;
        UiAlign align_v = UiAlign::CENTER;
        UiAlign icon_side = UiAlign::LEFT;
        int     content_gap = DPI(4);

        Image icon_images[4];
        UiIconRenderMode icon_render_mode = UiIconRenderMode::MonoTint;

        bool underline        = false;
        int  underline_width  = DPI(1);
        int  underline_offset = DPI(1);

        void Serialize(Stream& s)
        {
            s % palette % metrics % skin
              % press_offset % overpaint
              % font % transparent
              % align_h % align_v % icon_side
              % content_gap;

            for(int i = 0; i < 4; i++)
                s % icon_images[i];

            s % icon_render_mode;
            s % underline % underline_width % underline_offset;
        }
    };

protected:
    Style  style_;
    mutable Style themed_style_;
    mutable uint64 theme_revision_ = 0;
    bool   has_custom_style_ = false;

    String text_;
    wchar  accesskey_ = 0;
    bool   has_access_mnemonic_ = false;
    bool   click_focus_ = true;

    bool checkable_ = false;
    bool checked_   = false;

    bool        mouse_over_   = false;
    bool        pressed_      = false;
    StyledState visual_state_ = ST_NORMAL;

    // Additional non-negative thickness removed from the styled surface for
    // pointer interaction. This is deliberately control state rather than
    // Style serialization: procedural shadow exclusion is automatic, while
    // baked decorative pixels are a control/asset integration choice.
    Rect interaction_inset_ = Rect(0, 0, 0, 0);

	Size           user_min_size_ = Size(0, 0);
	One<Animation> anim_;
	Size           icon_size_ = Size(0, 0);
	bool           icon_scale_to_content_ = false;
	Image          assigned_icon_images_[4];
	UiIconRenderMode assigned_icon_render_mode_ = UiIconRenderMode::MonoTint;
	bool           has_assigned_icon_render_mode_ = false;
    Color          assigned_icon_colors_[4];
    bool           has_assigned_icon_colors_ = false;

    Vector<String> lines_;
    Vector<Size>   line_sizes_;

    // Cached layout derived from text/icon blocks and styled content insets.
    mutable UiBlocksLayout layout_;
    mutable bool           minsize_dirty_ = true;
    mutable bool           layout_dirty_  = true;
    mutable Size           cached_minsize_;
    Rect                   layout_content_;

    void UpdateVisualState();
    void RebuildLook();
    void Activate_();

    // Theme/style invalidation helpers centralize cache resets for subclasses.
    void InvalidateStyleCache();
    Style& StyleEdit();
    void SyncThemeStyle();

    virtual Color AdjustInk(Color base_ink, StyledState st) const { return base_ink; }
    virtual Style ResolveThemeStyle() const;
    virtual Rect GetContentLayoutRect(const Rect& outer, const Style& style) const;

    // Text and block layout helpers feed GetMinSize() and Paint().
    void RebuildTextLines();
    void RebuildTextLinesFromStyle(const Style& st);
    Size GetTextBlockSize() const;
    Size GetStableIconSize() const;
    bool HasResolvedIcon() const;
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

	UiButton& SetIconSize(Size sz) { icon_size_ = sz; minsize_dirty_ = true; layout_dirty_ = true; RefreshLayout(); Refresh(); return *this; }
	UiButton& SetIconSize(int cx, int cy) { return SetIconSize(Size(cx, cy)); }
	Size      GetIconSize() const { return icon_size_; }
	UiButton& SetIconScaleToContent(bool on = true) { icon_scale_to_content_ = on; minsize_dirty_ = true; layout_dirty_ = true; layout_content_ = Rect(0, 0, 0, 0); RefreshLayout(); Refresh(); return *this; }
	bool      IsIconScaleToContent() const { return icon_scale_to_content_; }

	UiButton& SetIconRenderMode(UiIconRenderMode mode) { has_assigned_icon_render_mode_ = true; assigned_icon_render_mode_ = mode; Refresh(); return *this; }
    UiIconRenderMode GetIconRenderMode() const { return has_assigned_icon_render_mode_ ? assigned_icon_render_mode_ : GetEffectiveStyle().icon_render_mode; }

    UiButton& SetIconColor(Color base, int hot_pct = 0, int press_pct = 0)
    {
        has_assigned_icon_colors_ = true;
        assigned_icon_colors_[ST_NORMAL] = base;
        assigned_icon_colors_[ST_HOT] = hot_pct ? LtColor(base, hot_pct) : base;
        assigned_icon_colors_[ST_PRESSED] = press_pct ? DkColor(base, press_pct) : base;
        assigned_icon_colors_[ST_DISABLED] = DisabledColor(base);
        Refresh();
        return *this;
    }

    UiButton& SetIconSide(UiAlign side);

    UiButton& SetAlign(UiAlign h, UiAlign v);
    UiButton& SetAlignH(UiAlign h);
    UiButton& SetAlignV(UiAlign v);

    UiButton& SetContentGap(int gap);
    int       GetContentGap() const { return max(0, GetEffectiveStyle().content_gap); }
    UiButton& SetContentInset(const Rect& inset);
    UiButton& SetContentInset(int all) { return SetContentInset(Rect(all, all, all, all)); }
    Rect      GetContentInset() const { return GetEffectiveStyle().metrics.content_margin; }

    // Visual press feedback only: moves icon/text while ST_PRESSED is painted.
    // It intentionally does not affect the button face, layout, or hit bounds.
    UiButton& SetPressedContentOffset(Point offset)
    {
        StyleEdit().press_offset = offset;
        Refresh();
        return *this;
    }
    UiButton& SetPressedContentOffset(int x, int y) { return SetPressedContentOffset(Point(x, y)); }
    Point     GetPressedContentOffset() const { return GetEffectiveStyle().press_offset; }

    // Pointer activation follows the painted styled surface, excluding any
    // procedural outer shadow reserved by StyledMetrics. interaction_inset is
    // an additional l/t/r/b thickness for image/9-slice assets that contain
    // decorative pixels inside that surface rectangle.
    UiButton& SetInteractionInset(const Rect& inset)
    {
        interaction_inset_ = UiNonNegativeThickness(inset);
        Refresh();
        return *this;
    }
    UiButton& SetInteractionInset(int all) { return SetInteractionInset(Rect(all, all, all, all)); }
    Rect      GetInteractionInset() const { return interaction_inset_; }
    Rect      GetInteractionRect() const
    {
        Rect r = UiStyledSurfaceRect(Rect(GetSize()), GetEffectiveStyle().metrics);
        if(!UiIsZeroThicknessRect(interaction_inset_))
            r = UiApplyThicknessRect(r, interaction_inset_);
        return r;
    }
    bool      IsInteractionPoint(Point p) const { return GetInteractionRect().Contains(p); }

    UiButton& ClickFocus(bool on = true);

    UiButton& SetCheckable(bool on = true);
    bool      IsCheckable() const { return checkable_; }

    UiButton& SetChecked(bool on = true);
    bool      IsChecked() const { return checked_; }

    UiButton& Toggle() { return SetChecked(!checked_); }

    UiButton& SetCustomStyle(const Style& s);
    UiButton& ClearCustomStyle();
    bool      HasCustomStyle() const { return has_custom_style_; }
    const Style& GetStyle() const { return GetEffectiveStyle(); }
    const Style& GetCustomStyle() const { return style_; }
    const Style& GetEffectiveStyle() const;
    static const Style& StyleDefault();

    UiButton& SetUnderline(bool on = true, int thickness = DPI(1), int offset = 0);

    StyledPalette& StyledPaletteRef() { return StyleEdit().palette; }
    StyledMetrics& StyledMetricsRef() { return StyleEdit().metrics; }
    StyledSkin&    StyledSkinRef()    { return StyleEdit().skin; }
    void OnStyleChanged();

    StyledPalette& StylePalette() { return StyleEdit().palette; }
    StyledMetrics& StyleMetrics() { return StyleEdit().metrics; }
    StyledSkin&    StyleSkin()    { return StyleEdit().skin; }

    template <class T>
    UiButton& Animate(const T& from, const T& to, int ms, Event<const T&> setter,
                      Easing::Fn curve = Easing::OutCubic(), Event<> on_finish = {});

    // Button actions remain synchronous. Callers that rebuild surrounding UI
    // should defer heavy refresh themselves if they are inside sensitive input
    // paths.
    Event<> WhenPush;
    Event<> WhenAction;

    Event<Draw&, const Rect&, const StyledPalette&, const StyledMetrics&, const StyledSkin&, StyledState, bool> WhenPaintBackground;
    Event<Draw&, const Rect&, const StyledPalette&, const StyledMetrics&, const StyledSkin&, StyledState, bool> WhenPaintForeground;

    virtual void Paint(Draw& w) override;
    virtual void Layout() override;

    virtual void LeftDown(Point p, dword keyflags) override;
    virtual void LeftUp(Point p, dword keyflags) override;
    virtual void MouseEnter(Point p, dword keyflags) override;
    virtual void MouseMove(Point p, dword keyflags) override;
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
