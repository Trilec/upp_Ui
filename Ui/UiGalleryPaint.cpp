#include <Ui/UiGallery.h>
#include <Ui/UiTheme.h>

namespace Upp {

namespace {

void DrawGalleryFrame(Draw& w, const Rect& rect, Color color, int width)
{
    if(rect.IsEmpty() || IsNull(color))
        return;
    int fw = min(max(1, width), max(1, min(rect.GetWidth(), rect.GetHeight()) / 2));
    w.DrawRect(rect.left, rect.top, rect.GetWidth(), fw, color);
    w.DrawRect(rect.left, rect.bottom - fw, rect.GetWidth(), fw, color);
    w.DrawRect(rect.left, rect.top, fw, rect.GetHeight(), color);
    w.DrawRect(rect.right - fw, rect.top, fw, rect.GetHeight(), color);
}

} // namespace

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
    Rect marquee;
    if(marquee_active_)
        marquee = GetMarqueeRect() & viewport_;

    w.Clip(viewport_);

    // Marquee fill sits behind tiles so it remains visible in the gallery gaps
    // without washing over thumbnail/text content. The frame is drawn last.
    if(!marquee.IsEmpty() && !IsNull(style.marquee_fill))
        w.DrawRect(marquee, style.marquee_fill);

    if(!range.IsEmpty()) {
        // Uniform geometry also bounds damage queries: hover/selection damage
        // should not walk every tile in a large viewport.
        Rect damage = w.GetPaintRect() & viewport_;
        int row_extent = max(1, item_size_.cy + gap_);
        int col_extent = max(1, item_size_.cx + gap_);
        int first_row = max(range.first / columns_,
                            max(0, damage.top - viewport_.top + scroll_y_ - inset_.top) / row_extent);
        int last_row = min(range.last / columns_,
                           max(0, damage.bottom - 1 - viewport_.top + scroll_y_ - inset_.top) / row_extent);
        int first_col = max(0, max(0, damage.left - viewport_.left - inset_.left) / col_extent);
        int last_col = min(columns_ - 1,
                           max(0, damage.right - 1 - viewport_.left - inset_.left) / col_extent);
        for(int row = first_row; !damage.IsEmpty() && row <= last_row; row++) {
            for(int col = first_col; col <= last_col; col++) {
                int i = row * columns_ + col;
                if(!range.Contains(i))
                    continue;
                Rect rect = GetItemRect(i);
                if(rect.right <= viewport_.left || rect.left >= viewport_.right ||
                   rect.bottom <= viewport_.top || rect.top >= viewport_.bottom ||
                   !w.IsPainting(rect))
                    continue;
                const UiItemRender *render = FindPreparedItemRender(i);
                if(render)
                    render->Paint(w, GetItemRenderState(i));
                if(IsSelected(i))
                    DrawGalleryFrame(w, rect, style.selection_frame, style.selection_frame_width);
                last_paint_item_count_++;
            }
        }
    }

    if(!marquee.IsEmpty())
        DrawGalleryFrame(w, marquee, style.marquee_frame, style.marquee_frame_width);
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
