#include <Ui/UiGallery.h>
#include <Ui/UiGridLayout.h>
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

void UiGallery::BindModel(UiListModel& model)
{
    for(int i = 0; i < bound_models_.GetCount(); i++)
        if(bound_models_[i] == &model)
            return;
    bound_models_.Add(&model);

    Ptr<UiGallery> self = this;
    UiListModel* observed = &model;
    model.WhenChange << [self, observed](const UiModelChange& change) {
        if(self && self->model_ == observed)
            self->HandleModelChange(change);
    };
}

void UiGallery::HandleModelChange(const UiModelChange& change)
{
    if(IsStructuralModelChange(change)) {
        if(marquee_candidate_ || marquee_active_)
            EndMarquee(true);
        RemapModelSelection(selected_, change);
        cursor_ = RemapModelIndex(cursor_, change);
        anchor_ = RemapModelIndex(anchor_, change);
        hot_ = RemapModelIndex(hot_, change);
        pressed_ = RemapModelIndex(pressed_, change);
    }

    model_revision_ = -1;
    SyncModel();

    if(change.kind == UI_MODEL_UPDATE && geometry_valid_) {
        int start = max(0, change.a);
        int count = max(1, change.b);
        int end = start + count - 1;
        InvalidateItemRenderData(start, end);
        PrepareItemRenders();

        UiVisibleRange visible = GetVisibleRange(true);
        if(!visible.IsEmpty() && end >= visible.first && start <= visible.last) {
            int a = max(start, visible.first);
            int b = min(end, visible.last);
            for(int i = a; i <= b; i++)
                Refresh(GetItemRect(i));
        }
        return;
    }

    InvalidateItemRenderData();
    InvalidateGeometry();
}

void UiGallery::SyncModel()
{
    if(!model_)
        return;
    int revision = model_->GetRevision();
    if(revision == model_revision_)
        return;
    model_revision_ = revision;

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
    if(geometry_valid_)
        ClampScroll();
}

void UiGallery::InvalidateGeometry()
{
    geometry_valid_ = false;
    RefreshLayout();
    Refresh();
}

Rect UiGallery::GetBaseViewportRect() const
{
    return UiStyledInnerRect(GetSize(), GetEffectiveStyle().metrics, GetEffectiveStyle().skin);
}

void UiGallery::UpdateGeometry()
{
    geometry_build_count_++;
    SyncModel();
    SyncThemeStyle();

    Rect base = GetBaseViewportRect();
    int bar_width = max(DPI(12), vscroll_.GetMinSize().cx);
    bool show_bar = false;
    int columns = 1;
    int rows = 0;
    int content_height = 0;
    Rect vp = base;
    int count = model_ ? model_->GetCount() : 0;

    for(int pass = 0; pass < 2; pass++) {
        vp = base;
        if(show_bar)
            vp.right = max(vp.left, vp.right - bar_width);
        int available = max(1, vp.GetWidth() - inset_.left - inset_.right);
        columns = UiGridLayout::ComputeColumns(available, item_size_.cx, gap_);
        rows = count > 0 ? (count + columns - 1) / columns : 0;
        if(rows > 0) {
            int64 h = (int64)inset_.top + UiUniformContentExtent(rows, item_size_.cy, gap_) + inset_.bottom;
            content_height = h >= INT_MAX ? INT_MAX : (int)h;
        }
        else
            content_height = 0;
        bool need = content_height > vp.GetHeight();
        if(need == show_bar)
            break;
        show_bar = need;
    }

    viewport_ = vp;
    columns_ = max(1, columns);
    rows_ = max(0, rows);
    int grid_width = 0;
    if(count > 0) {
        int64 w = (int64)inset_.left + UiUniformContentExtent(columns_, item_size_.cx, gap_) + inset_.right;
        grid_width = w >= INT_MAX ? INT_MAX : (int)w;
    }
    content_size_ = Size(max(viewport_.GetWidth(), grid_width), max(0, content_height));
    geometry_valid_ = true;
    ClampScroll();

    updating_scrollbar_ = true;
    if(show_bar) {
        vscroll_.Show();
        vscroll_.SetRect(base.right - bar_width, base.top, bar_width, max(0, base.GetHeight()));
        vscroll_.SetRange(0, content_size_.cy, max(0, viewport_.GetHeight())).SetPos(scroll_y_);
    }
    else {
        vscroll_.Hide();
        scroll_y_ = 0;
    }
    updating_scrollbar_ = false;
}

int UiGallery::GetMaxScroll() const
{
    return max(0, content_size_.cy - max(0, viewport_.GetHeight()));
}

void UiGallery::ClampScroll()
{
    scroll_y_ = clamp(scroll_y_, 0, GetMaxScroll());
}

UiVisibleRange UiGallery::GetVisibleRange(bool include_overscan) const
{
    if(!geometry_valid_)
        const_cast<UiGallery*>(this)->UpdateGeometry();
    if(!model_ || model_->IsEmpty())
        return UiVisibleRange();

    int viewport_height = max(0, viewport_.GetHeight());
    int logical_scroll = max(0, scroll_y_ - inset_.top);
    int effective_height = viewport_height;
    if(scroll_y_ < inset_.top)
        effective_height = max(0, viewport_height - (inset_.top - scroll_y_));
    return UiComputeGridVisibleRange(model_->GetCount(),
                                     columns_,
                                     logical_scroll,
                                     effective_height,
                                     item_size_.cy,
                                     gap_,
                                     include_overscan ? overscan_rows_ : 0);
}

void UiGallery::UpdateVisibleRangeNotification()
{
    if(!geometry_valid_)
        return;
    UiVisibleRange range = GetVisibleRange(true);
    if(range.first == notified_range_.first && range.last == notified_range_.last)
        return;
    notified_range_ = range;
    if(WhenVisibleRange)
        WhenVisibleRange(range.first, range.last);
}

Rect UiGallery::GetItemRect(int index) const
{
    if(!geometry_valid_)
        const_cast<UiGallery*>(this)->UpdateGeometry();
    if(!model_ || index < 0 || index >= model_->GetCount())
        return Rect(0, 0, 0, 0);
    int columns = max(1, columns_);
    int row = index / columns;
    int col = index % columns;
    int x = viewport_.left + inset_.left + col * (item_size_.cx + gap_);
    int y = viewport_.top + inset_.top - scroll_y_ + row * (item_size_.cy + gap_);
    return RectC(x, y, item_size_.cx, item_size_.cy);
}

int UiGallery::HitTestItem(Point p) const
{
    if(!geometry_valid_ || !model_ || !viewport_.Contains(p))
        return -1;
    int x = p.x - viewport_.left - inset_.left;
    int y = p.y - viewport_.top + scroll_y_ - inset_.top;
    if(x < 0 || y < 0)
        return -1;
    int col_extent = item_size_.cx + gap_;
    int row_extent = item_size_.cy + gap_;
    int col = x / max(1, col_extent);
    int row = y / max(1, row_extent);
    if(col < 0 || col >= columns_ || row < 0 || row >= rows_)
        return -1;
    if(x % max(1, col_extent) >= item_size_.cx || y % max(1, row_extent) >= item_size_.cy)
        return -1;
    int index = row * columns_ + col;
    return index >= 0 && index < model_->GetCount() ? index : -1;
}

} // namespace Upp
