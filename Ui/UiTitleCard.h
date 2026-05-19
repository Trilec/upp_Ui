#ifndef _Ui_UiTitleCard_h_
#define _Ui_UiTitleCard_h_

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.
    UiTitleCard
    ===========

    Purpose
    - Styled title/header card for labeled information blocks.

    Intent
    - Provide one shared header-oriented surface for cards, accordion headers,
      and other title/subtitle/copy compositions.

    Thread context
    - GUI thread only.

    Usage
    - Use SetTitle(), SetSubTitle(), SetCopy(), and media helpers to build
      display-only header compositions.

    Changelog
    - 2026-03: added release-standard header documentation.
*/

#include <CtrlCore/CtrlCore.h>
#include <Ui/UiStyle.h>
#include <Ui/UiDraw.h>

namespace Upp {

class UiTitleCard : public Ctrl, public CtrlStyled<UiTitleCard> {
public:
    typedef UiTitleCard CLASSNAME;

    struct Style : ChStyle<Style> {
        StyledPalette palette;
        StyledMetrics metrics;
        StyledSkin    skin;

        Font title_font    = SansSerifZ(20).Bold();
        Font subtitle_font = SansSerifZ(13).Bold();
        Font copy_font     = SansSerifZ(11);
        Color title_color;
        Color subtitle_color;
        Color copy_color;

        UiAlign text_align_h = UiAlign::LEFT;
        UiAlign media_side   = UiAlign::LEFT; // LEFT/RIGHT/TOP/BOTTOM
        UiAlign media_align_h = UiAlign::CENTER;
        UiAlign media_align_v = UiAlign::CENTER;

        int media_reserve = DPI(72);
        int media_share_percent = 0; // 0 = disabled, otherwise 10..90 of content axis
        int media_gap     = DPI(10);
        int media_min     = DPI(24);
        bool media_auto_fit = true;
        bool preserve_media_aspect = true;
        bool media_tint_mono = false;

        bool title_line = true;
        UiLineStyle title_line_style = SOLID;
        UiSpan title_line_length = LARGE;
        int  title_line_thickness = DPI(1);
        Color title_line_color;
        int  title_line_gap_above = DPI(5);
        int  title_line_gap_below = DPI(5);

        bool card_line = false;
        UiLineStyle card_line_style = SOLID;
        UiSpan card_line_length = LARGE;
        int  card_line_thickness = DPI(1);
        Color card_line_color;

        int title_subtitle_gap = DPI(3);
        int subtitle_copy_gap  = DPI(4);

        bool transparent = false;
        bool hover_enabled = false;

        void Serialize(Stream& s)
        {
            int rs = (int)title_line_style;
            int re = (int)title_line_length;
            int bls = (int)card_line_style;
            int ble = (int)card_line_length;
            s % palette % metrics % skin
              % title_font % subtitle_font % copy_font
              % title_color % subtitle_color % copy_color
              % text_align_h % media_side % media_align_h % media_align_v
              % media_reserve % media_share_percent % media_gap % media_min % media_auto_fit % preserve_media_aspect % media_tint_mono
              % title_line % rs % re % title_line_thickness % title_line_color % title_line_gap_above % title_line_gap_below
              % card_line % bls % ble % card_line_thickness % card_line_color
              % title_subtitle_gap % subtitle_copy_gap
              % transparent % hover_enabled;
            title_line_style = (UiLineStyle)rs;
            title_line_length = (UiSpan)re;
            card_line_style = (UiLineStyle)bls;
            card_line_length = (UiSpan)ble;
        }
    };

    UiTitleCard();

    static const Style& StyleDefault();

    UiTitleCard& SetCustomStyle(const Style& s);
    UiTitleCard& ClearCustomStyle();
    bool         HasCustomStyle() const { return has_custom_style_; }
    const Style& GetStyle() const { return GetEffectiveStyle(); }
    const Style& GetCustomStyle() const { return style_; }

    StyledPalette& StyledPaletteRef() { return StyleEdit().palette; }
    StyledMetrics& StyledMetricsRef() { return StyleEdit().metrics; }
    StyledSkin&    StyledSkinRef()    { return StyleEdit().skin; }
    void OnStyleChanged();

    UiTitleCard& SetTitle(const String& s);
    UiTitleCard& SetSubTitle(const String& s);
    UiTitleCard& SetCopyText(const String& s);

    UiTitleCard& SetMedia(const Image& img, Size preferred = Size(0, 0));
    UiTitleCard& ClearMedia();
    UiTitleCard& SetMediaSide(UiAlign side);
    UiTitleCard& SetMediaAlign(UiAlign h, UiAlign v);
    UiTitleCard& SetMediaReserve(int px);
    UiTitleCard& SetMediaSharePercent(int pct);
    UiTitleCard& SetMediaAutoFit(bool on = true);

    UiTitleCard& SetTitleLine(UiSpan ex, int thickness = 1, UiLineStyle style = SOLID, Color c = Null);
    UiTitleCard& SetCardLine(UiSpan ex, int thickness = 1, UiLineStyle style = SOLID, Color c = Null);
    UiTitleCard& ShowTitleLine(bool on = true);
    UiTitleCard& ShowCardLine(bool on = true);
    UiTitleCard& EnableHover(bool on = true);

    UiTitleCard& SetSelectable(bool on = true);

    UiTitleCard& SetSizeMin(Size sz) { user_min_size_ = sz; RefreshLayout(); Refresh(); return *this; }
    UiTitleCard& SetSizeMin(int cx, int cy) { return SetSizeMin(Size(cx, cy)); }

    virtual Size GetMinSize() const override;
    virtual void Paint(Draw& w) override;
    virtual void MouseEnter(Point, dword) override;
    virtual void MouseLeave() override;
    virtual void LeftDown(Point, dword) override;
    virtual void LeftUp(Point, dword) override;

    Event<Draw&, const Rect&,
          const StyledPalette&, const StyledMetrics&, const StyledSkin&,
          StyledState, bool> WhenPaintBackground;

    Event<Draw&, const Rect&,
          const StyledPalette&, const StyledMetrics&, const StyledSkin&,
          StyledState, bool> WhenPaintForeground;

private:
    void InvalidateStyleCache();
    Style& StyleEdit();
    void SyncThemeStyle();
    const Style& GetEffectiveStyle() const;
    void InvalidateTextCache();
    void RebuildTextCache();

    void DrawLine(Draw& w, int x, int y, int cx, Color c, int thickness, UiLineStyle style) const;
    int  GetLineWidth(UiSpan length, int title_cx, int text_cx) const;
    Size GetTextBlockSize() const;
    Rect GetMediaRect(const Rect& content) const;

private:
    Style  style_;
    mutable Style themed_style_;
    mutable uint64 theme_revision_ = 0;
    bool has_custom_style_ = false;
    String title_;
    String subtitle_;
    String copy_;

    Image media_;
    Size  media_pref_ = Size(0, 0);

    bool hot_ = false;
    bool down_ = false;

    Size user_min_size_ = Size(0, 0);

    mutable bool text_metrics_dirty_ = true;
    mutable Size title_size_ = Size(0, 0);
    mutable Size subtitle_size_ = Size(0, 0);
    mutable Size copy_size_ = Size(0, 0);
    mutable Size text_block_size_ = Size(0, 0);
};

}

#endif

