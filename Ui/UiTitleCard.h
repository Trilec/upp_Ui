#ifndef _Ui_UiTitleCard_h_
#define _Ui_UiTitleCard_h_

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

        UiAlign text_align_h = UiAlign::LEFT;
        UiAlign media_side   = UiAlign::RIGHT; // LEFT/RIGHT/TOP/BOTTOM
        UiAlign media_align_h = UiAlign::CENTER;
        UiAlign media_align_v = UiAlign::CENTER;

        int media_reserve = DPI(72);
        int media_share_percent = 0; // 0 = disabled, otherwise 10..90 of content axis
        int media_gap     = DPI(10);
        int media_min     = DPI(24);
        bool preserve_media_aspect = true;

        bool show_rule = true;
        UiLineStyle rule_style = SOLID;
        UiSpan rule_extent = LARGE;
        int  rule_thickness = DPI(1);
        int  rule_gap_above = DPI(5);
        int  rule_gap_below = DPI(5);

        bool show_bottom_line = false;
        UiLineStyle bottom_line_style = SOLID;
        UiSpan bottom_line_extent = LARGE;
        int  bottom_line_thickness = DPI(1);
        Color bottom_line_color;

        int title_subtitle_gap = DPI(3);
        int subtitle_copy_gap  = DPI(4);

        bool transparent = false;
        bool hover_enabled = false;

        void Serialize(Stream& s)
        {
            int rs = (int)rule_style;
            int re = (int)rule_extent;
            int bls = (int)bottom_line_style;
            int ble = (int)bottom_line_extent;
            s % palette % metrics % skin
              % title_font % subtitle_font % copy_font
              % text_align_h % media_side % media_align_h % media_align_v
              % media_reserve % media_share_percent % media_gap % media_min % preserve_media_aspect
              % show_rule % rs % re % rule_thickness % rule_gap_above % rule_gap_below
              % show_bottom_line % bls % ble % bottom_line_thickness % bottom_line_color
              % title_subtitle_gap % subtitle_copy_gap
              % transparent % hover_enabled;
            rule_style = (UiLineStyle)rs;
            rule_extent = (UiSpan)re;
            bottom_line_style = (UiLineStyle)bls;
            bottom_line_extent = (UiSpan)ble;
        }
    };

    UiTitleCard();

    static const Style& StyleDefault();

    UiTitleCard& SetStyle(const Style& s);
    UiTitleCard& ClearStyleOverride();
    bool         HasStyleOverride() const { return has_style_override_; }
    const Style& GetStyle() const { return GetEffectiveStyle(); }

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

    UiTitleCard& SetRuleStyle(UiLineStyle st);
    UiTitleCard& SetRuleExtent(UiSpan ex);
    UiTitleCard& SetBottomLine(UiSpan ex, int thickness = 1, UiLineStyle style = SOLID, Color c = Null);
    UiTitleCard& ShowRule(bool on = true);
    UiTitleCard& ShowBottomLine(bool on = true);
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

    void DrawRule(Draw& w, int x, int y, int cx, Color c, int thickness, UiLineStyle style) const;
    int  GetRuleWidth(int title_cx, int text_cx) const;
    Size GetTextBlockSize() const;
    Rect GetMediaRect(const Rect& content) const;

private:
    Style  style_;
    mutable Style themed_style_;
    mutable uint64 theme_revision_ = 0;
    bool has_style_override_ = false;
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
