#include <Ui/UiMatrixSelector.h>
#include <Ui/UiTheme.h>
#include <Painter/Painter.h>
#include <cmath>

namespace Upp {

static Color MatrixInk_(const StyledPalette& p, StyledState st)
{
    Color c = p.ink[st];
    return IsNull(c) ? SColorText() : c;
}

static Color MatrixOverlayColor_(const UiMatrixSelector::Style& s)
{
    if(!IsNull(s.overlay_color))
        return s.overlay_color;
    Color c = s.selected_palette.frame[ST_NORMAL];
    if(IsNull(c))
        c = s.selected_palette.icon[ST_NORMAL];
    if(IsNull(c))
        c = s.selected_palette.ink[ST_NORMAL];
    return IsNull(c) ? SColorHighlight() : c;
}

static String MatrixCellCompactLabel_(const UiMatrixSelector::Cell& cell)
{
    if(!cell.short_label.IsEmpty())
        return cell.short_label;
    if(!cell.label.IsEmpty())
        return cell.label;
    return AsString(cell.value);
}

const UiMatrixSelector::Style& UiMatrixSelector::StyleDefault()
{
    static Style s;
    return s;
}

UiMatrixSelector::UiMatrixSelector()
    : style_(StyleDefault())
    , themed_style_(StyleDefault())
    , role_(UiRole::Standard)
    , selected_role_(UiRole::Accent)
    , readout_role_(UiRole::Subtle)
{
    Transparent();
    WantFocus();
    LoadPreset(UiMatrixPreset::Position9);
}

UiMatrixSelector::Style UiMatrixSelector::ResolveThemeStyle() const
{
    Style out;

    UiPanel::Style surface = UiTheme::ResolvePanel(UiPanelRole::Surface);
    out.surface_palette = surface.palette;
    out.surface_metrics = surface.metrics;
    out.surface_skin = surface.skin;
    out.surface_metrics.face_enabled = false;
    out.surface_metrics.frame_enabled = false;
    out.surface_metrics.shadow.enabled = false;
    out.surface_metrics.content_margin = Rect(DPI(6), DPI(6), DPI(6), DPI(6));

    UiButton::Style cell = UiTheme::ResolveButton(role_);
    out.cell_palette = cell.palette;
    out.cell_metrics = cell.metrics;
    out.cell_skin = cell.skin;
    out.cell_metrics.content_margin = Rect(DPI(4), DPI(3), DPI(4), DPI(3));
    out.cell_font = cell.font;

    UiButton::Style selected = UiTheme::ResolveButton(selected_role_);
    out.selected_palette = selected.palette;

    UiButton::Style readout = UiTheme::ResolveButton(readout_role_);
    out.readout_palette = readout.palette;
    out.readout_metrics = readout.metrics;
    out.readout_skin = readout.skin;
    out.readout_metrics.content_margin = Rect(DPI(8), DPI(4), DPI(8), DPI(4));
    out.readout_font = readout.font;

    out.cell_gap = 0;
    out.readout_gap = DPI(10);
    out.readout_width = DPI(104);
    out.glyph_inset = DPI(9);
    out.icon_inset = DPI(7);
    out.overlay_width = DPI(2);
    out.pair_arrow_size = DPI(7);
    out.overlay_color = Null;
    return out;
}

void UiMatrixSelector::InvalidateThemeStyle()
{
    theme_revision_ = 0;
    RefreshLayout();
    Refresh();
}

void UiMatrixSelector::SyncThemeStyle() const
{
    if(has_custom_style_)
        return;
    uint64 revision = UiTheme::GetRevision();
    if(theme_revision_ == revision)
        return;
    themed_style_ = ResolveThemeStyle();
    theme_revision_ = revision;
}

const UiMatrixSelector::Style& UiMatrixSelector::GetStyle() const
{
    if(has_custom_style_)
        return style_;
    SyncThemeStyle();
    return themed_style_;
}

UiMatrixSelector::Style& UiMatrixSelector::StyleEdit()
{
    if(!has_custom_style_) {
        style_ = GetStyle();
        has_custom_style_ = true;
    }
    return style_;
}

UiMatrixSelector& UiMatrixSelector::SetCustomStyle(const Style& s)
{
    style_ = s;
    has_custom_style_ = true;
    RefreshLayout();
    Refresh();
    return *this;
}

UiMatrixSelector& UiMatrixSelector::ClearCustomStyle()
{
    if(has_custom_style_) {
        has_custom_style_ = false;
        InvalidateThemeStyle();
    }
    return *this;
}

UiMatrixSelector& UiMatrixSelector::SetRole(UiRole role)
{
    role_ = role;
    if(!has_custom_style_)
        InvalidateThemeStyle();
    return *this;
}

UiMatrixSelector& UiMatrixSelector::SetSelectedRole(UiRole role)
{
    selected_role_ = role;
    if(!has_custom_style_)
        InvalidateThemeStyle();
    return *this;
}

UiMatrixSelector& UiMatrixSelector::SetReadoutRole(UiRole role)
{
    readout_role_ = role;
    if(!has_custom_style_)
        InvalidateThemeStyle();
    return *this;
}

void UiMatrixSelector::AddCell(const char* short_label, const char* label, const Value& value,
                               UiMatrixGlyph glyph, bool visible, bool enabled)
{
    Cell& c = cells_.Add();
    c.short_label = short_label ? short_label : "";
    c.label = label ? String(label) : c.short_label;
    c.value = value;
    c.glyph = glyph;
    c.visible = visible;
    c.enabled = enabled;
}

void UiMatrixSelector::LoadPreset(UiMatrixPreset preset)
{
    preset_ = preset;
    cells_.Clear();
    custom_path_.Clear();
    overlay_ = UiMatrixOverlay::None;
    pair_first_ = pair_second_ = -1;
    selected_ = hover_ = pressed_ = -1;

    switch(preset) {
    case UiMatrixPreset::Position9:
        rows_ = cols_ = 3;
        AddCell("TL", "Top left", "top_left");
        AddCell("T", "Top", "top");
        AddCell("TR", "Top right", "top_right");
        AddCell("L", "Left", "left");
        AddCell("CNT", "Center", "center");
        AddCell("R", "Right", "right");
        AddCell("BL", "Bottom left", "bottom_left");
        AddCell("B", "Bottom", "bottom");
        AddCell("BR", "Bottom right", "bottom_right");
        selected_ = 4;
        break;

    case UiMatrixPreset::Compass8:
        rows_ = cols_ = 3;
        AddCell("", "Northwest", "northwest", UiMatrixGlyph::ArrowNW);
        AddCell("", "North", "north", UiMatrixGlyph::ArrowN);
        AddCell("", "Northeast", "northeast", UiMatrixGlyph::ArrowNE);
        AddCell("", "West", "west", UiMatrixGlyph::ArrowW);
        AddCell("", "No direction", "none", UiMatrixGlyph::Dot, true, false);
        AddCell("", "East", "east", UiMatrixGlyph::ArrowE);
        AddCell("", "Southwest", "southwest", UiMatrixGlyph::ArrowSW);
        AddCell("", "South", "south", UiMatrixGlyph::ArrowS);
        AddCell("", "Southeast", "southeast", UiMatrixGlyph::ArrowSE);
        selected_ = 5;
        break;

    case UiMatrixPreset::Region5:
        rows_ = cols_ = 3;
        AddCell("", "", Null, UiMatrixGlyph::None, false, false);
        AddCell("TOP", "Top", "top");
        AddCell("", "", Null, UiMatrixGlyph::None, false, false);
        AddCell("L", "Left", "left");
        AddCell("CNT", "Center", "center");
        AddCell("R", "Right", "right");
        AddCell("", "", Null, UiMatrixGlyph::None, false, false);
        AddCell("BTM", "Bottom", "bottom");
        AddCell("", "", Null, UiMatrixGlyph::None, false, false);
        selected_ = 4;
        break;

    case UiMatrixPreset::DramaticaQuad:
        rows_ = cols_ = 2;
        AddCell("A", "Upper left", "a");
        AddCell("B", "Upper right", "b");
        AddCell("C", "Lower left", "c");
        AddCell("D", "Lower right", "d");
        selected_ = 0;
        overlay_ = selection_mode_ == UiMatrixSelectionMode::Pair
                 ? UiMatrixOverlay::None
                 : UiMatrixOverlay::DynamicPairs;
        break;
    }

    RefreshLayout();
    Refresh();
}

UiMatrixSelector& UiMatrixSelector::SetPreset(UiMatrixPreset preset)
{
    if(preset_ != preset || cells_.IsEmpty())
        LoadPreset(preset);
    return *this;
}

UiMatrixSelector& UiMatrixSelector::SetOverlay(UiMatrixOverlay overlay)
{
    overlay_ = overlay;
    Refresh();
    return *this;
}

UiMatrixSelector& UiMatrixSelector::SetCustomPath(const Vector<int>& indices)
{
    custom_path_ <<= indices;
    overlay_ = UiMatrixOverlay::CustomPath;
    Refresh();
    return *this;
}

UiMatrixSelector& UiMatrixSelector::SetSelectionMode(UiMatrixSelectionMode mode)
{
    if(selection_mode_ == mode)
        return *this;
    selection_mode_ = mode;
    if(selection_mode_ == UiMatrixSelectionMode::Pair)
        overlay_ = UiMatrixOverlay::None;
    pair_first_ = pair_second_ = -1;
    if(!IsSelectableCell(selected_))
        selected_ = -1;
    Refresh();
    return *this;
}

bool UiMatrixSelector::IsSelectableCell(int index) const
{
    return index >= 0 && index < cells_.GetCount()
        && cells_[index].visible && cells_[index].enabled;
}

int UiMatrixSelector::FindCellByValue(const Value& value) const
{
    for(int i = 0; i < cells_.GetCount(); i++)
        if(IsSelectableCell(i) && cells_[i].value == value)
            return i;
    return -1;
}

UiMatrixSelector& UiMatrixSelector::SetPair(int first, int second, bool fire_action)
{
    if(!IsSelectableCell(first))
        return *this;
    if(second >= 0 && (!IsSelectableCell(second) || second == first))
        return *this;

    if(selection_mode_ != UiMatrixSelectionMode::Pair) {
        selection_mode_ = UiMatrixSelectionMode::Pair;
        overlay_ = UiMatrixOverlay::None;
    }
    bool changed = pair_first_ != first || pair_second_ != second;
    pair_first_ = first;
    pair_second_ = second;
    selected_ = second >= 0 ? second : first;
    Refresh();

    if(fire_action && changed) {
        if(WhenChanging)
            WhenChanging();
        if(pair_second_ >= 0 && WhenAction)
            WhenAction();
    }
    return *this;
}

UiMatrixSelector& UiMatrixSelector::ClearPair()
{
    if(pair_first_ >= 0 || pair_second_ >= 0) {
        pair_first_ = pair_second_ = -1;
        Refresh();
    }
    return *this;
}

UiMatrixPairOrientation UiMatrixSelector::GetPairOrientation() const
{
    if(!HasCompletePair() || cols_ <= 0)
        return UiMatrixPairOrientation::None;
    int r0 = pair_first_ / cols_;
    int c0 = pair_first_ % cols_;
    int r1 = pair_second_ / cols_;
    int c1 = pair_second_ % cols_;
    if(r0 == r1 && c0 != c1)
        return UiMatrixPairOrientation::Horizontal;
    if(c0 == c1 && r0 != r1)
        return UiMatrixPairOrientation::Vertical;
    if(r0 != r1 && c0 != c1)
        return UiMatrixPairOrientation::Diagonal;
    return UiMatrixPairOrientation::None;
}

UiMatrixRelationship UiMatrixSelector::GetDramaticaRelationship() const
{
    if(preset_ != UiMatrixPreset::DramaticaQuad || !HasCompletePair())
        return UiMatrixRelationship::None;
    switch(GetPairOrientation()) {
    case UiMatrixPairOrientation::Diagonal:   return UiMatrixRelationship::Dynamic;
    case UiMatrixPairOrientation::Horizontal: return UiMatrixRelationship::Companion;
    case UiMatrixPairOrientation::Vertical:   return UiMatrixRelationship::Dependent;
    default:                                  return UiMatrixRelationship::None;
    }
}

String UiMatrixSelector::GetPairOrientationName() const
{
    switch(GetPairOrientation()) {
    case UiMatrixPairOrientation::Horizontal: return "Horizontal";
    case UiMatrixPairOrientation::Vertical:   return "Vertical";
    case UiMatrixPairOrientation::Diagonal:   return "Diagonal";
    default:                                  return String();
    }
}

String UiMatrixSelector::GetRelationshipName() const
{
    switch(GetDramaticaRelationship()) {
    case UiMatrixRelationship::Dynamic:   return "Dynamic";
    case UiMatrixRelationship::Companion: return "Companion";
    case UiMatrixRelationship::Dependent: return "Dependent";
    default:                              return String();
    }
}

String UiMatrixSelector::GetPairDirectionLabel() const
{
    if(!HasPairStart())
        return String();
    String from = cells_[pair_first_].label;
    if(!HasCompletePair())
        return from + " -> ...";
    return from + " -> " + cells_[pair_second_].label;
}

String UiMatrixSelector::GetReadoutText() const
{
    if(selection_mode_ != UiMatrixSelectionMode::Pair)
        return GetSelectedLabel();
    if(!HasPairStart())
        return "Choose first point";
    if(!HasCompletePair())
        return "From " + cells_[pair_first_].label + " - choose second point";

    String relation = GetRelationshipName();
    String orientation = GetPairOrientationName();
    String out;
    if(!relation.IsEmpty())
        out << relation << " / ";
    out << orientation;
    String direction = GetPairDirectionLabel();
    if(!direction.IsEmpty())
        out << " - " << direction;
    return out;
}

UiMatrixSelector& UiMatrixSelector::ShowReadout(bool on)
{
    if(show_readout_ != on) {
        show_readout_ = on;
        RefreshLayout();
        Refresh();
    }
    return *this;
}

UiMatrixSelector& UiMatrixSelector::SetReadoutWidth(int px)
{
    StyleEdit().readout_width = max(0, px);
    RefreshLayout();
    Refresh();
    return *this;
}

UiMatrixSelector& UiMatrixSelector::SetReadoutGap(int px)
{
    StyleEdit().readout_gap = max(0, px);
    RefreshLayout();
    Refresh();
    return *this;
}

UiMatrixSelector& UiMatrixSelector::SetCellGap(int px)
{
    StyleEdit().cell_gap = max(0, px);
    RefreshLayout();
    Refresh();
    return *this;
}

UiMatrixSelector& UiMatrixSelector::SetCellRadius(int px)
{
    StyleEdit().cell_metrics.radius = max(0, px);
    Refresh();
    return *this;
}

UiMatrixSelector& UiMatrixSelector::ShowCellFace(bool on)
{
    StyleEdit().cell_metrics.face_enabled = on;
    Refresh();
    return *this;
}

UiMatrixSelector& UiMatrixSelector::ShowCellFrame(bool on)
{
    StyleEdit().cell_metrics.frame_enabled = on;
    Refresh();
    return *this;
}

UiMatrixSelector& UiMatrixSelector::SetCellFont(const Font& font)
{
    StyleEdit().cell_font = font;
    RefreshLayout();
    Refresh();
    return *this;
}

UiMatrixSelector& UiMatrixSelector::SetGlyphInset(int px)
{
    StyleEdit().glyph_inset = max(0, px);
    Refresh();
    return *this;
}

UiMatrixSelector& UiMatrixSelector::SetIconInset(int px)
{
    StyleEdit().icon_inset = max(0, px);
    Refresh();
    return *this;
}

UiMatrixSelector& UiMatrixSelector::SetOuterRadius(int px)
{
    StyleEdit().surface_metrics.radius = max(0, px);
    Refresh();
    return *this;
}

UiMatrixSelector& UiMatrixSelector::SetReadoutRadius(int px)
{
    StyleEdit().readout_metrics.radius = max(0, px);
    Refresh();
    return *this;
}

UiMatrixSelector& UiMatrixSelector::ShowReadoutFace(bool on)
{
    StyleEdit().readout_metrics.face_enabled = on;
    Refresh();
    return *this;
}

UiMatrixSelector& UiMatrixSelector::ShowReadoutFrame(bool on)
{
    StyleEdit().readout_metrics.frame_enabled = on;
    Refresh();
    return *this;
}

UiMatrixSelector& UiMatrixSelector::SetReadoutFont(const Font& font)
{
    StyleEdit().readout_font = font;
    RefreshLayout();
    Refresh();
    return *this;
}

UiMatrixSelector& UiMatrixSelector::ShowSurface(bool on)
{
    StyleEdit().surface_metrics.face_enabled = on;
    Refresh();
    return *this;
}

UiMatrixSelector& UiMatrixSelector::ShowSurfaceFrame(bool on)
{
    StyleEdit().surface_metrics.frame_enabled = on;
    Refresh();
    return *this;
}

UiMatrixSelector& UiMatrixSelector::SetSurfaceShadow(bool on)
{
    StyleEdit().surface_metrics.shadow.enabled = on;
    RefreshLayout();
    Refresh();
    return *this;
}

UiMatrixSelector& UiMatrixSelector::SetOverlayWidth(int px)
{
    StyleEdit().overlay_width = max(1, px);
    Refresh();
    return *this;
}

UiMatrixSelector& UiMatrixSelector::SetPairArrowSize(int px)
{
    StyleEdit().pair_arrow_size = max(2, px);
    Refresh();
    return *this;
}

UiMatrixSelector& UiMatrixSelector::SetOverlayColor(Color color)
{
    StyleEdit().overlay_color = color;
    Refresh();
    return *this;
}

UiMatrixSelector& UiMatrixSelector::SetCell(int index, const String& short_label,
                                            const String& label, const Value& value)
{
    if(index < 0 || index >= cells_.GetCount())
        return *this;
    cells_[index].short_label = short_label;
    cells_[index].label = IsNull(label) ? short_label : label;
    cells_[index].value = value;
    Refresh();
    return *this;
}

UiMatrixSelector& UiMatrixSelector::SetCellLabel(int index, const String& short_label, const String& label)
{
    if(index < 0 || index >= cells_.GetCount())
        return *this;
    cells_[index].short_label = short_label;
    cells_[index].label = IsNull(label) ? short_label : label;
    Refresh();
    return *this;
}

UiMatrixSelector& UiMatrixSelector::SetCellValue(int index, const Value& value)
{
    if(index >= 0 && index < cells_.GetCount())
        cells_[index].value = value;
    return *this;
}

UiMatrixSelector& UiMatrixSelector::SetCellIcon(int index, const Image& icon)
{
    if(index >= 0 && index < cells_.GetCount()) {
        cells_[index].icon = icon;
        Refresh();
    }
    return *this;
}

UiMatrixSelector& UiMatrixSelector::SetCellGlyph(int index, UiMatrixGlyph glyph)
{
    if(index >= 0 && index < cells_.GetCount()) {
        cells_[index].glyph = glyph;
        Refresh();
    }
    return *this;
}

UiMatrixSelector& UiMatrixSelector::EnableCell(int index, bool on)
{
    if(index >= 0 && index < cells_.GetCount()) {
        cells_[index].enabled = on;
        if(!on && selected_ == index)
            selected_ = -1;
        if(!on && pair_first_ == index)
            pair_first_ = pair_second_ = -1;
        else if(!on && pair_second_ == index)
            pair_second_ = -1;
        Refresh();
    }
    return *this;
}

UiMatrixSelector& UiMatrixSelector::ShowCell(int index, bool on)
{
    if(index >= 0 && index < cells_.GetCount()) {
        cells_[index].visible = on;
        if(!on && selected_ == index)
            selected_ = -1;
        if(!on && pair_first_ == index)
            pair_first_ = pair_second_ = -1;
        else if(!on && pair_second_ == index)
            pair_second_ = -1;
        Refresh();
    }
    return *this;
}

UiMatrixSelector& UiMatrixSelector::SelectIndex(int index, bool fire_action)
{
    if(!IsSelectableCell(index))
        return *this;
    if(selected_ == index)
        return *this;
    selected_ = index;
    Refresh();
    if(fire_action) {
        if(WhenChanging)
            WhenChanging();
        if(WhenAction)
            WhenAction();
    }
    return *this;
}

String UiMatrixSelector::GetSelectedLabel() const
{
    return selected_ >= 0 && selected_ < cells_.GetCount() ? cells_[selected_].label : String();
}

void UiMatrixSelector::SetData(const Value& v)
{
    if(selection_mode_ == UiMatrixSelectionMode::Pair) {
        if(v.Is<ValueArray>()) {
            ValueArray values = v;
            if(values.GetCount() == 0) {
                ClearPair();
                return;
            }
            int first = FindCellByValue(values[0]);
            if(first < 0)
                return;
            int second = values.GetCount() >= 2 ? FindCellByValue(values[1]) : -1;
            if(values.GetCount() >= 2 && second < 0)
                return;
            SetPair(first, second, false);
            return;
        }
        int first = FindCellByValue(v);
        if(first >= 0)
            SetPair(first, -1, false);
        return;
    }

    int index = FindCellByValue(v);
    if(index >= 0)
        SelectIndex(index, false);
}

Value UiMatrixSelector::GetData() const
{
    if(selection_mode_ == UiMatrixSelectionMode::Pair) {
        ValueArray values;
        if(pair_first_ >= 0 && pair_first_ < cells_.GetCount())
            values.Add(cells_[pair_first_].value);
        if(pair_second_ >= 0 && pair_second_ < cells_.GetCount())
            values.Add(cells_[pair_second_].value);
        return values;
    }
    return selected_ >= 0 && selected_ < cells_.GetCount() ? cells_[selected_].value : Value();
}

Size UiMatrixSelector::GetMinSize() const
{
    const Style& s = GetStyle();
    int cell = DPI(34);
    int matrix_w = cols_ * cell + max(0, cols_ - 1) * s.cell_gap;
    int matrix_h = rows_ * cell + max(0, rows_ - 1) * s.cell_gap;
    int readout_w = show_readout_ ? s.readout_gap + max(DPI(70), s.readout_width) : 0;
    Size content(matrix_w + readout_w, max(matrix_h, DPI(38)));
    Size natural = UiStyledOuterSizeFromContent(content, s.surface_metrics, s.surface_skin);
    natural.cx = max(natural.cx, user_min_size_.cx);
    natural.cy = max(natural.cy, user_min_size_.cy);
    return natural;
}

Rect UiMatrixSelector::GetMatrixRect() const
{
    const Style& s = GetStyle();
    Rect inner = UiStyledInnerRect(Rect(GetSize()), s.surface_metrics, s.surface_skin);
    if(inner.IsEmpty())
        return inner;

    int reserved = show_readout_ ? s.readout_gap + max(0, s.readout_width) : 0;
    int available_w = max(0, inner.GetWidth() - reserved);
    int side = min(available_w, inner.GetHeight());
    side = max(0, side);
    int x = inner.left + (available_w - side) / 2;
    int y = inner.top + (inner.GetHeight() - side) / 2;
    return RectC(x, y, side, side);
}

Rect UiMatrixSelector::GetReadoutRect() const
{
    if(!show_readout_)
        return Rect(0, 0, 0, 0);
    const Style& s = GetStyle();
    Rect inner = UiStyledInnerRect(Rect(GetSize()), s.surface_metrics, s.surface_skin);
    Rect matrix = GetMatrixRect();
    int left = matrix.right + s.readout_gap;
    int width = min(max(0, s.readout_width), max(0, inner.right - left));
    int h = min(inner.GetHeight(), max(DPI(42), matrix.GetHeight() / 3));
    int y = inner.top + (inner.GetHeight() - h) / 2;
    return RectC(left, y, width, h);
}

Rect UiMatrixSelector::GetCellRect(int index) const
{
    if(index < 0 || index >= cells_.GetCount() || rows_ <= 0 || cols_ <= 0)
        return Rect(0, 0, 0, 0);
    const Style& s = GetStyle();
    Rect matrix = GetMatrixRect();
    int row = index / cols_;
    int col = index % cols_;
    if(row >= rows_)
        return Rect(0, 0, 0, 0);

    int total_gap_x = max(0, cols_ - 1) * s.cell_gap;
    int total_gap_y = max(0, rows_ - 1) * s.cell_gap;
    int usable_w = max(0, matrix.GetWidth() - total_gap_x);
    int usable_h = max(0, matrix.GetHeight() - total_gap_y);

    int x0 = matrix.left + (usable_w * col) / cols_ + col * s.cell_gap;
    int x1 = matrix.left + (usable_w * (col + 1)) / cols_ + col * s.cell_gap;
    int y0 = matrix.top + (usable_h * row) / rows_ + row * s.cell_gap;
    int y1 = matrix.top + (usable_h * (row + 1)) / rows_ + row * s.cell_gap;
    return Rect(x0, y0, x1, y1);
}

int UiMatrixSelector::HitTest(Point p) const
{
    for(int i = 0; i < cells_.GetCount(); i++)
        if(IsSelectableCell(i) && GetCellRect(i).Contains(p))
            return i;
    return -1;
}

bool UiMatrixSelector::IsCellSelectedVisual(int index) const
{
    if(selection_mode_ == UiMatrixSelectionMode::Pair)
        return index == pair_first_ || index == pair_second_;
    return index == selected_;
}

void UiMatrixSelector::SetHover(int index)
{
    if(hover_ != index) {
        hover_ = index;
        Refresh();
    }
}

void UiMatrixSelector::DrawGlyphAA(Draw& w, const Rect& r, UiMatrixGlyph glyph, Color color) const
{
    if(glyph == UiMatrixGlyph::None || r.IsEmpty())
        return;

    ImageBuffer ib(r.GetSize());
    BufferPainter p(ib, MODE_ANTIALIASED);
    p.Clear(RGBAZero());

    double cx = r.GetWidth() / 2.0;
    double cy = r.GetHeight() / 2.0;
    double radius = max(2.0, min(r.GetWidth(), r.GetHeight()) * 0.23);

    if(glyph == UiMatrixGlyph::Dot) {
        p.Begin();
        p.Circle(cx, cy, max(1.5, radius * 0.22));
        p.Fill(color);
        p.End();
        w.DrawImage(r.left, r.top, ib);
        return;
    }

    double dx = 0, dy = 0;
    switch(glyph) {
    case UiMatrixGlyph::ArrowN:  dy = -1; break;
    case UiMatrixGlyph::ArrowNE: dx = 1; dy = -1; break;
    case UiMatrixGlyph::ArrowE:  dx = 1; break;
    case UiMatrixGlyph::ArrowSE: dx = 1; dy = 1; break;
    case UiMatrixGlyph::ArrowS:  dy = 1; break;
    case UiMatrixGlyph::ArrowSW: dx = -1; dy = 1; break;
    case UiMatrixGlyph::ArrowW:  dx = -1; break;
    case UiMatrixGlyph::ArrowNW: dx = -1; dy = -1; break;
    default: break;
    }

    double len = sqrt(dx * dx + dy * dy);
    if(len <= 0)
        return;
    dx /= len;
    dy /= len;
    double px = -dy;
    double py = dx;
    double sx = cx - dx * radius * 0.72;
    double sy = cy - dy * radius * 0.72;
    double ex = cx + dx * radius;
    double ey = cy + dy * radius;
    double head = radius * 0.48;
    double wing = radius * 0.42;

    p.Begin();
    p.Move(sx, sy);
    p.Line(ex, ey);
    p.Stroke(max(1.4, radius * 0.16), color);
    p.End();

    p.Begin();
    p.Move(ex, ey);
    p.Line(ex - dx * head + px * wing, ey - dy * head + py * wing);
    p.Line(ex - dx * head - px * wing, ey - dy * head - py * wing);
    p.Close();
    p.Fill(color);
    p.End();

    w.DrawImage(r.left, r.top, ib);
}

void UiMatrixSelector::DrawCellContent(Draw& w, const Rect& r, const Cell& cell,
                                       const StyledPalette& palette, StyledState state) const
{
    const Style& s = GetStyle();
    Rect content = UiStyledInnerRect(r, s.cell_metrics, s.cell_skin);
    Color ink = MatrixInk_(palette, state);

    bool has_visual = !IsNull(cell.icon) || cell.glyph != UiMatrixGlyph::None;
    if(!IsNull(cell.icon)) {
        Rect ir = content.Deflated(s.icon_inset);
        Size isz = cell.icon.GetSize();
        if(isz.cx > 0 && isz.cy > 0 && !ir.IsEmpty()) {
            double scale = min(double(ir.GetWidth()) / isz.cx, double(ir.GetHeight()) / isz.cy);
            Size target(max(1, int(isz.cx * scale)), max(1, int(isz.cy * scale)));
            Point at(ir.left + (ir.GetWidth() - target.cx) / 2,
                     ir.top + (ir.GetHeight() - target.cy) / 2);
            w.DrawImage(Rect(at, target), cell.icon);
        }
    }
    else if(cell.glyph != UiMatrixGlyph::None)
        DrawGlyphAA(w, content.Deflated(s.glyph_inset), cell.glyph, ink);

    if(!has_visual && !cell.short_label.IsEmpty()) {
        Size ts = GetTextSize(cell.short_label, s.cell_font);
        w.DrawText(r.left + (r.GetWidth() - ts.cx) / 2,
                   r.top + (r.GetHeight() - ts.cy) / 2,
                   cell.short_label, s.cell_font, ink);
    }
}

Vector<int> UiMatrixSelector::ResolveOverlayPath() const
{
    Vector<int> path;
    if(rows_ != 2 || cols_ != 2 || cells_.GetCount() < 4)
        return path;

    switch(overlay_) {
    case UiMatrixOverlay::PathU:         path << 0 << 2 << 3 << 1; break;
    case UiMatrixOverlay::PathZ:         path << 0 << 1 << 2 << 3; break;
    case UiMatrixOverlay::PathButterfly: path << 0 << 3 << 1 << 2; break;
    case UiMatrixOverlay::CustomPath:    path <<= custom_path_; break;
    default: break;
    }
    return path;
}

void UiMatrixSelector::DrawOverlayAA(Draw& w, const Rect& matrix) const
{
    if(overlay_ == UiMatrixOverlay::None || matrix.IsEmpty() || rows_ != 2 || cols_ != 2)
        return;

    const Style& s = GetStyle();
    Color color = MatrixOverlayColor_(s);
    ImageBuffer ib(matrix.GetSize());
    BufferPainter p(ib, MODE_ANTIALIASED);
    p.Clear(RGBAZero());

    Pointf c[4];
    for(int i = 0; i < 4; i++) {
        Rect r = GetCellRect(i);
        c[i] = Pointf(r.CenterPoint().x - matrix.left, r.CenterPoint().y - matrix.top);
    }

    auto line = [&](int a, int b) {
        p.Begin();
        p.Move(c[a]);
        p.Line(c[b]);
        p.Stroke(max(1, s.overlay_width), color);
        p.End();
    };

    switch(overlay_) {
    case UiMatrixOverlay::DynamicPairs:
        line(0, 3); line(1, 2);
        break;
    case UiMatrixOverlay::CompanionPairs:
        line(0, 1); line(2, 3);
        break;
    case UiMatrixOverlay::DependentPairs:
        line(0, 2); line(1, 3);
        break;
    default: {
        Vector<int> path = ResolveOverlayPath();
        if(path.GetCount() >= 2) {
            p.Begin();
            bool started = false;
            for(int i = 0; i < path.GetCount(); i++) {
                int index = path[i];
                if(index < 0 || index >= 4)
                    continue;
                if(!started) {
                    p.Move(c[index]);
                    started = true;
                }
                else
                    p.Line(c[index]);
            }
            if(started)
                p.Stroke(max(1, s.overlay_width), color);
            p.End();
        }
        break;
    }
    }

    w.DrawImage(matrix.left, matrix.top, ib);
}

void UiMatrixSelector::DrawPairAA(Draw& w, const Rect& matrix) const
{
    if(selection_mode_ != UiMatrixSelectionMode::Pair || !HasCompletePair() || matrix.IsEmpty())
        return;

    Rect a = GetCellRect(pair_first_);
    Rect b = GetCellRect(pair_second_);
    if(a.IsEmpty() || b.IsEmpty())
        return;

    const Style& s = GetStyle();
    Color color = MatrixOverlayColor_(s);
    ImageBuffer ib(matrix.GetSize());
    BufferPainter p(ib, MODE_ANTIALIASED);
    p.Clear(RGBAZero());

    Pointf c0(a.CenterPoint().x - matrix.left, a.CenterPoint().y - matrix.top);
    Pointf c1(b.CenterPoint().x - matrix.left, b.CenterPoint().y - matrix.top);
    double dx = c1.x - c0.x;
    double dy = c1.y - c0.y;
    double len = sqrt(dx * dx + dy * dy);
    if(len <= 0)
        return;
    dx /= len;
    dy /= len;
    double px = -dy;
    double py = dx;

    double endpoint_inset = max(4.0, min(min(a.GetWidth(), a.GetHeight()), min(b.GetWidth(), b.GetHeight())) * 0.18);
    Pointf start(c0.x + dx * endpoint_inset, c0.y + dy * endpoint_inset);
    Pointf end(c1.x - dx * endpoint_inset, c1.y - dy * endpoint_inset);

    p.Begin();
    p.Move(start);
    p.Line(end);
    p.Stroke(max(1, s.overlay_width), color);
    p.End();

    double head = max(3, s.pair_arrow_size);
    double wing = head * 0.62;
    p.Begin();
    p.Move(end);
    p.Line(end.x - dx * head + px * wing, end.y - dy * head + py * wing);
    p.Line(end.x - dx * head - px * wing, end.y - dy * head - py * wing);
    p.Close();
    p.Fill(color);
    p.End();

    w.DrawImage(matrix.left, matrix.top, ib);
}

void UiMatrixSelector::DrawReadout(Draw& w, const Rect& rect, StyledState state) const
{
    if(rect.IsEmpty())
        return;
    const Style& s = GetStyle();
    UiPaintStyledBackground(w, rect, s.readout_palette, s.readout_metrics, s.readout_skin, state, false);
    Color ink = MatrixInk_(s.readout_palette, state);
    Font font = s.readout_font;
    int line_h = max(1, font.GetHeight());

    Vector<String> lines;
    if(selection_mode_ != UiMatrixSelectionMode::Pair) {
        String text = GetSelectedLabel();
        lines.Add(text.IsEmpty() ? String("None") : text);
    }
    else if(!HasPairStart()) {
        lines.Add("Choose first");
    }
    else if(!HasCompletePair()) {
        lines.Add("From " + MatrixCellCompactLabel_(cells_[pair_first_]));
        lines.Add("Choose second");
    }
    else {
        String relationship = GetRelationshipName();
        String orientation = GetPairOrientationName();
        if(!relationship.IsEmpty())
            lines.Add(relationship);
        if(!orientation.IsEmpty())
            lines.Add(orientation);
        lines.Add(MatrixCellCompactLabel_(cells_[pair_first_]) + " -> "
                  + MatrixCellCompactLabel_(cells_[pair_second_]));
    }

    int gap = DPI(2);
    int total_h = lines.GetCount() * line_h + max(0, lines.GetCount() - 1) * gap;
    int y = rect.top + max(0, (rect.GetHeight() - total_h) / 2);
    Rect content = rect.Deflated(DPI(5), DPI(2));
    for(const String& line : lines) {
        Size ts = GetTextSize(line, font);
        int x = content.left + max(0, (content.GetWidth() - ts.cx) / 2);
        DrawTextEllipsis(w, x, y, max(0, content.right - x), line, "...", font, ink);
        y += line_h + gap;
    }
}

void UiMatrixSelector::Paint(Draw& w)
{
    const Style& s = GetStyle();
    Rect outer = Rect(GetSize());
    StyledState base = IsEnabled() && IsShowEnabled() ? ST_NORMAL : ST_DISABLED;

    UiPaintStyledBackground(w, outer, s.surface_palette, s.surface_metrics, s.surface_skin, base, HasFocus());

    for(int i = 0; i < cells_.GetCount(); i++) {
        const Cell& cell = cells_[i];
        if(!cell.visible)
            continue;
        StyledState state = !IsEnabled() || !IsShowEnabled() || !cell.enabled ? ST_DISABLED
                          : pressed_ == i ? ST_PRESSED
                          : hover_ == i ? ST_HOT
                          : ST_NORMAL;
        const StyledPalette& palette = IsCellSelectedVisual(i) ? s.selected_palette : s.cell_palette;
        UiPaintStyledBackground(w, GetCellRect(i), palette, s.cell_metrics, s.cell_skin, state, false);
    }

    DrawOverlayAA(w, GetMatrixRect());
    DrawPairAA(w, GetMatrixRect());

    for(int i = 0; i < cells_.GetCount(); i++) {
        const Cell& cell = cells_[i];
        if(!cell.visible)
            continue;
        StyledState state = !IsEnabled() || !IsShowEnabled() || !cell.enabled ? ST_DISABLED
                          : pressed_ == i ? ST_PRESSED
                          : hover_ == i ? ST_HOT
                          : ST_NORMAL;
        const StyledPalette& palette = IsCellSelectedVisual(i) ? s.selected_palette : s.cell_palette;
        DrawCellContent(w, GetCellRect(i), cell, palette, state);
    }

    if(show_readout_)
        DrawReadout(w, GetReadoutRect(), base);
}

void UiMatrixSelector::ActivateIndex(int index)
{
    if(!IsSelectableCell(index))
        return;

    if(selection_mode_ == UiMatrixSelectionMode::SingleCell) {
        bool changed = selected_ != index;
        selected_ = index;
        Refresh();
        if(changed && WhenChanging)
            WhenChanging();
        if(WhenAction)
            WhenAction();
        return;
    }

    selected_ = index;
    if(pair_first_ < 0 || pair_second_ >= 0) {
        bool changed = pair_first_ != index || pair_second_ >= 0;
        pair_first_ = index;
        pair_second_ = -1;
        Refresh();
        if(changed && WhenChanging)
            WhenChanging();
        return;
    }

    if(index == pair_first_) {
        Refresh();
        return;
    }

    pair_second_ = index;
    Refresh();
    if(WhenChanging)
        WhenChanging();
    if(WhenAction)
        WhenAction();
}

void UiMatrixSelector::LeftDown(Point p, dword)
{
    if(!IsEnabled() || !IsShowEnabled())
        return;
    SetFocus();
    pressed_ = HitTest(p);
    SetHover(pressed_);
    if(pressed_ >= 0)
        SetCapture();
    Refresh();
}

void UiMatrixSelector::LeftUp(Point p, dword)
{
    int was = pressed_;
    pressed_ = -1;
    if(HasCapture())
        ReleaseCapture();
    int hit = HitTest(p);
    SetHover(hit);
    if(was >= 0 && hit == was)
        ActivateIndex(hit);
    Refresh();
}

void UiMatrixSelector::MouseMove(Point p, dword)
{
    SetHover(HitTest(p));
}

void UiMatrixSelector::MouseLeave()
{
    if(!HasCapture())
        SetHover(-1);
}

int UiMatrixSelector::FindNextEnabled(int from, int dx, int dy) const
{
    if(rows_ <= 0 || cols_ <= 0)
        return -1;
    int row = from >= 0 ? from / cols_ : 0;
    int col = from >= 0 ? from % cols_ : 0;
    for(int n = 0; n < rows_ * cols_; n++) {
        row += dy;
        col += dx;
        if(row < 0 || row >= rows_ || col < 0 || col >= cols_)
            return from;
        int i = row * cols_ + col;
        if(IsSelectableCell(i))
            return i;
    }
    return from;
}

bool UiMatrixSelector::Key(dword key, int)
{
    if(key == K_SPACE || key == K_ENTER) {
        if(IsSelectableCell(selected_)) {
            ActivateIndex(selected_);
            return true;
        }
        return false;
    }

    int next = selected_;
    if(key == K_LEFT) next = FindNextEnabled(selected_, -1, 0);
    else if(key == K_RIGHT) next = FindNextEnabled(selected_, 1, 0);
    else if(key == K_UP) next = FindNextEnabled(selected_, 0, -1);
    else if(key == K_DOWN) next = FindNextEnabled(selected_, 0, 1);
    else
        return false;

    if(next >= 0 && next != selected_) {
        selected_ = next;
        if(selection_mode_ == UiMatrixSelectionMode::SingleCell && WhenChanging)
            WhenChanging();
        Refresh();
    }
    return true;
}

void UiMatrixSelector::GotFocus()
{
    Refresh();
}

void UiMatrixSelector::LostFocus()
{
    pressed_ = -1;
    if(HasCapture())
        ReleaseCapture();
    Refresh();
}

}
