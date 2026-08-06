                if(!IsNull(items_[i].value.color) && selected_.Find(i) < 0)
                    selected_.Add(i);
        }
        else if(toggle) {
            int found = selected_.Find(index);
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
        active_index_ = selected_.Find(index) >= 0 ? index
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
        if(IsNull(items_[index].value.color) || selected_.Find(index) >= 0)
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
    Event<int, Vector<UiColorPicker::SlotValue>> WhenDropGroup;

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
            tiles_[i]->SetDisplay(items_[i], selected_.Find(i) >= 0, i == active_index_, compact_);
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
