#include <Ui/UiGallery.h>

namespace Upp {

void UiGallery::EnsureItemRender() const
{
    UiGallery *self = const_cast<UiGallery *>(this);
    if(!self->item_render_)
        self->item_render_ = new UiItemRenderImage;
}

UiGallery& UiGallery::SetItemRender(const UiItemRender& render)
{
    item_render_ = render.Clone();
    ResetItemRenderPool();
    RefreshLayout();
    Refresh();
    return *this;
}

const UiItemRender& UiGallery::GetItemRender() const
{
    EnsureItemRender();
    return *item_render_;
}

void UiGallery::ResetItemRenderPool()
{
    item_render_pool_.Clear();
    prepared_render_model_ = nullptr;
    prepared_render_range_ = UiVisibleRange();
    last_render_layout_count_ = 0;
}

void UiGallery::InvalidateItemRenderData(int first, int last)
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

void UiGallery::PrepareItemRenders()
{
    EnsureItemRender();
    last_render_layout_count_ = 0;

    if(prepared_render_model_ != model_) {
        InvalidateItemRenderData();
        prepared_render_model_ = model_;
    }

    UiVisibleRange range = GetVisibleRange(true);
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
        if(slot.render->PrepareLayout(GetItemRect(index), UiDirection::V))
            last_render_layout_count_++;
    }

    for(int i = needed; i < item_render_pool_.GetCount(); i++)
        item_render_pool_[i].index = -1;
}

UiItemRender* UiGallery::FindPreparedItemRender(int index)
{
    if(prepared_render_range_.IsEmpty() || !prepared_render_range_.Contains(index))
        return nullptr;
    int slot_index = index - prepared_render_range_.first;
    if(slot_index < 0 || slot_index >= item_render_pool_.GetCount())
        return nullptr;
    ItemRenderSlot& slot = item_render_pool_[slot_index];
    return slot.index == index ? slot.render.operator->() : nullptr;
}

const UiItemRender* UiGallery::FindPreparedItemRender(int index) const
{
    return const_cast<UiGallery *>(this)->FindPreparedItemRender(index);
}

UiItemRenderState UiGallery::GetItemRenderState(int index) const
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
