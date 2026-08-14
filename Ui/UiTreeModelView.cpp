#include <Ui/UiTree.h>

namespace Upp {

void UiTree::BindModel(UiTreeModel& model)
{
    for(int i = 0; i < bound_models_.GetCount(); i++)
        if(bound_models_[i] == &model)
            return;

    bound_models_.Add(&model);
    Ptr<UiTree> self = this;
    UiTreeModel* observed = &model;
    model.WhenChange << [self, observed](const UiModelChange&) {
        if(self && self->model_ == observed)
            self->HandleModelChange(UiModelChange());
    };
}

void UiTree::HandleModelChange(const UiModelChange&)
{
    model_revision_ = -1;
    SyncModel();
    ResetRenderPools();
    PrepareItemRenders();
    UpdateAttachedCtrls();
    RefreshLayout();
    Refresh();
}

void UiTree::ScrollTo(UiTreeNodeRef node)
{
    int row = FindVisibleRow(node.id);
    if(row < 0)
        return;
    Rect vp = GetViewportRect();
    if(vp.IsEmpty())
        return;

    int rh = max(DPI(18), GetEffectiveStyle().row_height);
    int64 top64 = (int64)row * rh;
    int top = top64 >= INT_MAX ? INT_MAX : (int)top64;
    int bottom = top > INT_MAX - rh ? INT_MAX : top + rh;
    if(top < scroll_y_)
        scroll_y_ = top;
    else if(bottom > scroll_y_ + vp.GetHeight())
        scroll_y_ = bottom - vp.GetHeight();

    ClampScroll();
    PrepareItemRenders();
    UpdateAttachedCtrls();
    Refresh();
}

void UiTree::ScrollToSelection()
{
    if(cursor_id_ >= 0)
        ScrollTo(UiTreeNodeRef{cursor_id_});
}

void UiTree::SyncModel()
{
    if(!model_)
        return;
    int revision = model_->GetRevision();
    if(model_revision_ == revision)
        return;

    model_revision_ = revision;
    RebuildVisibleRows();
    if(cursor_id_ >= 0 && !model_->IsValid(UiTreeNodeRef{cursor_id_}))
        cursor_id_ = visible_rows_.IsEmpty() ? -1 : visible_rows_[0].id;
    if(anchor_id_ >= 0 && !model_->IsValid(UiTreeNodeRef{anchor_id_}))
        anchor_id_ = cursor_id_;
    if(hot_id_ >= 0 && !model_->IsValid(UiTreeNodeRef{hot_id_}))
        hot_id_ = -1;
    if(editing_id_ >= 0 && !model_->IsValid(UiTreeNodeRef{editing_id_}))
        CancelRename();
    for(int i = loading_ids_.GetCount() - 1; i >= 0; i--)
        if(!model_->IsValid(UiTreeNodeRef{loading_ids_[i]}))
            loading_ids_.Remove(i);
    for(int i = selected_ids_.GetCount() - 1; i >= 0; i--)
        if(!model_->IsValid(UiTreeNodeRef{selected_ids_[i]}))
            selected_ids_.Remove(i);
    if(selection_mode_ == UITREESEL_SINGLE && selected_ids_.GetCount() > 1)
        SelectSingle(UiTreeNodeRef{cursor_id_});
    ClampScroll();
    UpdateAttachedCtrls();
}

void UiTree::EnsureLazyChildren(UiTreeNodeRef node)
{
    if(!model_ || !model_->IsValid(node))
        return;
    UiModelItem item = model_->Get(node);
    if(!item.lazy_children || item.lazy_loaded || IsNodeLoading(node))
        return;
    loading_ids_.FindAdd(node.id);
    if(WhenLazyLoad)
        WhenLazyLoad(node);
    if(model_->GetChildCount(node) > 0) {
        item = model_->Get(node);
        item.lazy_loaded = true;
        item.lazy_children = false;
        model_->Set(node, item);
        loading_ids_.RemoveKey(node.id);
    }
}

void UiTree::RebuildVisibleRows()
{
    visible_rows_.Clear();
    visible_row_ids_.Clear();
    visible_lookup_build_count_++;
    ResetRenderPools();
    if(!model_ || !model_->IsValid(model_->Root()))
        return;

    UiTreeNodeRef root = model_->Root();
    expanded_ids_.FindAdd(root.id);
    if(root_visible_)
        AddVisibleSubtree(root.id, 0);
    else {
        int n = model_->GetChildCount(root);
        for(int i = 0; i < n; i++)
            AddVisibleSubtree(model_->GetChild(root, i).id, 0);
    }
}

void UiTree::AddVisibleSubtree(int id, int depth)
{
    UiTreeNodeRef node{id};
    if(!model_->IsValid(node))
        return;

    VisibleRow& row = visible_rows_.Add();
    row.id = id;
    row.depth = depth;
    visible_row_ids_.FindAdd(id);
    const UiModelItem& item = model_->Get(node);
    row.has_children = model_->GetChildCount(node) > 0 || item.lazy_children || IsNodeLoading(node);
    row.expanded = row.has_children && IsExpanded(node);
    if(!row.expanded)
        return;

    int n = model_->GetChildCount(node);
    if(n == 0 && IsNodeLoading(node)) {
        VisibleRow& ph = visible_rows_.Add();
        ph.id = id;
        ph.depth = depth + 1;
        ph.placeholder = true;
        return;
    }
    for(int i = 0; i < n; i++)
        AddVisibleSubtree(model_->GetChild(node, i).id, depth + 1);
}

int UiTree::FindVisibleRow(int id) const
{
    return id >= 0 ? visible_row_ids_.Find(id) : -1;
}

int UiTree::GetVisibleRowIndex(UiTreeNodeRef node) const
{
    const_cast<UiTree *>(this)->SyncModel();
    return FindVisibleRow(node.id);
}

void UiTree::ClampScroll()
{
    Rect vp = GetViewportRect();
    int max_scroll = max(0, GetTotalHeight() - max(0, vp.GetHeight()));
    scroll_y_ = clamp(scroll_y_, 0, max_scroll);
}

Rect UiTree::GetViewportRect() const
{
    const Style& style = GetEffectiveStyle();
    return UiStyledInnerRect(GetSize(), style.metrics, style.skin);
}

int UiTree::GetTotalHeight() const
{
    int rh = max(DPI(18), GetEffectiveStyle().row_height);
    return UiUniformContentExtent(visible_rows_.GetCount(), rh, 0);
}

UiVisibleRange UiTree::GetVisibleRange(int overscan_rows) const
{
    Rect vp = GetViewportRect();
    return UiComputeLinearVisibleRange(visible_rows_.GetCount(), scroll_y_,
                                       max(0, vp.GetHeight()),
                                       max(DPI(18), GetEffectiveStyle().row_height),
                                       0, max(0, overscan_rows));
}

Rect UiTree::GetRowRect(int row) const
{
    Rect vp = GetViewportRect();
    int rh = max(DPI(18), GetEffectiveStyle().row_height);
    int64 y64 = (int64)vp.top - scroll_y_ + (int64)row * rh;
    int y = y64 <= INT_MIN ? INT_MIN : y64 >= INT_MAX ? INT_MAX : (int)y64;
    int bottom = y > INT_MAX - rh ? INT_MAX : y + rh;
    return Rect(vp.left, y, vp.right, bottom);
}

int UiTree::HitTestRow(Point p) const
{
    Rect vp = GetViewportRect();
    if(vp.IsEmpty() || !vp.Contains(p))
        return -1;
    int rh = max(DPI(18), GetEffectiveStyle().row_height);
    int64 local = (int64)p.y - vp.top + scroll_y_;
    if(local < 0)
        return -1;
    int64 row64 = local / rh;
    return row64 >= 0 && row64 < visible_rows_.GetCount() ? (int)row64 : -1;
}

UiTreeNodeRef UiTree::GetNodeAt(Point p) const
{
    int row = HitTestRow(p);
    return row >= 0 && !visible_rows_[row].placeholder
         ? UiTreeNodeRef{visible_rows_[row].id} : UiTreeNodeRef{-1};
}

UiTree::DropInfo UiTree::GetDropInfo() const
{
    DropInfo out;
    out.parent = UiTreeNodeRef{drop_parent_id_};
    out.hover = UiTreeNodeRef{drop_hover_id_};
    out.insert_pos = drop_insert_pos_;
    out.into = drop_into_;
    out.valid = drop_parent_id_ >= 0;
    return out;
}

UiTree::DropInfo UiTree::TrackDropTarget(Point p)
{
    DropTarget target = GetDropTarget(p);
    if(target.valid)
        SetDropTarget(target);
    else
        ClearDropTarget();
    return GetDropInfo();
}

void UiTree::ClearTrackedDropTarget()
{
    ClearDropTarget();
}

Rect UiTree::GetGlyphRect(const Rect& row, int depth) const
{
    const Style& style = GetEffectiveStyle();
    int left = row.left + style.h_padding + depth * style.indent_px;
    int side = max(DPI(8), style.glyph_size);
    int top = row.top + (row.GetHeight() - side) / 2;
    return RectC(left, top, side, side);
}

Rect UiTree::GetToggleHitRect(const Rect& row, int depth, bool has_children) const
{
    if(!has_children)
        return Rect(0, 0, 0, 0);
    const Style& style = GetEffectiveStyle();
    Rect glyph = GetGlyphRect(row, depth);
    return Rect(row.left, row.top,
                min(glyph.right + style.content_gap + style.branch_hit_extra, row.right),
                row.bottom);
}

Rect UiTree::GetIconRect(const Rect& row, int depth, bool has_glyph) const
{
    const Style& style = GetEffectiveStyle();
    Rect glyph = GetGlyphRect(row, depth);
    int side = max(DPI(12), style.icon_size);
    int left = has_glyph ? glyph.right + style.content_gap : glyph.left;
    int top = row.top + (row.GetHeight() - side) / 2;
    return RectC(left, top, side, side);
}

Rect UiTree::GetMetadataRect(const Rect& row, int depth, bool has_glyph, bool has_icon) const
{
    const Style& style = GetEffectiveStyle();
    Rect base = has_icon ? GetIconRect(row, depth, has_glyph) : GetGlyphRect(row, depth);
    int side = max(DPI(6), style.metadata_size);
    int left = base.right + style.metadata_gap;
    int top = row.top + (row.GetHeight() - side) / 2;
    return RectC(left, top, side, side);
}

Vector<Rect> UiTree::GetAccessoryRects(const Rect& row, int node_id) const
{
    Vector<Rect> out;
    int q = GetNodeCtrlIndex(UiTreeNodeRef{node_id});
    if(q < 0)
        return out;

    const Vector<Ptr<Ctrl>>& ctrls = node_ctrls_[q];
    const Style& style = GetEffectiveStyle();
    int right = row.right - style.h_padding;
    for(int i = ctrls.GetCount() - 1; i >= 0; i--) {
        Ptr<Ctrl> ctrl = ctrls[i];
        if(!ctrl) {
            out.Insert(0, Rect(0, 0, 0, 0));
            continue;
        }
        Size sz = ctrl->GetMinSize();
        int w = max(DPI(24), sz.cx);
        int h = min(max(DPI(20), sz.cy), max(DPI(20), row.GetHeight() - DPI(4)));
        int x = right - w;
        int y = row.top + (row.GetHeight() - h) / 2;
        out.Insert(0, RectC(x, y, w, h));
        right = x - style.accessory_gap;
    }
    return out;
}

Rect UiTree::GetAccessoryRect(const Rect& row, int node_id, int index) const
{
    Vector<Rect> ars = GetAccessoryRects(row, node_id);
    return index >= 0 && index < ars.GetCount() ? ars[index] : Rect(0, 0, 0, 0);
}

Vector<Rect> UiTree::GetColumnRects(const Rect& row, const UiModelItem& item) const
{
    Vector<Rect> out;
    const Style& style = GetEffectiveStyle();
    int count = min(column_widths_.GetCount(), item.columns.GetCount());
    int right = row.right - style.h_padding;
    for(int i = count - 1; i >= 0; i--) {
        int w = max(DPI(16), column_widths_[i]);
        int x = right - w;
        out.Insert(0, RectC(x, row.top, w, row.GetHeight()));
        right = x - style.accessory_gap;
    }
    return out;
}

int UiTree::HitTestColumn(const Rect& row, const UiModelItem& item, Point p) const
{
    Vector<Rect> cols = GetColumnRects(row, item);
    for(int i = 0; i < min(cols.GetCount(), item.columns.GetCount()); i++)
        if(cols[i].Contains(p))
            return i;
    return -1;
}

Rect UiTree::GetTextRect(const Rect& row, int depth, bool has_glyph,
                         bool has_icon, bool has_metadata, int node_id) const
{
    const Style& style = GetEffectiveStyle();
    Rect glyph = GetGlyphRect(row, depth);
    Rect icon = GetIconRect(row, depth, has_glyph);
    Rect metadata = GetMetadataRect(row, depth, has_glyph, has_icon);
    Vector<Rect> accessories = GetAccessoryRects(row, node_id);
    Vector<Rect> columns;
    UiTreeNodeRef node{node_id};
    if(model_ && model_->IsValid(node))
        columns = GetColumnRects(row, model_->Get(node));

    int left = glyph.left;
    if(has_metadata)
        left = metadata.right + style.content_gap;
    else if(has_icon)
        left = icon.right + style.content_gap;
    else if(has_glyph)
        left = glyph.right + style.content_gap;

    int right = row.right - style.h_padding;
    if(!accessories.IsEmpty())
        right = min(right, accessories[0].left - style.accessory_gap);
    if(!columns.IsEmpty())
        right = min(right, columns[0].left - style.accessory_gap);
    left = max(left, row.left + style.h_padding + depth * style.indent_px);
    return Rect(left, row.top, max(left, right), row.bottom);
}

Rect UiTree::GetItemContentRect(const Rect& row, int depth, bool has_glyph, int node_id) const
{
    const Style& style = GetEffectiveStyle();
    Rect glyph = GetGlyphRect(row, depth);
    int left = has_glyph ? glyph.right + style.content_gap : glyph.left;
    int right = row.right - style.h_padding;

    Vector<Rect> accessories = GetAccessoryRects(row, node_id);
    if(!accessories.IsEmpty())
        right = min(right, accessories[0].left - style.accessory_gap);
    UiTreeNodeRef node{node_id};
    if(model_ && model_->IsValid(node)) {
        Vector<Rect> columns = GetColumnRects(row, model_->Get(node));
        if(!columns.IsEmpty())
            right = min(right, columns[0].left - style.accessory_gap);
    }
    left = max(left, row.left + style.h_padding + depth * style.indent_px);
    return Rect(left, row.top, max(left, right), row.bottom);
}

void UiTree::UpdateAttachedCtrls()
{
    Rect vp = GetViewportRect();
    for(int i = 0; i < node_ctrls_.GetCount(); i++) {
        int node_id = node_ctrls_.GetKey(i);
        Vector<Ptr<Ctrl>>& ctrls = node_ctrls_[i];
        int row = FindVisibleRow(node_id);
        if(row < 0 || vp.IsEmpty()) {
            for(int j = 0; j < ctrls.GetCount(); j++)
                if(ctrls[j]) ctrls[j]->Hide();
            continue;
        }

        Rect rr = GetRowRect(row);
        if(rr.bottom <= vp.top || rr.top >= vp.bottom) {
            for(int j = 0; j < ctrls.GetCount(); j++)
                if(ctrls[j]) ctrls[j]->Hide();
            continue;
        }

        Vector<Rect> ars = GetAccessoryRects(rr, node_id);
        for(int j = 0; j < ctrls.GetCount(); j++) {
            if(!ctrls[j])
                continue;
            if(j >= ars.GetCount() || ars[j].IsEmpty())
                ctrls[j]->Hide();
            else {
                ctrls[j]->SetRect(ars[j]);
                ctrls[j]->Show();
            }
        }
    }

    if(editing_) {
        int row = FindVisibleRow(editing_id_);
        if(row < 0 || vp.IsEmpty()) {
            CancelRename();
            return;
        }
        const VisibleRow& vr = visible_rows_[row];
        Rect rr = GetRowRect(row);
        if(rr.bottom <= vp.top || rr.top >= vp.bottom || vr.placeholder) {
            CancelRename();
            return;
        }
        const UiModelItem& item = model_->Get(UiTreeNodeRef{editing_id_});
        bool has_icon = GetEffectiveStyle().show_icons && !IsNull(item.icon);
        bool has_metadata = GetEffectiveStyle().show_metadata_marker && item.has_metadata;
        Rect tr = GetTextRect(rr, vr.depth, vr.has_children, has_icon, has_metadata, vr.id);
        if(!tr.IsEmpty()) {
            inline_editor_.SetRect(tr.Deflated(0, DPI(2)));
            inline_editor_.Show();
        }
    }
}

void UiTree::Layout()
{
    SyncModel();
    ClampScroll();
    PrepareItemRenders();
    UpdateAttachedCtrls();
}

Size UiTree::GetContentSize() const
{
    const_cast<UiTree *>(this)->SyncModel();
    const Style& style = GetEffectiveStyle();
    Font font = style.font;
    int width = style.metrics.content_margin.left + style.metrics.content_margin.right
              + style.h_padding * 2 + DPI(24);
    int column_w = 0;
    for(int i = 0; i < column_widths_.GetCount(); i++)
        column_w += max(DPI(16), column_widths_[i]) + (i ? style.accessory_gap : 0);
    for(const VisibleRow& vr : visible_rows_) {
        if(vr.placeholder)
            continue;
        UiTreeNodeRef ref{vr.id};
        if(!model_ || !model_->IsValid(ref))
            continue;
        const UiModelItem& item = model_->Get(ref);
        int row_w = vr.depth * style.indent_px + style.glyph_size + style.content_gap;
        if(style.show_icons && !IsNull(item.icon))
            row_w += style.icon_size + style.content_gap;
        row_w += GetTextSize(item.text, item.use_custom_font ? item.custom_font : font).cx;
        if(!item.right_text.IsEmpty())
            row_w += style.content_gap
                   + GetTextSize(item.right_text, item.use_custom_font ? item.custom_font : font).cx;
        if(column_w > 0)
            row_w += style.accessory_gap + column_w;
        width = max(width, style.metrics.content_margin.left + style.metrics.content_margin.right
                         + style.h_padding * 2 + row_w);
    }
    width = max(width, style.metrics.content_margin.left + style.metrics.content_margin.right
                     + style.h_padding * 2 + style.indent_px * 3 + DPI(220));
    int height = style.metrics.content_margin.top + style.metrics.content_margin.bottom + GetTotalHeight();
    return Size(width, max(0, height));
}

Size UiTree::GetMinSize() const
{
    const_cast<UiTree *>(this)->SyncModel();
    const Style& style = GetEffectiveStyle();
    int sample_rows = max(1, min(2, visible_rows_.GetCount()));
    int width = style.metrics.content_margin.left + style.metrics.content_margin.right
              + style.h_padding * 2 + style.indent_px + DPI(120);
    int height = style.metrics.content_margin.top + style.metrics.content_margin.bottom
               + sample_rows * max(DPI(18), style.row_height);
    return UiStyledOuterSizeFromContent(Size(width, height), style.metrics, style.skin);
}

} // namespace Upp
