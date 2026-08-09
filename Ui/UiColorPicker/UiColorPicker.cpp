#include <Ui/UiColorPicker/UiColorPicker.h>
#include <Ui/UiColorPicker/UiColorPickerPaletteLab.h>

#include <cmath>

#ifdef PLATFORM_WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace Upp {

using namespace UiColorPickerPaletteLab;

namespace {

static int ClampByte_(int value)
{
    return minmax(value, 0, 255);
}

static Color AlphaComposite_(Color foreground, int alpha, Color background)
{
    int a = ClampByte_(alpha);
    int ia = 255 - a;
    return Color((foreground.GetR() * a + background.GetR() * ia + 127) / 255,
                 (foreground.GetG() * a + background.GetG() * ia + 127) / 255,
                 (foreground.GetB() * a + background.GetB() * ia + 127) / 255);
}

static Color CheckerColor_(int x, int y, int tile = 4)
{
    bool alternate = ((x / max(1, tile)) + (y / max(1, tile))) & 1;
    Color paper = SColorPaper();
    Color ink = SColorText();
    return alternate ? Blend(paper, ink, 34) : Blend(paper, ink, 18);
}

static Color SurfaceColor_(UiRole role = UiRole::Subtle)
{
    const UiPanel::Style& style = UiTheme::ResolvePanel(role);
    return style.palette.face[ST_NORMAL].IsSolid() ? style.palette.face[ST_NORMAL].color
                                                   : SColorPaper();
}

static Color FrameColor_(UiRole role = UiRole::Subtle)
{
    const UiPanel::Style& style = UiTheme::ResolvePanel(role);
    return IsNull(style.palette.frame[ST_NORMAL])
         ? Blend(SurfaceColor_(role), SColorText(), 36)
         : style.palette.frame[ST_NORMAL];
}

static void DrawFrame_(Draw& draw, const Rect& rect, Color color, int width = 1)
{
    if(rect.IsEmpty() || IsNull(color))
        return;
    width = max(1, width);
    draw.DrawRect(rect.left, rect.top, rect.GetWidth(), width, color);
    draw.DrawRect(rect.left, rect.bottom - width, rect.GetWidth(), width, color);
    draw.DrawRect(rect.left, rect.top, width, rect.GetHeight(), color);
    draw.DrawRect(rect.right - width, rect.top, width, rect.GetHeight(), color);
}

static void DrawAlphaSwatch_(Draw& draw, const Rect& rect, Color color, int alpha)
{
    if(rect.IsEmpty() || IsNull(color))
        return;
    alpha = ClampByte_(alpha);
    int tile = max(2, DPI(4));
    for(int y = rect.top; y < rect.bottom; y += tile) {
        for(int x = rect.left; x < rect.right; x += tile) {
            Color background = CheckerColor_(x - rect.left, y - rect.top, tile);
            Color output = alpha >= 255 ? color : AlphaComposite_(color, alpha, background);
            draw.DrawRect(x, y,
                          min(tile, rect.right - x),
                          min(tile, rect.bottom - y), output);
        }
    }
}

static Image MakeAlphaSwatchImage_(Color color, int alpha, Size size, bool split = false)
{
    if(size.IsEmpty() || IsNull(color))
        return Image();
    ImageBuffer buffer(size);
    int tile = max(2, DPI(4));
    for(int y = 0; y < size.cy; y++) {
        RGBA *row = buffer[y];
        for(int x = 0; x < size.cx; x++) {
            Color background = CheckerColor_(x, y, tile);
            Color output = split && x < size.cx / 2
                         ? color
                         : AlphaComposite_(color, alpha, background);
            row[x] = RGBA(output);
            row[x].a = 255;
        }
    }
    return Image(buffer);
}

static bool ReadScreenColor_(Color& color)
{
#ifdef PLATFORM_WIN32
    POINT point;
    if(!::GetCursorPos(&point))
        return false;
    HDC dc = ::GetDC(nullptr);
    if(!dc)
        return false;
    COLORREF pixel = ::GetPixel(dc, point.x, point.y);
    ::ReleaseDC(nullptr, dc);
    if(pixel == CLR_INVALID)
        return false;
    color = Color(GetRValue(pixel), GetGValue(pixel), GetBValue(pixel));
    return true;
#else
    (void)color;
    return false;
#endif
}

static Color SampleImage_(const Image& image, Pointf normalized, int radius = 2)
{
    if(image.IsEmpty())
        return Null;
    Size size = image.GetSize();
    int cx = minmax(int(normalized.x * size.cx), 0, size.cx - 1);
    int cy = minmax(int(normalized.y * size.cy), 0, size.cy - 1);
    int64 sr = 0, sg = 0, sb = 0, weight = 0;
    Color centre(image[cy][cx].r, image[cy][cx].g, image[cy][cx].b);
    double cl = 0.0, ca = 0.0, cb = 0.0;
    ColorToLab(centre, cl, ca, cb);
    for(int y = max(0, cy - radius); y <= min(size.cy - 1, cy + radius); y++) {
        const RGBA *row = image[y];
        for(int x = max(0, cx - radius); x <= min(size.cx - 1, cx + radius); x++) {
            if(row[x].a < 16)
                continue;
            Color sample(row[x].r, row[x].g, row[x].b);
            double l = 0.0, a = 0.0, b = 0.0;
            ColorToLab(sample, l, a, b);
            double distance = sqrt((l - cl) * (l - cl) + (a - ca) * (a - ca) + (b - cb) * (b - cb));
            if(distance > 18.0)
                continue;
            int w = radius + 2 - max(abs(x - cx), abs(y - cy));
            sr += row[x].r * w;
            sg += row[x].g * w;
            sb += row[x].b * w;
            weight += w;
        }
    }
    return weight > 0 ? Color(int(sr / weight), int(sg / weight), int(sb / weight)) : centre;
}

static Point WheelPoint_(int hue, double saturation, const Rect& wheel)
{
    double angle = (NormalizeHue(hue) - 90.0) * M_PI / 180.0;
    double radius = minmax(saturation, 0.0, 100.0) / 100.0 * wheel.GetWidth() * 0.5;
    Point centre = wheel.CenterPoint();
    return Point(centre.x + int(cos(angle) * radius + 0.5),
                 centre.y + int(sin(angle) * radius + 0.5));
}

static void PointToWheel_(Point point, const Rect& wheel, int& hue, int& saturation)
{
    Point centre = wheel.CenterPoint();
    double dx = point.x - centre.x;
    double dy = point.y - centre.y;
    double radius = max(1.0, wheel.GetWidth() * 0.5);
    hue = NormalizeHue(int(atan2(dy, dx) * 180.0 / M_PI + 90.0 + 0.5));
    saturation = minmax(int(sqrt(dx * dx + dy * dy) / radius * 100.0 + 0.5), 0, 100);
}

class CommitLineEdit_ : public UiLineEdit {
public:
    typedef CommitLineEdit_ CLASSNAME;
    Event<> WhenCommit;

    virtual void LostFocus() override
    {
        UiLineEdit::LostFocus();
        if(WhenCommit)
            WhenCommit();
    }
};

struct DisplaySwatch_ : Moveable<DisplaySwatch_> {
    UiColorPicker::SlotValue value;
    int family_id = -1;
    int hero_number = 0;
    bool hero = false;
    bool gamut_mapped = false;
};

class PaletteDragSource_ {
public:
    virtual ~PaletteDragSource_() {}
    virtual Vector<UiColorPicker::SlotValue> GetPaletteDragValues() const = 0;
};

class SwatchFlow_;

class SwatchTile_ : public UiToolButton {
public:
    typedef SwatchTile_ CLASSNAME;

    void Configure(SwatchFlow_& owner, int index)
    {
        owner_ = &owner;
        index_ = index;
        SetText("");
        SetContentInset(0);
        SetContentGap(0);
        WantFocus();
    }

    void SetDisplay(const DisplaySwatch_& display, bool selected, bool active, bool compact)
    {
        display_ = display;
        selected_ = selected;
        active_ = active;
        compact_ = compact;
        if(IsNull(display.value.color))
            Tip("Empty User Stash cell");
        else
            Tip(display.value.label.IsEmpty()
                ? FormatHex8(display.value.color, display.value.alpha)
                : Format("%s — %s", display.value.label, FormatHex8(display.value.color, display.value.alpha)));
        Refresh();
    }

    virtual Size GetMinSize() const override
    {
        return Size(DPI(26), DPI(26));
    }

    virtual void Paint(Draw& draw) override
    {
        UiToolButton::Paint(draw);
        Rect rect(Point(0, 0), GetSize());
        Rect swatch = rect.Deflated(compact_ ? DPI(2) : DPI(3));
        DrawAlphaSwatch_(draw, swatch, display_.value.color, display_.value.alpha);
        Color frame = active_ ? SColorHighlight()
                    : selected_ ? Blend(SColorHighlight(), SColorPaper(), 65)
                                : FrameColor_();
        DrawFrame_(draw, rect.Deflated(DPI(1)), frame, active_ ? DPI(2) : DPI(1));
        if(display_.hero_number > 0 && !compact_) {
            int diameter = DPI(15);
            Rect badge = RectC(swatch.left + DPI(2), swatch.top + DPI(2), diameter, diameter);
            draw.DrawEllipse(badge, SColorPaper(), DPI(1), SColorShadow());
            String number = AsString(display_.hero_number);
            Font font = SansSerif().Height(DPI(9)).Bold();
            Size text = GetTextSize(number, font);
            draw.DrawText(badge.left + (badge.GetWidth() - text.cx) / 2,
                          badge.top + (badge.GetHeight() - text.cy) / 2,
                          number, font, SColorText());
        }
        if(display_.hero && !compact_) {
            int diameter = DPI(13);
            Rect hero = RectC(swatch.right - diameter - DPI(2), swatch.bottom - diameter - DPI(2), diameter, diameter);
            draw.DrawEllipse(hero, SColorPaper(), DPI(1), SColorHighlight());
            String mark = "*";
            Font font = SansSerif().Height(DPI(10)).Bold();
            Size text = GetTextSize(mark, font);
            draw.DrawText(hero.left + (hero.GetWidth() - text.cx) / 2,
                          hero.top + (hero.GetHeight() - text.cy) / 2 - DPI(1),
                          mark, font, SColorHighlight());
        }
        if(display_.gamut_mapped && !compact_) {
            String mark = "!";
            Font font = SansSerif().Height(DPI(9)).Bold();
            Size text = GetTextSize(mark, font);
            draw.DrawText(swatch.right - text.cx - DPI(2), swatch.top + DPI(1), mark, font, SColorHighlight());
        }
    }

    virtual void LeftDown(Point point, dword flags) override;
    virtual void LeftUp(Point point, dword flags) override;
    virtual void LeftDouble(Point point, dword flags) override;
    virtual void LeftDrag(Point point, dword flags) override;
    virtual void MouseMove(Point point, dword flags) override;
    virtual void DragEnter() override;
    virtual void DragAndDrop(Point point, PasteClip& clip) override;
    virtual void DragLeave() override;

private:
    SwatchFlow_ *owner_ = nullptr;
    int index_ = -1;
    DisplaySwatch_ display_;
    bool selected_ = false;
    bool active_ = false;
    bool compact_ = false;
};

class SwatchFlow_ : public UiBoxLayout, public PaletteDragSource_ {
public:
    typedef SwatchFlow_ CLASSNAME;

    SwatchFlow_()
        : UiBoxLayout(UiDirection::H)
    {
        SetWrap(UiBoxWrap::Snap)
            .SetWrapAutoResize(true)
            .SetWrapRowsExpand(false)
            .SetGap(DPI(3))
            .SetInset(DPI(1));
        WantFocus();
    }

    void SetSnapColumns(int columns)
    {
        columns_ = max(1, columns);
        SetWrapSnapCount(columns_);
    }

    void SetCompact(bool compact)
    {
        if(compact_ == compact)
            return;
        compact_ = compact;
        SyncTiles();
    }

    void EnableDropTarget(bool on = true)
    {
        accept_drop_ = on;
    }

    void SetItems(const Vector<DisplaySwatch_>& items, bool preserve_selection = false)
    {
        Vector<int> old_selected = clone(selected_);
        int old_active = active_index_;
        items_ = clone(items);
        if(tiles_.GetCount() != items_.GetCount()) {
            ClearItems();
            tiles_.Clear();
            UiBoxLayout::PauseScope pause(*this);
            for(int i = 0; i < items_.GetCount(); i++) {
                One<SwatchTile_>& tile = tiles_.Add();
                tile.Create();
                tile->Configure(*this, i);
                Add(*tile).Fixed(DPI(26)).MinMaxHeight(DPI(26), DPI(26));
            }
        }
        selected_.Clear();
        active_index_ = -1;
        anchor_index_ = -1;
        if(preserve_selection) {
            for(int index : old_selected)
                if(index >= 0 && index < items_.GetCount())
                    selected_.Add(index);
            if(old_active >= 0 && old_active < items_.GetCount())
                active_index_ = old_active;
            if(!selected_.IsEmpty())
                anchor_index_ = selected_[0];
        }
        SyncTiles();
    }

    void SetSlotValues(const Vector<UiColorPicker::SlotValue>& values, bool preserve_selection = false)
    {
        Vector<DisplaySwatch_> display;
        for(const UiColorPicker::SlotValue& value : values) {
            DisplaySwatch_& item = display.Add();
            item.value = value;
        }
        SetItems(display, preserve_selection);
    }

    const Vector<DisplaySwatch_>& GetItems() const { return items_; }

    void SetSelectedIndex(int index)
    {
        selected_.Clear();
        active_index_ = index >= 0 && index < items_.GetCount() ? index : -1;
        anchor_index_ = active_index_;
        if(active_index_ >= 0)
            selected_.Add(active_index_);
        SyncTiles();
    }

    void SetSelectedIndices(const Vector<int>& indices, int active = -1)
    {
        selected_.Clear();
        for(int index : indices)
            if(index >= 0 && index < items_.GetCount() && FindIndex(selected_, index) < 0)
                selected_.Add(index);
        Sort(selected_);
        active_index_ = active >= 0 && FindIndex(selected_, active) >= 0
                      ? active
                      : selected_.IsEmpty() ? -1 : selected_[0];
        anchor_index_ = active_index_;
        SyncTiles();
    }

    int GetActiveIndex() const { return active_index_; }
    const Vector<int>& GetSelectedIndices() const { return selected_; }

    Vector<UiColorPicker::SlotValue> GetSelectedValues() const
    {
        Vector<UiColorPicker::SlotValue> output;
        for(int index : selected_)
            if(index >= 0 && index < items_.GetCount() && !IsNull(items_[index].value.color))
                output.Add(items_[index].value);
        return output;
    }

    virtual Vector<UiColorPicker::SlotValue> GetPaletteDragValues() const override
    {
        if(drag_origin_ >= 0 && FindIndex(selected_, drag_origin_) < 0 && drag_origin_ < items_.GetCount()) {
            Vector<UiColorPicker::SlotValue> one;
            one.Add(items_[drag_origin_].value);
            return one;
        }
        return GetSelectedValues();
    }

    void SelectAll()
    {
        selected_.Clear();
        for(int i = 0; i < items_.GetCount(); i++)
            if(!IsNull(items_[i].value.color))
                selected_.Add(i);
        active_index_ = selected_.IsEmpty() ? -1 : selected_[0];
        anchor_index_ = active_index_;
        SyncTiles();
        FireSelection();
    }

    void HandleTileDown(int index, dword flags)
    {
        if(index < 0 || index >= items_.GetCount() || IsNull(items_[index].value.color))
            return;
        SetFocus();
        bool range = (flags & K_SHIFT) && anchor_index_ >= 0;
        bool toggle = (flags & K_CTRL) != 0;
        if(range) {
            if(!toggle)
                selected_.Clear();
            int first = min(anchor_index_, index);
            int last = max(anchor_index_, index);
            for(int i = first; i <= last; i++)
                if(!IsNull(items_[i].value.color) && FindIndex(selected_, i) < 0)
                    selected_.Add(i);
        }
        else if(toggle) {
            int found = FindIndex(selected_, index);
            if(found >= 0)
                selected_.Remove(found);
            else
                selected_.Add(index);
            anchor_index_ = index;
        }
        else {
            selected_.Clear();
            selected_.Add(index);
            anchor_index_ = index;
        }
        Sort(selected_);
        active_index_ = FindIndex(selected_, index) >= 0 ? index
                      : selected_.IsEmpty() ? -1 : selected_[0];
        drag_selecting_ = true;
        SyncTiles();
        FireSelection();
    }

    void HandleTileDouble(int index)
    {
        if(index < 0 || index >= items_.GetCount() || IsNull(items_[index].value.color))
            return;
        SetSelectedIndex(index);
        FireSelection();
        if(WhenActivate)
            WhenActivate(index, items_[index].value);
    }

    void HandleTileMove(int index, dword flags)
    {
        if(!drag_selecting_ || !(flags & K_MOUSELEFT) || index < 0 || index >= items_.GetCount())
            return;
        if(IsNull(items_[index].value.color) || FindIndex(selected_, index) >= 0)
            return;
        selected_.Add(index);
        Sort(selected_);
        active_index_ = index;
        SyncTiles();
        FireSelection();
    }

    void EndPointerSelection()
    {
        drag_selecting_ = false;
    }

    void BeginTileDrag(int index)
    {
        if(index < 0 || index >= items_.GetCount() || IsNull(items_[index].value.color))
            return;
        drag_origin_ = index;
        Vector<UiColorPicker::SlotValue> values = GetPaletteDragValues();
        if(values.IsEmpty())
            return;
        VectorMap<String, ClipData> payload = InternalClip<PaletteDragSource_>(*this, "uicolor-palette-group");
        String text;
        for(int i = 0; i < values.GetCount(); i++) {
            if(i)
                text << '\n';
            text << FormatHex8(values[i].color, values[i].alpha);
        }
        Append(payload, text);
        DoDragAndDrop(payload,
                      MakeAlphaSwatchImage_(values[0].color, values[0].alpha,
                                            Size(DPI(34), DPI(34)), true),
                      DND_COPY);
        drag_origin_ = -1;
    }

    void HandleDragEnter()
    {
        if(accept_drop_) {
            drop_hot_ = true;
            Refresh();
        }
    }

    void HandleDrop(int target_index, PasteClip& clip)
    {
        if(!accept_drop_ || !IsAvailableInternal<PaletteDragSource_>(clip, "uicolor-palette-group")) {
            clip.Reject();
            drop_hot_ = false;
            Refresh();
            return;
        }
        AcceptInternal<PaletteDragSource_>(clip, "uicolor-palette-group");
        clip.SetAction(DND_COPY);
        if(clip.IsPaste()) {
            const PaletteDragSource_ *source = GetInternalPtr<PaletteDragSource_>(clip, "uicolor-palette-group");
            if(source && WhenDropGroup)
                WhenDropGroup(target_index, source->GetPaletteDragValues());
            drop_hot_ = false;
            Refresh();
        }
    }

    void HandleDragLeave()
    {
        if(drop_hot_) {
            drop_hot_ = false;
            Refresh();
        }
    }

    Event<int, UiColorPicker::SlotValue> WhenActivate;
    Event<int, UiColorPicker::SlotValue> WhenSelection;
    Event<int, const Vector<UiColorPicker::SlotValue>&> WhenDropGroup;

    virtual void Paint(Draw& draw) override
    {
        UiBoxLayout::Paint(draw);
        if(drop_hot_)
            DrawFrame_(draw, Rect(Point(0, 0), GetSize()).Deflated(DPI(1)), SColorHighlight(), DPI(2));
    }

private:
    void SyncTiles()
    {
        for(int i = 0; i < tiles_.GetCount(); i++)
            tiles_[i]->SetDisplay(items_[i], FindIndex(selected_, i) >= 0, i == active_index_, compact_);
        RefreshLayout();
        Refresh();
    }

    void FireSelection()
    {
        if(active_index_ >= 0 && active_index_ < items_.GetCount() && WhenSelection)
            WhenSelection(active_index_, items_[active_index_].value);
    }

    Vector<DisplaySwatch_> items_;
    Vector<One<SwatchTile_>> tiles_;
    Vector<int> selected_;
    int active_index_ = -1;
    int anchor_index_ = -1;
    int drag_origin_ = -1;
    int columns_ = 6;
    bool compact_ = false;
    bool accept_drop_ = false;
    bool drop_hot_ = false;
    bool drag_selecting_ = false;
};

void SwatchTile_::LeftDown(Point point, dword flags)
{
    if(owner_)
        owner_->HandleTileDown(index_, flags);
    UiToolButton::LeftDown(point, flags);
}

void SwatchTile_::LeftUp(Point point, dword flags)
{
    if(owner_)
        owner_->EndPointerSelection();
    UiToolButton::LeftUp(point, flags);
}

void SwatchTile_::LeftDouble(Point, dword)
{
    if(owner_)
        owner_->HandleTileDouble(index_);
}

void SwatchTile_::LeftDrag(Point, dword)
{
    if(owner_)
        owner_->BeginTileDrag(index_);
}

void SwatchTile_::MouseMove(Point point, dword flags)
{
    if(owner_)
        owner_->HandleTileMove(index_, flags);
    UiToolButton::MouseMove(point, flags);
}

void SwatchTile_::DragEnter()
{
    if(owner_)
        owner_->HandleDragEnter();
}

void SwatchTile_::DragAndDrop(Point, PasteClip& clip)
{
    if(owner_)
        owner_->HandleDrop(index_, clip);
}

void SwatchTile_::DragLeave()
{
    if(owner_)
        owner_->HandleDragLeave();
}

class PickerSlider_ : public UiSlider {
public:
    PickerSlider_(UiDirection direction = UiDirection::H)
        : UiSlider(direction)
    {
    }

    virtual Size GetMinSize() const override
    {
        Size size = UiSlider::GetMinSize();
        if(GetDirection() == UiDirection::H)
            size.cx = DPI(50);
        else
            size.cy = DPI(50);
        return size;
    }
};

class LabeledDrop_ : public UiBoxLayout {
public:
    LabeledDrop_()
        : UiBoxLayout(UiDirection::V)
    {
        SetGap(DPI(1));
        Add(label_).Fixed(DPI(12));
        Add(drop_).Expand(1);
        Add(slider_row_).Fixed(0);
        label_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        slider_row_.SetGap(DPI(4));
        slider_row_.Add(slider_).Expand(1);
        slider_row_.Add(slider_value_).Fixed(DPI(28));
        slider_value_.SetAlign(UiAlign::RIGHT, UiAlign::CENTER);
        slider_.SetTrackSize(Size(DPI(72), DPI(5)))
               .SetThumbSize(Size(DPI(14), DPI(18)));
        slider_.WhenChanging = [=] { SliderChanged(); };
        slider_.WhenAction = [=] { SliderChanged(); };
    }

    LabeledDrop_& SetLabel(const String& text)
    {
        label_.SetText(text);
        // Base-swatch count is intentionally a live 2..12 slider.  The hidden
        // dropdown remains the state/event adapter so persistence and existing
        // picker wiring keep one authority for the count value.
        if(text == "Base swatches")
            UseSliderRange(2, 12, 6);
        return *this;
    }

    LabeledDrop_& UseSliderRange(int minimum, int maximum, int value)
    {
        slider_mode_ = true;
        slider_min_ = minimum;
        slider_max_ = max(minimum, maximum);
        slider_syncing_ = true;
        slider_.SetRange(slider_min_, slider_max_).SetStep(1).SetValue(minmax(value, slider_min_, slider_max_));
        slider_value_.SetText(AsString(minmax(value, slider_min_, slider_max_)));
        slider_syncing_ = false;
        ItemAt(1).Fixed(0);
        ItemAt(2).Expand(1);
        RefreshLayout();
        return *this;
    }

    UiDropdown& Drop() { return drop_; }

    virtual void Layout() override
    {
        if(slider_mode_) {
            Value data = drop_.GetData();
            if(!IsNull(data)) {
                int value = minmax((int)data, slider_min_, slider_max_);
                if(int(slider_.GetValue() + 0.5) != value) {
                    slider_syncing_ = true;
                    slider_.SetValue(value);
                    slider_value_.SetText(AsString(value));
                    slider_syncing_ = false;
                }
            }
            int slider_width = max(DPI(52), GetSize().cx - DPI(28) - DPI(4));
            slider_.SetTrackSize(Size(slider_width, DPI(5)));
        }
        UiBoxLayout::Layout();
        if(slider_mode_)
            slider_row_.Layout();
    }

private:
    void SliderChanged()
    {
        if(slider_syncing_)
            return;
        int value = minmax(int(slider_.GetValue() + 0.5), slider_min_, slider_max_);
        slider_value_.SetText(AsString(value));
        drop_.SetDataSilently(value);
        if(drop_.WhenSelectData)
            drop_.WhenSelectData(Value(value));
    }

    UiLabel label_;
    UiDropdown drop_;
    UiBoxLayout slider_row_ { UiDirection::H };
    PickerSlider_ slider_ { UiDirection::H };
    UiLabel slider_value_;
    int slider_min_ = 0;
    int slider_max_ = 100;
    bool slider_mode_ = false;
    bool slider_syncing_ = false;
};

class SliderRow_ : public UiBoxLayout {
public:
    typedef SliderRow_ CLASSNAME;

    SliderRow_()
        : UiBoxLayout(UiDirection::H)
    {
        SetGap(DPI(4));
        Add(label_).Fixed(DPI(78));
        Add(slider_).Expand(1);
        Add(edit_).Fixed(DPI(48));
        label_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        edit_.SetTextAlign(UiAlign::RIGHT);
        slider_.SetTrackSize(Size(DPI(120), DPI(5)))
               .SetThumbSize(Size(DPI(14), DPI(18)));
        slider_.WhenChanging = [=] {
            if(syncing_)
                return;
            int value = slider_.GetValue();
            SyncEdit(value);
            if(WhenValue)
                WhenValue(value, false);
        };
        slider_.WhenAction = [=] {
            if(syncing_)
                return;
            int value = slider_.GetValue();
            SyncEdit(value);
            if(WhenValue)
                WhenValue(value, true);
        };
        edit_.WhenAction = [=] { CommitEdit(); };
        edit_.WhenCommit = [=] { CommitEdit(); };
    }

    SliderRow_& Configure(const String& label, int minimum, int maximum, int value)
    {
        minimum_ = minimum;
        maximum_ = max(minimum, maximum);
        label_.SetText(label);
        slider_.SetRange(minimum_, maximum_).SetStep(1);
        SetValue(value);
        return *this;
    }

    SliderRow_& SetLabel(const String& label) { label_.SetText(label); return *this; }
    void SetValue(int value)
    {
        value = minmax(value, minimum_, maximum_);
        syncing_ = true;
        slider_.SetValue(value);
        SyncEdit(value);
        syncing_ = false;
    }
    int GetValue() const { return slider_.GetValue(); }
    UiSlider& Slider() { return slider_; }
    CommitLineEdit_& Edit() { return edit_; }

    Event<int, bool> WhenValue;

    virtual void Layout() override
    {
        int slider_width = max(DPI(52), GetSize().cx - DPI(78) - DPI(48) - DPI(8));
        slider_.SetTrackSize(Size(slider_width, DPI(5)));
        UiBoxLayout::Layout();
    }

private:
    void SyncEdit(int value)
    {
        String text = AsString(value);
        if(edit_.GetTextUtf8() != text) {
            edit_.SetTextUtf8(text);
            edit_.ClearDirty();
        }
    }

    void CommitEdit()
    {
        if(syncing_)
            return;
        int value = StrInt(edit_.GetTextUtf8());
        value = minmax(value, minimum_, maximum_);
        SetValue(value);
        if(WhenValue)
            WhenValue(value, true);
    }

    UiLabel label_;
    PickerSlider_ slider_ { UiDirection::H };
    CommitLineEdit_ edit_;
    int minimum_ = 0;
    int maximum_ = 100;
    bool syncing_ = false;
};

class NumericRow_ : public UiBoxLayout {
public:
    typedef NumericRow_ CLASSNAME;

    NumericRow_()
        : UiBoxLayout(UiDirection::H)
    {
        SetGap(DPI(4));
        Add(label_).Fixed(DPI(28));
        Add(slider_).Expand(1);
        Add(edit_).Fixed(DPI(70));
        label_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        edit_.SetTextAlign(UiAlign::RIGHT);
        slider_.SetTrackSize(Size(DPI(120), DPI(5)))
               .SetThumbSize(Size(DPI(14), DPI(18)));
        slider_.WhenChanging = [=] { SliderChanged(false); };
        slider_.WhenAction = [=] { SliderChanged(true); };
        edit_.WhenAction = [=] { CommitEdit(); };
        edit_.WhenCommit = [=] { CommitEdit(); };
    }

    void Configure(const String& label, double minimum, double maximum, int precision, int steps, bool alpha)
    {
        label_.SetText(label);
        minimum_ = minimum;
        maximum_ = maximum;
        precision_ = precision;
        steps_ = max(1, steps);
        alpha_ = alpha;
        slider_.SetRange(0, steps_).SetStep(1);
    }

    void SetValue(double value)
    {
        value = minmax(value, minimum_, maximum_);
        syncing_ = true;
        int position = int((value - minimum_) / max(1e-12, maximum_ - minimum_) * steps_ + 0.5);
        slider_.SetValue(position);
        String text = precision_ > 0 ? Format("%.*f", precision_, value)
                                     : AsString(int(value + (value >= 0 ? 0.5 : -0.5)));
        if(!(edit_.HasFocus() && edit_.IsDirty())) {
            edit_.SetTextUtf8(text);
            edit_.ClearDirty();
        }
        syncing_ = false;
    }

    double GetValue() const
    {
        return minimum_ + slider_.GetValue() / double(max(1, steps_)) * (maximum_ - minimum_);
    }

    void EnableAlpha(bool enabled)
    {
        if(alpha_) {
            label_.Enable(enabled);
            slider_.Enable(enabled);
            edit_.Enable(enabled);
        }
    }

    Event<double, bool> WhenValue;

    virtual void Layout() override
    {
        int slider_width = max(DPI(52), GetSize().cx - DPI(28) - DPI(70) - DPI(8));
        slider_.SetTrackSize(Size(slider_width, DPI(5)));
        UiBoxLayout::Layout();
    }

private:
    void SliderChanged(bool final_commit)
    {
        if(syncing_)
            return;
        double value = GetValue();
        SetValue(value);
        if(WhenValue)
            WhenValue(value, final_commit);
    }

    void CommitEdit()
    {
        if(syncing_)
            return;
        double value = StrDbl(edit_.GetTextUtf8());
        value = minmax(value, minimum_, maximum_);
        SetValue(value);
        if(WhenValue)
            WhenValue(value, true);
    }

    UiLabel label_;
    PickerSlider_ slider_ { UiDirection::H };
    CommitLineEdit_ edit_;
    double minimum_ = 0.0;
    double maximum_ = 1.0;
    int precision_ = 4;
    int steps_ = 10000;
    bool alpha_ = false;
    bool syncing_ = false;
};

class ReadoutRow_ : public UiBoxLayout {
public:
    typedef ReadoutRow_ CLASSNAME;

    ReadoutRow_()
        : UiBoxLayout(UiDirection::H)
    {
        SetGap(DPI(2));
        Add(title_).Fixed(DPI(60));
        Add(edit_).Expand(1);
        Add(copy_).Fixed(DPI(24));
        title_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        edit_.SetTextAlign(UiAlign::LEFT);
        copy_.SetText("")
             .SetIcon(ICON_CONTENT_CONTENT_COPY_48())
             .SetIconSize(DPI(13), DPI(13))
             .SetIconRenderMode(UiIconRenderMode::MonoTint);
        copy_.WhenAction = [=] {
            if(!edit_.IsEmpty())
                WriteClipboardText(edit_.GetTextUtf8());
        };
        edit_.WhenAction = [=] { Commit(); };
        edit_.WhenCommit = [=] { Commit(); };
    }

    void SetTitle(const String& title) { title_.SetText(title); }
    void SetValue(const String& value)
    {
        if(edit_.HasFocus() && edit_.IsDirty())
            return;
        edit_.SetTextUtf8(value);
        edit_.ClearDirty();
    }
    String GetValue() const { return edit_.GetTextUtf8(); }
    Event<String> WhenCommitText;

private:
    void Commit()
    {
        if(WhenCommitText)
            WhenCommitText(edit_.GetTextUtf8());
        edit_.ClearDirty();
    }

    UiLabel title_;
    CommitLineEdit_ edit_;
    UiToolButton copy_;
};

class ColorField_ : public Ctrl {
public:
    typedef ColorField_ CLASSNAME;

    ColorField_() { NoWantFocus(); }

    void SetState(UiColorPicker::SpectrumMode mode, Color color, int remembered_hue, int gain)
    {
        bool invalidate = mode_ != mode || gain_ != gain || remembered_hue_ != remembered_hue;
        if(mode == UiColorPicker::SPECTRUM_RGB_SPECTRUM)
            invalidate = invalidate || color_.GetB() != color.GetB();
        mode_ = mode;
        color_ = color;
        remembered_hue_ = remembered_hue;
        gain_ = minmax(gain, 0, 100);
        if(invalidate) {
            cache_ = Image();
            cache_key_ = UiRasterCacheKey();
        }
        Refresh();
    }

    Event<Color, bool> WhenColor;

    virtual Size GetMinSize() const override { return Size(DPI(220), DPI(210)); }

    virtual void Paint(Draw& draw) override
    {
        Rect rect(Point(0, 0), GetSize());
        draw.DrawRect(rect, SurfaceColor_());
        const Image& image = EnsureCache();
        if(!image.IsEmpty())
            draw.DrawImage(rect.left, rect.top, image);
        Point marker = Marker();
        Rect outer = RectC(marker.x - DPI(5), marker.y - DPI(5), DPI(10), DPI(10));
        DrawFrame_(draw, outer, White(), DPI(2));
        DrawFrame_(draw, outer.Deflated(DPI(2)), Black(), DPI(1));
    }

    virtual void LeftDown(Point point, dword) override
    {
        SetCapture();
        Pick(point, false);
    }

    virtual void MouseMove(Point point, dword flags) override
    {
        if(HasCapture() && (flags & K_MOUSELEFT))
            Pick(point, false);
    }

    virtual void LeftUp(Point point, dword) override
    {
        if(HasCapture())
            ReleaseCapture();
        Pick(point, true);
    }

private:
    Color Sample(Point point) const
    {
        int width = max(1, GetSize().cx - 1);
        int height = max(1, GetSize().cy - 1);
        int x = minmax(point.x, 0, width);
        int y = minmax(point.y, 0, height);
        switch(mode_) {
        case UiColorPicker::SPECTRUM_HUE_STRIP: {
            int hue = int(x / double(width) * 359.0 + 0.5);
            int saturation = 100 - int(y / double(height) * 100.0 + 0.5);
            return HsvToColor(hue, saturation, gain_);
        }
        case UiColorPicker::SPECTRUM_RGB_SPECTRUM:
            return Color(ClampByte_(int(x / double(width) * 255.0 + 0.5)),
                         ClampByte_(255 - int(y / double(height) * 255.0 + 0.5)),
                         color_.GetB());
        case UiColorPicker::SPECTRUM_HSV_WHEEL: {
            Point centre(width / 2, height / 2);
            double dx = x - centre.x;
            double dy = centre.y - y;
            double radius = max(1.0, min(width, height) * 0.5);
            if(sqrt(dx * dx + dy * dy) > radius)
                return Null;
            int hue = NormalizeHue(int(atan2(dy, dx) * 180.0 / M_PI + 0.5));
            int saturation = minmax(int(sqrt(dx * dx + dy * dy) / radius * 100.0 + 0.5), 0, 100);
            return HsvToColor(hue, saturation, gain_);
        }
        case UiColorPicker::SPECTRUM_HSV_RECT:
        default: {
            int saturation = int(x / double(width) * 100.0 + 0.5);
            int value = 100 - int(y / double(height) * 100.0 + 0.5);
            return HsvToColor(remembered_hue_, saturation, value);
        }
        }
    }

    void Pick(Point point, bool final_commit)
    {
        Color color = Sample(point);
        if(!IsNull(color) && WhenColor)
            WhenColor(color, final_commit);
    }

    Point Marker() const
    {
        int width = max(1, GetSize().cx - 1);
        int height = max(1, GetSize().cy - 1);
        int hue = 0, saturation = 0, value = 0;
        ColorToHsv(color_, hue, saturation, value);
        if(saturation == 0 || value == 0)
            hue = remembered_hue_;
        switch(mode_) {
        case UiColorPicker::SPECTRUM_HUE_STRIP:
            return Point(int(hue / 359.0 * width + 0.5),
                         int((100 - saturation) / 100.0 * height + 0.5));
        case UiColorPicker::SPECTRUM_RGB_SPECTRUM:
            return Point(int(color_.GetR() / 255.0 * width + 0.5),
                         int((255 - color_.GetG()) / 255.0 * height + 0.5));
        case UiColorPicker::SPECTRUM_HSV_WHEEL: {
            double angle = hue * M_PI / 180.0;
            double radius = saturation / 100.0 * min(width, height) * 0.5;
            Point centre(width / 2, height / 2);
            return Point(centre.x + int(cos(angle) * radius + 0.5),
                         centre.y - int(sin(angle) * radius + 0.5));
        }
        case UiColorPicker::SPECTRUM_HSV_RECT:
        default:
            return Point(int(saturation / 100.0 * width + 0.5),
                         int((100 - value) / 100.0 * height + 0.5));
        }
    }

    const Image& EnsureCache() const
    {
        Size size = GetSize();
        if(size.IsEmpty())
            return cache_;
        UiRasterCacheKeyBuilder builder("palette-lab-color-field");
        builder.Add(size).Add((int)mode_).Add(remembered_hue_).Add(gain_).Add(color_.GetB());
        UiRasterCacheKey key = builder.Build();
        if(cache_.IsEmpty() || !(cache_key_ == key)) {
            UiRasterCachePolicy policy = UiRasterPolicyAA("palette-lab-color-field");
            policy.allow_scale_from_bucket = false;
            policy.max_single_image_bytes = 2 * 1024 * 1024;
            cache_ = UiRasterCache::Get(key, policy, [=] {
                ImageBuffer buffer(size);
                for(int y = 0; y < size.cy; y++) {
                    RGBA *row = buffer[y];
                    for(int x = 0; x < size.cx; x++) {
                        Color color = Sample(Point(x, y));
                        if(IsNull(color))
                            row[x] = RGBAZero();
                        else {
                            row[x] = RGBA(color);
                            row[x].a = 255;
                        }
                    }
                }
                return Image(buffer);
            });
            cache_key_ = key;
        }
        return cache_;
    }

    UiColorPicker::SpectrumMode mode_ = UiColorPicker::SPECTRUM_HUE_STRIP;
    Color color_ = Color(0, 120, 212);
    int remembered_hue_ = 200;
    int gain_ = 100;
    mutable UiRasterCacheKey cache_key_;
    mutable Image cache_;
};

class PaletteWheel_ : public Ctrl {
public:
    typedef PaletteWheel_ CLASSNAME;

    enum Mode {
        PASSIVE,
        GENERATOR
    };

    PaletteWheel_() { WantFocus(); }

    void SetPassive(const Vector<DisplaySwatch_>& items, const Vector<int>& selected, int active)
    {
        mode_ = PASSIVE;
        passive_items_ = clone(items);
        passive_selected_ = clone(selected);
        active_index_ = active;
        Refresh();
    }

    void SetGenerator(const GeneratorRecipe& recipe,
                      const Vector<GeneratedSwatch>& generated,
                      int active_family)
    {
        mode_ = GENERATOR;
        recipe_ = clone(recipe);
        generated_ = clone(generated);
        active_family_ = active_family;
        Refresh();
    }

    Event<int> WhenPassivePick;
    Event<int, int, int, int, bool> WhenAnchorChange;

    virtual Size GetMinSize() const override { return Size(DPI(180), DPI(180)); }

    virtual void Paint(Draw& draw) override
    {
        Rect outer(Point(0, 0), GetSize());
        draw.DrawRect(outer, SurfaceColor_());
        Rect wheel = WheelRect();
        if(wheel.IsEmpty())
            return;
        EnsureCache(wheel.GetSize());
        draw.DrawImage(wheel.left, wheel.top, cache_);
        DrawGuides(draw, wheel);
        if(mode_ == PASSIVE)
            DrawPassive(draw, wheel);
        else
            DrawGenerator(draw, wheel);
        DrawFrame_(draw, outer, FrameColor_());
    }

    virtual void LeftDown(Point point, dword) override
    {
        Rect wheel = WheelRect();
        if(mode_ == PASSIVE) {
            int index = HitPassive(point, wheel);
            if(index >= 0 && WhenPassivePick)
                WhenPassivePick(index);
            return;
        }
        drag_family_ = HitFamily(point, wheel);
        if(drag_family_ >= 0 && drag_family_ < recipe_.families.GetCount() && !recipe_.families[drag_family_].locked) {
            SetCapture();
            DragAnchor(point, false);
        }
    }

    virtual void MouseMove(Point point, dword flags) override
    {
        if(mode_ == GENERATOR && HasCapture() && (flags & K_MOUSELEFT))
            DragAnchor(point, false);
    }

    virtual void LeftUp(Point point, dword) override
    {
        if(mode_ == GENERATOR && HasCapture()) {
            DragAnchor(point, true);
            ReleaseCapture();
        }
        drag_family_ = -1;
    }

private:
    Rect WheelRect() const
    {
        Rect rect(Point(0, 0), GetSize());
        int side = max(0, min(rect.GetWidth(), rect.GetHeight()) - DPI(18));
        return RectC(rect.left + (rect.GetWidth() - side) / 2,
                     rect.top + (rect.GetHeight() - side) / 2, side, side);
    }

    void EnsureCache(Size size)
    {
        if(!cache_.IsEmpty() && cache_.GetSize() == size && cache_revision_ == UiTheme::GetRevision())
            return;
        ImageBuffer buffer(size);
        Point centre(size.cx / 2, size.cy / 2);
        double radius = max(1.0, min(size.cx, size.cy) * 0.5);
        Color neutral = SurfaceColor_();
        for(int y = 0; y < size.cy; y++) {
            RGBA *row = buffer[y];
            for(int x = 0; x < size.cx; x++) {
                double dx = x - centre.x;
                double dy = y - centre.y;
                double distance = sqrt(dx * dx + dy * dy);
                if(distance > radius) {
                    row[x] = RGBAZero();
                    continue;
                }
                int hue = NormalizeHue(int(atan2(dy, dx) * 180.0 / M_PI + 90.0 + 0.5));
                double saturation = distance / radius * 100.0;
                Color hue_color = HsvToColor(hue, saturation, 94);
                Color color = Blend(hue_color, neutral, int((1.0 - distance / radius) * 90.0));
                row[x] = RGBA(color);
                row[x].a = 255;
            }
        }
        cache_ = Image(buffer);
        cache_revision_ = UiTheme::GetRevision();
    }

    void DrawGuides(Draw& draw, const Rect& wheel)
    {
        Point centre = wheel.CenterPoint();
        Color guide = Blend(SColorText(), SurfaceColor_(), 185);
        for(int percent = 25; percent <= 75; percent += 25) {
            int radius = wheel.GetWidth() * percent / 200;
            draw.DrawEllipse(RectC(centre.x - radius, centre.y - radius, radius * 2, radius * 2),
                             Null, DPI(1), guide);
        }
        for(int angle = 0; angle < 360; angle += 45) {
            Point edge = WheelPoint_(angle, 100, wheel);
            draw.DrawLine(centre.x, centre.y, edge.x, edge.y, DPI(1), guide);
            Point label = WheelPoint_(angle, 88, wheel);
            String text = AsString(angle);
            Font font = SansSerif().Height(DPI(8));
            Size size = GetTextSize(text, font);
            draw.DrawText(label.x - size.cx / 2, label.y - size.cy / 2,
                          text, font, Blend(SColorText(), SurfaceColor_(), 130));
        }
    }

    void DrawPassive(Draw& draw, const Rect& wheel)
    {
        for(int pass = 0; pass < 2; pass++) {
            for(int i = 0; i < passive_items_.GetCount(); i++) {
                bool active = i == active_index_;
                if((pass == 0 && active) || (pass == 1 && !active))
                    continue;
                int hue = 0, saturation = 0, value = 0;
                ColorToHsv(passive_items_[i].value.color, hue, saturation, value);
                Point point = WheelPoint_(hue, saturation, wheel);
                int radius = active ? DPI(6) : DPI(4);
                Rect marker = RectC(point.x - radius, point.y - radius, radius * 2, radius * 2);
                draw.DrawEllipse(marker, passive_items_[i].value.color, DPI(1), White());
                if(active)
                    draw.DrawEllipse(marker.Inflated(DPI(3)), Null, DPI(2), SColorHighlight());
                else if(FindIndex(passive_selected_, i) >= 0)
                    draw.DrawEllipse(marker.Inflated(DPI(2)), Null, DPI(1), SColorHighlight());
            }
        }
    }

    void DrawGenerator(Draw& draw, const Rect& wheel)
    {
        Point centre = wheel.CenterPoint();
        double scale = GlobalSaturationScale(recipe_);
        Vector<Point> anchors;
        for(int i = 0; i < recipe_.families.GetCount(); i++) {
            const PaletteFamily& family = recipe_.families[i];
            int offset = recipe_.free_angles ? family.custom_offset : family.canonical_offset;
            double saturation = family.locked ? family.authored_saturation
                                              : family.authored_saturation * scale;
            anchors.Add(WheelPoint_(recipe_.base_hue + offset, saturation, wheel));
        }
        for(const Point& point : anchors) {
            draw.DrawLine(centre.x, centre.y, point.x, point.y, DPI(2), White());
            draw.DrawLine(centre.x, centre.y + DPI(1), point.x, point.y + DPI(1), DPI(1), SColorShadow());
        }
        for(const GeneratedSwatch& swatch : generated_) {
            int hue = 0, saturation = 0, value = 0;
            ColorToHsv(swatch.value.color, hue, saturation, value);
            Point point = WheelPoint_(hue, saturation, wheel);
            Rect marker = RectC(point.x - DPI(2), point.y - DPI(2), DPI(4), DPI(4));
            draw.DrawEllipse(marker, swatch.value.color, DPI(1), White());
        }
        for(int i = 0; i < anchors.GetCount(); i++) {
            const PaletteFamily& family = recipe_.families[i];
            int offset = recipe_.free_angles ? family.custom_offset : family.canonical_offset;
            double saturation = family.locked ? family.authored_saturation
                                              : family.authored_saturation * scale;
            Color color = HsvToColor(recipe_.base_hue + offset, minmax(saturation, 0.0, 100.0), 84 + family.gain);
            int radius = DPI(10);
            Rect marker = RectC(anchors[i].x - radius, anchors[i].y - radius, radius * 2, radius * 2);
            draw.DrawEllipse(marker.Inflated(DPI(2)), SColorPaper(), DPI(1), SColorShadow());
            draw.DrawEllipse(marker, color, DPI(2), White());
            if(i == active_family_)
                draw.DrawEllipse(marker.Inflated(DPI(4)), Null, DPI(2), SColorHighlight());
            String number = AsString(i + 1);
            Font font = SansSerif().Height(DPI(9)).Bold();
            Size text = GetTextSize(number, font);
            draw.DrawText(marker.left + (marker.GetWidth() - text.cx) / 2,
                          marker.top + (marker.GetHeight() - text.cy) / 2,
                          number, font, SColorText());
            if(family.locked) {
                String lock = "L";
                Font small = SansSerif().Height(DPI(7)).Bold();
                draw.DrawText(marker.right - DPI(2), marker.bottom - DPI(7), lock, small, SColorText());
            }
        }
    }

    int HitPassive(Point point, const Rect& wheel) const
    {
        int best = -1;
        int best_distance = INT_MAX;
        for(int i = 0; i < passive_items_.GetCount(); i++) {
            int hue = 0, saturation = 0, value = 0;
            ColorToHsv(passive_items_[i].value.color, hue, saturation, value);
            Point marker = WheelPoint_(hue, saturation, wheel);
            int dx = point.x - marker.x, dy = point.y - marker.y;
            int distance = dx * dx + dy * dy;
            if(distance <= DPI(10) * DPI(10) && distance < best_distance) {
                best = i;
                best_distance = distance;
            }
        }
        return best;
    }

    int HitFamily(Point point, const Rect& wheel) const
    {
        double scale = GlobalSaturationScale(recipe_);
        for(int i = recipe_.families.GetCount() - 1; i >= 0; i--) {
            const PaletteFamily& family = recipe_.families[i];
            int offset = recipe_.free_angles ? family.custom_offset : family.canonical_offset;
            double saturation = family.locked ? family.authored_saturation
                                              : family.authored_saturation * scale;
            Point marker = WheelPoint_(recipe_.base_hue + offset, saturation, wheel);
            int dx = point.x - marker.x, dy = point.y - marker.y;
            if(dx * dx + dy * dy <= DPI(16) * DPI(16))
                return i;
        }
        return -1;
    }

    void DragAnchor(Point point, bool final_commit)
    {
        if(drag_family_ < 0 || drag_family_ >= recipe_.families.GetCount())
            return;
        Rect wheel = WheelRect();
        int hue = 0, saturation = 0;
        PointToWheel_(point, wheel, hue, saturation);
        const PaletteFamily& family = recipe_.families[drag_family_];
        int base_hue = recipe_.base_hue;
        int custom_offset = family.custom_offset;
        if(recipe_.free_angles)
            custom_offset = NormalizeHue(hue - base_hue);
        else
            base_hue = NormalizeHue(hue - family.canonical_offset);
        double scale = GlobalSaturationScale(recipe_);
        int authored = minmax(int(saturation / max(0.0001, scale) + 0.5), 0, 100);
        if(WhenAnchorChange)
            WhenAnchorChange(drag_family_, base_hue, custom_offset, authored, final_commit);
    }

    Mode mode_ = PASSIVE;
    Vector<DisplaySwatch_> passive_items_;
    Vector<int> passive_selected_;
    int active_index_ = -1;
    GeneratorRecipe recipe_;
    Vector<GeneratedSwatch> generated_;
    int active_family_ = 0;
    int drag_family_ = -1;
    Image cache_;
    uint64 cache_revision_ = 0;
};

class ImageCanvas_ : public Ctrl {
public:
    typedef ImageCanvas_ CLASSNAME;

    ImageCanvas_() { WantFocus(); }

    void SetImage(const Image& image)
    {
        image_ = image;
        zoom_ = 1.0;
        pan_ = Point(0, 0);
        drop_hot_ = false;
        Refresh();
    }

    void SetResult(const ImageAnalysisResult& result, bool show_mask)
    {
        result_ = clone(result);
        show_mask_ = show_mask;
        Refresh();
    }

    void SetSeeds(const ImageExclusionSeed seed[2])
    {
        seed_[0] = seed[0];
        seed_[1] = seed[1];
        Refresh();
    }

    void SetManualPoints(const Vector<Pointf>& points)
    {
        manual_points_ = clone(points);
        Refresh();
    }

    void ArmSeed(int index)
    {
        armed_seed_ = index >= 0 && index < 2 ? index : -1;
        SetFocus();
        Refresh();
    }

    int GetArmedSeed() const { return armed_seed_; }

    void Fit()
    {
        zoom_ = 1.0;
        pan_ = Point(0, 0);
        Refresh();
    }

    void ZoomBy(double factor)
    {
        zoom_ = minmax(zoom_ * factor, 0.25, 8.0);
        Refresh();
    }

    int GetZoomPercent() const { return int(zoom_ * 100.0 + 0.5); }

    Event<> WhenLoadRequest;
    Event<Image> WhenImageDrop;
    Event<int, Pointf> WhenSeedPlaced;
    Event<int, Pointf, bool> WhenSeedMoved;
    Event<int, Pointf, bool> WhenManualMoved;

    virtual Size GetMinSize() const override { return Size(DPI(260), DPI(210)); }

    virtual void Paint(Draw& draw) override
    {
        Rect rect(Point(0, 0), GetSize());
        draw.DrawRect(rect, SurfaceColor_());
        DrawFrame_(draw, rect, FrameColor_());
        if(image_.IsEmpty()) {
            String text = "Click, drop or paste an image";
            Font font = SansSerif().Height(DPI(11));
            Size size = GetTextSize(text, font);
            draw.DrawText(rect.left + (rect.GetWidth() - size.cx) / 2,
                          rect.top + (rect.GetHeight() - size.cy) / 2,
                          text, font, SColorDisabled());
            if(drop_hot_)
                DrawFrame_(draw, rect.Deflated(DPI(2)), SColorHighlight(), DPI(2));
            return;
        }
        Rect image_rect = ImageRect();
        draw.Clip(rect.Deflated(DPI(1)));
        draw.DrawImage(image_rect.left, image_rect.top,
                       image_rect.GetWidth(), image_rect.GetHeight(), image_);
        if(show_mask_ && result_.exclusion_mask.GetCount() == result_.proxy_size.cx * result_.proxy_size.cy &&
           result_.proxy_size.cx > 0 && result_.proxy_size.cy > 0)
            DrawMask(draw, image_rect);
        DrawMarkers(draw, image_rect);
        draw.End();
        if(armed_seed_ >= 0)
            DrawFrame_(draw, rect.Deflated(DPI(2)), SColorHighlight(), DPI(2));
        else if(drop_hot_)
            DrawFrame_(draw, rect.Deflated(DPI(2)), SColorHighlight(), DPI(2));
    }

    virtual void LeftDown(Point point, dword) override
    {
        SetFocus();
        if(image_.IsEmpty()) {
            if(WhenLoadRequest)
                WhenLoadRequest();
            return;
        }
        Rect image_rect = ImageRect();
        if(!image_rect.Contains(point))
            return;
        if(armed_seed_ >= 0) {
            Pointf normalized = NormalizePoint(point, image_rect);
            if(WhenSeedPlaced)
                WhenSeedPlaced(armed_seed_, normalized);
            armed_seed_ = -1;
            Refresh();
            return;
        }
        drag_seed_ = HitSeed(point, image_rect);
        if(drag_seed_ >= 0) {
            SetCapture();
            MoveSeed(point, image_rect, false);
            return;
        }
        drag_manual_ = HitManual(point, image_rect);
        if(drag_manual_ >= 0) {
            SetCapture();
            MoveManual(point, image_rect, false);
            return;
        }
        panning_ = true;
        pan_anchor_ = point;
        SetCapture();
    }

    virtual void MouseMove(Point point, dword flags) override
    {
        if(!HasCapture() || !(flags & K_MOUSELEFT))
            return;
        Rect image_rect = ImageRect();
        if(drag_seed_ >= 0)
            MoveSeed(point, image_rect, false);
        else if(drag_manual_ >= 0)
            MoveManual(point, image_rect, false);
        else if(panning_) {
            pan_ += point - pan_anchor_;
            pan_anchor_ = point;
            Refresh();
        }
    }

    virtual void LeftUp(Point point, dword) override
    {
        if(!HasCapture())
            return;
        Rect image_rect = ImageRect();
        if(drag_seed_ >= 0)
            MoveSeed(point, image_rect, true);
        else if(drag_manual_ >= 0)
            MoveManual(point, image_rect, true);
        ReleaseCapture();
        drag_seed_ = -1;
        drag_manual_ = -1;
        panning_ = false;
    }

    virtual void MouseWheel(Point, int zdelta, dword) override
    {
        if(zdelta)
            ZoomBy(zdelta > 0 ? 1.15 : 1.0 / 1.15);
    }

    virtual void DragEnter() override
    {
        drop_hot_ = true;
        Refresh();
    }

    virtual void DragAndDrop(Point, PasteClip& clip) override
    {
        if(IsAvailableImage(clip)) {
            AcceptImage(clip);
            clip.SetAction(DND_COPY);
            if(clip.IsPaste()) {
                Image dropped = GetImage(clip);
                if(!dropped.IsEmpty() && WhenImageDrop)
                    WhenImageDrop(dropped);
                drop_hot_ = false;
                Refresh();
            }
            return;
        }
        if(IsAvailableFiles(clip)) {
            AcceptFiles(clip);
            if(clip.IsPaste()) {
                Vector<String> files = GetFiles(clip);
                if(!files.IsEmpty()) {
                    Image dropped = StreamRaster::LoadFileAny(files[0]);
                    if(!dropped.IsEmpty() && WhenImageDrop)
                        WhenImageDrop(dropped);
                }
                drop_hot_ = false;
                Refresh();
            }
            return;
        }
        clip.Reject();
        drop_hot_ = false;
        Refresh();
    }

    virtual void DragLeave() override
    {
        drop_hot_ = false;
        Refresh();
    }

private:
    Rect ImageRect() const
    {
        Rect rect(Point(0, 0), GetSize());
        Size source = image_.GetSize();
        if(source.IsEmpty())
            return Rect(0, 0, 0, 0);
        double scale = min(rect.GetWidth() / double(max(1, source.cx)),
                           rect.GetHeight() / double(max(1, source.cy))) * zoom_;
        Size target(max(1, int(source.cx * scale + 0.5)),
                    max(1, int(source.cy * scale + 0.5)));
        return RectC(rect.left + (rect.GetWidth() - target.cx) / 2 + pan_.x,
                     rect.top + (rect.GetHeight() - target.cy) / 2 + pan_.y,
                     target.cx, target.cy);
    }

    Pointf NormalizePoint(Point point, const Rect& image_rect) const
    {
        return Pointf(minmax((point.x - image_rect.left) / double(max(1, image_rect.GetWidth())), 0.0, 1.0),
                      minmax((point.y - image_rect.top) / double(max(1, image_rect.GetHeight())), 0.0, 1.0));
    }

    Point ScreenPoint(Pointf point, const Rect& image_rect) const
    {
        return Point(image_rect.left + int(point.x * image_rect.GetWidth() + 0.5),
                     image_rect.top + int(point.y * image_rect.GetHeight() + 0.5));
    }

    void DrawMask(Draw& draw, const Rect& image_rect)
    {
        Size proxy = result_.proxy_size;
        int stripe = max(2, DPI(4));
        for(int y = 0; y < proxy.cy; y += 2) {
            int sy0 = image_rect.top + y * image_rect.GetHeight() / proxy.cy;
            int sy1 = image_rect.top + min(proxy.cy, y + 2) * image_rect.GetHeight() / proxy.cy;
            for(int x = 0; x < proxy.cx; x += 2) {
                int index = y * proxy.cx + x;
                if(index < 0 || index >= result_.exclusion_mask.GetCount() || !result_.exclusion_mask[index])
                    continue;
                int sx0 = image_rect.left + x * image_rect.GetWidth() / proxy.cx;
                int sx1 = image_rect.left + min(proxy.cx, x + 2) * image_rect.GetWidth() / proxy.cx;
                Color mask = ((sx0 + sy0) / stripe) & 1 ? Color(36, 39, 44) : Color(72, 76, 84);
                draw.DrawRect(sx0, sy0, max(1, sx1 - sx0), max(1, sy1 - sy0), mask);
            }
        }
    }

    void DrawMarkers(Draw& draw, const Rect& image_rect)
    {
        for(int i = 0; i < 2; i++) {
            if(!seed_[i].enabled || !seed_[i].placed)
                continue;
            Point point = ScreenPoint(seed_[i].position, image_rect);
            int radius = DPI(9);
            Rect marker = RectC(point.x - radius, point.y - radius, radius * 2, radius * 2);
            draw.DrawEllipse(marker.Inflated(DPI(2)), SColorPaper(), DPI(1), SColorShadow());
            draw.DrawEllipse(marker, seed_[i].color, DPI(2), i == 0 ? Color(255, 220, 40) : Color(80, 190, 255));
            String text = i == 0 ? "A" : "B";
            Font font = SansSerif().Height(DPI(8)).Bold();
            Size size = GetTextSize(text, font);
            draw.DrawText(marker.left + (marker.GetWidth() - size.cx) / 2,
                          marker.top + (marker.GetHeight() - size.cy) / 2,
                          text, font, SColorText());
        }
        for(int i = 0; i < manual_points_.GetCount(); i++) {
            Point point = ScreenPoint(manual_points_[i], image_rect);
            int radius = DPI(8);
            Rect marker = RectC(point.x - radius, point.y - radius, radius * 2, radius * 2);
            Color color = SampleImage_(image_, manual_points_[i], 2);
            draw.DrawEllipse(marker.Inflated(DPI(2)), SColorPaper(), DPI(1), SColorShadow());
            draw.DrawEllipse(marker, color, DPI(2), White());
            String text = AsString(i + 1);
            Font font = SansSerif().Height(DPI(8)).Bold();
            Size size = GetTextSize(text, font);
            draw.DrawText(marker.left + (marker.GetWidth() - size.cx) / 2,
                          marker.top + (marker.GetHeight() - size.cy) / 2,
                          text, font, SColorText());
        }
        if(manual_points_.IsEmpty()) {
            int count = min(result_.representative_positions.GetCount(), result_.swatches.GetCount());
            for(int i = 0; i < count; i++) {
                Point point = ScreenPoint(result_.representative_positions[i], image_rect);
                int radius = DPI(7);
                Rect marker = RectC(point.x - radius, point.y - radius, radius * 2, radius * 2);
                Color color = result_.swatches[i].value.color;
                draw.DrawEllipse(marker.Inflated(DPI(2)), SColorPaper(), DPI(1), SColorShadow());
                draw.DrawEllipse(marker, color, DPI(2), White());
                String text = AsString(i + 1);
                Font font = SansSerif().Height(DPI(7)).Bold();
                Size size = GetTextSize(text, font);
                Color ink = IsDark(color) ? White() : Black();
                draw.DrawText(marker.left + (marker.GetWidth() - size.cx) / 2,
                              marker.top + (marker.GetHeight() - size.cy) / 2,
                              text, font, ink);
            }
        }
    }

    int HitSeed(Point point, const Rect& image_rect) const
    {
        for(int i = 1; i >= 0; i--) {
            if(!seed_[i].enabled || !seed_[i].placed)
                continue;
            Point marker = ScreenPoint(seed_[i].position, image_rect);
            int dx = point.x - marker.x, dy = point.y - marker.y;
            if(dx * dx + dy * dy <= DPI(16) * DPI(16))
                return i;
        }
        return -1;
    }

    int HitManual(Point point, const Rect& image_rect) const
    {
        for(int i = manual_points_.GetCount() - 1; i >= 0; i--) {
            Point marker = ScreenPoint(manual_points_[i], image_rect);
            int dx = point.x - marker.x, dy = point.y - marker.y;
            if(dx * dx + dy * dy <= DPI(15) * DPI(15))
                return i;
        }
        return -1;
    }

    void MoveSeed(Point point, const Rect& image_rect, bool final_commit)
    {
        Pointf normalized = NormalizePoint(point, image_rect);
        if(WhenSeedMoved)
            WhenSeedMoved(drag_seed_, normalized, final_commit);
    }

    void MoveManual(Point point, const Rect& image_rect, bool final_commit)
    {
        Pointf normalized = NormalizePoint(point, image_rect);
        if(drag_manual_ >= 0 && drag_manual_ < manual_points_.GetCount())
            manual_points_[drag_manual_] = normalized;
        if(WhenManualMoved)
            WhenManualMoved(drag_manual_, normalized, final_commit);
        Refresh();
    }

    Image image_;
    ImageAnalysisResult result_;
    ImageExclusionSeed seed_[2];
    Vector<Pointf> manual_points_;
    int armed_seed_ = -1;
    int drag_seed_ = -1;
    int drag_manual_ = -1;
    bool show_mask_ = false;
    bool drop_hot_ = false;
    bool panning_ = false;
    Point pan_anchor_;
    Point pan_;
    double zoom_ = 1.0;
};

struct SharedSession_ {
    bool initialized = false;
    Vector<UiColorPicker::SlotValue> slots;
    Vector<UiColorPicker::SlotValue> previous;
    Vector<UiColorPicker::SlotValue> recent;
    Vector<UiColorPicker::SlotValue> stash;
    int active_slot = 0;
    bool alpha_enabled = true;
    int page = UiColorPicker::PAGE_COLOR;
    int spectrum = UiColorPicker::SPECTRUM_HUE_STRIP;
    int channel = UiColorPicker::CHANNEL_RGB_FLOAT;
    int palette_index = 0;
    GeneratorRecipe recipe;
    Image image;
    ImageAnalysisSettings image_settings;
    int image_gain = 0;
    int image_saturation = 100;
    int image_hero = -1;
    int image_hero_gain = 0;
    int stash_drop_mode = UiColorPicker::STASH_REPLACE;
};

static SharedSession_& SharedSessionState_()
{
    static SharedSession_ session;
    return session;
}

} // namespace

class UiColorPicker::Impl {
public:
    Impl(UiColorPicker& owner);
    ~Impl();

    void Layout();
    void SyncTheme();

    UiColorPicker& SetPageMode(PageMode mode);
    UiColorPicker& SetChannelMode(ChannelMode mode);
    UiColorPicker& SetSlotCount(int count);
    UiColorPicker& SetActiveSlot(int index);
    UiColorPicker& SetSlot(int index, Color color, int alpha, bool fire);
    UiColorPicker& SetSlotColor(int index, Color color, bool fire);
    UiColorPicker& SetSlotAlpha(int index, int alpha, bool fire);
    UiColorPicker& SetSlotLabel(int index, const String& label);
    UiColorPicker& SetAlphaEnabled(bool enabled);
    UiColorPicker& SetSpectrumMode(SpectrumMode mode);
    UiColorPicker& SetHarmonyMode(HarmonyMode mode);
    UiColorPicker& SetDistributionMode(DistributionMode mode);
    UiColorPicker& SetMediumMode(MediumMode mode);
    UiColorPicker& SetGeneratorCount(int count);
    UiColorPicker& SetGeneratorImage(const Image& image);
    UiColorPicker& ExtractGeneratorPalette(int count);
    UiColorPicker& AddUserSwatches(const Vector<SlotValue>& values, bool transactional);
    UiColorPicker& ClearUserSwatches();
    UiColorPicker& ClearRecentSwatches();
    UiColorPicker& EnableSessionPersistence(bool enabled);
    UiColorPicker& BeginEyedropper();

    PageMode GetPageMode() const { return page_mode_; }
    ChannelMode GetChannelMode() const { return channel_mode_; }
    int GetSlotCount() const { return slot_count_; }
    int GetActiveSlot() const { return active_slot_; }
    Color GetSlotColor(int index) const;
    int GetSlotAlpha(int index) const;
    SlotValue GetSlot(int index) const;
    Vector<SlotValue> GetSlots() const;
    String GetSlotLabel(int index) const;
    bool IsAlphaEnabled() const { return alpha_enabled_; }
    SpectrumMode GetSpectrumMode() const { return spectrum_mode_; }
    HarmonyMode GetHarmonyMode() const { return recipe_.harmony; }
    DistributionMode GetDistributionMode() const { return recipe_.distribution; }
    MediumMode GetMediumMode() const { return recipe_.medium; }
    int GetGeneratorCount() const { return recipe_.requested_count; }
    Vector<SlotValue> GetGeneratedPalette() const;
    const Image& GetGeneratorImage() const { return image_; }
    Vector<SlotValue> GetImagePalette() const;
    int GetUserSwatchCount() const { return stash_.GetCount(); }
    int GetRecentSwatchCount() const { return recent_.GetCount(); }
    Vector<SlotValue> GetUserSwatches() const { return clone(stash_); }
    bool IsSessionPersistenceEnabled() const { return session_persistence_; }
    bool IsEyedropperAvailable() const;
    bool IsEyedropperActive() const { return eyedropper_active_; }

    void Accept();
    void Cancel();
    void FinishLiveGesture();
    void StopEyedropper(bool commit);
    void SampleEyedropper(bool final_commit);

    bool Key(dword key, int count);
    void CancelMode();

private:
    void BuildTree();
    void ConfigureControls();
    void WireEvents();
    void BuildColorPage();
    void BuildPalettesPage();
    void BuildGeneratorPage();
    void BuildImagePage();

    void LoadSession();
    void SaveSession(bool include_slots);

    void SyncAll();
    void SyncSlots();
    void SyncPage();
    void SyncColorControls();
    void SyncChannelRows();
    void ConfigureChannelRows();
    void SyncReadouts();
    void SyncFooter(const SlotValue& value);
    void PushRecent(Color color, int alpha);
    void RefreshStash();
    void HandleStashDrop(int target, const Vector<SlotValue>& values);
    void AddActiveSelection();

    void CommitColor(Color color, bool final_commit);
    void CommitAlpha(int alpha, bool final_commit);
    void CommitSlotValue(int slot, Color color, int alpha, bool final_commit);
    bool ApplyColorText(const String& text, bool final_commit);
    void HandleChannelValue(int row, double value, bool final_commit);

    void PopulateStaticPaletteDrop();
    void SetStaticPalette(int index);
    void SyncPaletteSelection(int index, const SlotValue& value);
    void AddPaletteSelection(bool all);

    void PopulateGeneratorControls();
    void RefreshGenerator(bool preserve_selection = true);
    void SyncGeneratorSelection(int index, const SlotValue& value);
    void SyncSelectedFamily();
    void AddGeneratorSelection(bool all);
    void SendImageToGenerator();

    void PopulateImageControls();
    void LoadImage();
    void MarkImageStale();
    void AnalyzeCurrentImage(bool mask_only = false);
    void RefreshImageDisplay(bool preserve_selection = true);
    void SyncImageSelection(int index, const SlotValue& value);
    void AddImageSelection(bool all);
    void EnsureManualPoints();
    void ResampleManualPoint(int index);
    void SyncImageStatus();

    void SavePaletteJson();
    void LoadPaletteJson();

    UiColorPicker& owner_;

    Vector<SlotValue> slots_;
    Vector<SlotValue> previous_;
    Vector<SlotValue> opening_;
    Vector<SlotValue> recent_;
    Vector<SlotValue> stash_;
    int slot_count_ = 4;
    int active_slot_ = 0;
    bool alpha_enabled_ = true;
    StashDropMode stash_drop_mode_ = STASH_REPLACE;
    PageMode page_mode_ = PAGE_COLOR;
    SpectrumMode spectrum_mode_ = SPECTRUM_HUE_STRIP;
    ChannelMode channel_mode_ = CHANNEL_RGB_FLOAT;
    int remembered_hue_ = 200;

    bool syncing_ = false;
    bool live_gesture_ = false;
    int live_slot_ = -1;
    SlotValue live_origin_;
    dword last_live_callback_ = 0;
    bool session_persistence_ = true;
    bool eyedropper_active_ = false;
    bool eyedropper_dragging_ = false;

    int palette_index_ = 0;
    GeneratorRecipe recipe_;
    Vector<GeneratedSwatch> generated_;
    int active_family_ = 0;

    Image image_;
    ImageAnalysisSettings image_settings_;
    ImageAnalysisResult image_result_;
    Vector<GeneratedSwatch> image_display_;
    Vector<Pointf> manual_points_;
    bool image_stale_ = false;
    bool image_show_mask_ = false;
    int image_gain_ = 0;
    int image_saturation_ = 100;
    int image_hero_ = -1;
    int image_hero_gain_ = 0;

    SlotValue footer_value_;

    UiBoxLayout root_ { UiDirection::V };
    UiBoxLayout navigation_ { UiDirection::H };
    UiButton page_button_[PAGE_COUNT];
    UiToolButton current_preview_;
    One<UiBoxLayout> slot_column_[8];
    UiToolButton primary_slot_[8];
    UiToolButton previous_slot_[8];

    UiStack page_stack_;
    UiBoxLayout color_page_ { UiDirection::V };
    UiBoxLayout palettes_page_ { UiDirection::V };
    UiBoxLayout generator_page_ { UiDirection::V };
    UiBoxLayout image_page_ { UiDirection::V };

    UiBoxLayout stash_section_ { UiDirection::V };
    UiBoxLayout stash_header_ { UiDirection::H };
    UiLabel stash_title_;
    UiDropdown stash_mode_drop_;
    UiButton stash_add_selected_;
    UiButton stash_export_;
    UiButton stash_import_;
    UiButton stash_clear_;
    SwatchFlow_ stash_flow_;

    UiBoxLayout footer_ { UiDirection::H };
    UiLineEdit footer_hex_;
    UiLineEdit footer_detail_;
    UiButton accept_button_;
    UiButton cancel_button_;

    UiBoxLayout color_columns_ { UiDirection::H };
    UiBoxLayout color_left_ { UiDirection::V };
    UiBoxLayout color_right_ { UiDirection::V };
    LabeledDrop_ spectrum_field_;
    One<ColorField_> color_field_;
    UiBoxLayout color_mode_row_ { UiDirection::H };
    UiDropdown channel_drop_;
    UiToolButton eyedropper_button_;
    UiLabel alpha_label_;
    UiToggle alpha_toggle_;
    UiBoxLayout channel_rows_ { UiDirection::V };
    One<NumericRow_> numeric_row_[5];
    int channel_row_count_ = 4;
    One<SliderRow_> hue_row_;
    One<SliderRow_> value_row_;
    UiBoxLayout readout_stack_ { UiDirection::V };
    One<UiBoxLayout> readout_pair_[3];
    One<ReadoutRow_> readout_[6];

    UiBoxLayout palettes_top_ { UiDirection::H };
    LabeledDrop_ palette_field_;
    UiLabel palette_badge_;
    UiBoxLayout palettes_body_ { UiDirection::H };
    One<PaletteWheel_> palette_wheel_;
    UiScrollPanel palette_scroll_;
    SwatchFlow_ palette_flow_;
    UiBoxLayout palette_actions_ { UiDirection::H };
    UiLabel palette_summary_;
    UiButton palette_add_selected_;
    UiButton palette_add_all_;

    UiBoxLayout generator_top_ { UiDirection::H };
    LabeledDrop_ harmony_field_;
    LabeledDrop_ distribution_field_;
    LabeledDrop_ generator_medium_field_;
    LabeledDrop_ generator_count_field_;
    UiBoxLayout generator_body_ { UiDirection::H };
    One<PaletteWheel_> generator_wheel_;
    UiBoxLayout generator_right_ { UiDirection::V };
    SwatchFlow_ generator_flow_;
    UiBoxLayout free_angles_row_ { UiDirection::H };
    UiLabel free_angles_label_;
    UiToggle free_angles_toggle_;
    UiLabel selected_family_label_;
    UiButton family_lock_button_;
    One<SliderRow_> generator_gain_row_;
    One<SliderRow_> generator_saturation_row_;
    One<SliderRow_> family_gain_row_;
    UiBoxLayout generator_actions_ { UiDirection::H };
    UiButton generator_add_selected_;
    UiButton generator_add_all_;

    UiBoxLayout image_top_ { UiDirection::H };
    LabeledDrop_ image_analysis_field_;
    LabeledDrop_ image_coverage_field_;
    LabeledDrop_ image_medium_field_;
    LabeledDrop_ image_count_field_;
    UiBoxLayout image_body_ { UiDirection::H };
    UiBoxLayout image_left_ { UiDirection::V };
    One<ImageCanvas_> image_canvas_;
    UiBoxLayout image_navigation_ { UiDirection::H };
    UiButton image_load_button_;
    UiButton image_analyze_button_;
    UiButton image_fit_button_;
    UiButton image_zoom_out_;
    UiLabel image_zoom_label_;
    UiButton image_zoom_in_;
    UiBoxLayout image_right_ { UiDirection::V };
    SwatchFlow_ image_flow_;
    One<SliderRow_> image_gain_row_;
    One<SliderRow_> image_saturation_row_;
    UiBoxLayout hero_row_ { UiDirection::H };
    UiLabel hero_label_;
    UiButton hero_set_button_;
    One<SliderRow_> hero_gain_row_;
    UiBoxLayout exclusion_row_ { UiDirection::H };
    UiToolButton exclusion_button_[2];
    UiToggle exclusion_toggle_[2];
    UiLabel exclusion_label_[2];
    One<SliderRow_> tolerance_row_;
    UiBoxLayout image_actions_ { UiDirection::H };
    UiToggle show_exclusion_toggle_;
    UiLabel show_exclusion_label_;
    UiButton clear_exclusion_button_;
    UiButton image_send_generator_;
    UiLabel image_status_;
};

UiColorPicker::Impl::Impl(UiColorPicker& owner)
    : owner_(owner)
{
    slots_.SetCount(8);
    previous_.SetCount(8);
    opening_.SetCount(8);
    const Color defaults[8] = {
        Color(0, 120, 212), Color(255, 204, 0),
        Color(52, 199, 89), Color(255, 59, 48),
        Color(175, 82, 222), Color(90, 200, 250),
        Color(255, 149, 0), Color(100, 210, 255)
    };
    for(int i = 0; i < 8; i++) {
        slots_[i].color = defaults[i];
        slots_[i].alpha = 255;
        slots_[i].label = Format("C%d", i + 1);
        previous_[i] = slots_[i];
        opening_[i] = slots_[i];
    }
    recipe_.harmony = HARMONY_TRIAD;
    recipe_.distribution = DISTRIBUTION_BALANCED;
    recipe_.medium = MEDIUM_UI;
    recipe_.base_hue = 200;
    recipe_.requested_count = 6;
    ResetGeneratorFamilies(recipe_, false);
    image_settings_.requested_count = 6;

    BuildTree();
    ConfigureControls();
    WireEvents();
    LoadSession();
    opening_ = clone(slots_);
    PopulateStaticPaletteDrop();
    PopulateGeneratorControls();
    PopulateImageControls();
    SetStaticPalette(palette_index_);
    RefreshGenerator(false);
    RefreshStash();
    RefreshImageDisplay(false);
    SyncTheme();
    SyncAll();
}

UiColorPicker::Impl::~Impl()
{
    if(session_persistence_)
        SaveSession(false);
}

void UiColorPicker::Impl::BuildTree()
{
    owner_.Add(root_.SizePos());
    root_.SetGap(DPI(2)).SetInset(0);
    root_.Add(navigation_).Fixed(owner_.GetStyle().navigation_height);
    root_.Add(page_stack_).Expand(1);
    root_.Add(stash_section_).Fixed(owner_.GetStyle().stash_height);
    root_.Add(footer_).Fixed(owner_.GetStyle().footer_height);

    navigation_.SetGap(DPI(4)).SetInset(Rect(DPI(5), DPI(2), DPI(5), DPI(2)));
    for(int i = 0; i < PAGE_COUNT; i++)
        navigation_.Add(page_button_[i]).Fixed(DPI(66));
    navigation_.AddSpacer(1);
    navigation_.Add(current_preview_).Fixed(DPI(38));
    for(int i = 0; i < 8; i++) {
        slot_column_[i].Create(UiDirection::V);
        slot_column_[i]->SetGap(DPI(1));
        slot_column_[i]->Add(primary_slot_[i]).Expand(1);
        slot_column_[i]->Add(previous_slot_[i]).Fixed(DPI(7));
        navigation_.Add(*slot_column_[i]).Fixed(DPI(34));
    }

    page_stack_.AddPage(color_page_, "color");
    page_stack_.AddPage(palettes_page_, "palettes");
    page_stack_.AddPage(generator_page_, "generator");
    page_stack_.AddPage(image_page_, "image");

    stash_section_.SetGap(DPI(2)).SetInset(Rect(DPI(6), DPI(2), DPI(6), DPI(2)));
    stash_section_.Add(stash_header_).Fixed(DPI(30));
    stash_section_.Add(stash_flow_).Expand(1);
    stash_header_.SetGap(DPI(4));
    stash_header_.Add(stash_title_).Fixed(DPI(74));
    stash_header_.Add(stash_mode_drop_).Fixed(DPI(146));
    stash_header_.AddSpacer(1);
    stash_header_.Add(stash_add_selected_).Fixed(DPI(96));
    stash_header_.Add(stash_export_).Fixed(DPI(72));
    stash_header_.Add(stash_import_).Fixed(DPI(72));
    stash_header_.Add(stash_clear_).Fixed(DPI(52));

    footer_.SetGap(DPI(6)).SetInset(Rect(DPI(6), DPI(4), DPI(6), DPI(4)));
    footer_.Add(footer_hex_).Fixed(DPI(116));
    footer_.Add(footer_detail_).Expand(1);
    footer_.Add(accept_button_).Fixed(DPI(96));
    footer_.Add(cancel_button_).Fixed(DPI(96));

    BuildColorPage();
    BuildPalettesPage();
    BuildGeneratorPage();
    BuildImagePage();
}

void UiColorPicker::Impl::BuildColorPage()
{
    color_page_.SetGap(DPI(4)).SetInset(DPI(5));
    color_page_.Add(color_columns_).Expand(1);
    color_columns_.SetGap(DPI(7));
    color_columns_.Add(color_left_).Expand(6).MinWidth(DPI(250));
    color_columns_.Add(color_right_).Expand(5).MinWidth(DPI(300));

    color_field_.Create();
    color_left_.SetGap(DPI(4));
    color_left_.Add(spectrum_field_).Fixed(DPI(44));
    color_left_.Add(*color_field_).Expand(1).MinMaxMain(DPI(170), DPI(220));

    color_right_.SetGap(DPI(3));
    color_right_.Add(color_mode_row_).Fixed(DPI(28));
    color_mode_row_.SetGap(DPI(4));
    color_mode_row_.Add(channel_drop_).Expand(1);
    color_mode_row_.Add(eyedropper_button_).Fixed(DPI(26));
    color_mode_row_.Add(alpha_label_).Fixed(DPI(34));
    color_mode_row_.Add(alpha_toggle_).Fixed(DPI(42));

    color_right_.Add(channel_rows_).Fixed(DPI(104));
    channel_rows_.SetGap(DPI(2));
    for(int i = 0; i < 5; i++) {
        numeric_row_[i].Create();
        channel_rows_.Add(*numeric_row_[i]).Expand(1);
    }
    hue_row_.Create();
    value_row_.Create();
    color_right_.Add(*hue_row_).Fixed(DPI(24));
    color_right_.Add(*value_row_).Fixed(DPI(24));

    color_right_.Add(readout_stack_).Fixed(DPI(96));
    readout_stack_.SetGap(DPI(3));
    for(int row = 0; row < 3; row++) {
        readout_pair_[row].Create(UiDirection::H);
        readout_pair_[row]->SetGap(DPI(4));
        readout_stack_.Add(*readout_pair_[row]).Fixed(DPI(30));
        for(int column = 0; column < 2; column++) {
            int index = row * 2 + column;
            readout_[index].Create();
            readout_pair_[row]->Add(*readout_[index]).Expand(1);
        }
    }
}

void UiColorPicker::Impl::BuildPalettesPage()
{
    palettes_page_.SetGap(DPI(4)).SetInset(DPI(5));
    palettes_page_.Add(palettes_top_).Fixed(DPI(44));
    palettes_page_.Add(palettes_body_).Expand(1);
    palettes_page_.Add(palette_actions_).Fixed(DPI(30));

    palettes_top_.SetGap(DPI(6));
    palettes_top_.Add(palette_field_).Expand(1);
    palettes_top_.Add(palette_badge_).Fixed(DPI(120));

    palette_wheel_.Create();
    palettes_body_.SetGap(DPI(6));
    palettes_body_.Add(*palette_wheel_).Expand(4).MinWidth(DPI(190));
    palettes_body_.Add(palette_scroll_).Expand(6).MinWidth(DPI(300));
    palette_scroll_.SetScrollMode(UIPANELSCROLL_VERTICAL);
    palette_scroll_.Content().Add(palette_flow_);

    palette_actions_.SetGap(DPI(5));
    palette_actions_.Add(palette_summary_).Expand(1);
    palette_actions_.Add(palette_add_all_).Fixed(DPI(72));
}

void UiColorPicker::Impl::BuildGeneratorPage()
{
    generator_page_.SetGap(DPI(4)).SetInset(DPI(5));
    generator_page_.Add(generator_top_).Fixed(DPI(44));
    generator_page_.Add(generator_body_).Expand(1);

    generator_top_.SetGap(DPI(5));
    generator_top_.Add(harmony_field_).Expand(3);
    generator_top_.Add(distribution_field_).Expand(3);
    generator_top_.Add(generator_medium_field_).Expand(3);
    generator_top_.Add(generator_count_field_).Expand(2);

    generator_wheel_.Create();
    generator_body_.SetGap(DPI(7));
    generator_body_.Add(*generator_wheel_).Expand(5).MinWidth(DPI(220));
    generator_body_.Add(generator_right_).Expand(6).MinWidth(DPI(330));

    generator_right_.SetGap(DPI(2));
    generator_right_.Add(generator_flow_).Fixed(DPI(32));
    generator_right_.Add(free_angles_row_).Fixed(DPI(26));
    generator_gain_row_.Create();
    generator_saturation_row_.Create();
    family_gain_row_.Create();
    generator_right_.Add(*generator_gain_row_).Fixed(DPI(24));
    generator_right_.Add(*generator_saturation_row_).Fixed(DPI(24));
    generator_right_.Add(*family_gain_row_).Fixed(DPI(24));
    generator_right_.Add(generator_actions_).Fixed(DPI(28));

    free_angles_row_.SetGap(DPI(4));
    free_angles_row_.Add(free_angles_label_).Fixed(DPI(70));
    free_angles_row_.Add(free_angles_toggle_).Fixed(DPI(42));
    free_angles_row_.Add(selected_family_label_).Expand(1);
    free_angles_row_.Add(family_lock_button_).Fixed(DPI(92));

    generator_actions_.SetGap(DPI(5));
    generator_actions_.AddSpacer(1);
    generator_actions_.Add(generator_add_all_).Fixed(DPI(76));
}

void UiColorPicker::Impl::BuildImagePage()
{
    image_page_.SetGap(DPI(4)).SetInset(DPI(5));
    image_page_.Add(image_top_).Fixed(DPI(44));
    image_page_.Add(image_body_).Expand(1);
    image_page_.Add(image_status_).Fixed(DPI(18));

    image_top_.SetGap(DPI(5));
    image_top_.Add(image_analysis_field_).Expand(3);
    image_top_.Add(image_coverage_field_).Expand(3);
    image_top_.Add(image_medium_field_).Expand(3);
    image_top_.Add(image_count_field_).Expand(2);

    image_canvas_.Create();
    image_body_.SetGap(DPI(7));
    image_body_.Add(image_left_).Expand(6).MinWidth(DPI(300));
    image_body_.Add(image_right_).Expand(5).MinWidth(DPI(310));

    image_left_.SetGap(DPI(4));
    image_left_.Add(*image_canvas_).Expand(1);
    image_left_.Add(image_navigation_).Fixed(DPI(30));
    image_navigation_.SetGap(DPI(4));
    image_navigation_.Add(image_load_button_).Fixed(DPI(76));
    image_navigation_.Add(image_analyze_button_).Fixed(DPI(92));
    image_navigation_.AddSpacer(1);
    image_navigation_.Add(image_fit_button_).Fixed(DPI(44));
    image_navigation_.Add(image_zoom_out_).Fixed(DPI(27));
    image_navigation_.Add(image_zoom_label_).Fixed(DPI(48));
    image_navigation_.Add(image_zoom_in_).Fixed(DPI(27));

    image_right_.SetGap(DPI(2));
    image_right_.Add(image_flow_).Fixed(DPI(32));
    image_gain_row_.Create();
    image_saturation_row_.Create();
    hero_gain_row_.Create();
    tolerance_row_.Create();
    image_right_.Add(*image_gain_row_).Fixed(DPI(24));
    image_right_.Add(*image_saturation_row_).Fixed(DPI(24));
    image_right_.Add(hero_row_).Fixed(DPI(28));
    image_right_.Add(*hero_gain_row_).Fixed(DPI(24));
    image_right_.Add(exclusion_row_).Fixed(DPI(28));
    image_right_.Add(*tolerance_row_).Fixed(DPI(24));
    image_right_.Add(image_actions_).Fixed(DPI(28));

    hero_row_.SetGap(DPI(4));
    hero_row_.Add(hero_label_).Expand(1);
    hero_row_.Add(hero_set_button_).Fixed(DPI(72));

    exclusion_row_.SetGap(DPI(3));
    for(int i = 0; i < 2; i++) {
        exclusion_row_.Add(exclusion_button_[i]).Fixed(DPI(30));
        exclusion_row_.Add(exclusion_label_[i]).Fixed(DPI(13));
        exclusion_row_.Add(exclusion_toggle_[i]).Fixed(DPI(38));
    }
    exclusion_row_.AddSpacer(1);

    image_actions_.SetGap(DPI(4));
    image_actions_.Add(show_exclusion_label_).Fixed(DPI(32));
    image_actions_.Add(show_exclusion_toggle_).Fixed(DPI(38));
    image_actions_.Add(clear_exclusion_button_).Fixed(DPI(48));
    image_actions_.AddSpacer(1);
    image_actions_.Add(image_send_generator_).Fixed(DPI(104));
}

void UiColorPicker::Impl::ConfigureControls()
{
    const char *page_name[PAGE_COUNT] = { "Color", "Palettes", "Generator", "Image" };
    for(int i = 0; i < PAGE_COUNT; i++) {
        page_button_[i].SetText(page_name[i]);
        page_button_[i].SetCheckable(true);
    }

    current_preview_.SetText("")
                    .SetContentInset(0)
                    .SetIconRenderMode(UiIconRenderMode::PreserveColor);
    for(int i = 0; i < 8; i++) {
        primary_slot_[i].SetText("")
                        .SetCheckable(true)
                        .SetContentInset(0)
                        .SetIconRenderMode(UiIconRenderMode::PreserveColor);
        previous_slot_[i].SetText("")
                         .SetContentInset(0)
                         .SetIconRenderMode(UiIconRenderMode::PreserveColor);
    }

    stash_title_.SetText("User Stash");
    stash_title_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
    stash_mode_drop_.Clear();
    stash_mode_drop_.Add("Replace", (int)STASH_REPLACE);
    stash_mode_drop_.Add("Mix", (int)STASH_MIX);
    stash_mode_drop_.Add("Add", (int)STASH_ADD);
    stash_mode_drop_.Add("Subtract", (int)STASH_SUBTRACT);
    stash_mode_drop_.Add("Multiply", (int)STASH_MULTIPLY);
    stash_mode_drop_.SetDataSilently((int)STASH_REPLACE);
    stash_add_selected_.SetText("Add selected");
    stash_export_.SetText("Export");
    stash_import_.SetText("Import");
    stash_clear_.SetText("Clear");
    stash_flow_.SetCompact(true);
    stash_flow_.SetSnapColumns(14);
    stash_flow_.EnableDropTarget(true);

    accept_button_.SetText("OK");
    cancel_button_.SetText("Cancel");
    UiLineEdit::Style footer_style = UiTheme::ResolveEdit(UiRole::Subtle);
    footer_style.font = Monospace().Height(DPI(10));
    footer_hex_.SetCustomStyle(footer_style);
    footer_detail_.SetCustomStyle(footer_style);

    spectrum_field_.SetLabel("Spectrum");
    spectrum_field_.Drop().Clear();
    spectrum_field_.Drop().Add("Hue strip", (int)SPECTRUM_HUE_STRIP);
    spectrum_field_.Drop().Add("HSV rectangle", (int)SPECTRUM_HSV_RECT);
    spectrum_field_.Drop().Add("RGB spectrum", (int)SPECTRUM_RGB_SPECTRUM);
    spectrum_field_.Drop().Add("HSV wheel", (int)SPECTRUM_HSV_WHEEL);

    channel_drop_.Clear();
    channel_drop_.Add("RGB Float", (int)CHANNEL_RGB_FLOAT);
    channel_drop_.Add("RGB 8-bit", (int)CHANNEL_RGB_INT);
    channel_drop_.Add("HSB / HSV", (int)CHANNEL_HSV);
    channel_drop_.Add("HLS", (int)CHANNEL_HSL);
    channel_drop_.Add("TMI", (int)CHANNEL_TMI);
    channel_drop_.Add("CMYK", (int)CHANNEL_CMYK);
    channel_drop_.Add("CIE Lab", (int)CHANNEL_LAB);
    eyedropper_button_.SetText("")
                       .SetIcon(ICON_DESIGN_FORMAT_PAINT_48())
                       .SetIconSize(DPI(14), DPI(14))
                       .SetIconRenderMode(UiIconRenderMode::MonoTint)
                       .SetCheckable(true);
    eyedropper_button_.Tip("Sample a colour from the screen");
    eyedropper_button_.Enable(IsEyedropperAvailable());
    alpha_label_.SetText("Alpha");
    alpha_label_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
    alpha_toggle_.SetOn(alpha_enabled_);

    hue_row_->Configure("Hue", 0, 359, remembered_hue_);
    value_row_->Configure("Gain", 0, 100, 100);
    hue_row_->Slider().WhenPaintTrack = [=](Draw& draw, const UiSlider::PaintContext& context, bool& handled) {
        int denominator = max(1, context.track.GetWidth() - 1);
        for(int x = 0; x < context.track.GetWidth(); x++) {
            int hue = int(x / double(denominator) * 359.0 + 0.5);
            draw.DrawRect(context.track.left + x, context.track.top, 1, context.track.GetHeight(),
                          HsvToColor(hue, 100, 100));
        }
        handled = true;
    };
    hue_row_->Slider().WhenPaintActiveTrack = [=](Draw&, const UiSlider::PaintContext&, bool& handled) { handled = true; };
    value_row_->Slider().WhenPaintTrack = [=](Draw& draw, const UiSlider::PaintContext& context, bool& handled) {
        int hue = remembered_hue_, saturation = 0, value = 0, measured_hue = 0;
        ColorToHsv(GetSlotColor(active_slot_), measured_hue, saturation, value);
        if(saturation > 0 && value > 0)
            hue = measured_hue;
        int denominator = max(1, context.track.GetWidth() - 1);
        for(int x = 0; x < context.track.GetWidth(); x++)
            draw.DrawRect(context.track.left + x, context.track.top, 1, context.track.GetHeight(),
                          HsvToColor(hue, saturation, x / double(denominator) * 100.0));
        handled = true;
    };
    value_row_->Slider().WhenPaintActiveTrack = [=](Draw&, const UiSlider::PaintContext&, bool& handled) { handled = true; };

    const char *readout_title[6] = { "HSV-A", "RGB HEX", "HLS-A", "RGB Float", "CMYK-A", "RGB 8-bit" };
    for(int i = 0; i < 6; i++)
        readout_[i]->SetTitle(readout_title[i]);
    ConfigureChannelRows();

    palette_field_.SetLabel("Palette set");
    palette_badge_.SetAlign(UiAlign::RIGHT, UiAlign::CENTER);
    palette_flow_.SetSnapColumns(6);
    palette_summary_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
    palette_add_selected_.SetText("Add selected");
    palette_add_all_.SetText("Add all");

    harmony_field_.SetLabel("Harmony");
    distribution_field_.SetLabel("Distribution");
    generator_medium_field_.SetLabel("Medium");
    generator_count_field_.SetLabel("Base swatches");
    generator_flow_.SetSnapColumns(6);
    free_angles_label_.SetText("Free angles");
    free_angles_label_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
    selected_family_label_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
    family_lock_button_.SetText("Lock family");
    generator_gain_row_->Configure("Global Gain", -50, 50, recipe_.global_gain);
    generator_saturation_row_->Configure("Global Saturation", 0, 150, recipe_.global_saturation);
    family_gain_row_->Configure("Family Gain", -50, 50, 0);
    generator_add_selected_.SetText("Add selected");
    generator_add_all_.SetText("Add all");

    image_analysis_field_.SetLabel("Analysis");
    image_coverage_field_.SetLabel("Coverage");
    image_medium_field_.SetLabel("Output");
    image_count_field_.SetLabel("Base swatches");
    image_flow_.SetSnapColumns(6);
    image_load_button_.SetText("Load image");
    image_analyze_button_.SetText("Analyze");
    image_fit_button_.SetText("Fit");
    image_zoom_out_.SetText("-");
    image_zoom_in_.SetText("+");
    image_zoom_label_.SetText("100%");
    image_zoom_label_.SetAlign(UiAlign::CENTER, UiAlign::CENTER);
    image_gain_row_->Configure("Global Gain", -50, 50, image_gain_);
    image_saturation_row_->Configure("Global Saturation", 0, 150, image_saturation_);
    hero_label_.SetText("Hero: none");
    hero_label_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
    hero_set_button_.SetText("Set hero");
    hero_gain_row_->Configure("Hero Gain", -50, 50, image_hero_gain_);
    hero_gain_row_->Enable(false);
    for(int i = 0; i < 2; i++) {
        exclusion_button_[i].SetText("")
                            .SetContentInset(0)
                            .SetIconRenderMode(UiIconRenderMode::PreserveColor);
        exclusion_label_[i].SetText(i == 0 ? "A" : "B");
        exclusion_label_[i].SetAlign(UiAlign::CENTER, UiAlign::CENTER);
        exclusion_toggle_[i].SetOn(false);
        exclusion_button_[i].Tip(i == 0 ? "Arm background exclusion A" : "Arm background exclusion B");
    }
    tolerance_row_->Configure("Tolerance", 1, 100, image_settings_.tolerance);
    show_exclusion_label_.SetText("Show");
    show_exclusion_label_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
    clear_exclusion_button_.SetText("Clear");
    image_send_generator_.SetText("To Generator");
    image_status_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
}

void UiColorPicker::Impl::ConfigureChannelRows()
{
    struct Config { const char *label; double minimum; double maximum; int precision; int steps; bool alpha; } config[5];
    channel_row_count_ = channel_mode_ == CHANNEL_CMYK ? 5 : 4;
    switch(channel_mode_) {
    case CHANNEL_RGB_FLOAT:
        config[0] = { "R", 0, 1, 4, 10000, false };
        config[1] = { "G", 0, 1, 4, 10000, false };
        config[2] = { "B", 0, 1, 4, 10000, false };
        config[3] = { "A", 0, 1, 4, 10000, true };
        break;
    case CHANNEL_RGB_INT:
        config[0] = { "R", 0, 255, 0, 255, false };
        config[1] = { "G", 0, 255, 0, 255, false };
        config[2] = { "B", 0, 255, 0, 255, false };
        config[3] = { "A", 0, 255, 0, 255, true };
        break;
    case CHANNEL_HSV:
        config[0] = { "H", 0, 359, 0, 359, false };
        config[1] = { "S", 0, 100, 2, 10000, false };
        config[2] = { "V", 0, 100, 2, 10000, false };
        config[3] = { "A", 0, 100, 2, 10000, true };
        break;
    case CHANNEL_HSL:
        config[0] = { "H", 0, 359, 0, 359, false };
        config[1] = { "S", 0, 100, 2, 10000, false };
        config[2] = { "L", 0, 100, 2, 10000, false };
        config[3] = { "A", 0, 100, 2, 10000, true };
        break;
    case CHANNEL_TMI:
        config[0] = { "T", -100, 100, 2, 20000, false };
        config[1] = { "M", -100, 100, 2, 20000, false };
        config[2] = { "I", 0, 100, 2, 10000, false };
        config[3] = { "A", 0, 100, 2, 10000, true };
        break;
    case CHANNEL_CMYK:
        config[0] = { "C", 0, 100, 2, 10000, false };
        config[1] = { "M", 0, 100, 2, 10000, false };
        config[2] = { "Y", 0, 100, 2, 10000, false };
        config[3] = { "K", 0, 100, 2, 10000, false };
        config[4] = { "A", 0, 100, 2, 10000, true };
        break;
    case CHANNEL_LAB:
    default:
        config[0] = { "L*", 0, 100, 2, 10000, false };
        config[1] = { "a*", -128, 127, 2, 25500, false };
        config[2] = { "b*", -128, 127, 2, 25500, false };
        config[3] = { "A", 0, 100, 2, 10000, true };
        break;
    }
    for(int i = 0; i < 5; i++) {
        bool visible = i < channel_row_count_;
        numeric_row_[i]->Show(visible);
        if(visible) {
            numeric_row_[i]->Configure(config[i].label, config[i].minimum, config[i].maximum,
                                       config[i].precision, config[i].steps, config[i].alpha);
            numeric_row_[i]->EnableAlpha(alpha_enabled_);
        }
    }
}

void UiColorPicker::Impl::WireEvents()
{
    for(int i = 0; i < PAGE_COUNT; i++) {
        int index = i;
        page_button_[i].WhenAction = [=] { SetPageMode((PageMode)index); };
    }
    current_preview_.WhenAction = [=] { SetPageMode(PAGE_COLOR); };
    for(int i = 0; i < 8; i++) {
        int index = i;
        primary_slot_[i].WhenAction = [=] { SetActiveSlot(index); };
        previous_slot_[i].WhenAction = [=] {
            if(index >= slot_count_)
                return;
            SlotValue current = slots_[index];
            slots_[index] = previous_[index];
            previous_[index] = current;
            active_slot_ = index;
            PushRecent(slots_[index].color, slots_[index].alpha);
            SyncAll();
            SaveSession(true);
            if(owner_.WhenSlotChanged)
                owner_.WhenSlotChanged(index);
            if(owner_.WhenAction)
                owner_.WhenAction();
        };
    }

    stash_flow_.WhenSelection = [=](int, SlotValue value) { SyncFooter(value); };
    stash_flow_.WhenActivate = [=](int, SlotValue value) {
        CommitSlotValue(active_slot_, value.color, value.alpha, true);
    };
    stash_flow_.WhenDropGroup = [=](int target, const Vector<SlotValue>& values) { HandleStashDrop(target, values); };
    stash_mode_drop_.WhenSelectData = [=](const Value& data) {
        if(!IsNull(data)) {
            stash_drop_mode_ = (StashDropMode)minmax((int)data, (int)STASH_REPLACE, (int)STASH_MULTIPLY);
            SaveSession(false);
        }
    };
    stash_clear_.WhenAction = [=] {
        if(PromptYesNo("Clear the shared User Stash?"))
            ClearUserSwatches();
    };
    stash_add_selected_.WhenAction = [=] { AddActiveSelection(); };
    stash_export_.WhenAction = [=] { SavePaletteJson(); };
    stash_import_.WhenAction = [=] { LoadPaletteJson(); };
    accept_button_.WhenAction = [=] { Accept(); };
    cancel_button_.WhenAction = [=] { Cancel(); };

    spectrum_field_.Drop().WhenSelectData = [=](const Value& data) {
        if(!IsNull(data))
            SetSpectrumMode((SpectrumMode)(int)data);
    };
    channel_drop_.WhenSelectData = [=](const Value& data) {
        if(!IsNull(data))
            SetChannelMode((ChannelMode)(int)data);
    };
    eyedropper_button_.WhenAction = [=] {
        if(eyedropper_active_)
            StopEyedropper(false);
        else
            BeginEyedropper();
    };
    alpha_toggle_.WhenAction = [=] { SetAlphaEnabled(alpha_toggle_.IsOn()); };
    color_field_->WhenColor = [=](Color color, bool final_commit) { CommitColor(color, final_commit); };
    hue_row_->WhenValue = [=](int hue, bool final_commit) {
        if(syncing_)
            return;
        remembered_hue_ = NormalizeHue(hue);
        int old_hue = 0, saturation = 0, value = 0;
        ColorToHsv(GetSlotColor(active_slot_), old_hue, saturation, value);
        CommitColor(HsvToColor(remembered_hue_, saturation, value), final_commit);
    };
    value_row_->WhenValue = [=](int value, bool final_commit) {
        if(syncing_)
            return;
        int hue = 0, saturation = 0, old_value = 0;
        ColorToHsv(GetSlotColor(active_slot_), hue, saturation, old_value);
        if(saturation == 0 || old_value == 0)
            hue = remembered_hue_;
        CommitColor(HsvToColor(hue, saturation, value), final_commit);
    };
    for(int i = 0; i < 5; i++) {
        int row = i;
        numeric_row_[i]->WhenValue = [=](double value, bool final_commit) {
            HandleChannelValue(row, value, final_commit);
        };
    }
    for(int i = 0; i < 6; i++)
        readout_[i]->WhenCommitText = [=](String text) {
            if(!ApplyColorText(text, true))
                SyncReadouts();
        };

    palette_field_.Drop().WhenSelectData = [=](const Value& data) {
        if(!IsNull(data))
            SetStaticPalette((int)data);
    };
    palette_flow_.WhenSelection = [=](int index, SlotValue value) { SyncPaletteSelection(index, value); };
    palette_flow_.WhenActivate = [=](int, SlotValue value) {
        Vector<SlotValue> one;
        one.Add(value);
        AddUserSwatches(one, true);
    };
    palette_wheel_->WhenPassivePick = [=](int index) { palette_flow_.SetSelectedIndex(index); SyncPaletteSelection(index, palette_flow_.GetItems()[index].value); };
    palette_add_selected_.WhenAction = [=] { AddPaletteSelection(false); };
    palette_add_all_.WhenAction = [=] { AddPaletteSelection(true); };

    harmony_field_.Drop().WhenSelectData = [=](const Value& data) {
        if(!IsNull(data))
            SetHarmonyMode((HarmonyMode)(int)data);
    };
    distribution_field_.Drop().WhenSelectData = [=](const Value& data) {
        if(!IsNull(data))
            SetDistributionMode((DistributionMode)(int)data);
    };
    generator_medium_field_.Drop().WhenSelectData = [=](const Value& data) {
        if(!IsNull(data))
            SetMediumMode((MediumMode)(int)data);
    };
    generator_count_field_.Drop().WhenSelectData = [=](const Value& data) {
        if(!IsNull(data))
            SetGeneratorCount((int)data);
    };
    free_angles_toggle_.WhenAction = [=] {
        recipe_.free_angles = free_angles_toggle_.IsOn();
        if(!recipe_.free_angles)
            for(PaletteFamily& family : recipe_.families)
                family.custom_offset = family.canonical_offset;
        RefreshGenerator(true);
        SaveSession(false);
    };
    generator_wheel_->WhenAnchorChange = [=](int family, int base_hue, int custom_offset, int authored, bool) {
        if(family < 0 || family >= recipe_.families.GetCount())
            return;
        recipe_.base_hue = base_hue;
        recipe_.families[family].custom_offset = custom_offset;
        recipe_.families[family].authored_saturation = authored;
        active_family_ = family;
        RefreshGenerator(true);
        SaveSession(false);
    };
    generator_gain_row_->WhenValue = [=](int value, bool) {
        recipe_.global_gain = value;
        RefreshGenerator(true);
        SaveSession(false);
    };
    generator_saturation_row_->WhenValue = [=](int value, bool) {
        recipe_.global_saturation = value;
        RefreshGenerator(true);
        SaveSession(false);
    };
    family_gain_row_->WhenValue = [=](int value, bool) {
        if(active_family_ >= 0 && active_family_ < recipe_.families.GetCount()) {
            recipe_.families[active_family_].gain = value;
            RefreshGenerator(true);
            SaveSession(false);
        }
    };
    family_lock_button_.WhenAction = [=] {
        if(active_family_ >= 0 && active_family_ < recipe_.families.GetCount()) {
            recipe_.families[active_family_].locked = !recipe_.families[active_family_].locked;
            RefreshGenerator(true);
            SaveSession(false);
        }
    };
    generator_flow_.WhenSelection = [=](int index, SlotValue value) { SyncGeneratorSelection(index, value); };
    generator_flow_.WhenActivate = [=](int, SlotValue value) {
        CommitSlotValue(active_slot_, value.color, value.alpha, true);
    };
    generator_add_selected_.WhenAction = [=] { AddGeneratorSelection(false); };
    generator_add_all_.WhenAction = [=] { AddGeneratorSelection(true); };

    image_analysis_field_.Drop().WhenSelectData = [=](const Value& data) {
        if(IsNull(data))
            return;
        image_settings_.analysis = (ImageAnalysisMode)(int)data;
        EnsureManualPoints();
        MarkImageStale();
    };
    image_coverage_field_.Drop().WhenSelectData = [=](const Value& data) {
        if(!IsNull(data)) {
            image_settings_.coverage = (ImageCoverageMode)(int)data;
            MarkImageStale();
        }
    };
    image_medium_field_.Drop().WhenSelectData = [=](const Value& data) {
        if(!IsNull(data)) {
            image_settings_.medium = (MediumMode)(int)data;
            RefreshImageDisplay(true);
            SaveSession(false);
        }
    };
    image_count_field_.Drop().WhenSelectData = [=](const Value& data) {
        if(!IsNull(data)) {
            image_settings_.requested_count = minmax((int)data, 2, 12);
            EnsureManualPoints();
            MarkImageStale();
            RefreshImageDisplay(true);
        }
    };
    image_load_button_.WhenAction = [=] { LoadImage(); };
    image_canvas_->WhenLoadRequest = [=] { LoadImage(); };
    image_canvas_->WhenImageDrop = [=](Image image) {
        if(!image.IsEmpty())
            SetGeneratorImage(image);
    };
    image_analyze_button_.WhenAction = [=] { AnalyzeCurrentImage(false); };
    image_fit_button_.WhenAction = [=] { image_canvas_->Fit(); image_zoom_label_.SetText("100%"); };
    image_zoom_out_.WhenAction = [=] { image_canvas_->ZoomBy(1.0 / 1.2); image_zoom_label_.SetText(Format("%d%%", image_canvas_->GetZoomPercent())); };
    image_zoom_in_.WhenAction = [=] { image_canvas_->ZoomBy(1.2); image_zoom_label_.SetText(Format("%d%%", image_canvas_->GetZoomPercent())); };
    image_gain_row_->WhenValue = [=](int value, bool) { image_gain_ = value; RefreshImageDisplay(true); SaveSession(false); };
    image_saturation_row_->WhenValue = [=](int value, bool) { image_saturation_ = value; RefreshImageDisplay(true); SaveSession(false); };
    hero_set_button_.WhenAction = [=] {
        int selected = image_flow_.GetActiveIndex();
        if(selected >= 0 && selected < image_display_.GetCount()) {
            image_hero_ = selected;
            RefreshImageDisplay(true);
            SaveSession(false);
        }
    };
    hero_gain_row_->WhenValue = [=](int value, bool) { image_hero_gain_ = value; RefreshImageDisplay(true); SaveSession(false); };
    image_flow_.WhenSelection = [=](int index, SlotValue value) { SyncImageSelection(index, value); };
    image_flow_.WhenActivate = [=](int, SlotValue value) { CommitSlotValue(active_slot_, value.color, value.alpha, true); };
    image_canvas_->WhenSeedPlaced = [=](int index, Pointf position) {
        if(index < 0 || index >= 2)
            return;
        image_settings_.exclusion[index].enabled = true;
        image_settings_.exclusion[index].placed = true;
        image_settings_.exclusion[index].position = position;
        image_settings_.exclusion[index].color = SampleImage_(image_, position, 3);
        exclusion_toggle_[index].SetOn(true);
        exclusion_button_[index].SetIcon(MakeAlphaSwatchImage_(image_settings_.exclusion[index].color, 255, Size(DPI(24), DPI(18))));
        image_canvas_->SetSeeds(image_settings_.exclusion);
        MarkImageStale();
    };
    image_canvas_->WhenSeedMoved = [=](int index, Pointf position, bool) {
        if(index < 0 || index >= 2)
            return;
        image_settings_.exclusion[index].position = position;
        image_settings_.exclusion[index].color = SampleImage_(image_, position, 3);
        exclusion_button_[index].SetIcon(MakeAlphaSwatchImage_(image_settings_.exclusion[index].color, 255, Size(DPI(24), DPI(18))));
        image_canvas_->SetSeeds(image_settings_.exclusion);
        MarkImageStale();
    };
    image_canvas_->WhenManualMoved = [=](int index, Pointf position, bool) {
        if(index < 0 || index >= manual_points_.GetCount())
            return;
        manual_points_[index] = position;
        ResampleManualPoint(index);
        RefreshImageDisplay(true);
    };
    for(int i = 0; i < 2; i++) {
        int index = i;
        exclusion_button_[i].WhenAction = [=] { image_canvas_->ArmSeed(index); };
        exclusion_toggle_[i].WhenAction = [=] {
            bool enabled = exclusion_toggle_[index].IsOn();
            image_settings_.exclusion[index].enabled = enabled;
            if(enabled && !image_.IsEmpty()) {
                if(!image_settings_.exclusion[index].placed)
                    image_settings_.exclusion[index].position = index == 0 ? Pointf(0.33, 0.50) : Pointf(0.67, 0.50);
                image_settings_.exclusion[index].placed = true;
                image_settings_.exclusion[index].color = SampleImage_(image_, image_settings_.exclusion[index].position, 3);
                exclusion_button_[index].SetIcon(MakeAlphaSwatchImage_(image_settings_.exclusion[index].color, 255, Size(DPI(24), DPI(18))));
            }
            image_canvas_->SetSeeds(image_settings_.exclusion);
            MarkImageStale();
        };
    }
    tolerance_row_->WhenValue = [=](int value, bool) { image_settings_.tolerance = value; MarkImageStale(); };
    show_exclusion_toggle_.WhenAction = [=] {
        image_show_mask_ = show_exclusion_toggle_.IsOn();
        if(image_show_mask_ && image_stale_ && !image_.IsEmpty())
            AnalyzeCurrentImage(true);
        image_canvas_->SetResult(image_result_, image_show_mask_);
    };
    clear_exclusion_button_.WhenAction = [=] {
        for(int i = 0; i < 2; i++) {
            image_settings_.exclusion[i] = ImageExclusionSeed();
            exclusion_toggle_[i].SetOn(false);
            exclusion_button_[i].SetIcon(Image());
        }
        image_show_mask_ = false;
        show_exclusion_toggle_.SetOn(false);
        image_canvas_->SetSeeds(image_settings_.exclusion);
        MarkImageStale();
    };
    image_send_generator_.WhenAction = [=] { SendImageToGenerator(); };
}

void UiColorPicker::Impl::PopulateStaticPaletteDrop()
{
    palette_field_.Drop().Clear();
    const Vector<StaticPaletteDefinition>& library = StaticPaletteLibrary();
    for(int i = 0; i < library.GetCount(); i++)
        palette_field_.Drop().Add(library[i].name, i);
    palette_index_ = minmax(palette_index_, 0, max(0, library.GetCount() - 1));
    palette_field_.Drop().SetDataSilently(palette_index_);
}

void UiColorPicker::Impl::PopulateGeneratorControls()
{
    harmony_field_.Drop().Clear();
    harmony_field_.Drop().Add("Monochromatic", (int)HARMONY_MONOCHROMATIC);
    harmony_field_.Drop().Add("Analogous", (int)HARMONY_ANALOGOUS);
    harmony_field_.Drop().Add("Complementary", (int)HARMONY_COMPLEMENTARY);
    harmony_field_.Drop().Add("Split complementary", (int)HARMONY_SPLIT_COMPLEMENTARY);
    harmony_field_.Drop().Add("Triad", (int)HARMONY_TRIAD);
    harmony_field_.Drop().Add("Square / Tetrad", (int)HARMONY_SQUARE);
    harmony_field_.Drop().Add("Compound", (int)HARMONY_COMPOUND);

    distribution_field_.Drop().Clear();
    distribution_field_.Drop().Add("Balanced", (int)DISTRIBUTION_BALANCED);
    distribution_field_.Drop().Add("Dominant / 60-30-10", (int)DISTRIBUTION_DOMINANT);
    distribution_field_.Drop().Add("Accent pop", (int)DISTRIBUTION_ACCENT_POP);
    distribution_field_.Drop().Add("Tonal ramp", (int)DISTRIBUTION_TONAL_RAMP);
    distribution_field_.Drop().Add("Free-form", (int)DISTRIBUTION_FREE_FORM);

    generator_medium_field_.Drop().Clear();
    generator_medium_field_.Drop().Add("UI / interface", (int)MEDIUM_UI);
    generator_medium_field_.Drop().Add("Web / sRGB", (int)MEDIUM_WEB);
    generator_medium_field_.Drop().Add("Print / editorial", (int)MEDIUM_PRINT);
    generator_medium_field_.Drop().Add("Painting / illustration", (int)MEDIUM_PAINTING);
    generator_medium_field_.Drop().Add("Image / VFX", (int)MEDIUM_IMAGE_VFX);

    generator_count_field_.Drop().Clear();
    for(int count = 2; count <= 12; count++)
        generator_count_field_.Drop().Add(AsString(count), count);

    harmony_field_.Drop().SetDataSilently((int)recipe_.harmony);
    distribution_field_.Drop().SetDataSilently((int)recipe_.distribution);
    generator_medium_field_.Drop().SetDataSilently((int)recipe_.medium);
    generator_count_field_.Drop().SetDataSilently(recipe_.requested_count);
    free_angles_toggle_.SetOn(recipe_.free_angles);
    generator_gain_row_->SetValue(recipe_.global_gain);
    generator_saturation_row_->SetValue(recipe_.global_saturation);
}

void UiColorPicker::Impl::PopulateImageControls()
{
    image_analysis_field_.Drop().Clear();
    image_analysis_field_.Drop().Add("Representative", (int)IMAGE_REPRESENTATIVE);
    image_analysis_field_.Drop().Add("Interface / UI", (int)IMAGE_INTERFACE);
    image_analysis_field_.Drop().Add("Accent finder", (int)IMAGE_ACCENT_FINDER);
    image_analysis_field_.Drop().Add("Nature / scene", (int)IMAGE_NATURE_SCENE);
    image_analysis_field_.Drop().Add("Paint / material", (int)IMAGE_PAINT_MATERIAL);
    image_analysis_field_.Drop().Add("Manual points", (int)IMAGE_MANUAL_POINTS);

    image_coverage_field_.Drop().Clear();
    image_coverage_field_.Drop().Add("Area weighted", (int)COVERAGE_AREA_WEIGHTED);
    image_coverage_field_.Drop().Add("Balanced", (int)COVERAGE_BALANCED);
    image_coverage_field_.Drop().Add("Border aware", (int)COVERAGE_BORDER_AWARE);
    image_coverage_field_.Drop().Add("Distinctive", (int)COVERAGE_DISTINCTIVE);

    image_medium_field_.Drop().Clear();
    image_medium_field_.Drop().Add("UI / interface", (int)MEDIUM_UI);
    image_medium_field_.Drop().Add("Web / sRGB", (int)MEDIUM_WEB);
    image_medium_field_.Drop().Add("Print / editorial", (int)MEDIUM_PRINT);
    image_medium_field_.Drop().Add("Painting", (int)MEDIUM_PAINTING);
    image_medium_field_.Drop().Add("Image / VFX", (int)MEDIUM_IMAGE_VFX);

    image_count_field_.Drop().Clear();
    for(int count = 2; count <= 12; count++)
        image_count_field_.Drop().Add(AsString(count), count);

    image_analysis_field_.Drop().SetDataSilently((int)image_settings_.analysis);
    image_coverage_field_.Drop().SetDataSilently((int)image_settings_.coverage);
    image_medium_field_.Drop().SetDataSilently((int)image_settings_.medium);
    image_count_field_.Drop().SetDataSilently(image_settings_.requested_count);
    tolerance_row_->SetValue(image_settings_.tolerance);
    image_gain_row_->SetValue(image_gain_);
    image_saturation_row_->SetValue(image_saturation_);
    hero_gain_row_->SetValue(image_hero_gain_);
}

void UiColorPicker::Impl::SetStaticPalette(int index)
{
    const Vector<StaticPaletteDefinition>& library = StaticPaletteLibrary();
    if(index < 0 || index >= library.GetCount())
        return;
    palette_index_ = index;
    const StaticPaletteDefinition& palette = library[index];
    Vector<DisplaySwatch_> display;
    for(const SlotValue& value : palette.swatches) {
        DisplaySwatch_& item = display.Add();
        item.value = value;
    }
    palette_flow_.SetSnapColumns(max(1, palette.preferred_columns));
    palette_flow_.SetItems(display);
    if(!display.IsEmpty())
        palette_flow_.SetSelectedIndex(0);
    palette_field_.Drop().SetDataSilently(index);
    palette_badge_.SetText(palette.authoritative ? "Reference" : palette.category);
    palette_badge_.Tip(palette.source_reference);
    if(!display.IsEmpty())
        SyncPaletteSelection(0, display[0].value);
    else
        palette_summary_.SetText("Empty palette");
    SaveSession(false);
    Layout();
}

void UiColorPicker::Impl::SyncPaletteSelection(int index, const SlotValue& value)
{
    const Vector<StaticPaletteDefinition>& library = StaticPaletteLibrary();
    if(palette_index_ < 0 || palette_index_ >= library.GetCount())
        return;
    const StaticPaletteDefinition& palette = library[palette_index_];
    palette_summary_.SetText(Format("%s · %s", palette.name,
                                    value.label.IsEmpty() ? Format("swatch %d", index + 1) : value.label));
    palette_wheel_->SetPassive(palette_flow_.GetItems(), palette_flow_.GetSelectedIndices(), index);
    SyncFooter(value);
}

void UiColorPicker::Impl::AddPaletteSelection(bool all)
{
    Vector<SlotValue> values;
    if(all) {
        for(const DisplaySwatch_& item : palette_flow_.GetItems())
            values.Add(item.value);
    }
    else
        values = palette_flow_.GetSelectedValues();
    AddUserSwatches(values, !all);
}

void UiColorPicker::Impl::RefreshGenerator(bool preserve_selection)
{
    generated_ = GeneratePalette(recipe_);
    Vector<DisplaySwatch_> display;
    for(const GeneratedSwatch& generated : generated_) {
        DisplaySwatch_& item = display.Add();
        item.value = generated.value;
        item.family_id = generated.family_id;
        item.hero_number = generated.hero ? generated.family_id + 1 : 0;
        item.gamut_mapped = generated.gamut_mapped;
    }
    generator_flow_.SetItems(display, preserve_selection);
    if(generator_flow_.GetSelectedIndices().IsEmpty() && !display.IsEmpty())
        generator_flow_.SetSelectedIndex(0);
    if(!display.IsEmpty()) {
        int active = generator_flow_.GetActiveIndex();
        if(active < 0)
            active = 0;
        active_family_ = minmax(display[active].family_id, 0, max(0, recipe_.families.GetCount() - 1));
    }
    generator_wheel_->SetGenerator(recipe_, generated_, active_family_);
    harmony_field_.Drop().SetDataSilently((int)recipe_.harmony);
    distribution_field_.Drop().SetDataSilently((int)recipe_.distribution);
    generator_medium_field_.Drop().SetDataSilently((int)recipe_.medium);
    generator_count_field_.Drop().SetDataSilently(recipe_.requested_count);
    free_angles_toggle_.SetOn(recipe_.free_angles);
    generator_gain_row_->SetValue(recipe_.global_gain);
    generator_saturation_row_->SetValue(recipe_.global_saturation);
    SyncSelectedFamily();
}

void UiColorPicker::Impl::SyncGeneratorSelection(int index, const SlotValue& value)
{
    if(index >= 0 && index < generated_.GetCount())
        active_family_ = generated_[index].family_id;
    SyncSelectedFamily();
    generator_wheel_->SetGenerator(recipe_, generated_, active_family_);
    SyncFooter(value);
}

void UiColorPicker::Impl::SyncSelectedFamily()
{
    if(active_family_ < 0 || active_family_ >= recipe_.families.GetCount()) {
        selected_family_label_.SetText("No family");
        family_gain_row_->Enable(false);
        family_lock_button_.Enable(false);
        return;
    }
    PaletteFamily& family = recipe_.families[active_family_];
    selected_family_label_.SetText(Format("%d · %s", active_family_ + 1, family.role));
    family_gain_row_->Enable(true);
    family_gain_row_->SetLabel(Format("%s Gain", family.role));
    family_gain_row_->SetValue(family.gain);
    family_lock_button_.Enable(true);
    family_lock_button_.SetText(family.locked ? "Unlock family" : "Lock family");
}

void UiColorPicker::Impl::AddGeneratorSelection(bool all)
{
    Vector<SlotValue> values;
    if(all) {
        for(const GeneratedSwatch& item : generated_)
            values.Add(item.value);
    }
    else
        values = generator_flow_.GetSelectedValues();
    AddUserSwatches(values, !all);
}

void UiColorPicker::Impl::EnsureManualPoints()
{
    if(image_settings_.analysis != IMAGE_MANUAL_POINTS) {
        manual_points_.Clear();
        image_canvas_->SetManualPoints(manual_points_);
        return;
    }
    int count = minmax(image_settings_.requested_count, 2, 12);
    while(manual_points_.GetCount() > count)
        manual_points_.Remove(manual_points_.GetCount() - 1);
    while(manual_points_.GetCount() < count) {
        int i = manual_points_.GetCount();
        int columns = count <= 6 ? count : 6;
        int rows = (count + columns - 1) / columns;
        manual_points_.Add(Pointf(((i % columns) + 0.5) / columns,
                                  ((i / columns) + 0.5) / rows));
    }
    image_canvas_->SetManualPoints(manual_points_);
}

void UiColorPicker::Impl::ResampleManualPoint(int index)
{
    if(image_settings_.analysis != IMAGE_MANUAL_POINTS || image_.IsEmpty() ||
       index < 0 || index >= manual_points_.GetCount())
        return;
    while(image_result_.swatches.GetCount() < manual_points_.GetCount()) {
        GeneratedSwatch& swatch = image_result_.swatches.Add();
        swatch.value.alpha = 255;
        swatch.source_index = image_result_.swatches.GetCount() - 1;
    }
    Color color = SampleImage_(image_, manual_points_[index], 3);
    image_result_.swatches[index].value.color = color;
    image_result_.swatches[index].value.alpha = 255;
    image_result_.swatches[index].value.label = Format("Manual %d", index + 1);
    image_result_.representative_positions = clone(manual_points_);
    image_result_.original_size = image_.GetSize();
    image_result_.proxy = MakeAnalysisProxy(image_, 512);
    image_result_.proxy_size = image_result_.proxy.GetSize();
    image_stale_ = false;
}

void UiColorPicker::Impl::MarkImageStale()
{
    if(!image_.IsEmpty())
        image_stale_ = true;
    SyncImageStatus();
    SaveSession(false);
}

void UiColorPicker::Impl::AnalyzeCurrentImage(bool mask_only)
{
    if(image_.IsEmpty()) {
        image_status_.SetText("Load an image before analysis.");
        return;
    }
    ImageAnalysisResult candidate = AnalyzeImage(image_, image_settings_);
    if(!candidate.IsValid()) {
        if(mask_only && !candidate.proxy.IsEmpty()) {
            image_result_.proxy = candidate.proxy;
            image_result_.proxy_size = candidate.proxy_size;
            image_result_.exclusion_mask = pick(candidate.exclusion_mask);
            image_result_.ignored_fraction = candidate.ignored_fraction;
            image_result_.diagnostic = candidate.diagnostic;
            image_canvas_->SetResult(image_result_, true);
        }
        image_status_.SetText(candidate.diagnostic);
        return;
    }
    if(mask_only && image_result_.IsValid()) {
        image_result_.proxy = candidate.proxy;
        image_result_.proxy_size = candidate.proxy_size;
        image_result_.exclusion_mask = pick(candidate.exclusion_mask);
        image_result_.ignored_fraction = candidate.ignored_fraction;
        image_result_.diagnostic = candidate.diagnostic;
    }
    else {
        image_result_ = pick(candidate);
        if(image_settings_.analysis == IMAGE_MANUAL_POINTS) {
            EnsureManualPoints();
            for(int i = 0; i < manual_points_.GetCount(); i++)
                ResampleManualPoint(i);
        }
        image_stale_ = false;
        image_hero_ = -1;
    }
    RefreshImageDisplay(false);
    image_canvas_->SetResult(image_result_, image_show_mask_);
    SaveSession(false);
}

void UiColorPicker::Impl::RefreshImageDisplay(bool preserve_selection)
{
    Vector<GeneratedSwatch> source = clone(image_result_.swatches);
    if(source.GetCount() > image_settings_.requested_count)
        source.SetCount(image_settings_.requested_count);
    image_display_ = ApplyImagePostProcessing(source, image_settings_.medium,
                                              image_gain_, image_saturation_,
                                              image_hero_, image_hero_gain_);
    Vector<DisplaySwatch_> display;
    for(int i = 0; i < image_display_.GetCount(); i++) {
        DisplaySwatch_& item = display.Add();
        item.value = image_display_[i].value;
        item.hero_number = i + 1;
        item.hero = i == image_hero_;
        item.gamut_mapped = image_display_[i].gamut_mapped;
    }
    image_flow_.SetItems(display, preserve_selection);
    if(image_flow_.GetSelectedIndices().IsEmpty() && !display.IsEmpty())
        image_flow_.SetSelectedIndex(0);
    hero_gain_row_->Enable(image_hero_ >= 0 && image_hero_ < image_display_.GetCount());
    hero_label_.SetText(image_hero_ >= 0 && image_hero_ < image_display_.GetCount()
                      ? Format("Hero: %d", image_hero_ + 1)
                      : "Hero: none");
    image_gain_row_->SetValue(image_gain_);
    image_saturation_row_->SetValue(image_saturation_);
    hero_gain_row_->SetValue(image_hero_gain_);
    image_canvas_->SetImage(image_);
    image_canvas_->SetResult(image_result_, image_show_mask_);
    image_canvas_->SetSeeds(image_settings_.exclusion);
    image_canvas_->SetManualPoints(manual_points_);
    SyncImageStatus();
}

void UiColorPicker::Impl::SyncImageSelection(int, const SlotValue& value)
{
    SyncFooter(value);
}

void UiColorPicker::Impl::AddImageSelection(bool all)
{
    Vector<SlotValue> values;
    if(all) {
        for(const GeneratedSwatch& item : image_display_)
            values.Add(item.value);
    }
    else
        values = image_flow_.GetSelectedValues();
    AddUserSwatches(values, true);
}

void UiColorPicker::Impl::SyncImageStatus()
{
    if(image_.IsEmpty()) {
        image_analyze_button_.SetText("Analyze");
        image_status_.SetText("No image loaded.");
        return;
    }
    image_analyze_button_.SetText(image_result_.IsValid() ? "Reanalyze" : "Analyze");
    if(image_stale_)
        image_status_.SetText("Settings changed — Reanalyze is pending; the last valid palette remains visible.");
    else if(image_result_.IsValid())
        image_status_.SetText(Format("%s  Proxy %dx%d · ignored %.1f%%",
                                     image_result_.diagnostic,
                                     image_result_.proxy_size.cx, image_result_.proxy_size.cy,
                                     image_result_.ignored_fraction * 100.0));
    else
        image_status_.SetText(Format("Loaded %dx%d. Choose settings, then Analyze.", image_.GetSize().cx, image_.GetSize().cy));
    image_analyze_button_.SetCustomStyle(UiTheme::ResolveButton(image_stale_ ? UiRole::Alert : UiRole::Accent));
}

void UiColorPicker::Impl::SendImageToGenerator()
{
    if(image_display_.IsEmpty())
        return;
    recipe_.harmony = HARMONY_CUSTOM;
    recipe_.requested_count = minmax(image_display_.GetCount(), 2, 12);
    recipe_.families.Clear();
    int base_hue = 0, base_saturation = 0, base_value = 0;
    ColorToHsv(image_display_[0].value.color, base_hue, base_saturation, base_value);
    recipe_.base_hue = base_hue;
    for(int i = 0; i < image_display_.GetCount(); i++) {
        int hue = 0, saturation = 0, value = 0;
        ColorToHsv(image_display_[i].value.color, hue, saturation, value);
        PaletteFamily& family = recipe_.families.Add();
        family.id = i;
        family.canonical_offset = NormalizeHue(hue - base_hue);
        family.custom_offset = family.canonical_offset;
        family.authored_saturation = saturation;
        family.gain = value - base_value;
        family.priority = image_display_.GetCount() - i;
        family.role = i == 0 ? "Primary" : Format("Family %d", i + 1);
    }
    recipe_.free_angles = true;
    PopulateGeneratorControls();
    RefreshGenerator(false);
    SetPageMode(PAGE_GENERATOR);
    SaveSession(false);
}

void UiColorPicker::Impl::LoadImage()
{
    FileSel selector;
    selector.Type("Image files", "*.png *.jpg *.jpeg *.bmp *.tif *.tiff");
    if(!selector.ExecuteOpen())
        return;
    Image loaded = StreamRaster::LoadFileAny(~selector);
    if(loaded.IsEmpty()) {
        Exclamation("Unable to load the selected image.");
        return;
    }
    SetGeneratorImage(loaded);
}

UiColorPicker& UiColorPicker::Impl::SetPageMode(PageMode mode)
{
    if(mode < PAGE_COLOR || mode >= PAGE_COUNT)
        mode = PAGE_COLOR;
    if(page_mode_ == mode) {
        SyncPage();
        Layout();
        return owner_;
    }
    page_mode_ = mode;
    SyncPage();
    Layout();
    SaveSession(false);
    if(owner_.WhenPageChanged)
        owner_.WhenPageChanged(page_mode_);
    return owner_;
}

UiColorPicker& UiColorPicker::Impl::SetChannelMode(ChannelMode mode)
{
    if(mode < CHANNEL_RGB_FLOAT || mode >= CHANNEL_COUNT)
        mode = CHANNEL_RGB_FLOAT;
    if(channel_mode_ == mode) {
        SyncChannelRows();
        return owner_;
    }
    channel_mode_ = mode;
    ConfigureChannelRows();
    SyncChannelRows();
    channel_drop_.SetDataSilently((int)mode);
    SaveSession(false);
    if(owner_.WhenChannelModeChanged)
        owner_.WhenChannelModeChanged(channel_mode_);
    return owner_;
}

UiColorPicker& UiColorPicker::Impl::SetSlotCount(int count)
{
    count = minmax(count, 1, 8);
    if(slot_count_ == count)
        return owner_;
    slot_count_ = count;
    active_slot_ = min(active_slot_, slot_count_ - 1);
    SyncAll();
    SaveSession(true);
    return owner_;
}

UiColorPicker& UiColorPicker::Impl::SetActiveSlot(int index)
{
    if(index < 0 || index >= slot_count_ || index == active_slot_)
        return owner_;
    FinishLiveGesture();
    active_slot_ = index;
    SyncAll();
    SaveSession(false);
    if(owner_.WhenSlotChanged)
        owner_.WhenSlotChanged(index);
    return owner_;
}

UiColorPicker& UiColorPicker::Impl::SetSlot(int index, Color color, int alpha, bool fire)
{
    if(index < 0 || index >= slots_.GetCount() || IsNull(color))
        return owner_;
    alpha = ClampByte_(alpha);
    if(slots_[index].color == color && slots_[index].alpha == alpha)
        return owner_;
    previous_[index] = slots_[index];
    slots_[index].color = color;
    slots_[index].alpha = alpha;
    if(index == active_slot_)
        SyncAll();
    else
        SyncSlots();
    if(fire) {
        PushRecent(color, alpha);
        SaveSession(true);
        if(owner_.WhenAction)
            owner_.WhenAction();
    }
    return owner_;
}

UiColorPicker& UiColorPicker::Impl::SetSlotColor(int index, Color color, bool fire)
{
    return SetSlot(index, color, index >= 0 && index < slots_.GetCount() ? slots_[index].alpha : 255, fire);
}

UiColorPicker& UiColorPicker::Impl::SetSlotAlpha(int index, int alpha, bool fire)
{
    return SetSlot(index, index >= 0 && index < slots_.GetCount() ? slots_[index].color : Black(), alpha, fire);
}

UiColorPicker& UiColorPicker::Impl::SetSlotLabel(int index, const String& label)
{
    if(index < 0 || index >= slots_.GetCount())
        return owner_;
    slots_[index].label = label;
    SyncSlots();
    SaveSession(true);
    return owner_;
}

UiColorPicker& UiColorPicker::Impl::SetAlphaEnabled(bool enabled)
{
    alpha_enabled_ = enabled;
    alpha_toggle_.SetOn(enabled);
    for(int i = 0; i < 5; i++)
        numeric_row_[i]->EnableAlpha(enabled);
    SyncAll();
    SaveSession(false);
    return owner_;
}
UiColorPicker& UiColorPicker::Impl::SetSpectrumMode(SpectrumMode mode)
{
    if(mode < SPECTRUM_HSV_RECT || mode > SPECTRUM_HSV_WHEEL)
        mode = SPECTRUM_HUE_STRIP;
    spectrum_mode_ = mode;
    spectrum_field_.Drop().SetDataSilently((int)mode);
    SyncColorControls();
    SaveSession(false);
    return owner_;
}

UiColorPicker& UiColorPicker::Impl::SetHarmonyMode(HarmonyMode mode)
{
    if(mode == HARMONY_IMAGE_EXTRACT) {
        SetPageMode(PAGE_IMAGE);
        return owner_;
    }
    if(mode < HARMONY_CUSTOM || mode > HARMONY_MONOCHROMATIC)
        mode = HARMONY_TRIAD;
    recipe_.harmony = mode;
    recipe_.free_angles = false;
    ResetGeneratorFamilies(recipe_, true);
    active_family_ = 0;
    RefreshGenerator(false);
    SaveSession(false);
    return owner_;
}

UiColorPicker& UiColorPicker::Impl::SetDistributionMode(DistributionMode mode)
{
    if(mode < DISTRIBUTION_BALANCED || mode >= DISTRIBUTION_COUNT)
        mode = DISTRIBUTION_BALANCED;
    recipe_.distribution = mode;
    RefreshGenerator(true);
    SaveSession(false);
    return owner_;
}

UiColorPicker& UiColorPicker::Impl::SetMediumMode(MediumMode mode)
{
    if(mode < MEDIUM_UI || mode >= MEDIUM_COUNT)
        mode = MEDIUM_UI;
    recipe_.medium = mode;
    RefreshGenerator(true);
    SaveSession(false);
    return owner_;
}

UiColorPicker& UiColorPicker::Impl::SetGeneratorCount(int count)
{
    recipe_.requested_count = minmax(count, 2, 12);
    RefreshGenerator(true);
    SaveSession(false);
    return owner_;
}

UiColorPicker& UiColorPicker::Impl::SetGeneratorImage(const Image& image)
{
    image_ = image;
    image_result_ = ImageAnalysisResult();
    image_display_.Clear();
    image_hero_ = -1;
    manual_points_.Clear();
    image_stale_ = !image_.IsEmpty();
    image_canvas_->SetImage(image_);
    for(int i = 0; i < 2; i++) {
        if(!image_settings_.exclusion[i].enabled || image_.IsEmpty())
            continue;
        if(!image_settings_.exclusion[i].placed)
            image_settings_.exclusion[i].position = i == 0 ? Pointf(0.33, 0.50) : Pointf(0.67, 0.50);
        image_settings_.exclusion[i].placed = true;
        image_settings_.exclusion[i].color = SampleImage_(image_, image_settings_.exclusion[i].position, 3);
        exclusion_toggle_[i].SetOn(true);
        exclusion_button_[i].SetIcon(MakeAlphaSwatchImage_(image_settings_.exclusion[i].color, 255, Size(DPI(24), DPI(18))));
    }
    image_canvas_->SetSeeds(image_settings_.exclusion);
    EnsureManualPoints();
    RefreshImageDisplay(false);
    SetPageMode(PAGE_IMAGE);
    SaveSession(false);
    return owner_;
}

UiColorPicker& UiColorPicker::Impl::ExtractGeneratorPalette(int count)
{
    image_settings_.requested_count = minmax(count, 2, 12);
    AnalyzeCurrentImage(false);
    return owner_;
}

UiColorPicker& UiColorPicker::Impl::AddUserSwatches(const Vector<SlotValue>& values, bool transactional)
{
    if(values.IsEmpty())
        return owner_;
    Vector<SlotValue> destination = clone(stash_);
    int rejected = 0;
    bool accepted = AddUniqueTransactional(destination, values, 28, false, &rejected);
    if(!accepted && transactional) {
        Exclamation(Format("The User Stash has insufficient space for this %d-colour transfer.", values.GetCount()));
        return owner_;
    }
    if(!accepted) {
        for(const SlotValue& value : values) {
            Vector<SlotValue> one;
            one.Add(value);
            if(!AddUniqueTransactional(destination, one, 28, false, nullptr))
                break;
        }
    }
    stash_ = pick(destination);
    RefreshStash();
    SaveSession(false);
    return owner_;
}

UiColorPicker& UiColorPicker::Impl::ClearUserSwatches()
{
    stash_.Clear();
    RefreshStash();
    SaveSession(false);
    return owner_;
}

UiColorPicker& UiColorPicker::Impl::ClearRecentSwatches()
{
    recent_.Clear();
    SaveSession(false);
    return owner_;
}

UiColorPicker& UiColorPicker::Impl::EnableSessionPersistence(bool enabled)
{
    session_persistence_ = enabled;
    if(enabled)
        LoadSession();
    return owner_;
}

Color UiColorPicker::Impl::GetSlotColor(int index) const
{
    return index >= 0 && index < slots_.GetCount() ? slots_[index].color : Black();
}

int UiColorPicker::Impl::GetSlotAlpha(int index) const
{
    return index >= 0 && index < slots_.GetCount() && alpha_enabled_ ? slots_[index].alpha : 255;
}

UiColorPicker::SlotValue UiColorPicker::Impl::GetSlot(int index) const
{
    if(index < 0 || index >= slots_.GetCount())
        return SlotValue();
    SlotValue value = slots_[index];
    if(!alpha_enabled_)
        value.alpha = 255;
    return value;
}

Vector<UiColorPicker::SlotValue> UiColorPicker::Impl::GetSlots() const
{
    Vector<SlotValue> values;
    for(int i = 0; i < slot_count_; i++)
        values.Add(GetSlot(i));
    return values;
}

String UiColorPicker::Impl::GetSlotLabel(int index) const
{
    return index >= 0 && index < slots_.GetCount() ? slots_[index].label : String();
}

Vector<UiColorPicker::SlotValue> UiColorPicker::Impl::GetGeneratedPalette() const
{
    Vector<SlotValue> values;
    for(const GeneratedSwatch& item : generated_)
        values.Add(item.value);
    return values;
}

Vector<UiColorPicker::SlotValue> UiColorPicker::Impl::GetImagePalette() const
{
    Vector<SlotValue> values;
    for(const GeneratedSwatch& item : image_display_)
        values.Add(item.value);
    return values;
}

bool UiColorPicker::Impl::IsEyedropperAvailable() const
{
#ifdef PLATFORM_WIN32
    return true;
#else
    return false;
#endif
}

void UiColorPicker::Impl::SyncAll()
{
    if(active_slot_ < 0 || active_slot_ >= slots_.GetCount())
        return;
    bool old_sync = syncing_;
    syncing_ = true;
    SyncPage();
    SyncSlots();
    SyncColorControls();
    SyncChannelRows();
    SyncReadouts();
    SyncFooter(GetSlot(active_slot_));
    syncing_ = old_sync;
}

void UiColorPicker::Impl::SyncPage()
{
    page_stack_.SetActivePage((int)page_mode_);
    for(int i = 0; i < PAGE_COUNT; i++)
        page_button_[i].SetChecked(i == (int)page_mode_);
    SyncTheme();
}

void UiColorPicker::Impl::SyncSlots()
{
    SlotValue active = GetSlot(active_slot_);
    current_preview_.SetIcon(MakeAlphaSwatchImage_(active.color, active.alpha, Size(DPI(34), DPI(20)), alpha_enabled_));
    current_preview_.SetIconSize(DPI(34), DPI(20));
    current_preview_.Tip(Format("Current: %s", FormatHex8(active.color, active.alpha)));
    for(int i = 0; i < 8; i++) {
        bool visible = i < slot_count_;
        slot_column_[i]->Show(visible);
        if(!visible)
            continue;
        SlotValue value = GetSlot(i);
        SlotValue previous = previous_[i];
        if(!alpha_enabled_)
            previous.alpha = 255;
        primary_slot_[i].SetIcon(MakeAlphaSwatchImage_(value.color, value.alpha, Size(DPI(31), DPI(16)), alpha_enabled_));
        primary_slot_[i].SetIconSize(DPI(31), DPI(16));
        primary_slot_[i].SetChecked(i == active_slot_);
        primary_slot_[i].Tip(Format("%s: %s", value.label, FormatHex8(value.color, value.alpha)));
        previous_slot_[i].SetIcon(MakeAlphaSwatchImage_(previous.color, previous.alpha, Size(DPI(31), DPI(5)), false));
        previous_slot_[i].SetIconSize(DPI(31), DPI(5));
        previous_slot_[i].Tip(Format("Previous %s", FormatHex8(previous.color, previous.alpha)));
    }
}

void UiColorPicker::Impl::SyncColorControls()
{
    Color color = GetSlotColor(active_slot_);
    int hue = 0, saturation = 0, value = 0;
    ColorToHsv(color, hue, saturation, value);
    if(saturation > 0 && value > 0)
        remembered_hue_ = hue;
    else
        hue = remembered_hue_;
    spectrum_field_.Drop().SetDataSilently((int)spectrum_mode_);
    channel_drop_.SetDataSilently((int)channel_mode_);
    alpha_toggle_.SetOn(alpha_enabled_);
    hue_row_->SetValue(hue);
    value_row_->SetValue(value);
    color_field_->SetState(spectrum_mode_, color, hue, value);
}

void UiColorPicker::Impl::SyncChannelRows()
{
    Color color = GetSlotColor(active_slot_);
    int alpha = slots_[active_slot_].alpha;
    switch(channel_mode_) {
    case CHANNEL_RGB_FLOAT:
        numeric_row_[0]->SetValue(color.GetR() / 255.0);
        numeric_row_[1]->SetValue(color.GetG() / 255.0);
        numeric_row_[2]->SetValue(color.GetB() / 255.0);
        numeric_row_[3]->SetValue(alpha / 255.0);
        break;
    case CHANNEL_RGB_INT:
        numeric_row_[0]->SetValue(color.GetR());
        numeric_row_[1]->SetValue(color.GetG());
        numeric_row_[2]->SetValue(color.GetB());
        numeric_row_[3]->SetValue(alpha);
        break;
    case CHANNEL_HSV: {
        int h = 0, s = 0, v = 0;
        ColorToHsv(color, h, s, v);
        if(s == 0 || v == 0)
            h = remembered_hue_;
        numeric_row_[0]->SetValue(h);
        numeric_row_[1]->SetValue(s);
        numeric_row_[2]->SetValue(v);
        numeric_row_[3]->SetValue(alpha / 2.55);
        break;
    }
    case CHANNEL_HSL: {
        int h = 0, s = 0, l = 0;
        ColorToHsl(color, h, s, l);
        numeric_row_[0]->SetValue(h);
        numeric_row_[1]->SetValue(s);
        numeric_row_[2]->SetValue(l);
        numeric_row_[3]->SetValue(alpha / 2.55);
        break;
    }
    case CHANNEL_TMI: {
        double t = 0.0, m = 0.0, intensity = 0.0;
        ColorToTmi(color, t, m, intensity);
        numeric_row_[0]->SetValue(t);
        numeric_row_[1]->SetValue(m);
        numeric_row_[2]->SetValue(intensity);
        numeric_row_[3]->SetValue(alpha / 2.55);
        break;
    }
    case CHANNEL_CMYK: {
        int c = 0, m = 0, y = 0, k = 0;
        ColorToCmyk(color, c, m, y, k);
        numeric_row_[0]->SetValue(c);
        numeric_row_[1]->SetValue(m);
        numeric_row_[2]->SetValue(y);
        numeric_row_[3]->SetValue(k);
        numeric_row_[4]->SetValue(alpha / 2.55);
        break;
    }
    case CHANNEL_LAB:
    default: {
        double l = 0.0, a = 0.0, b = 0.0;
        ColorToLab(color, l, a, b);
        numeric_row_[0]->SetValue(l);
        numeric_row_[1]->SetValue(a);
        numeric_row_[2]->SetValue(b);
        numeric_row_[3]->SetValue(alpha / 2.55);
        break;
    }
    }
}

void UiColorPicker::Impl::SyncReadouts()
{
    Color color = GetSlotColor(active_slot_);
    int alpha = GetSlotAlpha(active_slot_);
    int h = 0, s = 0, v = 0;
    int hh = 0, hs = 0, l = 0;
    int c = 0, m = 0, y = 0, k = 0;
    ColorToHsv(color, h, s, v);
    if(s == 0 || v == 0)
        h = remembered_hue_;
    ColorToHsl(color, hh, hs, l);
    ColorToCmyk(color, c, m, y, k);
    readout_[0]->SetTitle(alpha_enabled_ ? "HSV-A" : "HSV");
    readout_[0]->SetValue(alpha_enabled_ ? Format("hsv(%d, %d%%, %d%%, %.4f)", h, s, v, alpha / 255.0)
                                         : Format("hsv(%d, %d%%, %d%%)", h, s, v));
    readout_[1]->SetValue(alpha_enabled_ ? FormatHex8(color, alpha) : FormatHex(color));
    readout_[2]->SetTitle(alpha_enabled_ ? "HLS-A" : "HLS");
    readout_[2]->SetValue(alpha_enabled_ ? Format("hsl(%d, %d%%, %d%%, %.4f)", hh, hs, l, alpha / 255.0)
                                         : Format("hsl(%d, %d%%, %d%%)", hh, hs, l));
    readout_[3]->SetValue(alpha_enabled_ ? Format("%.4f, %.4f, %.4f, %.4f",
                                                 color.GetR() / 255.0, color.GetG() / 255.0,
                                                 color.GetB() / 255.0, alpha / 255.0)
                                         : Format("%.4f, %.4f, %.4f", color.GetR() / 255.0,
                                                  color.GetG() / 255.0, color.GetB() / 255.0));
    readout_[4]->SetTitle(alpha_enabled_ ? "CMYK-A" : "CMYK");
    readout_[4]->SetValue(alpha_enabled_ ? Format("cmyk(%d%%, %d%%, %d%%, %d%%, %.4f)", c, m, y, k, alpha / 255.0)
                                         : Format("cmyk(%d%%, %d%%, %d%%, %d%%)", c, m, y, k));
    readout_[5]->SetValue(alpha_enabled_ ? Format("%d, %d, %d, %d", color.GetR(), color.GetG(), color.GetB(), alpha)
                                         : Format("%d, %d, %d", color.GetR(), color.GetG(), color.GetB()));
}

void UiColorPicker::Impl::SyncFooter(const SlotValue& value)
{
    if(IsNull(value.color))
        return;
    footer_value_ = value;
    int h = 0, s = 0, v = 0;
    ColorToHsv(value.color, h, s, v);
    footer_hex_.SetTextUtf8(alpha_enabled_ ? FormatHex8(value.color, value.alpha) : FormatHex(value.color));
    footer_detail_.SetTextUtf8(Format("RGB %d, %d, %d · HSV %d, %d, %d",
                                      value.color.GetR(), value.color.GetG(), value.color.GetB(), h, s, v));
    footer_hex_.ClearDirty();
    footer_detail_.ClearDirty();
}

void UiColorPicker::Impl::PushRecent(Color color, int alpha)
{
    for(int i = 0; i < recent_.GetCount(); i++) {
        if(recent_[i].color == color && recent_[i].alpha == alpha) {
            recent_.Remove(i);
            break;
        }
    }
    SlotValue value;
    value.color = color;
    value.alpha = ClampByte_(alpha);
    value.label = "Recent";
    recent_.Insert(0, value);
    while(recent_.GetCount() > 12)
        recent_.Remove(recent_.GetCount() - 1);
}

void UiColorPicker::Impl::RefreshStash()
{
    Vector<DisplaySwatch_> display;
    display.SetCount(28);
    for(int i = 0; i < 28; i++) {
        if(i < stash_.GetCount())
            display[i].value = stash_[i];
        else {
            display[i].value.color = Null;
            display[i].value.alpha = 255;
            display[i].value.label = "Empty";
        }
    }
    stash_flow_.SetItems(display, true);
}

void UiColorPicker::Impl::HandleStashDrop(int target, const Vector<SlotValue>& values)
{
    if(values.IsEmpty())
        return;
    if(values.GetCount() == 1 && target >= 0 && target < stash_.GetCount()) {
        const SlotValue& source = values[0];
        SlotValue& destination = stash_[target];
        if(stash_drop_mode_ == STASH_REPLACE)
            destination = source;
        else {
            auto combine = [&](int a, int b) {
                switch(stash_drop_mode_) {
                case STASH_ADD: return ClampByte_(a + b);
                case STASH_SUBTRACT: return ClampByte_(a - b);
                case STASH_MULTIPLY: return ClampByte_(a * b / 255);
                case STASH_MIX: return (a + b + 1) / 2;
                default: return b;
                }
            };
            destination.color = Color(combine(destination.color.GetR(), source.color.GetR()),
                                      combine(destination.color.GetG(), source.color.GetG()),
                                      combine(destination.color.GetB(), source.color.GetB()));
            destination.alpha = source.alpha;
        }
        RefreshStash();
        stash_flow_.SetSelectedIndex(target);
        SyncFooter(stash_[target]);
        SaveSession(false);
        return;
    }
    AddUserSwatches(values, true);
}

void UiColorPicker::Impl::AddActiveSelection()
{
    switch(page_mode_) {
    case PAGE_COLOR: {
        Vector<SlotValue> values;
        values.Add(slots_[active_slot_]);
        AddUserSwatches(values, true);
        break;
    }
    case PAGE_PALETTES:
        AddPaletteSelection(false);
        break;
    case PAGE_GENERATOR:
        AddGeneratorSelection(false);
        break;
    case PAGE_IMAGE:
        AddImageSelection(false);
        break;
    default:
        break;
    }
}

void UiColorPicker::Impl::CommitColor(Color color, bool final_commit)
{
    if(!IsNull(color))
        CommitSlotValue(active_slot_, color, slots_[active_slot_].alpha, final_commit);
}

void UiColorPicker::Impl::CommitAlpha(int alpha, bool final_commit)
{
    CommitSlotValue(active_slot_, slots_[active_slot_].color, alpha, final_commit);
}

void UiColorPicker::Impl::CommitSlotValue(int slot, Color color, int alpha, bool final_commit)
{
    if(slot < 0 || slot >= slots_.GetCount() || IsNull(color))
        return;
    alpha = ClampByte_(alpha);
    bool changed = slots_[slot].color != color || slots_[slot].alpha != alpha;
    if(!changed && !live_gesture_)
        return;
    if(!live_gesture_ || live_slot_ != slot) {
        if(live_gesture_)
            FinishLiveGesture();
        live_origin_ = slots_[slot];
        live_slot_ = slot;
        live_gesture_ = true;
    }
    slots_[slot].color = color;
    slots_[slot].alpha = alpha;
    if(slot == active_slot_)
        SyncAll();
    else
        SyncSlots();
    if(final_commit) {
        previous_[slot] = live_origin_;
        live_gesture_ = false;
        live_slot_ = -1;
        PushRecent(color, alpha);
        SaveSession(true);
        if(owner_.WhenAction)
            owner_.WhenAction();
    }
    else {
        dword now = msecs();
        if(last_live_callback_ == 0 || now - last_live_callback_ >= 16) {
            last_live_callback_ = now;
            if(owner_.WhenChanging)
                owner_.WhenChanging();
        }
    }
}

void UiColorPicker::Impl::FinishLiveGesture()
{
    if(!live_gesture_ || live_slot_ < 0 || live_slot_ >= slots_.GetCount())
        return;
    int slot = live_slot_;
    previous_[slot] = live_origin_;
    live_gesture_ = false;
    live_slot_ = -1;
    PushRecent(slots_[slot].color, slots_[slot].alpha);
    SaveSession(true);
    SyncSlots();
    if(owner_.WhenAction)
        owner_.WhenAction();
}

bool UiColorPicker::Impl::ApplyColorText(const String& text, bool final_commit)
{
    Color color;
    int alpha = 255;
    bool has_alpha = false;
    if(!UiColorPickerPaletteLab::ParseColorText(text, color, alpha, has_alpha))
        return false;
    CommitSlotValue(active_slot_, color, has_alpha ? alpha : slots_[active_slot_].alpha, final_commit);
    return true;
}

void UiColorPicker::Impl::HandleChannelValue(int row, double value, bool final_commit)
{
    if(syncing_ || row < 0 || row >= channel_row_count_)
        return;
    Color color = GetSlotColor(active_slot_);
    int alpha = slots_[active_slot_].alpha;
    switch(channel_mode_) {
    case CHANNEL_RGB_FLOAT: {
        double component[4] = { color.GetR() / 255.0, color.GetG() / 255.0, color.GetB() / 255.0, alpha / 255.0 };
        component[row] = value;
        color = Color(ClampByte_(int(component[0] * 255.0 + 0.5)),
                      ClampByte_(int(component[1] * 255.0 + 0.5)),
                      ClampByte_(int(component[2] * 255.0 + 0.5)));
        alpha = ClampByte_(int(component[3] * 255.0 + 0.5));
        break;
    }
    case CHANNEL_RGB_INT: {
        int component[4] = { color.GetR(), color.GetG(), color.GetB(), alpha };
        component[row] = ClampByte_(int(value + 0.5));
        color = Color(component[0], component[1], component[2]);
        alpha = component[3];
        break;
    }
    case CHANNEL_HSV: {
        int h = 0, s = 0, v = 0;
        ColorToHsv(color, h, s, v);
        double component[4] = { (double)h, (double)s, (double)v, alpha / 2.55 };
        component[row] = value;
        remembered_hue_ = NormalizeHue(int(component[0] + 0.5));
        color = HsvToColor(component[0], component[1], component[2]);
        alpha = ClampByte_(int(component[3] * 2.55 + 0.5));
        break;
    }
    case CHANNEL_HSL: {
        int h = 0, s = 0, l = 0;
        ColorToHsl(color, h, s, l);
        double component[4] = { (double)h, (double)s, (double)l, alpha / 2.55 };
        component[row] = value;
        color = HslToColor(component[0], component[1], component[2]);
        alpha = ClampByte_(int(component[3] * 2.55 + 0.5));
        break;
    }
    case CHANNEL_TMI: {
        double t = 0.0, m = 0.0, intensity = 0.0;
        ColorToTmi(color, t, m, intensity);
        double component[4] = { t, m, intensity, alpha / 2.55 };
        component[row] = value;
        color = TmiToColor(component[0], component[1], component[2]);
        alpha = ClampByte_(int(component[3] * 2.55 + 0.5));
        break;
    }
    case CHANNEL_CMYK: {
        int c = 0, m = 0, y = 0, k = 0;
        ColorToCmyk(color, c, m, y, k);
        double component[5] = { (double)c, (double)m, (double)y, (double)k, alpha / 2.55 };
        component[row] = value;
        color = CmykToColor(component[0], component[1], component[2], component[3]);
        alpha = ClampByte_(int(component[4] * 2.55 + 0.5));
        break;
    }
    case CHANNEL_LAB:
    default: {
        double l = 0.0, a = 0.0, b = 0.0;
        ColorToLab(color, l, a, b);
        double component[4] = { l, a, b, alpha / 2.55 };
        component[row] = value;
        color = LabToColor(component[0], component[1], component[2]);
        alpha = ClampByte_(int(component[3] * 2.55 + 0.5));
        break;
    }
    }
    if(!alpha_enabled_)
        alpha = slots_[active_slot_].alpha;
    CommitSlotValue(active_slot_, color, alpha, final_commit);
}

void UiColorPicker::Impl::Accept()
{
    FinishLiveGesture();
    opening_ = clone(slots_);
    SaveSession(true);
    if(owner_.WhenAccept)
        owner_.WhenAccept();
}

void UiColorPicker::Impl::Cancel()
{
    if(eyedropper_active_)
        StopEyedropper(false);
    live_gesture_ = false;
    live_slot_ = -1;
    slots_ = clone(opening_);
    SyncAll();
    SaveSession(false);
    if(owner_.WhenCancel)
        owner_.WhenCancel();
}

UiColorPicker& UiColorPicker::Impl::BeginEyedropper()
{
    if(eyedropper_active_ || !IsEyedropperAvailable())
        return owner_;
    FinishLiveGesture();
    live_origin_ = slots_[active_slot_];
    live_slot_ = active_slot_;
    eyedropper_active_ = true;
    eyedropper_dragging_ = false;
    eyedropper_button_.SetChecked(true);
    eyedropper_button_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Accent));
    owner_.SetFocus();
    owner_.SetCapture();
    return owner_;
}

void UiColorPicker::Impl::StopEyedropper(bool commit)
{
    if(!eyedropper_active_)
        return;
    eyedropper_active_ = false;
    eyedropper_dragging_ = false;
    if(commit)
        FinishLiveGesture();
    else {
        if(live_slot_ >= 0 && live_slot_ < slots_.GetCount())
            slots_[live_slot_] = live_origin_;
        live_gesture_ = false;
        live_slot_ = -1;
        SyncAll();
    }
    if(owner_.HasCapture())
        owner_.ReleaseCapture();
    eyedropper_button_.SetChecked(false);
    eyedropper_button_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Subtle));
}

void UiColorPicker::Impl::SampleEyedropper(bool final_commit)
{
    if(!eyedropper_active_)
        return;
    Color sample;
    if(!ReadScreenColor_(sample))
        return;
    if(!live_gesture_) {
        live_origin_ = slots_[active_slot_];
        live_slot_ = active_slot_;
        live_gesture_ = true;
    }
    CommitSlotValue(active_slot_, sample, slots_[active_slot_].alpha, false);
    if(final_commit)
        StopEyedropper(true);
}

bool UiColorPicker::Impl::Key(dword key, int)
{
    if(eyedropper_active_ && key == K_ESCAPE) {
        StopEyedropper(false);
        return true;
    }
    return false;
}

void UiColorPicker::Impl::CancelMode()
{
    if(eyedropper_active_)
        StopEyedropper(false);
}

void UiColorPicker::Impl::LoadSession()
{
    if(!session_persistence_)
        return;
    SharedSession_& session = SharedSessionState_();
    if(!session.initialized) {
        SaveSession(true);
        return;
    }
    if(!session.slots.IsEmpty()) {
        slot_count_ = min(8, session.slots.GetCount());
        for(int i = 0; i < slot_count_; i++)
            slots_[i] = session.slots[i];
    }
    if(session.previous.GetCount() >= slot_count_)
        for(int i = 0; i < slot_count_; i++)
            previous_[i] = session.previous[i];
    else
        previous_ = clone(slots_);
    recent_ = clone(session.recent);
    stash_.Clear();
    for(const SlotValue& value : session.stash)
        if(stash_.GetCount() < 28 && !IsNull(value.color))
            stash_.Add(value);
    active_slot_ = minmax(session.active_slot, 0, slot_count_ - 1);
    alpha_enabled_ = session.alpha_enabled;
    stash_drop_mode_ = (StashDropMode)minmax(session.stash_drop_mode, (int)STASH_REPLACE, (int)STASH_MULTIPLY);
    stash_mode_drop_.SetDataSilently((int)stash_drop_mode_);
    page_mode_ = (PageMode)minmax(session.page, (int)PAGE_COLOR, (int)PAGE_IMAGE);
    spectrum_mode_ = (SpectrumMode)minmax(session.spectrum, (int)SPECTRUM_HSV_RECT, (int)SPECTRUM_HSV_WHEEL);
    channel_mode_ = (ChannelMode)minmax(session.channel, (int)CHANNEL_RGB_FLOAT, (int)CHANNEL_COUNT - 1);
    palette_index_ = max(0, session.palette_index);
    recipe_ = clone(session.recipe);
    recipe_.requested_count = minmax(recipe_.requested_count, 2, 12);
    recipe_.global_gain = minmax(recipe_.global_gain, -50, 50);
    recipe_.global_saturation = minmax(recipe_.global_saturation, 0, 150);
    if(recipe_.harmony == HARMONY_IMAGE_EXTRACT)
        recipe_.harmony = HARMONY_TRIAD;
    if(recipe_.families.IsEmpty())
        ResetGeneratorFamilies(recipe_, false);
    image_ = session.image;
    image_settings_ = session.image_settings;
    image_settings_.requested_count = minmax(image_settings_.requested_count, 2, 12);
    image_gain_ = minmax(session.image_gain, -50, 50);
    image_saturation_ = minmax(session.image_saturation, 0, 150);
    image_hero_ = session.image_hero;
    image_hero_gain_ = minmax(session.image_hero_gain, -50, 50);
    image_stale_ = !image_.IsEmpty();
    image_canvas_->SetImage(image_);
    ConfigureChannelRows();
}

void UiColorPicker::Impl::SaveSession(bool include_slots)
{
    if(!session_persistence_)
        return;
    SharedSession_& session = SharedSessionState_();
    session.initialized = true;
    if(include_slots) {
        session.slots.Clear();
        session.previous.Clear();
        for(int i = 0; i < slot_count_; i++) {
            session.slots.Add(slots_[i]);
            session.previous.Add(previous_[i]);
        }
    }
    session.recent = clone(recent_);
    session.stash = clone(stash_);
    session.active_slot = active_slot_;
    session.alpha_enabled = alpha_enabled_;
    session.page = page_mode_;
    session.spectrum = spectrum_mode_;
    session.channel = channel_mode_;
    session.palette_index = palette_index_;
    session.recipe = clone(recipe_);
    session.image = image_;
    session.image_settings = image_settings_;
    session.image_gain = image_gain_;
    session.image_saturation = image_saturation_;
    session.image_hero = image_hero_;
    session.image_hero_gain = image_hero_gain_;
    session.stash_drop_mode = stash_drop_mode_;
}

void UiColorPicker::Impl::SavePaletteJson()
{
    if(stash_.IsEmpty()) {
        Exclamation("There is no User Stash palette to export.");
        return;
    }
    FileSel selector;
    selector.Type("Palette JSON", "*.json").DefaultExt("json").DefaultName("palette-lab.json");
    if(!selector.ExecuteSaveAs())
        return;
    ValueMap root;
    root.Set("format", "upp-uicolor-palette");
    root.Set("version", 2);
    ValueArray colors;
    for(const SlotValue& value : stash_) {
        ValueMap item;
        item.Set("hex", FormatHex8(value.color, value.alpha));
        item.Set("label", value.label);
        colors.Add(item);
    }
    root.Set("colors", colors);
    if(!SaveFile(~selector, AsJSON(root, true)))
        Exclamation("Unable to save the palette file.");
}

void UiColorPicker::Impl::LoadPaletteJson()
{
    FileSel selector;
    selector.Type("Palette JSON", "*.json");
    if(!selector.ExecuteOpen())
        return;
    Value root_value = ParseJSON(LoadFile(~selector));
    if(root_value.IsError() || !IsValueMap(root_value)) {
        Exclamation("The selected file is not valid palette JSON.");
        return;
    }
    ValueMap root = root_value;
    int colors_index = root.Find("colors");
    if(colors_index < 0 || !IsValueArray(root.GetValue(colors_index))) {
        Exclamation("The palette file contains no colour array.");
        return;
    }
    ValueArray colors = root.GetValue(colors_index);
    Vector<SlotValue> values;
    for(int i = 0; i < colors.GetCount() && values.GetCount() < 28; i++) {
        if(!IsValueMap(colors[i]))
            continue;
        ValueMap item = colors[i];
        int hex_index = item.Find("hex");
        if(hex_index < 0)
            continue;
        Color color;
        int alpha = 255;
        bool has_alpha = false;
        if(!UiColorPickerPaletteLab::ParseColorText(AsString(item.GetValue(hex_index)), color, alpha, has_alpha))
            continue;
        SlotValue& value = values.Add();
        value.color = color;
        value.alpha = has_alpha ? alpha : 255;
        int label_index = item.Find("label");
        value.label = label_index >= 0 ? AsString(item.GetValue(label_index)) : String();
    }
    if(values.IsEmpty()) {
        Exclamation("The palette file contains no recognised colours.");
        return;
    }
    int decision = stash_.IsEmpty() ? 1
                 : PromptYesNoCancel("Replace the current User Stash?\n\nYes replaces it. No adds unique colours to available cells.");
    if(decision < 0)
        return;
    if(decision > 0)
        stash_.Clear();
    AddUserSwatches(values, true);
}

void UiColorPicker::Impl::SyncTheme()
{
    for(int i = 0; i < PAGE_COUNT; i++) {
        bool active = i == page_mode_;
        UiButton::Style style = UiTheme::ResolveButton(UiRole::Subtle);
        style.metrics.face_enabled = false;
        style.metrics.frame_enabled = false;
        style.metrics.frame_width = 0;
        style.metrics.focus_enabled = false;
        style.underline = active;
        style.underline_width = DPI(3);
        style.underline_offset = DPI(3);
        page_button_[i].SetCustomStyle(style);
    }
    accept_button_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Accent));
    cancel_button_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Alert));
    palette_add_selected_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Accent));
    palette_add_all_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Subtle));
    generator_add_selected_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Accent));
    generator_add_all_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Subtle));
    image_load_button_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Subtle));
    image_send_generator_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Subtle));
    hero_set_button_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Subtle));
    family_lock_button_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Subtle));
    stash_add_selected_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Accent));
    stash_export_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Subtle));
    stash_import_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Subtle));
    stash_clear_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Subtle));
    clear_exclusion_button_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Subtle));
    eyedropper_button_.SetCustomStyle(UiTheme::ResolveToolButton(eyedropper_active_ ? UiRole::Accent : UiRole::Subtle));
    UiToolButton::Style swatch_style = UiTheme::ResolveToolButton(UiRole::Subtle);
    swatch_style.metrics.content_margin = Rect(0, 0, 0, 0);
    swatch_style.metrics.radius = DPI(3);
    current_preview_.SetCustomStyle(swatch_style);
    for(int i = 0; i < 8; i++) {
        primary_slot_[i].SetCustomStyle(swatch_style);
        previous_slot_[i].SetCustomStyle(swatch_style);
    }
    for(int i = 0; i < 2; i++)
        exclusion_button_[i].SetCustomStyle(swatch_style);
    SyncImageStatus();
}

void UiColorPicker::Impl::Layout()
{
    {
        UiBoxLayout::PauseScope pause(root_, false);
        root_.ItemAt(0).Fixed(owner_.GetStyle().navigation_height);
        root_.ItemAt(2).Fixed(max(owner_.GetStyle().stash_height, DPI(94)));
        root_.ItemAt(3).Fixed(owner_.GetStyle().footer_height);
    }
    Rect body = UiStyledInnerRect(owner_.GetSize(), owner_.GetStyle().metrics, owner_.GetStyle().skin);
    root_.SetRect(body);
    root_.Layout();
    navigation_.Layout();
    page_stack_.Layout();
    stash_section_.Layout();
    stash_header_.Layout();
    stash_flow_.Layout();
    footer_.Layout();

    if(page_mode_ == PAGE_COLOR) {
        color_page_.Layout();
        color_columns_.Layout();
        color_left_.Layout();
        color_right_.Layout();
        color_mode_row_.Layout();
        channel_rows_.Layout();
        readout_stack_.Layout();
        for(int i = 0; i < 3; i++)
            readout_pair_[i]->Layout();
    }
    else if(page_mode_ == PAGE_PALETTES) {
        palettes_page_.Layout();
        palettes_top_.Layout();
        palettes_body_.Layout();
        palette_actions_.Layout();
        palette_scroll_.Layout();
        Rect viewport = palette_scroll_.GetViewportRect();
        int width = max(DPI(120), viewport.GetWidth());
        int height = max(viewport.GetHeight(), palette_flow_.MeasureHeightForWidth(width));
        palette_flow_.SetRect(0, 0, width, height);
        palette_flow_.Layout();
        palette_scroll_.Layout();
    }
    else if(page_mode_ == PAGE_GENERATOR) {
        generator_page_.Layout();
        generator_top_.Layout();
        generator_body_.Layout();
        generator_right_.Layout();
        generator_flow_.Layout();
        free_angles_row_.Layout();
        generator_actions_.Layout();
    }
    else {
        image_page_.Layout();
        image_top_.Layout();
        image_body_.Layout();
        image_left_.Layout();
        image_navigation_.Layout();
        image_right_.Layout();
        image_flow_.Layout();
        hero_row_.Layout();
        exclusion_row_.Layout();
        image_actions_.Layout();
    }
}

const UiColorPicker::Style& UiColorPicker::StyleDefault()
{
    static Style style;
    ONCELOCK {
        style.metrics.face_enabled = true;
        style.metrics.frame_enabled = true;
        style.metrics.frame_width = DPI(1);
        style.metrics.radius = DPI(5);
        style.metrics.content_margin = Rect(DPI(2), DPI(2), DPI(2), DPI(2));
        style.metrics.focus_enabled = false;
        for(int i = 0; i < 4; i++) {
            style.palette.face[i] = UiFill::Solid(SColorPaper());
            style.palette.frame[i] = SColorShadow();
            style.palette.ink[i] = SColorText();
            style.palette.icon[i] = SColorText();
        }
    }
    return style;
}

UiColorPicker::UiColorPicker()
{
    impl_.Create(*this);
}

UiColorPicker::~UiColorPicker()
{
}

UiColorPicker& UiColorPicker::SetCustomStyle(const Style& style)
{
    style_ = style;
    has_custom_style_ = true;
    OnStyleChanged();
    return *this;
}

UiColorPicker& UiColorPicker::ClearCustomStyle()
{
    has_custom_style_ = false;
    InvalidateStyleCache();
    OnStyleChanged();
    return *this;
}

void UiColorPicker::OnStyleChanged()
{
    InvalidateStyleCache();
    if(impl_) {
        impl_->SyncTheme();
        impl_->Layout();
    }
    RefreshLayout();
    Refresh();
}

UiColorPicker::Style& UiColorPicker::StyleEdit()
{
    if(!has_custom_style_) {
        style_ = GetEffectiveStyle();
        has_custom_style_ = true;
    }
    InvalidateStyleCache();
    return style_;
}

void UiColorPicker::InvalidateStyleCache()
{
    theme_revision_ = 0;
}

void UiColorPicker::SyncThemeStyle()
{
    if(has_custom_style_)
        return;
    uint64 revision = UiTheme::GetRevision();
    if(theme_revision_ == revision && theme_revision_ != 0)
        return;
    themed_style_ = StyleDefault();
    const bool dark = UiThemeDetail::ResolveEffectiveMode(UiTheme::GetContext().mode) == UiThemeMode::Dark;
    Color face = dark ? Color(16, 18, 22) : Color(250, 252, 255);
    Color frame = dark ? Color(48, 52, 60) : Color(202, 210, 222);
    Color ink = dark ? Color(228, 232, 238) : Color(27, 33, 42);
    for(int i = 0; i < 4; i++) {
        themed_style_.palette.face[i] = UiFill::Solid(face);
        themed_style_.palette.frame[i] = frame;
        themed_style_.palette.ink[i] = ink;
        themed_style_.palette.icon[i] = ink;
    }
    theme_revision_ = revision;
}

const UiColorPicker::Style& UiColorPicker::GetEffectiveStyle() const
{
    if(has_custom_style_)
        return style_;
    const_cast<UiColorPicker *>(this)->SyncThemeStyle();
    return themed_style_;
}

UiColorPicker& UiColorPicker::SetPageMode(PageMode mode) { return impl_->SetPageMode(mode); }
UiColorPicker::PageMode UiColorPicker::GetPageMode() const { return impl_->GetPageMode(); }
UiColorPicker& UiColorPicker::SetChannelMode(ChannelMode mode) { return impl_->SetChannelMode(mode); }
UiColorPicker::ChannelMode UiColorPicker::GetChannelMode() const { return impl_->GetChannelMode(); }
UiColorPicker& UiColorPicker::SetSlotCount(int count) { return impl_->SetSlotCount(count); }
int UiColorPicker::GetSlotCount() const { return impl_->GetSlotCount(); }
UiColorPicker& UiColorPicker::SetActiveSlot(int index) { return impl_->SetActiveSlot(index); }
int UiColorPicker::GetActiveSlot() const { return impl_->GetActiveSlot(); }
UiColorPicker& UiColorPicker::SetSlotColor(int index, Color color, bool fire) { return impl_->SetSlotColor(index, color, fire); }
Color UiColorPicker::GetSlotColor(int index) const { return impl_->GetSlotColor(index); }
UiColorPicker& UiColorPicker::SetSlotAlpha(int index, int alpha, bool fire) { return impl_->SetSlotAlpha(index, alpha, fire); }
int UiColorPicker::GetSlotAlpha(int index) const { return impl_->GetSlotAlpha(index); }
UiColorPicker& UiColorPicker::SetSlot(int index, Color color, int alpha, bool fire) { return impl_->SetSlot(index, color, alpha, fire); }
UiColorPicker::SlotValue UiColorPicker::GetSlot(int index) const { return impl_->GetSlot(index); }
Vector<UiColorPicker::SlotValue> UiColorPicker::GetSlots() const { return impl_->GetSlots(); }
UiColorPicker& UiColorPicker::SetColor(Color color, bool fire) { return impl_->SetSlotColor(impl_->GetActiveSlot(), color, fire); }
Color UiColorPicker::GetColor() const { return impl_->GetSlotColor(impl_->GetActiveSlot()); }
UiColorPicker& UiColorPicker::SetAlpha(int alpha, bool fire) { return impl_->SetSlotAlpha(impl_->GetActiveSlot(), alpha, fire); }
int UiColorPicker::GetAlpha() const { return impl_->GetSlotAlpha(impl_->GetActiveSlot()); }
UiColorPicker& UiColorPicker::SetSlotLabel(int index, const String& label) { return impl_->SetSlotLabel(index, label); }
String UiColorPicker::GetSlotLabel(int index) const { return impl_->GetSlotLabel(index); }
UiColorPicker& UiColorPicker::SetAlphaEnabled(bool enabled) { return impl_->SetAlphaEnabled(enabled); }
bool UiColorPicker::IsAlphaEnabled() const { return impl_->IsAlphaEnabled(); }
UiColorPicker& UiColorPicker::SetSpectrumMode(SpectrumMode mode) { return impl_->SetSpectrumMode(mode); }
UiColorPicker::SpectrumMode UiColorPicker::GetSpectrumMode() const { return impl_->GetSpectrumMode(); }
UiColorPicker& UiColorPicker::SetHarmonyMode(HarmonyMode mode) { return impl_->SetHarmonyMode(mode); }
UiColorPicker::HarmonyMode UiColorPicker::GetHarmonyMode() const { return impl_->GetHarmonyMode(); }
UiColorPicker& UiColorPicker::SetDistributionMode(DistributionMode mode) { return impl_->SetDistributionMode(mode); }
UiColorPicker::DistributionMode UiColorPicker::GetDistributionMode() const { return impl_->GetDistributionMode(); }
UiColorPicker& UiColorPicker::SetMediumMode(MediumMode mode) { return impl_->SetMediumMode(mode); }
UiColorPicker::MediumMode UiColorPicker::GetMediumMode() const { return impl_->GetMediumMode(); }
UiColorPicker& UiColorPicker::SetGeneratorCount(int count) { return impl_->SetGeneratorCount(count); }
int UiColorPicker::GetGeneratorCount() const { return impl_->GetGeneratorCount(); }
Vector<UiColorPicker::SlotValue> UiColorPicker::GetGeneratedPalette() const { return impl_->GetGeneratedPalette(); }
UiColorPicker& UiColorPicker::SetGeneratorImage(const Image& image) { return impl_->SetGeneratorImage(image); }
const Image& UiColorPicker::GetGeneratorImage() const { return impl_->GetGeneratorImage(); }
UiColorPicker& UiColorPicker::ExtractGeneratorPalette(int count) { return impl_->ExtractGeneratorPalette(count); }
Vector<UiColorPicker::SlotValue> UiColorPicker::GetImagePalette() const { return impl_->GetImagePalette(); }
UiColorPicker& UiColorPicker::AddUserSwatch(Color color) { return AddUserSwatch(color, GetAlpha()); }
UiColorPicker& UiColorPicker::AddUserSwatch(Color color, int alpha)
{
    SlotValue value;
    value.color = color;
    value.alpha = alpha;
    Vector<SlotValue> values;
    values.Add(value);
    return impl_->AddUserSwatches(values, true);
}
UiColorPicker& UiColorPicker::AddUserSwatches(const Vector<SlotValue>& values, bool transactional) { return impl_->AddUserSwatches(values, transactional); }
UiColorPicker& UiColorPicker::ClearUserSwatches() { return impl_->ClearUserSwatches(); }
UiColorPicker& UiColorPicker::ClearRecentSwatches() { return impl_->ClearRecentSwatches(); }
int UiColorPicker::GetUserSwatchCount() const { return impl_->GetUserSwatchCount(); }
int UiColorPicker::GetRecentSwatchCount() const { return impl_->GetRecentSwatchCount(); }
Vector<UiColorPicker::SlotValue> UiColorPicker::GetUserSwatches() const { return impl_->GetUserSwatches(); }
UiColorPicker& UiColorPicker::EnableSessionPersistence(bool enabled) { return impl_->EnableSessionPersistence(enabled); }
bool UiColorPicker::IsSessionPersistenceEnabled() const { return impl_->IsSessionPersistenceEnabled(); }

void UiColorPicker::ClearSharedSession()
{
    SharedSessionState_() = SharedSession_();
}

bool UiColorPicker::IsScreenEyedropperAvailable() const { return impl_->IsEyedropperAvailable(); }
UiColorPicker& UiColorPicker::BeginScreenEyedropper() { return impl_->BeginEyedropper(); }
String UiColorPicker::FormatActiveHex() const { return FormatHex(GetColor()); }
String UiColorPicker::FormatActiveHex8() const { return FormatHex8(GetColor(), GetAlpha()); }
String UiColorPicker::FormatSlotHex8(int index) const
{
    SlotValue value = GetSlot(index);
    return index >= 0 && index < GetSlotCount() ? FormatHex8(value.color, value.alpha) : String();
}
String UiColorPicker::FormatActiveRgb8() const { Color c = GetColor(); return Format("%d, %d, %d", c.GetR(), c.GetG(), c.GetB()); }
String UiColorPicker::FormatActiveRgbUnit() const { Color c = GetColor(); return Format("%.4f, %.4f, %.4f", c.GetR() / 255.0, c.GetG() / 255.0, c.GetB() / 255.0); }
String UiColorPicker::FormatActiveHsv() const { int h = 0, s = 0, v = 0; ColorToHsv(GetColor(), h, s, v); return Format("%d, %d, %d", h, s, v); }
String UiColorPicker::FormatActiveAlpha() const { return AsString(GetAlpha()); }

bool UiColorPicker::ParseColorText(const String& text, Color& color, int& alpha)
{
    bool has_alpha = false;
    if(!UiColorPickerPaletteLab::ParseColorText(text, color, alpha, has_alpha))
        return false;
    if(!has_alpha)
        alpha = 255;
    return true;
}

void UiColorPicker::Paint(Draw& draw)
{
    const Style& style = GetEffectiveStyle();
    Rect rect(Point(0, 0), GetSize());
    StyledState state = IsEnabled() && IsShowEnabled() ? ST_NORMAL : ST_DISABLED;
    UiPaintStyledBackground(draw, rect, style.palette, style.metrics, style.skin, state, false);
    UiPaintStyledForeground(draw, rect, style.palette, style.metrics, style.skin, state, false);
}

void UiColorPicker::Layout()
{
    if(impl_)
        impl_->Layout();
}

Size UiColorPicker::GetMinSize() const
{
    // Responsive RC2 minimum: compact page rows fit the Designer's normal
    // 720x520 outer dialog while the two-row User Stash remains fully visible.
    return Size(DPI(640), DPI(460));
}

void UiColorPicker::SetData(const Value& value)
{
    if(value.Is<Color>()) {
        SetColor((Color)value, true);
        return;
    }
    if(value.Is<ValueArray>()) {
        ValueArray array = value;
        SetSlotCount(min(8, array.GetCount()));
        for(int i = 0; i < array.GetCount() && i < 8; i++)
            if(array[i].Is<Color>())
                SetSlotColor(i, (Color)array[i], false);
    }
}

Value UiColorPicker::GetData() const
{
    if(GetSlotCount() == 1)
        return GetColor();
    ValueArray array;
    for(int i = 0; i < GetSlotCount(); i++)
        array.Add(GetSlotColor(i));
    return array;
}

void UiColorPicker::LeftDown(Point, dword)
{
    if(impl_ && impl_->IsEyedropperAvailable())
        impl_->SampleEyedropper(false);
}

void UiColorPicker::LeftUp(Point, dword)
{
    if(impl_)
        impl_->SampleEyedropper(true);
}

void UiColorPicker::MouseMove(Point, dword)
{
    if(impl_)
        impl_->SampleEyedropper(false);
}

bool UiColorPicker::Key(dword key, int count)
{
    if(impl_ && impl_->GetPageMode() == PAGE_IMAGE && key == K_CTRL_V && IsClipboardAvailableImage()) {
        Image pasted = ReadClipboardImage();
        if(!pasted.IsEmpty()) {
            impl_->SetGeneratorImage(pasted);
            return true;
        }
    }
    if(impl_ && impl_->Key(key, count))
        return true;
    return Ctrl::Key(key, count);
}

Image UiColorPicker::CursorImage(Point point, dword flags)
{
    if(impl_ && impl_->IsEyedropperActive())
        return Image::Cross();
    return Ctrl::CursorImage(point, flags);
}

void UiColorPicker::CancelMode()
{
    if(impl_)
        impl_->CancelMode();
    Ctrl::CancelMode();
}

} // namespace Upp
