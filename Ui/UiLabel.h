#ifndef _Ui_UiLabel_h_
#define _Ui_UiLabel_h_

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.
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
    - Use SetText(), SetIcon(), SetMargin(), SetContentGap(), and SetCustomStyle() to configure static content.
    - Use GetData()/SetData() only for generic control binding scenarios.

    Changelog
    - 2026-03: added release-standard header documentation.
    - 2026-04: aligned icon and rich-span icon rendering with shared
      UiIconRenderMode.
    - 2026-04: replaced label icon scaling with explicit icon sizing so
      layout and paint use the requested icon size directly.
    - 2026-04: simplified public content spacing to outer content_margin plus
      one content_gap so labels no longer expose separate icon/text margins.
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
        UiIconRenderMode icon_render_mode = UiIconRenderMode::PreserveColor;

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
        UiAlign icon_side = UiAlign::LEFT;
        int     content_gap = DPI(6);

        Font font = StdFont();
        bool transparent = true;

        bool underline = false;
        int  underline_width = DPI(1);
        int  underline_offset = 0;

        bool nowrap = false;

        void Serialize(Stream& s)
        {
            s % palette % metrics % skin
              % align_h % align_v % icon_side
              % content_gap
              % font % transparent
              % underline % underline_width % underline_offset
              % nowrap;
        }
    };

private:
    Style  style_;
    mutable Style themed_style_;
    mutable uint64 theme_revision_ = 0;
    bool   has_custom_style_ = false;

    String text_;
    wchar  accesskey_ = 0;
    bool   has_access_mnemonic_ = false;

    Image  icon_;
    UiIconRenderMode icon_render_mode_ = UiIconRenderMode::PreserveColor;
    Size   icon_size_ = Size(0, 0);

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
    Size GetStableIconSize() const;
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
    UiLabel& AddIconSpan(const Image& icon, UiIconRenderMode render_mode = UiIconRenderMode::PreserveColor);
    UiLabel& AddBulletSpan(Color color = Null, int size = DPI(6));
    UiLabel& AddNewlineSpan();

    UiLabel& SetAnsiText(const String& ansi);
    UiLabel& AppendAnsiText(const String& ansi);

    UiLabel& SetIcon(const Image& img);
    UiLabel& SetIcon(const Image& img, UiIconRenderMode render_mode);
    UiLabel& SetIconRenderMode(UiIconRenderMode render_mode);
    UiLabel& ClearIcon();
    Image    GetIcon() const { return icon_; }
    UiLabel& SetIconSize(Size sz);
    UiLabel& SetIconSize(int cx, int cy) { return SetIconSize(Size(cx, cy)); }
    Size     GetIconSize() const { return icon_size_; }

    UiLabel& SetIconSide(UiAlign where);
    UiLabel& SetAlign(UiAlign h, UiAlign v);
    UiLabel& SetAlignH(UiAlign h);
    UiLabel& SetAlignV(UiAlign v);

    UiLabel& SetContentGap(int gap);
    int      GetContentGap() const { return max(0, GetEffectiveStyle().content_gap); }

    UiLabel& SetIconColor(Color base, int hot_pct = 0, int press_pct = 0)
    {
        CtrlStyled<UiLabel>::SetIconColor(base, hot_pct, press_pct);
        return *this;
    }

    UiLabel& SetCustomStyle(const Style& s);
    UiLabel& ClearCustomStyle();
    bool     HasCustomStyle() const { return has_custom_style_; }
    const Style& GetStyle() const { return GetEffectiveStyle(); }
    const Style& GetCustomStyle() const { return style_; }
    const Style& GetEffectiveStyle() const;
    static const Style& StyleDefault();

    UiLabel& SetUnderline(bool on = true, int thickness = DPI(1), int offset = 0);
    UiLabel& SetSelectable(bool on = true);
    bool     IsSelectable() const { return selectable_text_; }

    StyledPalette& StyledPaletteRef() { return StyleEdit().palette; }
    StyledMetrics& StyledMetricsRef() { return StyleEdit().metrics; }
    StyledSkin&    StyledSkinRef()    { return StyleEdit().skin; }

    void OnStyleChanged();

    Size GetContentSize() const;

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

