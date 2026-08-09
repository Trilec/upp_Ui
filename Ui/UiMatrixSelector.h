#ifndef _Ui_UiMatrixSelector_h_
#define _Ui_UiMatrixSelector_h_

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    UiMatrixSelector
    ================

    Purpose
    - Compact styled matrix selector for position, direction, region, and
      ordered two-cell choices.

    Intent
    - Stay lightweight: one Ctrl, no child buttons/layouts, and no parallel
      drawing system.
    - Reuse shared Ui palette/metrics/skin primitives while owning the small
      matrix geometry, hit testing, glyphs, readout, pair arrow, and default
      indication directly.
    - Keep common uses controlled through presets while allowing labels,
      semantic values, and icons to be replaced by the application.

    Removed relationship/pattern experiments — reference only
    - Dynamic: Dramatica diagonal relationship between opposite quad cells.
    - Companion: Dramatica horizontal relationship between adjacent quad cells.
    - Dependent: Dramatica vertical relationship between adjacent quad cells.
    - U path: ordered four-cell traversal 0 -> 2 -> 3 -> 1, visually forming a U.
    - Z path: ordered four-cell traversal 0 -> 1 -> 2 -> 3, visually forming a Z.
    - Butterfly path: ordered crossing traversal 0 -> 3 -> 1 -> 2.
    - CustomPath: caller-supplied ordered quad traversal rendered as one path.
      These were removed from UiMatrixSelector so the control remains generic;
      they may inform a future relationship/pattern-specific control.

    Thread context
    - GUI thread only.

    Changelog
    - 2026-08: initial Position9, Compass8, Region5, and quad presets.
    - 2026-08: added ordered two-cell selection with direction-preserving arrow.
    - 2026-08: renamed the generic quad preset to QuadPair, removed Dramatica-
      specific pattern API, and added role-derived default-cell indication.
*/

#include <CtrlCore/CtrlCore.h>
#include <Ui/UiStyle.h>
#include <Ui/UiDraw.h>

namespace Upp {

enum class UiRole : byte;

enum class UiMatrixPreset : byte {
    Position9,
    Compass8,
    Region5,
    QuadPair,
};

enum class UiMatrixGlyph : byte {
    None,
    ArrowN,
    ArrowNE,
    ArrowE,
    ArrowSE,
    ArrowS,
    ArrowSW,
    ArrowW,
    ArrowNW,
    Dot,
};

enum class UiMatrixSelectionMode : byte {
    SingleCell,
    Pair,
};

enum class UiMatrixPairOrientation : byte {
    None,
    Horizontal,
    Vertical,
    Diagonal,
};

class UiMatrixSelector : public Ctrl {
public:
    typedef UiMatrixSelector CLASSNAME;

    struct Cell : Moveable<Cell> {
        String short_label;
        String label;
        Value  value;
        Image  icon;
        UiMatrixGlyph glyph = UiMatrixGlyph::None;
        bool visible = true;
        bool enabled = true;
    };

    struct Style : ChStyle<Style> {
        StyledPalette surface_palette;
        StyledMetrics surface_metrics;
        StyledSkin    surface_skin;

        StyledPalette cell_palette;
        StyledMetrics cell_metrics;
        StyledSkin    cell_skin;

        StyledPalette selected_palette;

        StyledPalette readout_palette;
        StyledMetrics readout_metrics;
        StyledSkin    readout_skin;

        Font cell_font = StdFont();
        Font readout_font = StdFont();

        int cell_gap = 0;
        int readout_gap = DPI(10);
        int readout_width = DPI(104);
        int glyph_inset = DPI(9);
        int icon_inset = DPI(7);
        int pair_line_width = DPI(2);
        int pair_arrow_size = DPI(7);
        int default_dash = DPI(4);
        int default_dash_gap = DPI(3);
        int default_frame_width = DPI(1);
        Color pair_color = Null;

        void Serialize(Stream& s)
        {
            s % surface_palette % surface_metrics % surface_skin
              % cell_palette % cell_metrics % cell_skin
              % selected_palette
              % readout_palette % readout_metrics % readout_skin
              % cell_font % readout_font
              % cell_gap % readout_gap % readout_width
              % glyph_inset % icon_inset
              % pair_line_width % pair_arrow_size
              % default_dash % default_dash_gap % default_frame_width
              % pair_color;
        }
    };

    UiMatrixSelector();

    static const Style& StyleDefault();
    UiMatrixSelector& SetCustomStyle(const Style& s);
    UiMatrixSelector& ClearCustomStyle();
    bool HasCustomStyle() const { return has_custom_style_; }
    const Style& GetStyle() const;
    const Style& GetCustomStyle() const { return style_; }

    UiMatrixSelector& SetRole(UiRole role);
    UiMatrixSelector& SetSelectedRole(UiRole role);
    UiMatrixSelector& SetReadoutRole(UiRole role);

    UiMatrixSelector& SetPreset(UiMatrixPreset preset);
    UiMatrixPreset GetPreset() const { return preset_; }

    UiMatrixSelector& SetSelectionMode(UiMatrixSelectionMode mode);
    UiMatrixSelectionMode GetSelectionMode() const { return selection_mode_; }
    bool IsPairSelection() const { return selection_mode_ == UiMatrixSelectionMode::Pair; }

    UiMatrixSelector& SetPair(int first, int second, bool fire_action = false);
    UiMatrixSelector& ClearPair();
    int GetPairStartIndex() const { return pair_first_; }
    int GetPairEndIndex() const { return pair_second_; }
    bool HasPairStart() const { return pair_first_ >= 0; }
    bool HasCompletePair() const { return pair_first_ >= 0 && pair_second_ >= 0; }
    UiMatrixPairOrientation GetPairOrientation() const;
    String GetPairOrientationName() const;
    String GetPairDirectionLabel() const;
    String GetReadoutText() const;

    UiMatrixSelector& SetDefault(int index);
    UiMatrixSelector& ClearDefault();
    int GetDefaultIndex() const { return default_index_; }
    bool HasDefault() const { return default_index_ >= 0; }
    UiMatrixSelector& ShowDefault(bool on = true);
    bool IsDefaultShown() const { return show_default_; }
    bool IsDefaultSelected() const;

    UiMatrixSelector& ShowReadout(bool on = true);
    bool IsReadoutShown() const { return show_readout_; }
    UiMatrixSelector& SetReadoutWidth(int px);
    UiMatrixSelector& SetReadoutGap(int px);

    UiMatrixSelector& SetCellGap(int px);
    UiMatrixSelector& SetCellRadius(int px);
    UiMatrixSelector& ShowCellFace(bool on = true);
    UiMatrixSelector& ShowCellFrame(bool on = true);
    UiMatrixSelector& SetCellFont(const Font& font);
    UiMatrixSelector& SetGlyphInset(int px);
    UiMatrixSelector& SetIconInset(int px);

    UiMatrixSelector& SetOuterRadius(int px);
    UiMatrixSelector& ShowSurface(bool on = true);
    UiMatrixSelector& ShowSurfaceFrame(bool on = true);
    UiMatrixSelector& SetSurfaceShadow(bool on = true);

    UiMatrixSelector& SetReadoutRadius(int px);
    UiMatrixSelector& ShowReadoutFace(bool on = true);
    UiMatrixSelector& ShowReadoutFrame(bool on = true);
    UiMatrixSelector& SetReadoutFont(const Font& font);

    UiMatrixSelector& SetPairLineWidth(int px);
    UiMatrixSelector& SetPairArrowSize(int px);
    UiMatrixSelector& SetPairColor(Color color);
    UiMatrixSelector& SetDefaultDash(int dash, int gap);
    UiMatrixSelector& SetDefaultFrameWidth(int px);

    int GetRows() const { return rows_; }
    int GetColumns() const { return cols_; }
    int GetCellCount() const { return cells_.GetCount(); }
    const Cell& GetCell(int index) const { return cells_[index]; }

    UiMatrixSelector& SetCell(int index, const String& short_label,
                              const String& label, const Value& value);
    UiMatrixSelector& SetCellLabel(int index, const String& short_label, const String& label = Null);
    UiMatrixSelector& SetCellValue(int index, const Value& value);
    UiMatrixSelector& SetCellIcon(int index, const Image& icon);
    UiMatrixSelector& SetCellGlyph(int index, UiMatrixGlyph glyph);
    UiMatrixSelector& EnableCell(int index, bool on = true);
    UiMatrixSelector& ShowCell(int index, bool on = true);

    UiMatrixSelector& SelectIndex(int index, bool fire_action = false);
    int GetSelectedIndex() const { return selected_; }
    String GetSelectedLabel() const;

    virtual void SetData(const Value& v) override;
    virtual Value GetData() const override;

    UiMatrixSelector& SetSizeMin(Size sz) { user_min_size_ = Size(max(0, sz.cx), max(0, sz.cy)); RefreshLayout(); return *this; }
    UiMatrixSelector& SetSizeMin(int cx, int cy) { return SetSizeMin(Size(cx, cy)); }

    Rect GetMatrixRect() const;
    Rect GetReadoutRect() const;
    Rect GetCellRect(int index) const;
    int HitTest(Point p) const;

    Event<> WhenChanging;
    Event<> WhenAction;

    virtual Size GetMinSize() const override;
    virtual void Paint(Draw& w) override;
    virtual void LeftDown(Point p, dword flags) override;
    virtual void LeftUp(Point p, dword flags) override;
    virtual void MouseMove(Point p, dword flags) override;
    virtual void MouseLeave() override;
    virtual bool Key(dword key, int count) override;
    virtual void GotFocus() override;
    virtual void LostFocus() override;

private:
    void InvalidateThemeStyle();
    void SyncThemeStyle() const;
    Style ResolveThemeStyle() const;
    Style& StyleEdit();

    void LoadPreset(UiMatrixPreset preset);
    void AddCell(const char* short_label, const char* label, const Value& value,
                 UiMatrixGlyph glyph = UiMatrixGlyph::None,
                 bool visible = true, bool enabled = true);

    bool IsSelectableCell(int index) const;
    bool IsCellSelectedVisual(int index) const;
    int FindCellByValue(const Value& value) const;
    int FindNextEnabled(int from, int dx, int dy) const;
    void ActivateIndex(int index);
    void SetHover(int index);
    void DrawCellContent(Draw& w, const Rect& r, const Cell& cell,
                         const StyledPalette& palette, StyledState state) const;
    void DrawGlyphAA(Draw& w, const Rect& r, UiMatrixGlyph glyph, Color color) const;
    void DrawPairAA(Draw& w, const Rect& matrix) const;
    void DrawDefaultFrame(Draw& w) const;
    void DrawReadout(Draw& w, const Rect& rect, StyledState state) const;

private:
    mutable Style themed_style_;
    Style style_;
    mutable uint64 theme_revision_ = 0;
    bool has_custom_style_ = false;

    UiRole role_;
    UiRole selected_role_;
    UiRole readout_role_;

    UiMatrixPreset preset_ = UiMatrixPreset::Position9;
    UiMatrixSelectionMode selection_mode_ = UiMatrixSelectionMode::SingleCell;
    Vector<Cell> cells_;
    int rows_ = 3;
    int cols_ = 3;

    bool show_readout_ = true;
    bool show_default_ = true;
    int selected_ = -1;
    int default_index_ = -1;
    int pair_first_ = -1;
    int pair_second_ = -1;
    int hover_ = -1;
    int pressed_ = -1;
    Size user_min_size_ = Size(0, 0);
};

}

#endif
