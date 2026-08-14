#include <Ui/UiGallery.h>
#include <Ui/UiTheme.h>

namespace Upp {

void UiGallery::Paint(Draw& w)
{
    SyncModel();
    const Style& style = GetEffectiveStyle();
    UiPaintStyledSurface(w, GetSize(), style.palette, style.metrics, style.skin,
                         IsEnabled() ? ST_NORMAL : ST_DISABLED,
                         HasFocus(), false, false);

    last_paint_item_count_ = 0;
    if(!geometry_valid_ || !model_ || model_->IsEmpty() || viewport_.IsEmpty())
        return;

    UiVisibleRange range = GetVisibleRange(false);
    w.Clip(viewport_);
    if(!range.IsEmpty()) {
        for(int i = range.first; i <= range.last; i++) {
            Rect rect = GetItemRect(i);
            if(rect.right <= viewport_.left || rect.left >= viewport_.right ||
               rect.bottom <= viewport_.top || rect.top >= viewport_.bottom)
                continue;
            const UiItemRender *render = FindPreparedItemRender(i);
            if(render)
                render->Paint(w, GetItemRenderState(i));
            last_paint_item_count_++;
        }
    }

    if(marquee_active_) {
        Rect r = GetMarqueeRect() & viewport_;
        if(!r.IsEmpty()) {
            w.DrawRect(r, style.marquee_fill);
            int fw = max(1, style.marquee_frame_width);
            w.DrawRect(r.left, r.top, r.GetWidth(), fw, style.marquee_frame);
            w.DrawRect(r.left, r.bottom - fw, r.GetWidth(), fw, style.marquee_frame);
            w.DrawRect(r.left, r.top, fw, r.GetHeight(), style.marquee_frame);
            w.DrawRect(r.right - fw, r.top, fw, r.GetHeight(), style.marquee_frame);
        }
    }
    w.End();
}

void UiGallery::Layout()
{
    UpdateGeometry();
    PrepareItemRenders();
    UpdateVisibleRangeNotification();
}

Size UiGallery::GetMinSize() const
{
    const Style& style = GetEffectiveStyle();
    Size content(item_size_.cx + inset_.left + inset_.right,
                 item_size_.cy + inset_.top + inset_.bottom);
    return UiStyledOuterSizeFromContent(content, style.metrics, style.skin);
}

} // namespace Upp
