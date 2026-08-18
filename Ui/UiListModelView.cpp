#include <Ui/UiList.h>
#include <Ui/UiTheme.h>

namespace Upp {

namespace {

bool IsStructuralModelChange(const UiModelChange& change)
{
    return change.kind == UI_MODEL_INSERT || change.kind == UI_MODEL_ERASE ||
           change.kind == UI_MODEL_MOVE || change.kind == UI_MODEL_CLEAR ||
           change.kind == UI_MODEL_RESET;
}

int RemapModelIndex(int index, const UiModelChange& change)
{
    if(index < 0)
        return index;

    switch(change.kind) {
    case UI_MODEL_INSERT: {
        int count = max(1, change.b);
        return index >= change.a ? index + count : index;
    }
    case UI_MODEL_ERASE: {
        int count = max(1, change.b);
        if(index < change.a)
            return index;
        if(index < change.a + count)
            return -1;
        return index - count;
    }
    case UI_MODEL_MOVE:
        if(change.a == change.b)
            return index;
        if(index == change.a)
            return change.b;
        if(change.a < change.b && index > change.a && index <= change.b)
            return index - 1;
        if(change.b < change.a && index >= change.b && index < change.a)
            return index + 1;
        return index;
    case UI_MODEL_CLEAR:
    case UI_MODEL_RESET:
        return -1;
    default:
        return index;
    }
}

void RemapModelSelection(Index<int>& selection, const UiModelChange& change)
{
    Index<int> remapped;
    remapped.Reserve(selection.GetCount());
    for(int i = 0; i < selection.GetCount(); i++) {
        int index = RemapModelIndex(selection[i], change);
        if(index >= 0)
            remapped.FindAdd(index);
    }
    selection = pick(remapped);
}

} // namespace

void UiList::BindModel(UiListModel& model)
{
    for(int i = 0; i < bound_models_.GetCount(); i++) {
        if(bound_models_[i] == &model)
            return;
    }

    bound_models_.Add(&model);
    Ptr<UiList> self = this;
    UiListModel* observed = &model;
    model.WhenChange << [self, observed](const UiModelChange& change) {
        if(self && self->model_ == observed)
            self->HandleModelChange(change);
    };
}

void UiList::HandleModelChange(const UiModelChange& change)
{
    if(IsStructuralModelChange(change)) {
        if(drag_candidate_)
            EndRowDrag(true);
        RemapModelSelection(selected_, change);
        cursor_ = RemapModelIndex(cursor_, change);
        anchor_ = RemapModelIndex(anchor_, change);
        hot_ = RemapModelIndex(hot_, change);
        pressed_ = RemapModelIndex(pressed_, change);
        hot_drag_ = RemapModelIndex(hot_drag_, change);
        pressed_drag_ = RemapModelIndex(pressed_drag_, change);
        if(editing_) {
            int next = RemapModelIndex(editing_index_, change);
            if(next < 0)
                CancelRename();
            else
                editing_index_ = next;
        }
    }

    model_revision_ = -1;
    SyncModel();

    if(change.kind == UI_MODEL_UPDATE) {
        int start = max(0, change.a);
        int count = max(1, change.b);
        int end = start + count - 1;
        InvalidateItemRenderData(start, end);
        PrepareItemRenders();

        UiVisibleRange visible = GetVisibleRange();
        if(!visible.IsEmpty() && end >= visible.first && start <= visible.last) {
            int a = max(start, visible.first);
            int b = min(end, visible.last);
            for(int i = a; i <= b; i++)
                Refresh(GetRowRect(i));
        }
        if(editing_ && editing_index_ >= start && editing_index_ <= end)
            RefreshLayout();
        return;
    }

    InvalidateItemRenderData();
    RefreshLayout();
    Refresh();
}

UiList& UiList::ShowDragHandle(bool on)
{
    StyleEdit().show_drag_handle = on;
    RefreshLayout();
    Refresh();
    return *this;
}

UiList& UiList::SetDragSide(UiAlign side)
{
    if(side != UiAlign::LEFT && side != UiAlign::RIGHT)
        side = UiAlign::RIGHT;
    StyleEdit().drag_side = side;
    RefreshLayout();
    Refresh();
    return *this;
}

UiList& UiList::SetDragGlyph(const Image& glyph)
{
    StyleEdit().drag_glyph = glyph;
    Refresh();
    return *this;
}

UiList& UiList::SetCursor(int index)
{
    SyncModel();
    if(!IsSelectableIndex(index))
        return *this;
    SelectSingle(index);
    ScrollTo(index);
    return *this;
}

void UiList::SyncModel()
{
    if(!model_)
        return;
    int rev = model_->GetRevision();
    if(rev == model_revision_)
        return;
    model_revision_ = rev;

    int count = model_->GetCount();
    for(int i = selected_.GetCount() - 1; i >= 0; i--) {
        int index = selected_[i];
        if(index < 0 || index >= count || !IsSelectableIndex(index))
            selected_.Remove(i);
    }
    if(cursor_ >= count || (cursor_ >= 0 && !IsSelectableIndex(cursor_)))
        cursor_ = -1;
    if(anchor_ >= count || (anchor_ >= 0 && !IsSelectableIndex(anchor_)))
        anchor_ = cursor_;
    if(hot_ >= count)
        hot_ = -1;
    if(pressed_ >= count)
        pressed_ = -1;
    if(editing_) {
        bool valid = editing_index_ >= 0 && editing_index_ < count;
        if(valid) {
            const UiModelItem& item = model_->Get(editing_index_);
            valid = item.enabled && !item.group_header && item.editable;
        }
        if(!valid)
            CancelRename();
    }
    ClampScroll();
}

void UiList::ClampScroll()
{
    Rect vp = GetViewportRect();
    int max_scroll = max(0, GetTotalHeight() - max(0, vp.GetHeight()));
    scroll_y_ = clamp(scroll_y_, 0, max_scroll);
}

Rect UiList::GetViewportRect() const
{
    return UiStyledInnerRect(GetSize(), GetEffectiveStyle().metrics, GetEffectiveStyle().skin);
}

int UiList::GetTotalHeight() const
{
    if(!model_)
        return 0;
    int rh = max(DPI(18), GetEffectiveStyle().row_height);
    int sp = max(0, GetEffectiveStyle().item_spacing);
    return UiUniformContentExtent(model_->GetCount(), rh, sp);
}

UiVisibleRange UiList::GetVisibleRange(int overscan_rows) const
{
    if(!model_)
        return UiVisibleRange();
    Rect vp = GetViewportRect();
    int rh = max(DPI(18), GetEffectiveStyle().row_height);
    int sp = max(0, GetEffectiveStyle().item_spacing);
    return UiComputeLinearVisibleRange(model_->GetCount(), scroll_y_, vp.GetHeight(), rh, sp, overscan_rows);
}

Rect UiList::GetRowRect(int row) const
{
    Rect vp = GetViewportRect();
    int rh = max(DPI(18), GetEffectiveStyle().row_height);
    int sp = max(0, GetEffectiveStyle().item_spacing);
    int extent = rh + sp;
    int64 y64 = (int64)vp.top - scroll_y_ + (int64)row * extent;
    int y = y64 <= INT_MIN ? INT_MIN : y64 >= INT_MAX ? INT_MAX : (int)y64;
    int bottom = y > INT_MAX - rh ? INT_MAX : y + rh;
    return Rect(vp.left, y, vp.right, bottom);
}

int UiList::HitTestRow(Point p) const
{
    Rect vp = GetViewportRect();
    if(!vp.Contains(p) || !model_)
        return -1;
    int rh = max(DPI(18), GetEffectiveStyle().row_height);
    int sp = max(0, GetEffectiveStyle().item_spacing);
    int extent = rh + sp;
    int local = p.y - vp.top + scroll_y_;
    int row = local / max(1, extent);
    if(row < 0 || row >= model_->GetCount())
        return -1;
    if(local % max(1, extent) >= rh)
        return -1;
    return row;
}

Rect UiList::GetCheckRect(const Rect& row) const
{
    const Style& style = GetEffectiveStyle();
    int size = min(style.check_size, row.GetHeight() - DPI(6));
    int y = row.top + (row.GetHeight() - size) / 2;
    int x = row.left + style.h_padding;
    return RectC(x, y, size, size);
}

Rect UiList::GetIconRect(const Rect& row, bool has_check) const
{
    const Style& style = GetEffectiveStyle();
    int size = min(style.icon_size, row.GetHeight() - DPI(6));
    int y = row.top + (row.GetHeight() - size) / 2;
    int x = row.left + style.h_padding;
    if(has_check)
        x = GetCheckRect(row).right + style.content_gap;
    return RectC(x, y, size, size);
}

Rect UiList::GetMetadataRect(const Rect& row, bool has_check, bool has_icon) const
{
    const Style& style = GetEffectiveStyle();
    int size = min(style.metadata_size, row.GetHeight() - DPI(8));
    int y = row.top + (row.GetHeight() - size) / 2;
    int x = row.left + style.h_padding;
    if(drag_reorder_enabled_ && style.show_drag_handle && style.drag_side == UiAlign::LEFT)
        x = GetDragRect(row).right + style.drag_gap;
    if(has_check)
        x = GetCheckRect(row).right + style.content_gap;
    if(has_icon)
        x = GetIconRect(row, has_check).right + style.content_gap;
    return RectC(x, y, size, size);
}

Rect UiList::GetDragRect(const Rect& row) const
{
    const Style& style = GetEffectiveStyle();
    if(!drag_reorder_enabled_ || !style.show_drag_handle)
        return Rect(0, 0, 0, 0);
    int size = min(style.drag_size, row.GetHeight() - DPI(6));
    int y = row.top + (row.GetHeight() - size) / 2;
    int x = style.drag_side == UiAlign::LEFT
          ? row.left + style.h_padding
          : row.right - style.h_padding - size;
    return RectC(x, y, size, size);
}

Rect UiList::GetRightTextRect(const Rect& row, const UiModelItem& item) const
{
    if(item.right_text.IsEmpty())
        return Rect(0, 0, 0, 0);
    const Style& style = GetEffectiveStyle();
    Font font = item.use_custom_font ? item.custom_font : style.font;
    Size sz = GetTextSize(item.right_text, font);
    int extra = style.right_text_as_badge ? style.badge_h_padding * 2 : DPI(4);
    int w = min(sz.cx + extra, max(0, row.GetWidth() / 2));
    int right = row.right - style.h_padding;
    if(drag_reorder_enabled_ && style.show_drag_handle && style.drag_side == UiAlign::RIGHT)
        right = GetDragRect(row).left - style.drag_gap;
    return Rect(max(row.left, right - w), row.top, right, row.bottom);
}

Rect UiList::GetTextRect(const Rect& row, bool has_check, bool has_icon, bool has_metadata, const UiModelItem& item) const
{
    const Style& style = GetEffectiveStyle();
    int left = row.left + style.h_padding;
    if(drag_reorder_enabled_ && style.show_drag_handle && style.drag_side == UiAlign::LEFT)
        left = GetDragRect(row).right + style.drag_gap;
    if(has_check)
        left = GetCheckRect(row).right + style.content_gap;
    if(has_icon)
        left = GetIconRect(row, has_check).right + style.content_gap;
    if(has_metadata)
        left = GetMetadataRect(row, has_check, has_icon).right + style.metadata_gap;
    Rect right = GetRightTextRect(row, item);
    int right_edge = right.IsEmpty() ? row.right - style.h_padding : right.left - style.right_gap;
    if(drag_reorder_enabled_ && style.show_drag_handle && style.drag_side == UiAlign::RIGHT)
        right_edge = min(right_edge, GetDragRect(row).left - style.drag_gap);
    return Rect(left, row.top, max(left, right_edge), row.bottom);
}

} // namespace Upp
