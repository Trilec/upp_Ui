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
            .SetWrapRowsExpand(true)
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
                Add(*tile).Expand(1).MinMaxWidth(compact_ ? DPI(18) : DPI(28), INT_MAX)
                           .MinMaxHeight(compact_ ? DPI(18) : DPI(28), INT_MAX);
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
