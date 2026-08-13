#include <Ui/UiList.h>
#include <Ui/UiTheme.h>

namespace Upp {

static StyledState UiListPaintState(bool enabled, bool pressed, bool hot)
{
    if(!enabled) return ST_DISABLED;
    if(pressed) return ST_PRESSED;
    if(hot) return ST_HOT;
    return ST_NORMAL;
}

static void DrawAlignedListPaintText(Draw& w, const Rect& r, const String& text, Font font, Color ink, int align)
{
    if(r.IsEmpty() || text.IsEmpty()) return;
    Size sz = GetTextSize(text, font);
    int x = r.left;
    if(align == ALIGN_RIGHT) x = max(r.left, r.right - sz.cx);
    else if(align == ALIGN_CENTER) x = max(r.left, r.left + (r.GetWidth() - sz.cx) / 2);
    int y = r.top + max(0, (r.GetHeight() - sz.cy) / 2);
    w.DrawText(x, y, text, font, ink);
}

void UiList::PaintCheck(Draw& w, const Rect& r, const UiModelItem& item, bool selected) const
{
    if(r.IsEmpty())
        return;
    const Style& style = GetEffectiveStyle();
    StyledPalette p;
    StyledMetrics m;
    m.face_enabled = true;
    m.frame_enabled = true;
    m.frame_width = DPI(1);
    m.radius = DPI(3);
    for(int i = 0; i < 4; i++) {
        p.face[i] = UiFill::Solid(White());
        p.frame[i] = style.check_frame;
        p.ink[i] = style.check_fill;
    }
    if(selected) {
        for(int i = 0; i < 4; i++)
            p.frame[i] = style.selected_frame;
    }
    if(item.checked) {
        for(int i = 0; i < 4; i++)
            p.face[i] = UiFill::Solid(selected ? style.selected_frame : style.check_fill);
    }
    UiPaintFaceFrameDash(w, r, p, m, item.enabled ? ST_NORMAL : ST_DISABLED);
    if(item.checked) {
        Color ink = White();
        int pad = max(2, r.GetWidth() / 5);
        int x1 = r.left + pad;
        int y1 = r.top + r.GetHeight() / 2;
        int x2 = r.left + r.GetWidth() / 2 - 1;
        int y2 = r.bottom - pad - 1;
        int x3 = r.right - pad - 1;
        int y3 = r.top + pad;
        w.DrawLine(x1, y1, x2, y2, 2, ink);
        w.DrawLine(x2, y2, x3, y3, 2, ink);
    }
}

void UiList::PaintRow(Draw& w, int index, const Rect& row) const
{
    if(!model_ || index < 0 || index >= model_->GetCount() || row.IsEmpty())
        return;

    const Style& style = GetEffectiveStyle();
    const UiModelItem& item = model_->Get(index);
    bool selected = IsSelected(index);
    bool hot = index == hot_;
    bool pressed = index == pressed_;
    StyledState st = UiListPaintState(item.enabled, pressed, hot);

    Rect rr = row;

    if(index > 0 && item.separator_before)
        w.DrawRect(row.left, row.top, row.GetWidth(), 1, style.separator_color);

    bool underline_state = (selected && style.selected_as_underline) || (!selected && hot && style.hot_as_underline);

    if(selected || hot) {
        Color accent = selected ? style.selected_frame : style.hot_frame;
        if(underline_state) {
            int thickness = max(DPI(1), style.state_underline_thickness);
            w.DrawRect(rr.left, rr.bottom - thickness, rr.GetWidth(), thickness, accent);
        }
        else {
            StyledPalette p;
            StyledMetrics m;
            m.face_enabled = true;
            m.frame_enabled = style.row_state_frame_enabled;
            m.frame_width = DPI(1);
            m.radius = style.row_radius;
            for(int i = 0; i < 4; i++) {
                p.face[i] = UiFill::Solid(selected ? style.selected_face : style.hot_face);
                p.frame[i] = selected ? style.selected_frame : style.hot_frame;
                p.ink[i] = selected ? style.selected_ink : style.hot_ink;
            }
            UiPaintFaceFrameDash(w, rr, p, m, st);
        }
    }

    bool has_check = style.show_checks && (item.has_check || item.checked);
    bool has_icon = style.show_icons && !IsNull(item.icon);
    bool has_metadata = style.show_metadata_marker && item.has_metadata;
    bool has_drag = drag_reorder_enabled_ && style.show_drag_handle;

    if(has_drag) {
        Rect dr = GetDragRect(rr);
        Color drag_ink = style.muted_ink;
        if(index == drag_from_ && dragging_)
            drag_ink = style.selected_frame;
        else if(index == hot_drag_ || index == pressed_drag_)
            drag_ink = style.hot_ink;
        UiPaintStyledIcon(w, dr, IsNull(style.drag_glyph) ? ICON_DESIGN_DRAG_INDICATOR_48() : style.drag_glyph,
                          true, true, UiIconRenderMode::MonoTint, drag_ink, item.enabled);
    }

    if(has_check)
        PaintCheck(w, GetCheckRect(rr), item, selected);

    if(has_icon) {
        Color icon_ink = !IsNull(item.custom_ink_color)
                       ? item.custom_ink_color
                       : (selected ? style.selected_ink : (item.enabled ? style.muted_ink : style.disabled_ink));
        UiPaintStyledIcon(w, GetIconRect(rr, has_check), item.icon, true, true,
                          item.icon_render_mode, icon_ink, item.enabled);
    }

    if(has_metadata) {
        Rect mr = GetMetadataRect(rr, has_check, has_icon);
        Color c = IsNull(item.metadata_color) ? style.metadata_default : item.metadata_color;
        w.DrawRect(mr, c);
    }

    Rect tx = GetTextRect(rr, has_check, has_icon, has_metadata, item);
    Rect rx = GetRightTextRect(rr, item);
    Font font = item.use_custom_font ? item.custom_font : style.font;
    if(item.group_header && !item.use_custom_font)
        font.Bold();
    Color ink = !IsNull(item.custom_ink_color)
              ? item.custom_ink_color
              : (selected ? style.selected_ink : (item.enabled ? style.ink : style.disabled_ink));

    DrawAlignedListPaintText(w, tx, item.text, font, ink, item.text_align);

    if(item.underline) {
        Color uc = IsNull(item.underline_color) ? ink : item.underline_color;
        Size tsz = GetTextSize(item.text, font);
        int ux = tx.left;
        if(item.text_align == ALIGN_RIGHT)
            ux = max(tx.left, tx.right - tsz.cx);
        else if(item.text_align == ALIGN_CENTER)
            ux = max(tx.left, tx.left + (tx.GetWidth() - tsz.cx) / 2);
        int uy = min(tx.bottom - 2, tx.top + max(0, (tx.GetHeight() - tsz.cy) / 2) + tsz.cy + 1);
        w.DrawRect(ux, uy, min(tx.right - ux, tsz.cx), 1, uc);
    }

    if(!rx.IsEmpty()) {
        Font rf = style.font;
        if(item.group_header)
            rf.Bold();
        Color rink = selected ? style.selected_ink : (item.enabled ? style.muted_ink : style.disabled_ink);
        Rect text_rx = rx;
        if(style.right_text_as_badge) {
            StyledPalette p;
            StyledMetrics m;
            m.face_enabled = !IsNull(style.badge_face);
            m.frame_enabled = !IsNull(style.badge_frame);
            m.frame_width = DPI(1);
            m.radius = style.badge_radius;
            m.focus_enabled = false;
            for(int i = 0; i < 4; i++) {
                p.face[i] = UiFill::Solid(style.badge_face);
                p.frame[i] = style.badge_frame;
                p.ink[i] = style.badge_ink;
            }
            UiPaintFaceFrameDash(w, rx.Deflated(0, DPI(2)), p, m, st);
            rink = item.enabled ? style.badge_ink : style.disabled_ink;
            text_rx = rx.Deflated(style.badge_h_padding, 0);
        }
        DrawAlignedListPaintText(w, text_rx, item.right_text, rf, rink, item.right_text_align);
    }

    if(style.show_row_separator && model_ && index + 1 < model_->GetCount())
        w.DrawRect(row.left, row.bottom - 1, row.GetWidth(), 1, style.separator_color);
}

void UiList::Paint(Draw& w)
{
    SyncModel();
    const Style& style = GetEffectiveStyle();
    UiPaintStyledSurface(w, GetSize(), style.palette, style.metrics, style.skin,
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
}

Size UiList::GetMinSize() const
{
    const Style& style = GetEffectiveStyle();
    int rows_h = style.row_height * 4 + max(0, style.item_spacing) * 3;
    return UiStyledOuterSizeFromContent(Size(DPI(180), max(DPI(80), rows_h)), style.metrics, style.skin);
}

} // namespace Upp
