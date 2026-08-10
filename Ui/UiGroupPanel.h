#ifndef _Ui_UiGroupPanel_h_
#define _Ui_UiGroupPanel_h_

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    UiGroupPanel
    ============

    Purpose
    - Public header for the UiGroupPanel component.

    Intent
    - Provide one framed identity/header surface with one optional header-content
      root and one optional body-content root. Each slot remains single-child;
      callers place a layout in a slot when they need multiple controls.

    Thread context
    - GUI thread only.

    Usage
    - Include this header where the component is used or extended. Keep implementation details in the matching .cpp when present.

    Changelog
    - 2026-06: normalized the top-level header documentation.
*/

#include <CtrlCore/CtrlCore.h>
#include <Ui/UiStyle.h>
#include <Ui/UiDraw.h>

namespace Upp {

class UiGroupPanel : public Ctrl, public CtrlStyled<UiGroupPanel> {
public:
    typedef UiGroupPanel CLASSNAME;

    enum HeaderMode : byte {
        Outside,
        Center,
        Inside
    };

    struct Style : ChStyle<Style> {
        StyledPalette palette;
        StyledMetrics metrics;
        StyledSkin    skin;

        Font title_font = SansSerifZ(10).Bold();
        Font subtitle_font = SansSerifZ(9);
        Color title_color;
        Color subtitle_color;

        UiAlign header_placement = UiAlign::TOP;
        UiAlign title_align_h = UiAlign::LEFT;
        UiAlign title_align_v = UiAlign::CENTER;
        HeaderMode header_mode = Inside;

        Rect inset = Rect(DPI(8), DPI(8), DPI(8), DPI(8));
        Rect header_inset = Rect(DPI(8), DPI(4), DPI(8), DPI(4));
        int header_gap = DPI(4);
        int icon_size = DPI(16);
        int icon_gap = DPI(5);
        int title_subtitle_gap = DPI(1);
        int separator_thickness = DPI(1);
        bool line_enabled = false;
        bool header_band_enabled = false;
        bool transparent = false;

        void Serialize(Stream& s);
    };

    UiGroupPanel();

    static const Style& StyleDefault();
    UiGroupPanel& SetCustomStyle(const Style& s);
    UiGroupPanel& ClearCustomStyle();
    bool HasCustomStyle() const { return has_custom_style_; }
    const Style& GetStyle() const { return GetEffectiveStyle(); }
    const Style& GetCustomStyle() const { return style_; }

    StyledPalette& StyledPaletteRef() { return StyleEdit().palette; }
    StyledMetrics& StyledMetricsRef() { return StyleEdit().metrics; }
    StyledSkin& StyledSkinRef() { return StyleEdit().skin; }
    void OnStyleChanged();

    UiGroupPanel& SetTitle(const String& s);
    UiGroupPanel& SetSubTitle(const String& s);
    UiGroupPanel& SetIcon(const Image& img);
    UiGroupPanel& ClearIcon();
    UiGroupPanel& SetHeaderPlacement(UiAlign side);
    UiGroupPanel& SetHeaderMode(HeaderMode mode);
    UiGroupPanel& SetTitleAlign(UiAlign horizontal, UiAlign vertical);
    UiGroupPanel& SetHeaderContentAlign(UiAlign horizontal, UiAlign vertical);
    UiGroupPanel& SetLine(bool on = true);
    UiGroupPanel& SetHeaderBand(bool on = true);
    UiGroupPanel& SetInset(const Rect& r);
    UiGroupPanel& SetHeaderInset(const Rect& r);
    UiGroupPanel& SetIconSize(int px);
    UiGroupPanel& SetLineThickness(int px);
    UiGroupPanel& SetTitleFont(Font f);
    UiGroupPanel& SetSubTitleFont(Font f);
    UiGroupPanel& SetTitleSubTitleGap(int px);

    UiGroupPanel& SetContent(Ctrl& ctrl);
    UiGroupPanel& ClearContent();
    Ctrl* GetContent() const { return content_; }
    UiGroupPanel& SetHeaderContent(Ctrl& ctrl);
    UiGroupPanel& ClearHeaderContent();
    Ctrl* GetHeaderContent() const { return header_content_; }
    Rect GetBodyRect() const;
    // Meaningful while empty: this is the authoritative prospective slot used
    // to align an attached header child.
    Rect GetHeaderContentRect() const;

    Size GetMinSize() const override;
    void Layout() override;
    void Paint(Draw& w) override;
    void ChildRemoved(Ctrl *child) override;

private:
    struct HeaderLayout {
        Rect header;
        Rect title;
        Rect content_region;
        Rect content;
    };

    void InvalidateStyleCache();
    Style& StyleEdit();
    void SyncThemeStyle();
    const Style& GetEffectiveStyle() const;
    Size GetTitleNaturalSize() const;
    Size GetHeaderContentNaturalSize() const;
    Size GetHeaderSize() const;
    Rect GetFrameRect(const Rect& face) const;
    Rect GetHeaderRect(const Rect& face) const;
    Rect GetBodyRect(const Rect& face) const;
    HeaderLayout ResolveHeaderLayout(const Rect& face) const;
    void PaintHeader(Draw& w, const HeaderLayout& layout, StyledState st) const;
    void PaintGroupFrame(Draw& w, const Rect& frame, const HeaderLayout& header, StyledState st) const;

    Style style_;
    mutable Style themed_style_;
    mutable uint64 theme_revision_ = 0;
    bool has_custom_style_ = false;
    String title_;
    String subtitle_;
    Image icon_;
    Ctrl* content_ = nullptr;
    Ctrl* header_content_ = nullptr;
    UiAlign header_content_align_h_ = UiAlign::DEFAULT;
    UiAlign header_content_align_v_ = UiAlign::CENTER;
};

} // namespace Upp

#endif
