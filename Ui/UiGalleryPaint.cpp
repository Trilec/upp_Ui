#include <Ui/UiGallery.h>
#include <Ui/UiTheme.h>

namespace Upp {

static StyledState UiGalleryStyledStatePaint(UiGalleryItemVisualState state)
{
    switch(state) {
    case UIGALLERYITEM_HOT:      return ST_HOT;
    case UIGALLERYITEM_SELECTED: return ST_PRESSED;
    case UIGALLERYITEM_DISABLED: return ST_DISABLED;
    case UIGALLERYITEM_NORMAL:
    default:                     return ST_NORMAL;
    }
}

static void DrawGalleryTextPaint(Draw& w, const Rect& r, const String& text, Font font, Color ink)
{
    if(r.IsEmpty() || text.IsEmpty())
        return;
    Size ts = GetTextSize(text, font);
    int x = r.left + max(0, (r.GetWidth() - ts.cx) / 2);
    int y = r.top + max(0, (r.GetHeight() - ts.cy) / 2);
    w.Clip(r);
    w.DrawText(x, y, text, font, ink);
    w.End();
}

void UiGallery::PaintDefaultItem(Draw& w, int index, const Rect& rect, UiGalleryItemVisualState state) const
{
    if(!model_ || index < 0 || index >= model_->GetCount() || rect.IsEmpty())
        return;

    const Style& style = GetEffectiveStyle();
    const UiModelItem& item = model_->Get(index);
    StyledState st = UiGalleryStyledStatePaint(state);
    UiPaintFaceFrameDash(w, rect, style.item_palette, style.item_metrics, st);

    Rect content = rect.Deflated(style.item_padding);
    if(content.IsEmpty())
        return;

    if(style.show_metadata_marker && item.has_metadata) {
        int ms = min(style.metadata_size, min(content.GetWidth(), content.GetHeight()));
        Rect mr = RectC(rect.right - style.metadata_inset - ms,
                        rect.top + style.metadata_inset,
                        ms, ms);
        w.DrawRect(mr, IsNull(item.metadata_color) ? style.metadata_default : item.metadata_color);
    }

    bool has_icon = style.show_icons && !IsNull(item.icon);
    bool has_description = style.show_description && !item.description.IsEmpty();
    int title_h = max(style.title_font.GetHeight() + DPI(2), DPI(16));
    int description_h = has_description ? max(style.description_font.GetHeight() + DPI(2), DPI(14)) : 0;
    int text_block_h = title_h + (has_description ? style.text_gap + description_h : 0);
    Rect text_block(content.left,
                    max(content.top, content.bottom - text_block_h),
                    content.right,
                    content.bottom);

    if(has_icon) {
        int max_icon_h = max(0, text_block.top - content.top - style.content_gap);
        int side = min(style.icon_size, min(content.GetWidth(), max_icon_h));
        if(side > 0) {
            Rect ir = RectC(content.left + (content.GetWidth() - side) / 2,
                            content.top + max(0, (max_icon_h - side) / 2),
                            side, side);
            Color icon_ink = !IsNull(item.custom_ink_color)
                           ? item.custom_ink_color
                           : style.item_palette.icon[st];
            UiPaintStyledIcon(w, ir, item.icon, true, true,
                              item.icon_render_mode, icon_ink, item.enabled);
        }
    }

    Font title_font = item.use_custom_font ? item.custom_font : style.title_font;
    Color title_ink = !IsNull(item.custom_ink_color)
                    ? item.custom_ink_color
                    : style.item_palette.ink[st];
    Rect title_rect(text_block.left, text_block.top, text_block.right, text_block.top + title_h);
    DrawGalleryTextPaint(w, title_rect, item.text, title_font, title_ink);

    if(has_description) {
        Rect dr(text_block.left, title_rect.bottom + style.text_gap, text_block.right, text_block.bottom);
        Color ink = item.enabled ? style.description_ink : style.item_palette.ink[ST_DISABLED];
        DrawGalleryTextPaint(w, dr, item.description, style.description_font, ink);
    }
}

void UiGallery::Paint(Draw& w)
{
    SyncModel();
    SyncThemeStyle();
    const Style& style = GetEffectiveStyle();
    UiPaintStyledSurface(w, GetSize(), style.palette, style.metrics, style.skin,
                         IsEnabled() ? ST_NORMAL : ST_DISABLED,
                         HasFocus(), false, false);

    last_paint_item_count_ = 0;
    if(!geometry_valid_ || !model_ || model_->IsEmpty() || viewport_.IsEmpty())
        return;

    UiVisibleRange range = GetVisibleRange(true);
    if(range.IsEmpty())
        return;

    w.Clip(viewport_);
    for(int i = range.first; i <= range.last; i++) {
        Rect rect = GetItemRect(i);
        if(rect.right <= viewport_.left || rect.left >= viewport_.right ||
           rect.bottom <= viewport_.top || rect.top >= viewport_.bottom)
            continue;
        UiGalleryItemVisualState state = GetItemVisualState(i);
        bool handled = false;
        if(WhenPaintItem)
            WhenPaintItem(w, i, model_->Get(i), rect, state, handled);
        if(!handled)
            PaintDefaultItem(w, i, rect, state);
        last_paint_item_count_++;
    }
    w.End();
}

void UiGallery::Layout()
{
    UpdateGeometry();
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
