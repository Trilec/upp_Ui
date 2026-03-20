#ifndef _Ui_UiLabel_h_
#define _Ui_UiLabel_h_

/*
    UiLabel
    =======

    Purpose
    - Styled text and icon label for display-only content.

    Intent
    - Serve as the baseline text/content presentation control for the Ui family,
      with cached layout and render-only painting.

    Thread context
    - GUI thread only.

    Usage
    - Use SetText(), SetIcon(), and SetStyle() to configure static content.
    - Use GetData()/SetData() only for generic control binding scenarios.

    Changelog
    - 2026-03: added release-standard header documentation.
*/

#include <CtrlCore/CtrlCore.h>
#include <CtrlLib/CtrlLib.h>
#include <Ui/UiStyle.h>
#include <Ui/UiDraw.h>

namespace Upp {

class UiLabel : public Ctrl, public CtrlStyled<UiLabel> {
public:
    typedef UiLabel CLASSNAME;

    struct Span : Moveable<Span> {
        enum Kind {
            TEXT,
            ICON,
            BULLET,
            NEWLINE
        } kind = TEXT;

        String text;
        Image  icon;
        bool   mono_icon = false;

        Color  ink;
        Color  bg;
        Font   font;
        bool   bold = false;
        bool   italic = false;
        bool   underline = false;

        int    bullet_size = DPI(6);
        Color  bullet_color;
    };

    struct Style : ChStyle<Style> {
        StyledPalette palette;
        StyledMetrics metrics;
        StyledSkin    skin;

        UiAlign align_h = UiAlign::LEFT;
        UiAlign align_v = UiAlign::CENTER;
        UiAlign icon_layout = UiAlign::LEFT;

        Rect icon_margin = Rect(DPI(2), 0, DPI(4), 0);
        Rect text_margin = Rect(0, 0, 0, 0);

        Font font = StdFont();
        bool transparent = true;

        bool underline = false;
        int  underline_width = DPI(1);
        int  underline_offset = 0;

        bool nowrap = false;

        void Serialize(Stream& s)
        {
            s % palette % metrics % skin
              % align_h % align_v % icon_layout
              % icon_margin % text_margin
              % font % transparent
              % underline % underline_width % underline_offset
              % nowrap;
        }
    };

private:
    Style  style_;
    mutable Style themed_style_;
    mutable uint64 theme_revision_ = 0;
    bool   has_style_override_ = false;

    String text_;
    wchar  accesskey_ = 0;
    bool   has_access_mnemonic_ = false;

    Image  icon_;
    bool   mono_icon_ = false;
    bool   icon_scale_ = true;

    Vector<String> lines_;
    Vector<Size>   line_sizes_;

    Size user_min_size_ = Size(0, 0);

    mutable bool minsize_dirty_ = true;
    mutable Size cached_minsize_ = Size(0, 0);
    mutable bool layout_dirty_ = true;
    mutable UiBlocksLayout layout_;
    mutable Rect layout_content_ = Rect(0, 0, 0, 0);

    bool selectable_text_ = false;
    int  sel_anchor_ = -1;
    int  sel_caret_ = -1;
    bool all_selected_ = false;
    bool selecting_drag_ = false;

    bool rich_enabled_ = false;
    Vector<Span> spans_;

    void InvalidateStyleCache();
    Style& StyleEdit();
    void SyncThemeStyle();
    void RebuildTextLines();
    void RebuildTextLinesFromStyle(const Style& st);
    String BuildPlainTextFromSpans() const;
    Size ComputeNaturalSize() const;
    Size GetTextBlockSize() const;
    void UpdateLayout(const Rect& content) const;
    void CopySelectionToClipboard() const;
    bool HasSelection() const;
    int  SelFrom() const;
    int  SelTo() const;
    int  HitTestTextPos(Point p) const;

public:
    UiLabel();

    UiLabel& SetText(const String& text);
    const String& GetText() const { return text_; }

    UiLabel& EnableRich(bool on = true);
    bool     IsRichEnabled() const { return rich_enabled_; }

    UiLabel& SetSpans(const Vector<Span>& spans);
    UiLabel& ClearSpans();
    UiLabel& AddSpan(const Span& span);
    UiLabel& SetSpan(int i, const Span& span);
    int      GetSpanCount() const { return spans_.GetCount(); }
    const Span& GetSpan(int i) const { return spans_[i]; }

    UiLabel& AddTextSpan(const String& text, Color ink = Null, bool bold = false, bool italic = false, bool underline = false);
    UiLabel& AddIconSpan(const Image& icon, bool mono = false);
    UiLabel& AddBulletSpan(Color color = Null, int size = DPI(6));
    UiLabel& AddNewlineSpan();

    UiLabel& SetAnsiText(const String& ansi);
    UiLabel& AppendAnsiText(const String& ansi);

    UiLabel& SetIcon(const Image& img);
    UiLabel& SetMonoIcon(const Image& img);
    UiLabel& ClearIcon();
    Image    GetIcon() const { return icon_; }

    UiLabel& SetIconLayout(UiAlign where);
    UiLabel& SetAlign(UiAlign h, UiAlign v);
    UiLabel& SetAlignH(UiAlign h);
    UiLabel& SetAlignV(UiAlign v);

    UiLabel& SetIconMargin(const Rect& m);
    UiLabel& SetIconMargin(int l, int t, int r, int b) { return SetIconMargin(Rect(l, t, r, b)); }
    UiLabel& SetIconMargin(int all) { return SetIconMargin(all, all, all, all); }
    Rect     GetIconMargin() const { return GetEffectiveStyle().icon_margin; }

    UiLabel& SetTextMargin(const Rect& m);
    UiLabel& SetTextMargin(int l, int t, int r, int b) { return SetTextMargin(Rect(l, t, r, b)); }
    UiLabel& SetTextMargin(int all) { return SetTextMargin(all, all, all, all); }
    Rect     GetTextMargin() const { return GetEffectiveStyle().text_margin; }

    UiLabel& SetIconScale(bool on = true)
    {
        icon_scale_ = on;
        Refresh();
        return *this;
    }
    bool IsIconScaled() const { return icon_scale_; }

    UiLabel& SetIconColor(Color base, int hot_pct = 0, int press_pct = 0)
    {
        CtrlStyled<UiLabel>::SetIconColor(base, hot_pct, press_pct);
        return *this;
    }

    UiLabel& SetStyle(const Style& s);
    UiLabel& ClearStyleOverride();
    bool     HasStyleOverride() const { return has_style_override_; }
    const Style& GetStyle() const { return GetEffectiveStyle(); }
    const Style& GetEffectiveStyle() const;
    static const Style& StyleDefault();

    UiLabel& SetUnderline(bool on = true, int thickness = DPI(1), int offset = 0);
    UiLabel& SetSelectable(bool on = true);
    bool     IsSelectable() const { return selectable_text_; }

    StyledPalette& StyledPaletteRef() { return StyleEdit().palette; }
    StyledMetrics& StyledMetricsRef() { return StyleEdit().metrics; }
    StyledSkin&    StyledSkinRef()    { return StyleEdit().skin; }

    void OnStyleChanged();

    Size GetMinSize() const override;
    void Layout() override;

    UiLabel& SetSizeMin(Size sz);
    UiLabel& SetSizeMin(int cx, int cy) { return SetSizeMin(Size(cx, cy)); }

    UiLabel& SetSizeFixed(Size sz)        { return SetSizeMin(sz); }
    UiLabel& SetSizeFixed(int cx, int cy) { return SetSizeMin(Size(cx, cy)); }

    Event<Draw&, const Rect&, const StyledPalette&, const StyledMetrics&, const StyledSkin&, StyledState, bool> WhenPaintBackground;
    Event<Draw&, const Rect&, const StyledPalette&, const StyledMetrics&, const StyledSkin&, StyledState, bool> WhenPaintForeground;

    void   Paint(Draw& w) override;
    void   LeftDown(Point p, dword flags) override;
    void   MouseMove(Point p, dword flags) override;
    void   LeftUp(Point p, dword flags) override;
    bool   Key(dword key, int count) override;
    void   GotFocus() override;
    void   LostFocus() override;

    String GetDesc() const override;
    dword  GetAccessKeys() const override;
    void   AssignAccessKeys(dword used) override;

    void  SetData(const Value& v) override { SetText(AsString(v)); }
    Value GetData() const override         { return text_; }
};

} // namespace Upp

#endif


