#ifndef _Ui_UiLabel_h_
#define _Ui_UiLabel_h_

#include <CtrlCore/CtrlCore.h>
#include <CtrlLib/CtrlLib.h>   // Access keys, DPI helpers, etc.
#include <Ui/UiStyle.h>
#include <Ui/UiDraw.h>

namespace Upp {

// ============================================================================
// UiLabel
// ============================================================================
//
// A lightweight styled text + optional icon control.
// - Styling is driven by UiStyle.h primitives (StyledPalette/Metrics/Skin).
// - Drawing is done via UiDraw.h helpers (UiPaintStyledBackground/Text/Icon).
// - Layout uses neutral 2-block helpers (UiMeasureBlocksContent / UiComputeBlocksLayout):
//     support = icon, main = text
//
// Text notes:
// - '&X' in SetText marks access key X and is removed from the rendered text.
// - '&&' escapes a literal '&'.
// - Tabs are normalized to spaces (see SetText()).
// - Multiline is supported via '\n' (unless style.nowrap == true).
//
class UiLabel : public Ctrl, public CtrlStyled<UiLabel> {
public:
    typedef UiLabel CLASSNAME;

    // ------------------------------------------------------------------------
    // Style
    // ------------------------------------------------------------------------
    struct Style : ChStyle<Style> {
        StyledPalette palette;
        StyledMetrics metrics;
        StyledSkin    skin;

        // Overall alignment of the (icon+text) group within the content rect.
        UiAlign align_h = UiAlign::LEFT;
        UiAlign align_v = UiAlign::CENTER;

        // Where the icon sits relative to the text (LEFT/RIGHT/TOP/BOTTOM).
        UiAlign icon_layout = UiAlign::LEFT;

        // Per-block margins (thickness-rect semantics; negative expands).
        Rect icon_margin = Rect(DPI(2), 0, DPI(4), 0);
        Rect text_margin = Rect(0, 0, 0, 0);

        // Base font (used unless metrics.use_text_font == true).
        Font font = StdFont();

        // If true, label does not paint its own background.
        bool transparent = true;

        // Optional underline across the entire text block.
        bool underline        = false;
        int  underline_width  = DPI(1);
        int  underline_offset = 0;

        // If true, treat text as single-line (do not split on '\n').
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
    String text_;
    wchar  accesskey_ = 0;
    bool has_access_mnemonic_ = false;
    
    Image  icon_;
    bool   mono_icon_  = false;
    bool   icon_scale_ = true;

    Vector<String> lines_;
    Vector<Size>   line_sizes_;

    Size user_min_size_ = Size(0, 0);

    mutable bool minsize_dirty_  = true;
    mutable Size cached_minsize_ = Size(0, 0);

    // Cached 2-block layout (support=icon, main=text)
    mutable bool           layout_dirty_ = true;
    mutable UiBlocksLayout layout_;

    // Cached content rect used to build layout_
    mutable Rect layout_content_ = Rect(0, 0, 0, 0);

    // Internal helpers
    void RebuildTextLines();
    Size ComputeNaturalSize() const;
    Size GetTextBlockSize() const;
    void UpdateLayout(const Rect& content) const;

public:
    UiLabel();

    // Content
    UiLabel&       SetText(const String& text);
    const String&  GetText() const { return text_; }

    UiLabel& SetIcon(const Image& img);
    UiLabel& SetMonoIcon(const Image& img);
    UiLabel& ClearIcon();
    Image    GetIcon() const { return icon_; }

    // Layout / Alignment
    UiLabel& SetIconLayout(UiAlign where);

    UiLabel& SetAlign(UiAlign h, UiAlign v);
    UiLabel& SetAlignH(UiAlign h);
    UiLabel& SetAlignV(UiAlign v);

    UiLabel& SetIconMargin(const Rect& m);
    UiLabel& SetIconMargin(int l, int t, int r, int b) { return SetIconMargin(Rect(l, t, r, b)); }
    UiLabel& SetIconMargin(int all) { return SetIconMargin(all, all, all, all); }
    Rect     GetIconMargin() const { return style_.icon_margin; }

    UiLabel& SetTextMargin(const Rect& m);
    UiLabel& SetTextMargin(int l, int t, int r, int b) { return SetTextMargin(Rect(l, t, r, b)); }
    UiLabel& SetTextMargin(int all) { return SetTextMargin(all, all, all, all); }
    Rect     GetTextMargin() const { return style_.text_margin; }

    UiLabel& SetIconScale(bool on = true)
    {
        icon_scale_ = on;
        Refresh();
        return *this;
    }
    bool IsIconScaled() const { return icon_scale_; }

    // Styling
    UiLabel&       SetStyle(const Style& s);
    const Style&   GetStyle() const { return style_; }

    static const Style& StyleDefault();
    static const Style& StyleHeadline();
    static const Style& StyleSubheadline();
    static const Style& StyleTitle();
    static const Style& StyleCaption();
    static const Style& StyleBadge();
    static const Style& StyleFootnote();

    UiLabel& SetUnderline(bool on = true, int thickness = DPI(1), int offset = 0);

    // CtrlStyled (CRTP) interface
    StyledPalette& StyledPaletteRef() { return style_.palette; }
    StyledMetrics& StyledMetricsRef() { return style_.metrics; }
    StyledSkin&    StyledSkinRef()    { return style_.skin; }

    void OnStyleChanged();

    // Layout / Sizing
    Size GetMinSize() const override;
    void Layout() override;

    UiLabel& SetSizeMin(Size sz);
    UiLabel& SetSizeMin(int cx, int cy) { return SetSizeMin(Size(cx, cy)); }

    UiLabel& SetSizeFixed(Size sz)        { return SetSizeMin(sz); }
    UiLabel& SetSizeFixed(int cx, int cy) { return SetSizeMin(Size(cx, cy)); }

    // Paint hooks
    Event<Draw&, const Rect&,
          const StyledPalette&, const StyledMetrics&, const StyledSkin&,
          StyledState, bool> WhenPaintBackground;

    Event<Draw&, const Rect&,
          const StyledPalette&, const StyledMetrics&, const StyledSkin&,
          StyledState, bool> WhenPaintForeground;

    // Ctrl overrides
    void   Paint(Draw& w) override;

    String GetDesc() const override;
    dword  GetAccessKeys() const override;
    void   AssignAccessKeys(dword used) override;

    void  SetData(const Value& v) override { SetText(AsString(v)); }
    Value GetData() const override         { return text_; }
};

} // namespace Upp

#endif
