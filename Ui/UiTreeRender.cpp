#include <Ui/UiTree.h>

namespace Upp {

namespace {

StyledState UiTreeState_(bool enabled, bool pressed, bool hot)
{
    if(!enabled)
        return ST_DISABLED;
    if(pressed)
        return ST_PRESSED;
    if(hot)
        return ST_HOT;
    return ST_NORMAL;
}

One<UiItemRender> MakeTreeBasicRender_(const UiTree::Style& tree)
{
    UiItemRenderBasic basic;
    UiItemRenderStyle style = basic.GetStyle();
    style.show_face = false;
    style.metrics.face_enabled = false;
    style.metrics.frame_enabled = false;
    style.metrics.focus_enabled = false;
    style.metrics.shadow.enabled = false;
    style.metrics.radius = 0;
    style.metrics.content_margin = Rect(0, 0, 0, 0);
    style.title_font = tree.font;
    style.subtitle_font = tree.font;
    style.description_font = tree.font;
    style.right_font = tree.font;
    style.icon_size = tree.icon_size;
    style.metadata_size = tree.metadata_size;
    style.metadata_gap = tree.metadata_gap;
    style.content_gap = tree.content_gap;
    style.show_image = false;
    style.show_icon = tree.show_icons;
    style.show_subtitle = false;
    style.show_description = false;
    style.show_right_text = true;
    style.show_metadata = tree.show_metadata_marker;
    style.muted_ink = tree.glyph_color;
    style.metadata_default = tree.glyph_color;

    for(int i = 0; i < 4; i++) {
        style.palette.face[i] = UiFill::None();
        style.palette.frame[i] = Null;
        style.palette.ink[i] = tree.ink;
        style.palette.icon[i] = tree.glyph_color;
    }
    style.palette.ink[ST_HOT] = tree.hot_ink;
    style.palette.icon[ST_HOT] = tree.glyph_hot_color;
    style.palette.ink[ST_PRESSED] = tree.selected_ink;
    style.palette.icon[ST_PRESSED] = tree.glyph_selected_color;
    style.palette.ink[ST_DISABLED] = tree.disabled_ink;
    style.palette.icon[ST_DISABLED] = tree.disabled_ink;
    basic.SetCustomStyle(style);
    return basic.Clone();
}

}

void UiTree::EnsureItemRender()
{
    if(!item_render_)
        ConfigureDefaultItemRender();
}

void UiTree::ConfigureDefaultItemRender()
{
    if(custom_item_render_)
        return;
    const Style& tree = has_custom_style_ ? style_ : themed_style_;
    item_render_ = MakeTreeBasicRender_(tree);
}

UiTree& UiTree::SetItemRender(const UiItemRender& render)
{
    item_render_ = render.Clone();
    custom_item_render_ = true;
    ResetRenderPools();
    RefreshLayout();
    Refresh();
    return *this;
}

const UiItemRender& UiTree::GetItemRender() const
{
    const_cast<UiTree *>(this)->EnsureItemRender();
    return *item_render_;
}

UiTree& UiTree::SetColumnRender(int column, const UiItemRender& render)
{
    if(column < 0)
        return *this;
    for(int i = 0; i < column_render_overrides_.GetCount(); i++) {
        if(column_render_overrides_[i].column == column) {
            column_render_overrides_[i].render = render.Clone();
            ResetRenderPools();
            RefreshLayout();
            Refresh();
            return *this;
        }
    }
    ColumnRenderOverride& item = column_render_overrides_.Add();
    item.column = column;
    item.render = render.Clone();
    ResetRenderPools();
    RefreshLayout();
    Refresh();
    return *this;
}

UiTree& UiTree::ClearColumnRender(int column)
{
    for(int i = column_render_overrides_.GetCount() - 1; i >= 0; i--)
        if(column_render_overrides_[i].column == column) {
            column_render_overrides_.Remove(i);
            ResetRenderPools();
            RefreshLayout();
            Refresh();
            break;
        }
    return *this;
}

const UiItemRender& UiTree::ResolveColumnRender(int column) const
{
    for(int i = 0; i < column_render_overrides_.GetCount(); i++)
        if(column_render_overrides_[i].column == column && column_render_overrides_[i].render)
            return *column_render_overrides_[i].render;
    return GetItemRender();
}

void UiTree::ResetRenderPools()
{
    item_render_pool_.Clear();
    column_render_pool_.Clear();
    prepared_render_range_ = UiVisibleRange();
    last_render_layout_count_ = 0;
}

void UiTree::PrepareItemRenders()
{
    EnsureItemRender();
    last_render_layout_count_ = 0;
    prepared_render_range_ = GetVisibleRange(1);
    if(!model_ || prepared_render_range_.IsEmpty())
        return;

    Rect viewport = GetViewportRect();
    int count = prepared_render_range_.GetCount();
    while(item_render_pool_.GetCount() < count) {
        ItemRenderSlot& slot = item_render_pool_.Add();
        slot.render = item_render_->Clone();
    }

    int column_slot = 0;
    for(int i = 0; i < count; i++) {
        int row = prepared_render_range_.first + i;
        ItemRenderSlot& slot = item_render_pool_[i];
        const VisibleRow& vr = visible_rows_[row];
        if(vr.placeholder || !model_->IsValid(UiTreeNodeRef{vr.id})) {
            slot.row = -1;
            continue;
        }

        if(slot.row != row) {
            UiItemRenderData data = UiMakeItemRenderData(model_->Get(UiTreeNodeRef{vr.id}));
            data.has_check = false;
            if(!GetEffectiveStyle().show_icons)
                data.icon = Image();
            if(!GetEffectiveStyle().show_metadata_marker)
                data.has_metadata = false;
            slot.render->SetData(data);
            slot.row = row;
        }
        Rect primary = GetItemContentRect(GetRowRect(row), vr.depth, vr.has_children, vr.id);
        if(slot.render->PrepareLayout(primary, UiDirection::H))
            last_render_layout_count_++;

        const UiModelItem& item = model_->Get(UiTreeNodeRef{vr.id});
        Vector<Rect> columns = GetColumnRects(GetRowRect(row), item);
        for(int col = 0; col < columns.GetCount() && col < item.columns.GetCount(); col++) {
            if(!columns[col].Intersects(viewport))
                continue;
            while(column_render_pool_.GetCount() <= column_slot)
                column_render_pool_.Add();
            ColumnRenderSlot& cslot = column_render_pool_[column_slot++];
            const UiItemRender& prototype = ResolveColumnRender(col);
            if(!cslot.render || cslot.prototype != &prototype) {
                cslot.render = prototype.Clone();
                cslot.prototype = &prototype;
                cslot.row = cslot.column = -1;
            }
            if(cslot.row != row || cslot.column != col) {
                cslot.render->SetData(UiMakeItemRenderData(item.columns[col], item.enabled));
                cslot.row = row;
                cslot.column = col;
            }
            if(cslot.render->PrepareLayout(columns[col], UiDirection::H))
                last_render_layout_count_++;
        }
    }

    for(int i = count; i < item_render_pool_.GetCount(); i++)
        item_render_pool_[i].row = -1;
    for(int i = column_slot; i < column_render_pool_.GetCount(); i++) {
        column_render_pool_[i].row = -1;
        column_render_pool_[i].column = -1;
    }
}

UiItemRender* UiTree::FindPreparedItemRender(int row)
{
    if(prepared_render_range_.IsEmpty() || !prepared_render_range_.Contains(row))
        return nullptr;
    int i = row - prepared_render_range_.first;
    if(i < 0 || i >= item_render_pool_.GetCount())
        return nullptr;
    ItemRenderSlot& slot = item_render_pool_[i];
    return slot.row == row ? slot.render.operator->() : nullptr;
}

const UiItemRender* UiTree::FindPreparedItemRender(int row) const
{
    return const_cast<UiTree *>(this)->FindPreparedItemRender(row);
}

UiItemRender* UiTree::FindPreparedColumnRender(int row, int column)
{
    for(int i = 0; i < column_render_pool_.GetCount(); i++) {
        ColumnRenderSlot& slot = column_render_pool_[i];
        if(slot.row == row && slot.column == column)
            return slot.render.operator->();
    }
    return nullptr;
}

const UiItemRender* UiTree::FindPreparedColumnRender(int row, int column) const
{
    return const_cast<UiTree *>(this)->FindPreparedColumnRender(row, column);
}

UiItemRenderState UiTree::GetItemRenderState(int row) const
{
    UiItemRenderState state;
    if(row < 0 || row >= visible_rows_.GetCount() || visible_rows_[row].placeholder || !model_)
        return state;
    const VisibleRow& vr = visible_rows_[row];
    const UiModelItem& item = model_->Get(UiTreeNodeRef{vr.id});
    state.enabled = IsEnabled() && IsShowEnabled() && item.enabled;
    state.selected = IsSelected(UiTreeNodeRef{vr.id});
    state.hot = hot_id_ == vr.id;
    state.pressed = pressed_ && state.hot;
    state.focused = HasFocus() && cursor_id_ == vr.id;
    return state;
}

void UiTree::PaintDropTarget(Draw& w, const Rect& viewport) const
{
    if(drop_parent_id_ < 0 || !model_)
        return;

    const Style& style = GetEffectiveStyle();
    Color c = IsNull(style.metrics.focus_color) ? Color(56, 146, 255) : style.metrics.focus_color;
    int h = max(DPI(2), DPI(3));
    int x = viewport.left + style.h_padding;
    int cx = max(DPI(24), viewport.GetWidth() - style.h_padding * 2);
    auto draw_line = [&](int y) {
        y = clamp(y, viewport.top + h / 2, viewport.bottom - h / 2);
        w.DrawRect(x, y - h / 2, cx, h, c);
    };

    if(drop_hover_id_ >= 0) {
        int row = FindVisibleRow(drop_hover_id_);
        if(row < 0)
            return;
        Rect rr = GetRowRect(row);
        if(!viewport.Intersects(rr))
            return;

        if(drop_into_) {
            Rect box = rr.Deflated(0, max(0, style.v_padding / 2));
            if(box.IsEmpty())
                box = rr;
            StyledPalette pal;
            StyledMetrics met;
            met.face_enabled = false;
            met.frame_enabled = true;
            met.frame_width = DPI(2);
            met.radius = style.row_radius;
            pal.frame[ST_NORMAL] = c;
            UiPaintFaceFrameDash(w, box, pal, met, ST_NORMAL);
        }
        else {
            UiTreeNodeRef hover{drop_hover_id_};
            int y = rr.top;
            if(model_->IsValid(hover) && drop_insert_pos_ > model_->GetChildIndex(hover))
                y = rr.bottom;
            draw_line(y);
        }
        return;
    }

    int y = viewport.top + style.metrics.content_margin.top;
    if(!visible_rows_.IsEmpty()) {
        Rect last = GetRowRect(visible_rows_.GetCount() - 1);
        y = min(viewport.bottom - h, max(viewport.top + h, last.bottom));
    }
    draw_line(y);
}

void UiTree::PaintChevron(Draw& w, const Rect& r, bool expanded, bool selected, bool hot) const
{
    const Style& style = GetEffectiveStyle();
    Color color = selected ? style.glyph_selected_color : (hot ? style.glyph_hot_color : style.glyph_color);
    if(IsNull(color) || r.IsEmpty())
        return;

    if(style.glyph_style == UITREEGLYPH_CUSTOM
       && !IsNull(expanded ? style.expanded_icon : style.collapsed_icon)) {
        UiPaintStyledIcon(w, r, expanded ? style.expanded_icon : style.collapsed_icon,
                          true, true, style.icon_render_mode, color, true);
        return;
    }

    int cx = (r.left + r.right - 1) / 2;
    int cy = (r.top + r.bottom - 1) / 2;
    if(style.glyph_style == UITREEGLYPH_PLUSMINUS) {
        Rect box = r;
        Color box_face = selected ? style.selected_face : hot ? style.hot_face : SColorPaper();
        if(style.palette.face[ST_NORMAL].IsSolid() && !selected && !hot)
            box_face = style.palette.face[ST_NORMAL].color;
        w.DrawRect(box, box_face);
        w.DrawRect(box.left, box.top, box.GetWidth(), 1, color);
        w.DrawRect(box.left, box.bottom - 1, box.GetWidth(), 1, color);
        w.DrawRect(box.left, box.top, 1, box.GetHeight(), color);
        w.DrawRect(box.right - 1, box.top, 1, box.GetHeight(), color);
        w.DrawRect(box.left + DPI(2), cy, max(1, box.GetWidth() - DPI(4)), 1, color);
        if(!expanded)
            w.DrawRect(cx, box.top + DPI(2), 1, max(1, box.GetHeight() - DPI(4)), color);
        return;
    }

    ImageBuffer ib(r.GetSize());
    BufferPainter p(ib, MODE_ANTIALIASED);
    p.Clear(RGBAZero());
    double w0 = max(1, r.GetWidth());
    double h0 = max(1, r.GetHeight());
    double stroke = style.glyph_style == UITREEGLYPH_THICK_CHEVRON ? 2.25 : 1.65;
    p.Begin();
    if(expanded) {
        p.Move(w0 * 0.22, h0 * 0.36);
        p.Line(w0 * 0.50, h0 * 0.66);
        p.Line(w0 * 0.78, h0 * 0.36);
    }
    else {
        p.Move(w0 * 0.36, h0 * 0.22);
        p.Line(w0 * 0.66, h0 * 0.50);
        p.Line(w0 * 0.36, h0 * 0.78);
    }
    p.Stroke(stroke, color);
    p.End();
    w.DrawImage(r.left, r.top, ib);
}

void UiTree::PaintRow(Draw& w, int index, const Rect& row) const
{
    if(index < 0 || index >= visible_rows_.GetCount())
        return;

    const Style& style = GetEffectiveStyle();
    const VisibleRow& vr = visible_rows_[index];
    if(vr.placeholder) {
        int x = row.left + style.h_padding + vr.depth * style.indent_px + style.content_gap;
        int y = row.top + (row.GetHeight() - style.font.GetHeight()) / 2;
        DrawSmartText(w, x, y, max(0, row.right - x - style.h_padding),
                      "Loading...", style.font, style.disabled_ink, 0);
        return;
    }

    UiTreeNodeRef node{vr.id};
    if(!model_->IsValid(node))
        return;
    const UiModelItem& item = model_->Get(node);
    bool is_cursor = cursor_id_ == vr.id;
    bool is_selected = IsSelected(node);
    bool is_hot = hot_id_ == vr.id;
    bool enabled = IsEnabled() && IsShowEnabled() && item.enabled;

    Rect row_box = row.Deflated(0, max(0, style.v_padding / 2));
    if(row_box.IsEmpty())
        row_box = row;
    if(item.separator_before && !IsNull(style.line_color))
        w.DrawLine(row.left, row.top, row.right, row.top, 1, style.line_color);

    if(is_selected || is_hot) {
        StyledPalette pal;
        StyledMetrics met;
        met.face_enabled = true;
        met.frame_enabled = true;
        met.frame_width = DPI(1);
        met.radius = style.row_radius;
        pal.face[ST_NORMAL] = UiFill::Solid(is_selected ? style.selected_face : style.hot_face);
        pal.frame[ST_NORMAL] = is_selected ? style.selected_frame : style.hot_frame;
        UiPaintFaceFrameDash(w, row_box, pal, met, ST_NORMAL);
    }

    Rect glyph = GetGlyphRect(row, vr.depth);
    if(style.show_connector_lines && !IsNull(style.line_color)) {
        int gx = glyph.left + glyph.GetWidth() / 2;
        int gy = glyph.top + glyph.GetHeight() / 2;
        auto branch_x = [&](int depth) {
            Rect gr = GetGlyphRect(row, depth);
            return gr.left + gr.GetWidth() / 2;
        };
        auto has_next_below_depth = [&](int depth) {
            return index + 1 < visible_rows_.GetCount() && visible_rows_[index + 1].depth > depth;
        };
        for(int depth = 0; depth < vr.depth; depth++)
            if(has_next_below_depth(depth)) {
                int x = branch_x(depth);
                w.DrawLine(x, row.top, x, row.bottom, 1, style.line_color);
            }
        if(vr.depth > 0) {
            int x0 = branch_x(vr.depth - 1);
            int x1 = vr.has_children ? glyph.left - DPI(1) : gx;
            if(x1 > x0)
                w.DrawLine(x0, gy, x1, gy, 1, style.line_color);
        }
        if(vr.has_children && vr.expanded)
            w.DrawLine(gx, glyph.bottom, gx, row.bottom, 1, style.line_color);
    }

    if(vr.has_children)
        PaintChevron(w, glyph, vr.expanded, is_selected || is_cursor, is_hot);

    UiItemRenderState state = GetItemRenderState(index);
    if(const UiItemRender* render = FindPreparedItemRender(index))
        render->Paint(w, state);

    Vector<Rect> columns = GetColumnRects(row, item);
    int column_count = min(columns.GetCount(), item.columns.GetCount());
    for(int col = 0; col < column_count; col++)
        if(const UiItemRender* render = FindPreparedColumnRender(index, col))
            render->Paint(w, state);

    if(HasFocus() && is_cursor && style.metrics.focus_enabled) {
        StyledMetrics focus = style.metrics;
        focus.face_enabled = false;
        focus.frame_enabled = true;
        focus.frame_width = max(DPI(1), style.metrics.focus_margin);
        UiPaintFocusShape(w, row_box, focus, ST_NORMAL,
                          IsNull(style.metrics.focus_color) ? Color(65, 167, 248) : style.metrics.focus_color,
                          0, style.metrics.focus_margin, style.metrics.focus_alpha,
                          style.metrics.focus_margin, max(1.0, (double)style.metrics.focus_margin));
    }
}

void UiTree::Paint(Draw& w)
{
    SyncModel();
    Rect outer = GetSize();
    if(outer.IsEmpty())
        return;
    const Style& style = GetEffectiveStyle();
    StyledState st = IsEnabled() && IsShowEnabled() ? ST_NORMAL : ST_DISABLED;
    UiPaintStyledBackground(w, outer, style.palette, style.metrics, style.skin, st, false);

    last_paint_item_count_ = 0;
    Rect vp = GetViewportRect();
    UiVisibleRange range = GetVisibleRange();
    if(!vp.IsEmpty() && !range.IsEmpty()) {
        w.Clip(vp);
        for(int i = range.first; i <= range.last; i++) {
            Rect rr = GetRowRect(i);
            if(!rr.Intersects(vp))
                continue;
            PaintRow(w, i, rr);
            last_paint_item_count_++;
        }
        PaintDropTarget(w, vp);
        w.End();
    }
    UiPaintStyledForeground(w, outer, style.palette, style.metrics, style.skin, st, false);
}

} // namespace Upp
