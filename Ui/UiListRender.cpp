#include <Ui/UiList.h>

namespace Upp {

void UiList::EnsureItemRender() const
{
    UiList *self = const_cast<UiList *>(this);
    if(!self->item_render_)
        self->item_render_ = new UiItemRenderBasic;
}

UiList& UiList::SetItemRender(const UiItemRender& render)
{
    item_render_ = render.Clone();
    ResetItemRenderPool();
    RefreshLayout();
    Refresh();
    return *this;
}

const UiItemRender& UiList::GetItemRender() const
{
    EnsureItemRender();
    return *item_render_;
}

void UiList::ResetItemRenderPool()
{
    item_render_pool_.Clear();
    prepared_render_range_ = UiVisibleRange();
    prepared_render_model_ = nullptr;
    last_render_layout_count_ = 0;
}

void UiList::InvalidateItemRenderData(int first, int last)
{
    if(first < 0 || last < first) {
        for(int i = 0; i < item_render_pool_.GetCount(); i++)
            item_render_pool_[i].index = -1;
        prepared_render_range_ = UiVisibleRange();
        return;
    }

    for(int i = 0; i < item_render_pool_.GetCount(); i++) {
        int index = item_render_pool_[i].index;
        if(index >= first && index <= last)
            item_render_pool_[i].index = -1;
    }
}

void UiList::PrepareItemRenders()
{
    EnsureItemRender();
    last_render_layout_count_ = 0;

    if(prepared_render_model_ != model_) {
        InvalidateItemRenderData();
        prepared_render_model_ = model_;
    }

    UiVisibleRange range = GetVisibleRange(1);
    prepared_render_range_ = range;
    if(!model_ || range.IsEmpty())
        return;

    int needed = range.GetCount();
    while(item_render_pool_.GetCount() < needed) {
        ItemRenderSlot& slot = item_render_pool_.Add();
        slot.render = item_render_->Clone();
    }

    for(int slot_index = 0; slot_index < needed; slot_index++) {
        int index = range.first + slot_index;
        ItemRenderSlot& slot = item_render_pool_[slot_index];
        if(slot.index != index) {
            slot.render->SetData(UiMakeItemRenderData(model_->Get(index)));
            slot.index = index;
        }
        if(slot.render->PrepareLayout(GetRowRect(index), UiDirection::H))
            last_render_layout_count_++;
    }

    for(int i = needed; i < item_render_pool_.GetCount(); i++)
        item_render_pool_[i].index = -1;
}

UiItemRender* UiList::FindPreparedItemRender(int index)
{
    if(prepared_render_range_.IsEmpty() || !prepared_render_range_.Contains(index))
        return nullptr;
    int slot_index = index - prepared_render_range_.first;
    if(slot_index < 0 || slot_index >= item_render_pool_.GetCount())
        return nullptr;
    ItemRenderSlot& slot = item_render_pool_[slot_index];
    return slot.index == index ? slot.render.operator->() : nullptr;
}

const UiItemRender* UiList::FindPreparedItemRender(int index) const
{
    return const_cast<UiList *>(this)->FindPreparedItemRender(index);
}

UiItemRenderState UiList::GetItemRenderState(int index) const
{
    UiItemRenderState state;
    state.enabled = model_ && index >= 0 && index < model_->GetCount()
                  ? model_->Get(index).enabled && IsEnabled()
                  : false;
    state.selected = IsSelected(index);
    state.hot = index == hot_;
    state.pressed = index == pressed_;
    state.focused = HasFocus() && index == cursor_;
    return state;
}

} // namespace Upp
