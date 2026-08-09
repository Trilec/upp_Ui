#include <Ui/UiColorMatrix.h>
#include <Ui/UiColorPicker/UiColorPicker.h>
#include <Ui/UiTheme.h>
#include <Ui/UiDraw.h>

namespace Upp {

const UiColorMatrix::Style& UiColorMatrix::StyleDefault()
{
    static Style style;
    return style;
}

UiColorMatrix::UiColorMatrix()
    : style_(StyleDefault())
    , themed_style_(StyleDefault())
    , role_(UiRole::Subtle)
    , active_role_(UiRole::Accent)
{
    Transparent();
    WantFocus();
    SetColorCount(1);
}

UiColorMatrix::Style UiColorMatrix::ResolveThemeStyle() const
{
    Style out;

    UiPanel::Style surface = UiTheme::ResolvePanel(role_);
    out.surface_palette = surface.palette;
    out.surface_metrics = surface.metrics;
    out.surface_skin = surface.skin;
    out.surface_metrics.content_margin = Rect(DPI(4), DPI(4), DPI(4), DPI(4));
    out.surface_metrics.face_enabled = true;
    out.surface_metrics.frame_enabled = false;
    out.surface_metrics.shadow.enabled = false;

    UiButton::Style slot = UiTheme::ResolveButton(UiRole::Standard);
    out.slot_palette = slot.palette;
    out.slot_metrics = slot.metrics;
    out.slot_skin = slot.skin;
    out.slot_metrics.content_margin = Rect(0, 0, 0, 0);
    out.slot_metrics.face_enabled = true;
    out.slot_metrics.frame_enabled = true;
    out.slot_metrics.frame_width = max(1, out.slot_metrics.frame_width);
    out.slot_metrics.shadow.enabled = false;

    UiButton::Style active = UiTheme::ResolveButton(active_role_);
    out.active_palette = active.palette;

    out.slot_gap = DPI(4);
    out.minimum_slot_size = DPI(18);
    out.maximum_slot_size = DPI(42);
    return out;
}

void UiColorMatrix::InvalidateThemeStyle()
{
    theme_revision_ = 0;
    RefreshLayout();
    Refresh();
}

void UiColorMatrix::SyncThemeStyle() const
{
    if(has_custom_style_)
        return;
    uint64 revision = UiTheme::GetRevision();
    if(theme_revision_ == revision)
        return;
    themed_style_ = ResolveThemeStyle();
    theme_revision_ = revision;
}

const UiColorMatrix::Style& UiColorMatrix::GetStyle() const
{
    if(has_custom_style_)
        return style_;
    SyncThemeStyle();
    return themed_style_;
}

UiColorMatrix::Style& UiColorMatrix::StyleEdit()
{
    if(!has_custom_style_) {
        style_ = GetStyle();
        has_custom_style_ = true;
    }
    return style_;
}

UiColorMatrix& UiColorMatrix::SetCustomStyle(const Style& style)
{
    style_ = style;
    has_custom_style_ = true;
    RefreshLayout();
    Refresh();
    return *this;
}

UiColorMatrix& UiColorMatrix::ClearCustomStyle()
{
    if(has_custom_style_) {
        has_custom_style_ = false;
        InvalidateThemeStyle();
    }
    return *this;
}

UiColorMatrix& UiColorMatrix::SetRole(UiRole role)
{
    role_ = role;
    if(!has_custom_style_)
        InvalidateThemeStyle();
    return *this;
}

UiColorMatrix& UiColorMatrix::SetActiveRole(UiRole role)
{
    active_role_ = role;
    if(!has_custom_style_)
        InvalidateThemeStyle();
    return *this;
}

void UiColorMatrix::EnsureStorage(int count)
{
    count = minmax(count, 1, MAX_COLORS);
    int old = colors_.GetCount();
    colors_.SetCount(count);
    labels_.SetCount(count);
    for(int i = old; i < count; i++) {
        colors_[i] = White();
        labels_[i] = Format("C%d", i + 1);
    }
    active_ = minmax(active_, 0, count - 1);
}

UiColorMatrix& UiColorMatrix::SetColorCount(int count)
{
    count = minmax(count, 1, MAX_COLORS);
    if(colors_.GetCount() == count)
        return *this;
    EnsureStorage(count);
    RefreshLayout();
    Refresh();
    return *this;
}

UiColorMatrix& UiColorMatrix::SetColor(int index, Color color, bool fire)
{
    if(index < 0 || index >= colors_.GetCount() || IsNull(color))
        return *this;
    if(colors_[index] == color)
        return *this;
    colors_[index] = color;
    Refresh();
    if(fire) {
        if(WhenChanging)
            WhenChanging();
        if(WhenAction)
            WhenAction();
    }
    return *this;
}

Color UiColorMatrix::GetColor(int index) const
{
    return index >= 0 && index < colors_.GetCount() ? colors_[index] : Null;
}

UiColorMatrix& UiColorMatrix::SetColors(const Vector<Color>& colors, bool fire)
{
    int count = minmax(colors.GetCount(), 1, MAX_COLORS);
    EnsureStorage(count);
    bool changed = false;
    for(int i = 0; i < count; i++) {
        Color color = i < colors.GetCount() && !IsNull(colors[i]) ? colors[i] : White();
        if(colors_[i] != color) {
            colors_[i] = color;
            changed = true;
        }
    }
    RefreshLayout();
    Refresh();
    if(changed && fire) {
        if(WhenChanging)
            WhenChanging();
        if(WhenAction)
            WhenAction();
    }
    return *this;
}

UiColorMatrix& UiColorMatrix::SetColorLabel(int index, const String& label)
{
    if(index >= 0 && index < labels_.GetCount()) {
        labels_[index] = label;
        Refresh();
    }
    return *this;
}

String UiColorMatrix::GetColorLabel(int index) const
{
    return index >= 0 && index < labels_.GetCount() ? labels_[index] : String();
}

UiColorMatrix& UiColorMatrix::SetActiveIndex(int index, bool fire)
{
    if(index < 0 || index >= colors_.GetCount() || active_ == index)
        return *this;
    active_ = index;
    Refresh();
    if(fire && WhenSelect)
        WhenSelect(active_);
    return *this;
}

UiColorMatrix& UiColorMatrix::SetSlotGap(int px)
{
    StyleEdit().slot_gap = max(0, px);
    RefreshLayout();
    Refresh();
    return *this;
}

UiColorMatrix& UiColorMatrix::SetSlotRadius(int px)
{
    StyleEdit().slot_metrics.radius = max(0, px);
    Refresh();
    return *this;
}

UiColorMatrix& UiColorMatrix::SetSlotFrameWidth(int px)
{
    StyleEdit().slot_metrics.frame_width = max(0, px);
    StyleEdit().slot_metrics.frame_enabled = px > 0;
    Refresh();
    return *this;
}

UiColorMatrix& UiColorMatrix::ShowSlotFrame(bool on)
{
    StyleEdit().slot_metrics.frame_enabled = on;
    Refresh();
    return *this;
}

UiColorMatrix& UiColorMatrix::SetSlotShadow(bool on)
{
    StyleEdit().slot_metrics.shadow.enabled = on;
    RefreshLayout();
    Refresh();
    return *this;
}

UiColorMatrix& UiColorMatrix::SetSurfaceRadius(int px)
{
    StyleEdit().surface_metrics.radius = max(0, px);
    Refresh();
    return *this;
}

UiColorMatrix& UiColorMatrix::ShowSurface(bool on)
{
    StyleEdit().surface_metrics.face_enabled = on;
    Refresh();
    return *this;
}

UiColorMatrix& UiColorMatrix::ShowSurfaceFrame(bool on)
{
    StyleEdit().surface_metrics.frame_enabled = on;
    Refresh();
    return *this;
}

UiColorMatrix& UiColorMatrix::SetSurfaceShadow(bool on)
{
    StyleEdit().surface_metrics.shadow.enabled = on;
    RefreshLayout();
    Refresh();
    return *this;
}

UiColorMatrix& UiColorMatrix::SetMinimumSlotSize(int px)
{
    StyleEdit().minimum_slot_size = max(1, px);
    RefreshLayout();
    return *this;
}

UiColorMatrix& UiColorMatrix::SetMaximumSlotSize(int px)
{
    StyleEdit().maximum_slot_size = max(1, px);
    RefreshLayout();
    Refresh();
    return *this;
}

UiColorMatrix& UiColorMatrix::EnablePicker(bool on)
{
    picker_enabled_ = on;
    return *this;
}

UiColorMatrix& UiColorMatrix::SetPickerTitle(const String& title)
{
    picker_title_ = title;
    return *this;
}

bool UiColorMatrix::EditColors()
{
    if(!picker_enabled_ || colors_.IsEmpty())
        return false;

    Vector<Color> opening = clone(colors_);
    bool accepted = false;

    TopWindow dialog;
    dialog.Title(picker_title_.IsEmpty() ? String("Colors") : picker_title_);
    dialog.Sizeable().Zoomable();

    UiColorPicker picker;
    picker.SetSlotCount(colors_.GetCount());
    picker.SetActiveSlot(active_);
    picker.SetAlphaEnabled(false);
    for(int i = 0; i < colors_.GetCount(); i++) {
        picker.SetSlotColor(i, colors_[i], false);
        picker.SetSlotLabel(i, labels_[i]);
    }

    auto preview = [&] {
        int count = min(colors_.GetCount(), picker.GetSlotCount());
        for(int i = 0; i < count; i++)
            colors_[i] = picker.GetSlotColor(i);
        Refresh();
        if(WhenChanging)
            WhenChanging();
    };

    picker.WhenChanging = preview;
    picker.WhenSlotChanged = [=](int index) {
        active_ = minmax(index, 0, colors_.GetCount() - 1);
        Refresh();
        if(WhenSelect)
            WhenSelect(active_);
    };
    picker.WhenAccept = [&] {
        preview();
        accepted = true;
        dialog.Break(IDOK);
    };
    picker.WhenCancel = [&] {
        colors_ = clone(opening);
        Refresh();
        dialog.Break(IDCANCEL);
    };

    dialog.Add(picker.SizePos());
    dialog.SetRect(GetWorkArea().CenterRect(Size(DPI(860), DPI(570))));
    dialog.RunAppModal();

    if(!accepted) {
        colors_ = clone(opening);
        Refresh();
    }
    else if(WhenAction)
        WhenAction();
    return accepted;
}

void UiColorMatrix::ResolveGrid(const Rect& inner, int& columns, int& rows, int& slot_size) const
{
    const Style& style = GetStyle();
    const int count = max(1, colors_.GetCount());
    const int gap = max(0, style.slot_gap);

    columns = 1;
    rows = count;
    slot_size = 1;
    int best_size = -1;
    int best_empty = INT_MAX;

    for(int c = 1; c <= count; c++) {
        int r = (count + c - 1) / c;
        int available_w = inner.GetWidth() - max(0, c - 1) * gap;
        int available_h = inner.GetHeight() - max(0, r - 1) * gap;
        if(available_w <= 0 || available_h <= 0)
            continue;
        int side = min(available_w / c, available_h / r);
        side = min(side, max(1, style.maximum_slot_size));
        int empty = r * c - count;
        if(side > best_size || (side == best_size && empty < best_empty)) {
            best_size = side;
            best_empty = empty;
            columns = c;
            rows = r;
            slot_size = max(1, side);
        }
    }
}

Rect UiColorMatrix::GetSlotRect(int index) const
{
    if(index < 0 || index >= colors_.GetCount())
        return Rect(0, 0, 0, 0);

    const Style& style = GetStyle();
    Rect inner = UiStyledInnerRect(Rect(GetSize()), style.surface_metrics, style.surface_skin);
    if(inner.IsEmpty())
        return inner;

    int columns = 1, rows = 1, side = 1;
    ResolveGrid(inner, columns, rows, side);
    int row = index / columns;
    int column = index % columns;
    int row_count = min(columns, colors_.GetCount() - row * columns);
    int row_width = row_count * side + max(0, row_count - 1) * style.slot_gap;
    int grid_height = rows * side + max(0, rows - 1) * style.slot_gap;
    int x0 = inner.left + max(0, (inner.GetWidth() - row_width) / 2);
    int y0 = inner.top + max(0, (inner.GetHeight() - grid_height) / 2);
    return RectC(x0 + column * (side + style.slot_gap),
                 y0 + row * (side + style.slot_gap), side, side);
}

int UiColorMatrix::HitTest(Point p) const
{
    for(int i = 0; i < colors_.GetCount(); i++)
        if(GetSlotRect(i).Contains(p))
            return i;
    return -1;
}

Size UiColorMatrix::GetMinSize() const
{
    const Style& style = GetStyle();
    int count = max(1, colors_.GetCount());
    int columns = min(count, 4);
    int rows = (count + columns - 1) / columns;
    int slot = max(1, style.minimum_slot_size);
    Size content(columns * slot + max(0, columns - 1) * style.slot_gap,
                 rows * slot + max(0, rows - 1) * style.slot_gap);
    return UiStyledOuterSizeFromContent(content, style.surface_metrics, style.surface_skin);
}

void UiColorMatrix::Paint(Draw& draw)
{
    const Style& style = GetStyle();
    StyledState base = IsEnabled() && IsShowEnabled() ? ST_NORMAL : ST_DISABLED;
    UiPaintStyledBackground(draw, Rect(GetSize()), style.surface_palette,
                            style.surface_metrics, style.surface_skin, base, HasFocus());

    for(int i = 0; i < colors_.GetCount(); i++) {
        Rect rect = GetSlotRect(i);
        if(rect.IsEmpty())
            continue;
        StyledState state = !IsEnabled() || !IsShowEnabled() ? ST_DISABLED
                          : pressed_ == i ? ST_PRESSED
                          : hover_ == i ? ST_HOT : ST_NORMAL;
        StyledPalette palette = i == active_ ? style.active_palette : style.slot_palette;
        Color color = colors_[i];
        if(state == ST_DISABLED)
            color = Blend(color, SColorPaper(), 110);
        for(int slot = 0; slot < 4; slot++)
            palette.face[slot] = UiFill::Solid(color);
        UiPaintStyledBackground(draw, rect, palette, style.slot_metrics,
                                style.slot_skin, state, false);
    }
}

void UiColorMatrix::SetHover(int index)
{
    if(hover_ != index) {
        hover_ = index;
        Refresh();
    }
}

void UiColorMatrix::ActivateIndex(int index)
{
    if(index < 0 || index >= colors_.GetCount())
        return;
    bool changed = active_ != index;
    active_ = index;
    Refresh();
    if(changed && WhenSelect)
        WhenSelect(active_);
    EditColors();
}

void UiColorMatrix::LeftDown(Point p, dword)
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

void UiColorMatrix::LeftUp(Point p, dword)
{
    int was = pressed_;
    pressed_ = -1;
    int hit = HitTest(p);
    if(HasCapture())
        ReleaseCapture();
    SetHover(hit);
    if(was >= 0 && hit == was)
        ActivateIndex(hit);
    Refresh();
}

void UiColorMatrix::MouseMove(Point p, dword)
{
    SetHover(HitTest(p));
}

void UiColorMatrix::MouseLeave()
{
    if(!HasCapture())
        SetHover(-1);
}

bool UiColorMatrix::Key(dword key, int)
{
    if(colors_.IsEmpty())
        return false;
    if(key == K_SPACE || key == K_ENTER) {
        EditColors();
        return true;
    }

    Rect inner = UiStyledInnerRect(Rect(GetSize()), GetStyle().surface_metrics, GetStyle().surface_skin);
    int columns = 1, rows = 1, side = 1;
    ResolveGrid(inner, columns, rows, side);
    int next = active_;
    if(key == K_LEFT)
        next = max(0, active_ - 1);
    else if(key == K_RIGHT)
        next = min(colors_.GetCount() - 1, active_ + 1);
    else if(key == K_UP)
        next = max(0, active_ - columns);
    else if(key == K_DOWN)
        next = min(colors_.GetCount() - 1, active_ + columns);
    else
        return false;

    if(next != active_) {
        active_ = next;
        Refresh();
        if(WhenSelect)
            WhenSelect(active_);
    }
    return true;
}

void UiColorMatrix::GotFocus()
{
    Refresh();
}

void UiColorMatrix::LostFocus()
{
    pressed_ = -1;
    if(HasCapture())
        ReleaseCapture();
    Refresh();
}

void UiColorMatrix::SetData(const Value& value)
{
    if(value.Is<ValueArray>()) {
        ValueArray array = value;
        Vector<Color> colors;
        for(int i = 0; i < array.GetCount() && colors.GetCount() < MAX_COLORS; i++)
            if(array[i].Is<Color>())
                colors.Add((Color)array[i]);
        if(!colors.IsEmpty())
            SetColors(colors, false);
        return;
    }
    if(value.Is<Color>()) {
        Vector<Color> colors;
        colors.Add((Color)value);
        SetColors(colors, false);
    }
}

Value UiColorMatrix::GetData() const
{
    if(colors_.GetCount() == 1)
        return colors_[0];
    ValueArray array;
    for(const Color& color : colors_)
        array.Add(color);
    return array;
}

} // namespace Upp
