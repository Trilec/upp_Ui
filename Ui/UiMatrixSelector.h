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
    - Compact styled matrix selector for spatial, directional, regional, and
      quad-based choices.

    Intent
    - Stay lightweight: one Ctrl, no child buttons/layouts, and no parallel
      drawing system.
    - Reuse the shared Ui palette/metrics/skin primitives while owning the tiny
      matrix geometry, hit testing, glyphs, readout, and optional relationship
      overlays directly.
    - Keep common uses controlled through presets while allowing labels, values,
      icons, pair selection, and custom quad paths to be overridden when an
      application needs them.

    Thread context
    - GUI thread only.

    Changelog
    - 2026-08: initial Position9, Compass8, Region5, and DramaticaQuad presets.
    - 2026-08: added ordered two-cell relationship selection with automatic
      horizontal/vertical/diagonal and Dramatica relationship classification.
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
    DramaticaQuad,
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

enum class UiMatrixOverlay : byte {
    None,
    DynamicPairs,      // Dramatica quad diagonals.
    CompanionPairs,    // Dramatica quad horizontals.
    DependentPairs,    // Dramatica quad verticals.
    PathU,              // Ordered quad traversal rendered as a U.
    PathZ,              // Ordered quad traversal rendered as a Z.
    PathButterfly,      // Crossing/butterfly ordered traversal.
    CustomPath,         // Caller-supplied ordered cell indexes.
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

enum class UiMatrixRelationship : byte {
    None,
    Dynamic,            // Dramatica diagonal pair.
    Companion,          // Dramatica horizontal pair.
    Dependent,          // Dramatica vertical pair.
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
        int overlay_width = DPI(2);
        int pair_arrow_size = DPI(7);
        Color overlay_color = Null;

        void Serialize(Stream& s)
        {
            s % surface_palette % surface_metrics % surface_skin
              % cell_palette % cell_metrics % cell_skin
              % selected_palette
              % readout_palette % readout_metrics % readout_skin
              % cell_font % readout_font
              % cell_gap % readout_gap % readout_width
              % glyph_inset % icon_inset % overlay_width % pair_arrow_size
              % overlay_color;
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

    UiMatrixSelector& SetOverlay(UiMatrixOverlay overlay);
    UiMatrixOverlay GetOverlay() const { return overlay_; }
    UiMatrixSelector& SetCustomPath(const Vector<int>& indices);
    const Vector<int>& GetCustomPath() const { return custom_path_; }

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
    UiMatrixRelationship GetDramaticaRelationship() const;
    String GetPairOrientationName() const;
    String GetRelationshipName() const;
    String GetPairDirectionLabel() const;
    String GetReadoutText() const;

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

    UiMatrixSelector& SetOverlayWidth(int px);
    UiMatrixSelector& SetPairArrowSize(int px);
    UiMatrixSelector& SetOverlayColor(Color color);

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
    void DrawOverlayAA(Draw& w, const Rect& matrix) const;
    void DrawPairAA(Draw& w, const Rect& matrix) const;
    void DrawReadout(Draw& w, const Rect& rect, StyledState state) const;
    Vector<int> ResolveOverlayPath() const;

private:
    mutable Style themed_style_;
    Style style_;
    mutable uint64 theme_revision_ = 0;
    bool has_custom_style_ = false;

    UiRole role_;
    UiRole selected_role_;
    UiRole readout_role_;

    UiMatrixPreset preset_ = UiMatrixPreset::Position9;
    UiMatrixOverlay overlay_ = UiMatrixOverlay::None;
    UiMatrixSelectionMode selection_mode_ = UiMatrixSelectionMode::SingleCell;
    Vector<int> custom_path_;
    Vector<Cell> cells_;
    int rows_ = 3;
    int cols_ = 3;

    bool show_readout_ = true;
    int selected_ = -1;
    int pair_first_ = -1;
    int pair_second_ = -1;
    int hover_ = -1;
    int pressed_ = -1;
    Size user_min_size_ = Size(0, 0);
};

}

#endif
