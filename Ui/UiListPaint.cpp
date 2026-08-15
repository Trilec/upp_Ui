#include <Ui/UiList.h>
#include <Ui/UiTheme.h>

namespace Upp {

void UiList::PaintRow(Draw& w, int index, const Rect& row) const
{
    if(!model_ || index < 0 || index >= model_->GetCount() || row.IsEmpty())
        return;

    const Style& style = GetEffectiveStyle();
    const UiModelItem& item = model_->Get(index);

    if(index > 0 && item.separator_before)
        w.DrawRect(row.left, row.top, row.GetWidth(), 1, style.separator_color);

    const UiItemRender *render = FindPreparedItemRender(index);
    if(render)
        render->Paint(w, GetItemRenderState(index));

    if(drag_reorder_enabled_ && style.show_drag_handle) {
        Rect dr = GetDragRect(row);
        Color drag_ink = style.muted_ink;
        if(index == drag_from_ && dragging_)
            drag_ink = style.selected_frame;
        else if(index == hot_drag_ || index == pressed_drag_)
            drag_ink = style.hot_ink;
        UiPaintStyledIcon(w, dr,
                          IsNull(style.drag_glyph) ? ICON_DESIGN_DRAG_INDICATOR_48() : style.drag_glyph,
                          true, true, UiIconRenderMode::MonoTint, drag_ink, item.enabled);
    }

    if(style.show_row_separator && index + 1 < model_->GetCount())
        w.DrawRect(row.left, row.bottom - 1, row.GetWidth(), 1, style.separator_color);
}

void UiList::Paint(Draw& w)
{
    SyncModel();
    const Style& style = GetEffectiveStyle();

    StyledPalette viewport_palette = style.palette;
    StyledMetrics viewport_metrics = style.metrics;
    StyledSkin viewport_skin = style.skin;
    if(!has_custom_style_) {
        // Minimal List rows intentionally allow a transparent normal face, but
        // a standalone List still owns a viewport. Resolve only missing
        // viewport faces from the semantic Surface panel; row renderer styling
        // remains untouched and therefore keeps lightweight/transparent rows.
        const UiPanel::Style panel = UiTheme::ResolvePanel(UiPanelRole::Surface);
        for(int i = 0; i < 4; i++)
            if(viewport_palette.face[i].IsNone() && panel.palette.face[i].IsSolid())
                viewport_palette.face[i] = panel.palette.face[i];
        viewport_metrics.face_enabled = true;
        viewport_skin = StyledSkin();
    }

    UiPaintStyledSurface(w, GetSize(), viewport_palette, viewport_metrics, viewport_skin,
                         IsEnabled() ? ST_NORMAL : ST_DISABLED,
                         HasFocus(), false, false);

    last_paint_item_count_ = 0;
    Rect vp = GetViewportRect();
    UiVisibleRange visible = GetVisibleRange();
    if(vp.IsEmpty() || visible.IsEmpty())
        return;

    w.Clip(vp);
    for(int i = visible.first; i <= visible.last; i++) {
        Rect row = GetRowRect(i);
        if(row.bottom <= vp.top || row.top >= vp.bottom)
            continue;
        PaintRow(w, i, row);
        last_paint_item_count_++;
    }
    w.End();
}

void UiList::Layout()
{
    SyncModel();

    if(editing_ && editing_index_ >= 0 && model_ && editing_index_ < model_->GetCount()) {
        Rect row = GetRowRect(editing_index_);
        const UiModelItem& item = model_->Get(editing_index_);
        bool has_check = GetEffectiveStyle().show_checks && (item.has_check || item.checked);
        bool has_icon = GetEffectiveStyle().show_icons && !IsNull(item.icon);
        bool has_metadata = GetEffectiveStyle().show_metadata_marker && item.has_metadata;
        Rect tx = GetTextRect(row.Deflated(DPI(2), DPI(1)), has_check, has_icon, has_metadata, item);
        inline_editor_.SetRect(tx.left - DPI(2), tx.top + DPI(2),
                               max(DPI(80), tx.GetWidth() + DPI(4)),
                               max(DPI(22), tx.GetHeight() - DPI(4)));
    }
    else
        inline_editor_.Hide();

    UpdateDragMarker();
    PrepareItemRenders();
}

Size UiList::GetMinSize() const
{
    const Style& style = GetEffectiveStyle();
    int rows_h = style.row_height * 4 + max(0, style.item_spacing) * 3;
    return UiStyledOuterSizeFromContent(Size(DPI(180), max(DPI(80), rows_h)), style.metrics, style.skin);
}

} // namespace Upp
