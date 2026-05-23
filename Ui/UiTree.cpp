#include <Ui/UiTree.h>
#include <Ui/UiTheme.h>

namespace Upp {

bool UiTree::InlineEditor::Key(dword key, int count)
{
    if(key == K_ENTER) {
        if(WhenAccept)
            WhenAccept();
        return true;
    }
    if(key == K_ESCAPE) {
        if(WhenAbort)
            WhenAbort();
        return true;
    }
    return EditString::Key(key, count);
}

void UiTree::InlineEditor::LostFocus()
{
    EditString::LostFocus();
    if(WhenBlur)
        WhenBlur();
}

static StyledState UiTreeState_(bool enabled, bool pressed, bool hot)
{
    if(!enabled)
        return ST_DISABLED;
    if(pressed)
        return ST_PRESSED;
    if(hot)
        return ST_HOT;
    return ST_NORMAL;
}

template <class RowT>
static int UiFindVisibleRowIndex(const Vector<RowT>& rows, int id)
{
    for(int i = 0; i < rows.GetCount(); i++)
        if(rows[i].id == id)
            return i;
    return -1;
}

const UiTree::Style& UiTree::StyleDefault()
{
    static Style s;
    ONCELOCK {
        const Color text_primary = Color(17, 24, 39);
        const Color text_muted = Color(148, 163, 184);
        const Color line = Color(203, 213, 225);
        const Color glyph = Color(100, 116, 139);

        for(int i = 0; i < 4; i++) {
            s.palette.face[i] = UiFill::Solid(White());
            s.palette.frame[i] = Color(226, 232, 240);
            s.palette.ink[i] = text_primary;
            s.palette.icon[i] = glyph;
        }

        s.palette.face[ST_HOT] = UiFill::Solid(Color(248, 250, 252));
        s.palette.face[ST_PRESSED] = UiFill::Solid(Color(241, 245, 249));
        s.palette.face[ST_DISABLED] = UiFill::Solid(Color(248, 250, 252));
        s.palette.frame[ST_DISABLED] = Color(241, 245, 249);
        s.palette.ink[ST_DISABLED] = text_muted;
        s.palette.icon[ST_DISABLED] = text_muted;

        s.metrics = StyledMetrics();
        s.metrics.text_font = StdFont();
        s.metrics.use_text_font = false;
        s.metrics.face_enabled = true;
        s.metrics.frame_enabled = true;
        s.metrics.frame_width = DPI(1);
        s.metrics.radius = 0;
        s.metrics.content_margin = Rect(DPI(8), DPI(8), DPI(8), DPI(8));
        s.metrics.focus_enabled = true;
        s.metrics.focus_margin = DPI(2);
        s.metrics.focus_alpha = 180;
        s.metrics.focus_color = Color(65, 167, 248);
        s.skin = StyledSkin();

        s.font = StdFont();
        s.row_height = DPI(26);
        s.indent_px = DPI(18);
        s.glyph_size = DPI(12);
        s.icon_size = DPI(16);
        s.content_gap = DPI(6);
        s.item_spacing = 0;
        s.h_padding = DPI(6);
        s.v_padding = DPI(4);
        s.row_radius = DPI(4);
        s.branch_hit_extra = DPI(10);
        s.metadata_size = DPI(8);
        s.metadata_gap = DPI(6);
        s.accessory_gap = DPI(8);
        s.show_icons = true;
        s.show_connector_lines = false;
        s.show_metadata_marker = true;
        s.glyph_style = UITREEGLYPH_THICK_CHEVRON;
        s.icon_render_mode = UiIconRenderMode::MonoTint;

        s.ink = text_primary;
        s.disabled_ink = text_muted;
        s.hot_face = Color(245, 247, 250);
        s.hot_frame = Color(226, 232, 240);
        s.hot_ink = text_primary;
        s.selected_face = Color(232, 242, 255);
        s.selected_frame = Color(65, 167, 248);
        s.selected_ink = text_primary;
        s.line_color = line;
        s.glyph_color = glyph;
        s.glyph_hot_color = Color(71, 85, 105);
        s.glyph_selected_color = Color(37, 99, 235);
    }
    return s;
}

UiTree::UiTree()
    : style_(StyleDefault())
    , themed_style_(StyleDefault())
    , model_(&internal_model_)
{
    BackPaint();
    WantFocus();
    expanded_ids_.FindAdd(internal_model_.Root().id);

    inline_editor_.Hide();
    inline_editor_.WhenAccept = [=] { CommitRename(); };
    inline_editor_.WhenAbort = [=] { CancelRename(); };
    inline_editor_.WhenBlur = [=] {
        if(editing_)
            CommitRename();
    };
    Add(inline_editor_);

    SyncThemeStyle();
    SyncModel();
}

UiTree::Style& UiTree::StyleEdit()
{
    if(!has_custom_style_) {
        style_ = GetEffectiveStyle();
        has_custom_style_ = true;
    }
    theme_revision_ = 0;
    return style_;
}

const UiTree::Style& UiTree::GetEffectiveStyle() const
{
    if(has_custom_style_)
        return style_;
    const_cast<UiTree*>(this)->SyncThemeStyle();
    return themed_style_;
}

void UiTree::SyncThemeStyle()
{
    if(has_custom_style_)
        return;
    uint64 revision = UiTheme::GetRevision();
    if(theme_revision_ == revision)
        return;
    themed_style_ = UiTheme::ResolveTree();
    theme_revision_ = revision;
}

UiTree& UiTree::SetCustomStyle(const Style& s)
{
    style_ = s;
    has_custom_style_ = true;
    OnStyleChanged();
    return *this;
}

UiTree& UiTree::ClearCustomStyle()
{
    if(!has_custom_style_)
        return *this;
    has_custom_style_ = false;
    style_ = StyleDefault();
    theme_revision_ = 0;
    OnStyleChanged();
    return *this;
}

void UiTree::OnStyleChanged()
{
    UpdateAttachedCtrls();
    RefreshLayout();
    Refresh();
}

UiTree& UiTree::SetModel(UiTreeModel& model)
{
    CancelRename();
    model_ = &model;
    model_revision_ = -1;
    expanded_ids_.Clear();
    selected_ids_.Clear();
    if(model_->IsValid(model_->Root()))
        expanded_ids_.FindAdd(model_->Root().id);
    cursor_id_ = -1;
    anchor_id_ = -1;
    hot_id_ = -1;
    scroll_y_ = 0;
    SyncModel();
    UpdateAttachedCtrls();
    RefreshLayout();
    Refresh();
    return *this;
}

UiTree& UiTree::SetRootVisible(bool on)
{
    if(root_visible_ == on)
        return *this;
    root_visible_ = on;
    if(model_ && model_->IsValid(model_->Root()))
        expanded_ids_.FindAdd(model_->Root().id);
    RebuildVisibleRows();
    ClampScroll();
    UpdateAttachedCtrls();
    RefreshLayout();
    Refresh();
    return *this;
}

UiTree& UiTree::SetSelectionMode(UiTreeSelectionMode mode)
{
    if(selection_mode_ == mode)
        return *this;
    selection_mode_ = mode;
    if(selection_mode_ == UITREESEL_SINGLE && selected_ids_.GetCount() > 1)
        SelectSingle(UiTreeNodeRef{cursor_id_});
    Refresh();
    return *this;
}

UiTree& UiTree::ClearSelection()
{
    if(selected_ids_.IsEmpty())
        return *this;
    selected_ids_.Clear();
    cursor_id_ = -1;
    anchor_id_ = -1;
    NotifySelectionChange();
    return *this;
}

UiTree& UiTree::SelectNode(UiTreeNodeRef node, bool additive)
{
    if(!IsSelectableNode(node))
        return *this;
    if(selection_mode_ != UITREESEL_MULTI || !additive)
        SelectSingle(node);
    else
        ToggleSelection(node);
    return *this;
}
UiTree& UiTree::SelectAllVisible()
{
    if(selection_mode_ != UITREESEL_MULTI) {
        for(int i = 0; i < visible_rows_.GetCount(); i++) {
            UiTreeNodeRef node{visible_rows_[i].id};
            if(IsSelectableNode(node)) {
                SelectSingle(node);
                return *this;
            }
        }
        ClearSelection();
        return *this;
    }
    selected_ids_.Clear();
    for(int i = 0; i < visible_rows_.GetCount(); i++)
        if(IsSelectableNode(UiTreeNodeRef{visible_rows_[i].id}))
            selected_ids_.FindAdd(visible_rows_[i].id);
    cursor_id_ = selected_ids_.IsEmpty() ? -1 : selected_ids_[0];
    anchor_id_ = cursor_id_;
    NotifySelectionChange();
    return *this;
}

bool UiTree::IsSelected(UiTreeNodeRef node) const
{
    return node.IsValid() && selected_ids_.Find(node.id) >= 0;
}

Vector<UiTreeNodeRef> UiTree::GetSelection() const
{
    Vector<UiTreeNodeRef> out;
    out.Reserve(selected_ids_.GetCount());
    for(int i = 0; i < visible_rows_.GetCount(); i++) {
        int id = visible_rows_[i].id;
        if(selected_ids_.Find(id) >= 0)
            out.Add(UiTreeNodeRef{id});
    }
    for(int i = 0; i < selected_ids_.GetCount(); i++) {
        int id = selected_ids_[i];
        if(UiFindVisibleRowIndex(visible_rows_, id) < 0 && model_ && model_->IsValid(UiTreeNodeRef{id}))
            out.Add(UiTreeNodeRef{id});
    }
    return out;
}

void UiTree::SetData(const Value& v)
{
    SyncModel();

    if(IsNull(v)) {
        ClearSelection();
        return;
    }

    if(selection_mode_ == UITREESEL_MULTI || v.Is<ValueArray>()) {
        selected_ids_.Clear();
        ValueArray values;
        if(v.Is<ValueArray>())
            values = v;
        else
            values.Add(v);

        for(int i = 0; i < values.GetCount(); i++) {
            UiTreeNodeRef node = ResolveSelectionNode(values[i]);
            if(model_ && model_->IsValid(node))
                selected_ids_.FindAdd(node.id);
        }

        Vector<UiTreeNodeRef> selection = GetSelection();
        anchor_id_ = selection.IsEmpty() ? -1 : selection[0].id;
        cursor_id_ = selection.IsEmpty() ? -1 : selection.Top().id;
        NotifySelectionChange();
        return;
    }

    UiTreeNodeRef node = ResolveSelectionNode(v);
    if(IsSelectableNode(node))
        SelectSingle(node);
    else
        ClearSelection();
}

Value UiTree::GetData() const
{
    if(selection_mode_ == UITREESEL_MULTI) {
        ValueArray values;
        Vector<UiTreeNodeRef> selection = GetSelection();
        for(int i = 0; i < selection.GetCount(); i++)
            values.Add(GetSelectionToken(selection[i]));
        return values;
    }

    return selected_ids_.GetCount() > 0 ? GetSelectionToken(UiTreeNodeRef{selected_ids_[0]}) : Value();
}
bool UiTree::CanMoveSelection(UiTreeNodeRef new_parent, int pos) const
{
    return CanMoveNodes(GetSelection(), new_parent, pos);
}

bool UiTree::MoveSelection(UiTreeNodeRef new_parent, int pos)
{
    return MoveNodes(GetSelection(), new_parent, pos);
}

UiTree& UiTree::EnableDragDrop(bool on)
{
    if(dnd_enabled_ == on)
        return *this;
    dnd_enabled_ = on;
    if(!on) {
        drag_id_ = -1;
        ClearDropTarget();
    }
    return *this;
}
UiTree& UiTree::SetGlyphStyle(UiTreeGlyphStyle style)
{
    StyleEdit().glyph_style = style;
    OnStyleChanged();
    return *this;
}

UiTree& UiTree::SetGlyphImages(const Image& collapsed, const Image& expanded, UiIconRenderMode render_mode)
{
    Style& st = StyleEdit();
    st.collapsed_icon = collapsed;
    st.expanded_icon = expanded;
    st.icon_render_mode = render_mode;
    st.glyph_style = UITREEGLYPH_CUSTOM;
    OnStyleChanged();
    return *this;
}

UiTree& UiTree::ShowConnectorLines(bool on)
{
    StyleEdit().show_connector_lines = on;
    OnStyleChanged();
    return *this;
}

UiTree& UiTree::ShowMetadataMarker(bool on)
{
    StyleEdit().show_metadata_marker = on;
    OnStyleChanged();
    return *this;
}

UiTree& UiTree::EnableRenameOnDblClick(bool on)
{
    rename_on_dblclick_ = on;
    if(!on)
        CancelRename();
    return *this;
}

UiTree& UiTree::SetNodeLoading(UiTreeNodeRef node, bool on)
{
    if(!model_ || !model_->IsValid(node))
        return *this;
    if(on) {
        loading_ids_.FindAdd(node.id);
        EnsureLazyChildren(node);
    }
    else
        loading_ids_.RemoveKey(node.id);
    RefreshLayout();
    Refresh();
    return *this;
}

bool UiTree::IsNodeLoading(UiTreeNodeRef node) const
{
    return node.IsValid() && loading_ids_.Find(node.id) >= 0;
}

UiTree& UiTree::MarkNodeChildrenLoaded(UiTreeNodeRef node, bool loaded)
{
    if(!model_ || !model_->IsValid(node))
        return *this;
    UiModelItem item = model_->Get(node);
    item.lazy_loaded = loaded;
    if(loaded)
        item.lazy_children = false;
    model_->Set(node, item);
    loading_ids_.RemoveKey(node.id);
    return *this;
}

UiTree& UiTree::SetColumnWidths(const Vector<int>& widths)
{
    column_widths_ = clone(widths);
    Refresh();
    return *this;
}

UiTree& UiTree::ClearColumnWidths()
{
    column_widths_.Clear();
    Refresh();
    return *this;
}

int UiTree::GetNodeCtrlIndex(UiTreeNodeRef node) const
{
    return node_ctrls_.Find(node.id);
}

UiTree& UiTree::AddNodeCtrl(UiTreeNodeRef node, Ctrl& ctrl)
{
    if(!model_ || !model_->IsValid(node))
        return *this;

    int q = GetNodeCtrlIndex(node);
    if(q < 0) {
        Vector<Ptr<Ctrl>> v;
        v.Add(&ctrl);
        node_ctrls_.Add(node.id, pick(v));
    }
    else {
        Vector<Ptr<Ctrl>>& v = node_ctrls_[q];
        bool found = false;
        for(int i = 0; i < v.GetCount(); i++)
            if(v[i] == &ctrl)
                found = true;
        if(!found)
            v.Add(&ctrl);
    }

    if(ctrl.GetParent() != this)
        Add(ctrl);
    ctrl.Show();
    UpdateAttachedCtrls();
    Refresh();
    return *this;
}

UiTree& UiTree::SetNodeCtrl(UiTreeNodeRef node, Ctrl& ctrl)
{
    ClearNodeCtrls(node);
    return AddNodeCtrl(node, ctrl);
}

UiTree& UiTree::ClearNodeCtrls(UiTreeNodeRef node)
{
    int q = GetNodeCtrlIndex(node);
    if(q >= 0) {
        Vector<Ptr<Ctrl>>& v = node_ctrls_[q];
        for(int i = 0; i < v.GetCount(); i++)
            if(v[i])
                v[i]->Hide();
        node_ctrls_.Remove(q);
        UpdateAttachedCtrls();
        Refresh();
    }
    return *this;
}

UiTree& UiTree::ClearNodeCtrl(UiTreeNodeRef node)
{
    return ClearNodeCtrls(node);
}

Ctrl* UiTree::GetNodeCtrl(UiTreeNodeRef node, int index) const
{
    int q = GetNodeCtrlIndex(node);
    if(q < 0)
        return nullptr;
    const Vector<Ptr<Ctrl>>& v = node_ctrls_[q];
    return index >= 0 && index < v.GetCount() && v[index] ? (Ctrl*)v[index] : nullptr;
}

int UiTree::GetNodeCtrlCount(UiTreeNodeRef node) const
{
    int q = GetNodeCtrlIndex(node);
    return q >= 0 ? node_ctrls_[q].GetCount() : 0;
}

bool UiTree::IsExpanded(UiTreeNodeRef node) const
{
    return model_ && model_->IsValid(node) && expanded_ids_.Find(node.id) >= 0;
}

UiTree& UiTree::Expand(UiTreeNodeRef node, bool on, bool recursive)
{
    if(!model_ || !model_->IsValid(node))
        return *this;
    if(on) {
        expanded_ids_.FindAdd(node.id);
        EnsureLazyChildren(node);
    }
    else if(node.id != model_->Root().id)
        expanded_ids_.RemoveKey(node.id);

    if(recursive) {
        int n = model_->GetChildCount(node);
        for(int i = 0; i < n; i++)
            Expand(model_->GetChild(node, i), on, true);
    }

    RebuildVisibleRows();
    ClampScroll();
    UpdateAttachedCtrls();
    RefreshLayout();
    Refresh();
    return *this;
}

UiTree& UiTree::Toggle(UiTreeNodeRef node)
{
    if(!model_ || !model_->IsValid(node))
        return *this;
    const UiModelItem& item = model_->Get(node);
    bool expandable = model_->GetChildCount(node) > 0 || item.lazy_children || IsNodeLoading(node);
    if(!expandable)
        return *this;
    return Expand(node, !IsExpanded(node), false);
}
UiTree& UiTree::SetCursor(UiTreeNodeRef node)
{
    int new_id = (model_ && model_->IsValid(node)) ? node.id : -1;
    if(cursor_id_ == new_id)
        return *this;
    cursor_id_ = new_id;
    ScrollToSelection();
    Refresh();
    return *this;
}

void UiTree::ScrollTo(UiTreeNodeRef node)
{
    int row = UiFindVisibleRowIndex(visible_rows_, node.id);
    if(row < 0)
        return;
    Rect vp = GetViewportRect();
    if(vp.IsEmpty())
        return;

    int rh = max(DPI(18), GetEffectiveStyle().row_height);
    int top = row * rh;
    int bottom = top + rh;
    if(top < scroll_y_)
        scroll_y_ = top;
    else if(bottom > scroll_y_ + vp.GetHeight())
        scroll_y_ = bottom - vp.GetHeight();

    ClampScroll();
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
    int count = visible_rows_.GetCount();
    if(count <= 0)
        return 0;
    int rh = max(DPI(18), GetEffectiveStyle().row_height);
    return count * rh;
}

Rect UiTree::GetRowRect(int row) const
{
    Rect vp = GetViewportRect();
    int rh = max(DPI(18), GetEffectiveStyle().row_height);
    int y = vp.top + row * rh - scroll_y_;
    return Rect(vp.left, y, vp.right, y + rh);
}

int UiTree::HitTestRow(Point p) const
{
    Rect vp = GetViewportRect();
    if(vp.IsEmpty() || !vp.Contains(p))
        return -1;
    int rh = max(DPI(18), GetEffectiveStyle().row_height);
    int row = (p.y - vp.top + scroll_y_) / rh;
    return row >= 0 && row < visible_rows_.GetCount() ? row : -1;
}

UiTreeNodeRef UiTree::GetNodeAt(Point p) const
{
    int row = HitTestRow(p);
    return row >= 0 ? UiTreeNodeRef{visible_rows_[row].id} : UiTreeNodeRef{-1};
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
    return Rect(row.left, row.top, min(glyph.right + style.content_gap + style.branch_hit_extra, row.right), row.bottom);
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

Rect UiTree::GetTextRect(const Rect& row, int depth, bool has_glyph, bool has_icon, bool has_metadata, int node_id) const
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

void UiTree::PaintItemColumns(Draw& w, const Rect& row, const UiModelItem& item, bool enabled, bool selected) const
{
    Vector<Rect> cols = GetColumnRects(row, item);
    if(cols.IsEmpty())
        return;

    const Style& style = GetEffectiveStyle();
    Font font = item.use_custom_font ? item.custom_font : style.font;
    font.Height(max(DPI(8), font.GetHeight() - DPI(1)));
    Color fallback = selected ? style.glyph_selected_color : (enabled ? style.glyph_color : style.disabled_ink);
    int count = min(cols.GetCount(), item.columns.GetCount());
    for(int i = 0; i < count; i++) {
        const UiModelColumn& c = item.columns[i];
        Rect r = cols[i];
        Color ink = IsNull(c.ink) ? fallback : c.ink;
        if(!IsNull(c.icon)) {
            int side = min(max(DPI(10), style.icon_size - DPI(2)), max(0, min(r.GetWidth(), r.GetHeight()) - DPI(4)));
            Rect ir = RectC(r.left + (r.GetWidth() - side) / 2, r.top + (r.GetHeight() - side) / 2, side, side);
            UiPaintStyledIcon(w, ir, c.icon, true, true, c.icon_render_mode, ink, enabled);
        }
        else if(!c.text.IsEmpty()) {
            Size ts = GetTextSize(c.text, font);
            int x = c.align == ALIGN_RIGHT ? r.right - ts.cx - DPI(1) :
                    c.align == ALIGN_CENTER ? r.left + (r.GetWidth() - ts.cx) / 2 :
                    r.left + DPI(1);
            int y = r.top + (r.GetHeight() - font.GetHeight()) / 2;
            DrawSmartText(w, x, y, max(0, r.GetWidth()), c.text, font, ink, 0);
        }
    }
}

void UiTree::PaintChevron(Draw& w, const Rect& r, bool expanded, bool selected, bool hot) const
{
    const Style& style = GetEffectiveStyle();
    Color color = selected ? style.glyph_selected_color : (hot ? style.glyph_hot_color : style.glyph_color);
    if(IsNull(color) || r.IsEmpty())
        return;

    if(style.glyph_style == UITREEGLYPH_CUSTOM && !IsNull(expanded ? style.expanded_icon : style.collapsed_icon)) {
        UiPaintStyledIcon(w, r, expanded ? style.expanded_icon : style.collapsed_icon, true, true, style.icon_render_mode, color, true);
        return;
    }

    int cx = (r.left + r.right - 1) / 2;
    int cy = (r.top + r.bottom - 1) / 2;

    if(style.glyph_style == UITREEGLYPH_PLUSMINUS) {
        Rect box = r.Deflated(0, 0);
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
        DrawSmartText(w, x, y, max(0, row.right - x - style.h_padding), "Loading...", style.font, style.disabled_ink, 0);
        return;
    }

    UiTreeNodeRef node{vr.id};
    if(!model_->IsValid(node))
        return;

    const UiModelItem& item = model_->Get(node);
    bool is_cursor = (cursor_id_ == vr.id);
    bool is_selected = IsSelected(node);
    bool is_hot = (hot_id_ == vr.id);
    bool enabled = IsEnabled() && IsShowEnabled() && item.enabled;
    StyledState st = UiTreeState_(enabled, pressed_ && is_hot, is_hot);

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

    bool has_icon = style.show_icons && !IsNull(item.icon);
    if(has_icon) {
        Rect ir = GetIconRect(row, vr.depth, vr.has_children);
        Color icon_ink = item.custom_ink_color;
        if(IsNull(icon_ink)) {
            icon_ink = IsNull(style.palette.icon[st]) ? style.glyph_color : style.palette.icon[st];
            if(is_selected)
                icon_ink = style.glyph_selected_color;
        }
        UiPaintStyledIcon(w, ir, item.icon, true, true, item.icon_render_mode, icon_ink, enabled);
    }

    bool has_metadata = style.show_metadata_marker && item.has_metadata;
    if(has_metadata) {
        Rect mr = GetMetadataRect(row, vr.depth, vr.has_children, has_icon);
        Color mc = IsNull(item.metadata_color) ? style.glyph_color : item.metadata_color;
        w.DrawRect(mr, mc);
    }

    Rect text_r = GetTextRect(row, vr.depth, vr.has_children, has_icon, has_metadata, vr.id);
    Rect left_text_r = text_r;
    PaintItemColumns(w, row, item, enabled, is_selected);
    Font right_font = item.use_custom_font ? item.custom_font : style.font;
    if(!item.right_text.IsEmpty()) {
        Size rsz = GetTextSize(item.right_text, right_font);
        Rect rr = RectC(max(text_r.left, text_r.right - rsz.cx), row.top, rsz.cx, row.GetHeight());
        Color rink = enabled ? style.glyph_color : style.disabled_ink;
        int ry = rr.top + (rr.GetHeight() - right_font.GetHeight()) / 2;
        DrawSmartText(w, rr.left, ry, max(0, rr.GetWidth()), item.right_text, right_font, rink, 0);
        left_text_r.right = max(left_text_r.left, rr.left - style.content_gap);
    }

    Color ink = item.custom_ink_color;
    if(IsNull(ink))
        ink = is_selected ? style.selected_ink : (enabled ? style.ink : style.disabled_ink);
    Font font = item.use_custom_font ? item.custom_font : style.font;
    if(item.group_header)
        font.Bold();
    int ty = left_text_r.top + (left_text_r.GetHeight() - font.GetHeight()) / 2;
    DrawSmartText(w, left_text_r.left, ty, max(0, left_text_r.GetWidth()), item.text, font, ink, 0);

    if(item.underline && left_text_r.GetWidth() > 0) {
        Color uc = IsNull(item.underline_color) ? ink : item.underline_color;
        int uy = ty + font.GetHeight() + DPI(1);
        w.DrawRect(left_text_r.left, uy, max(1, min(left_text_r.GetWidth(), GetTextSize(item.text, font).cx)), 1, uc);
    }

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
void UiTree::MoveCursorBy(int delta)
{
    if(visible_rows_.IsEmpty()) {
        SetCursor(UiTreeNodeRef{-1});
        return;
    }
    int row = UiFindVisibleRowIndex(visible_rows_, cursor_id_);
    if(row < 0)
        row = 0;
    else
        row = clamp(row + delta, 0, visible_rows_.GetCount() - 1);
    while(row >= 0 && row < visible_rows_.GetCount() && !IsSelectableNode(UiTreeNodeRef{visible_rows_[row].id}))
        row += delta >= 0 ? 1 : -1;
    if(row >= 0 && row < visible_rows_.GetCount())
        SetCursor(UiTreeNodeRef{visible_rows_[row].id});
}

void UiTree::MoveCursorToEdge(bool end)
{
    if(visible_rows_.IsEmpty()) {
        SetCursor(UiTreeNodeRef{-1});
        return;
    }
    int row = end ? visible_rows_.GetCount() - 1 : 0;
    while(row >= 0 && row < visible_rows_.GetCount() && !IsSelectableNode(UiTreeNodeRef{visible_rows_[row].id}))
        row += end ? -1 : 1;
    if(row >= 0 && row < visible_rows_.GetCount())
        SetCursor(UiTreeNodeRef{visible_rows_[row].id});
}

bool UiTree::CommitRenameIfNeeded(Point p)
{
    if(!editing_)
        return false;
    if(inline_editor_.IsShown() && inline_editor_.GetRect().Contains(p))
        return false;
    CommitRename();
    return true;
}

void UiTree::BeginRename(UiTreeNodeRef node)
{
    if(!rename_on_dblclick_ || !model_ || !model_->IsValid(node))
        return;
    const UiModelItem& item = model_->Get(node);
    if(!item.editable)
        return;
    editing_ = true;
    editing_id_ = node.id;
    inline_editor_.SetData(item.text);
    inline_editor_.Show();
    UpdateAttachedCtrls();
    inline_editor_.SetFocus();
    inline_editor_.SetSelection(0, item.text.GetCount());
}

void UiTree::CommitRename()
{
    if(!editing_ || !model_ || !model_->IsValid(UiTreeNodeRef{editing_id_})) {
        CancelRename();
        return;
    }
    UiTreeNodeRef node{editing_id_};
    UiModelItem item = model_->Get(node);
    String text = AsString(inline_editor_.GetData());
    if(item.text != text) {
        item.text = text;
        model_->Set(node, item);
        if(WhenRename)
            WhenRename(node, text);
    }
    editing_ = false;
    editing_id_ = -1;
    inline_editor_.Hide();
    Refresh();
}

void UiTree::CancelRename()
{
    editing_ = false;
    editing_id_ = -1;
    inline_editor_.Hide();
}

void UiTree::UpdateAttachedCtrls()
{
    Rect vp = GetViewportRect();
    for(int i = 0; i < node_ctrls_.GetCount(); i++) {
        int node_id = node_ctrls_.GetKey(i);
        Vector<Ptr<Ctrl>>& ctrls = node_ctrls_[i];
        int row = UiFindVisibleRowIndex(visible_rows_, node_id);
        if(row < 0 || vp.IsEmpty()) {
            for(int j = 0; j < ctrls.GetCount(); j++)
                if(ctrls[j])
                    ctrls[j]->Hide();
            continue;
        }

        Rect rr = GetRowRect(row);
        if(rr.bottom <= vp.top || rr.top >= vp.bottom) {
            for(int j = 0; j < ctrls.GetCount(); j++)
                if(ctrls[j])
                    ctrls[j]->Hide();
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
        int row = UiFindVisibleRowIndex(visible_rows_, editing_id_);
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

void UiTree::Paint(Draw& w)
{
    SyncModel();
    Rect outer = GetSize();
    if(outer.IsEmpty())
        return;
    const Style& style = GetEffectiveStyle();
    StyledState st = IsEnabled() && IsShowEnabled() ? ST_NORMAL : ST_DISABLED;
    UiPaintStyledBackground(w, outer, style.palette, style.metrics, style.skin, st, false);
    Rect vp = GetViewportRect();
    if(!vp.IsEmpty()) {
        w.Clip(vp);
        for(int i = 0; i < visible_rows_.GetCount(); i++) {
            Rect rr = GetRowRect(i);
            if(rr.bottom <= vp.top)
                continue;
            if(rr.top >= vp.bottom)
                break;
            PaintRow(w, i, rr);
        }
        w.End();
    }
    UiPaintStyledForeground(w, outer, style.palette, style.metrics, style.skin, st, false);
}

void UiTree::Layout()
{
    SyncModel();
    ClampScroll();
    UpdateAttachedCtrls();
}

Size UiTree::GetContentSize() const
{
    const_cast<UiTree *>(this)->SyncModel();
    const Style& style = GetEffectiveStyle();
    int width = style.metrics.content_margin.left + style.metrics.content_margin.right + style.h_padding * 2 + style.indent_px * 3 + DPI(220);
    int height = style.metrics.content_margin.top + style.metrics.content_margin.bottom + GetTotalHeight();
    return Size(width, max(0, height));
}

Size UiTree::GetMinSize() const
{
    const_cast<UiTree *>(this)->SyncModel();
    const Style& style = GetEffectiveStyle();
    int sample_rows = max(3, min(8, visible_rows_.GetCount()));
    int width = style.metrics.content_margin.left + style.metrics.content_margin.right + style.h_padding * 2 + style.indent_px * 3 + DPI(220);
    int height = style.metrics.content_margin.top + style.metrics.content_margin.bottom + sample_rows * max(DPI(18), style.row_height);
    return UiStyledOuterSizeFromContent(Size(width, height), style.metrics, style.skin);
}

void UiTree::LeftDown(Point p, dword flags)
{
    CommitRenameIfNeeded(p);
    SetFocus();
    pressed_ = true;
    drag_id_ = -1;
    int row = HitTestRow(p);
    if(row >= 0) {
        const VisibleRow& vr = visible_rows_[row];
        if(vr.placeholder) {
            Refresh();
            return;
        }
        UiTreeNodeRef node{vr.id};
        Rect rr = GetRowRect(row);
        bool toggle_hit = vr.has_children && GetToggleHitRect(rr, vr.depth, vr.has_children).Contains(p);
        if(toggle_hit)
            Toggle(node);

        if(!IsSelectableNode(node)) {
            Refresh();
            return;
        }

        bool shift = (flags & K_SHIFT) != 0;
        bool ctrl = (flags & K_CTRL) != 0;
        SetCursor(node);

        if(selection_mode_ == UITREESEL_SINGLE)
            SelectSingle(node);
        else if(shift)
            SelectRangeTo(node, ctrl);
        else if(ctrl)
            ToggleSelection(node);
        else
            SelectSingle(node);

        if(dnd_enabled_ && !toggle_hit) {
            drag_id_ = node.id;
            SetCapture();
        }
    }
    else if((flags & (K_SHIFT | K_CTRL)) == 0)
        ClearSelection();
    Refresh();
}

void UiTree::LeftDouble(Point p, dword flags)
{
    int row = HitTestRow(p);
    if(row < 0)
        return;
    const VisibleRow& vr = visible_rows_[row];
    if(vr.placeholder)
        return;
    UiTreeNodeRef node{vr.id};
    const UiModelItem& item = model_->Get(node);
    Rect rr = GetRowRect(row);
    bool has_icon = GetEffectiveStyle().show_icons && !IsNull(item.icon);
    bool has_metadata = GetEffectiveStyle().show_metadata_marker && item.has_metadata;
    Rect tr = GetTextRect(rr, vr.depth, vr.has_children, has_icon, has_metadata, vr.id);
    LeftDown(p, flags);
    if(item.editable && tr.Contains(p)) {
        BeginRename(node);
        return;
    }
    if(vr.has_children)
        Toggle(node);
    if(WhenAction)
        WhenAction();
}

void UiTree::LeftUp(Point p, dword)
{
    int row = HitTestRow(p);
    pressed_ = false;
    drag_id_ = -1;
    if(HasCapture())
        ReleaseCapture();
    if(row >= 0)
        hot_id_ = visible_rows_[row].id;
    Refresh();
}

void UiTree::LeftDrag(Point p, dword)
{
    if(!dnd_enabled_ || editing_ || drag_id_ < 0)
        return;
    Vector<UiTreeNodeRef> nodes = GetDragNodes(UiTreeNodeRef{drag_id_});
    if(nodes.IsEmpty())
        return;
    pressed_ = false;
    if(HasCapture())
        ReleaseCapture();
    DoDragAndDrop(InternalClip(*this, "uitree-node"), BuildDragSample(nodes), DND_MOVE);
    drag_id_ = -1;
    ClearDropTarget();
}

void UiTree::MouseMove(Point p, dword)
{
    int row = HitTestRow(p);
    int id = (row >= 0 && !visible_rows_[row].placeholder) ? visible_rows_[row].id : -1;
    if(hot_id_ != id) {
        hot_id_ = id;
        Refresh();
    }
}

void UiTree::MouseLeave()
{
    if(hot_id_ >= 0 || pressed_) {
        hot_id_ = -1;
        pressed_ = false;
        Refresh();
    }
}

void UiTree::MouseWheel(Point p, int zdelta, dword)
{
    CommitRenameIfNeeded(p);
    Rect vp = GetViewportRect();
    if(vp.IsEmpty())
        return;
    int rows = max(1, vp.GetHeight() / max(DPI(18), GetEffectiveStyle().row_height));
    int step = max(1, rows / 2) * max(DPI(18), GetEffectiveStyle().row_height);
    if(zdelta > 0)
        scroll_y_ -= step;
    else if(zdelta < 0)
        scroll_y_ += step;
    ClampScroll();
    UpdateAttachedCtrls();
    Refresh();
}

void UiTree::DragEnter()
{
    Refresh();
}

void UiTree::DragAndDrop(Point p, PasteClip& d)
{
    if(!dnd_enabled_ || editing_ || !AcceptInternal<UiTree>(d, "uitree-node")) {
        ClearDropTarget();
        return;
    }

    const UiTree* src = GetInternalPtr<UiTree>(d, "uitree-node");
    if(!src || src != this || drag_id_ < 0) {
        ClearDropTarget();
        return;
    }

    Vector<UiTreeNodeRef> nodes = GetDragNodes(UiTreeNodeRef{drag_id_});
    DropTarget target = GetDropTarget(p);
    if(!target.valid || !CanMoveNodes(nodes, UiTreeNodeRef{target.parent_id}, target.insert_pos)) {
        ClearDropTarget();
        return;
    }

    d.SetAction(DND_MOVE);
    SetDropTarget(target);
    if(d.IsPaste()) {
        MoveNodes(nodes, UiTreeNodeRef{target.parent_id}, target.insert_pos);
        ClearDropTarget();
        drag_id_ = -1;
    }
}

void UiTree::DragRepeat(Point p)
{
    Rect vp = GetViewportRect();
    if(!vp.IsEmpty()) {
        int step = max(DPI(12), GetEffectiveStyle().row_height / 2);
        if(p.y <= vp.top + DPI(12))
            scroll_y_ -= step;
        else if(p.y >= vp.bottom - DPI(12))
            scroll_y_ += step;
        ClampScroll();
        UpdateAttachedCtrls();
    }
    if(drag_id_ >= 0) {
        Vector<UiTreeNodeRef> nodes = GetDragNodes(UiTreeNodeRef{drag_id_});
        DropTarget target = GetDropTarget(p);
        if(target.valid && CanMoveNodes(nodes, UiTreeNodeRef{target.parent_id}, target.insert_pos))
            SetDropTarget(target);
        else
            ClearDropTarget();
    }
}

void UiTree::DragLeave()
{
    ClearDropTarget();
}
bool UiTree::Key(dword key, int)
{
    SyncModel();
    if(editing_)
        return false;
    if(visible_rows_.IsEmpty())
        return false;
    if(cursor_id_ < 0)
        cursor_id_ = visible_rows_[0].id;

    if(key == K_CTRL_A && selection_mode_ == UITREESEL_MULTI) {
        SelectAllVisible();
        return true;
    }

    bool shift = (key & K_SHIFT) != 0;
    dword base = key & ~(K_SHIFT | K_CTRL | K_ALT);
    int before = cursor_id_;
    UiTreeNodeRef cur{cursor_id_};

    switch(base) {
    case K_UP:
        MoveCursorBy(-1);
        break;
    case K_DOWN:
        MoveCursorBy(1);
        break;
    case K_HOME:
        MoveCursorToEdge(false);
        break;
    case K_END:
        MoveCursorToEdge(true);
        break;
    case K_PAGEUP: {
        Rect vp = GetViewportRect();
        int extent = max(DPI(18), GetEffectiveStyle().row_height);
        int rows = max(1, vp.GetHeight() / max(1, extent));
        MoveCursorBy(-rows);
        break;
    }
    case K_PAGEDOWN: {
        Rect vp = GetViewportRect();
        int extent = max(DPI(18), GetEffectiveStyle().row_height);
        int rows = max(1, vp.GetHeight() / max(1, extent));
        MoveCursorBy(rows);
        break;
    }
    case K_LEFT:
        if((model_->GetChildCount(cur) > 0 || model_->Get(cur).lazy_children || IsNodeLoading(cur)) && IsExpanded(cur))
            Collapse(cur);
        else {
            UiTreeNodeRef parent = model_->GetParent(cur);
            if(model_->IsValid(parent) && (root_visible_ || parent.id != model_->Root().id))
                SetCursor(parent);
        }
        break;
    case K_RIGHT:
        if(model_->GetChildCount(cur) > 0 || model_->Get(cur).lazy_children || IsNodeLoading(cur)) {
            if(!IsExpanded(cur))
                Expand(cur);
            else
                SetCursor(model_->GetChild(cur, 0));
        }
        break;
    case K_F2:
        BeginRename(cur);
        return true;
    case K_SPACE:
        if(selection_mode_ == UITREESEL_MULTI) {
            ToggleSelection(UiTreeNodeRef{cursor_id_});
            return true;
        }
        if(model_->GetChildCount(cur) > 0 || model_->Get(cur).lazy_children || IsNodeLoading(cur))
            Toggle(cur);
        else if(WhenAction)
            WhenAction();
        return true;
    case K_ENTER:
        if(WhenAction)
            WhenAction();
        return true;
    default:
        return false;
    }

    if(before != cursor_id_) {
        if(selection_mode_ == UITREESEL_MULTI && shift)
            SelectRangeTo(UiTreeNodeRef{cursor_id_}, false);
        else
            SelectSingle(UiTreeNodeRef{cursor_id_});
    }
    return true;
}

void UiTree::SelectSingle(UiTreeNodeRef node)
{
    selected_ids_.Clear();
    if(IsSelectableNode(node)) {
        cursor_id_ = node.id;
        selected_ids_.FindAdd(node.id);
        anchor_id_ = node.id;
    }
    else {
        cursor_id_ = -1;
        anchor_id_ = -1;
    }
    NotifySelectionChange();
}

void UiTree::ToggleSelection(UiTreeNodeRef node)
{
    if(!IsSelectableNode(node))
        return;
    int q = selected_ids_.Find(node.id);
    if(q >= 0)
        selected_ids_.Remove(q);
    else
        selected_ids_.FindAdd(node.id);
    cursor_id_ = node.id;
    anchor_id_ = node.id;
    NotifySelectionChange();
}

void UiTree::SelectRangeTo(UiTreeNodeRef node, bool additive)
{
    if(!IsSelectableNode(node))
        return;
    if(anchor_id_ < 0)
        anchor_id_ = cursor_id_ >= 0 ? cursor_id_ : node.id;
    int a = UiFindVisibleRowIndex(visible_rows_, anchor_id_);
    int b = UiFindVisibleRowIndex(visible_rows_, node.id);
    if(a < 0 || b < 0) {
        SelectSingle(node);
        return;
    }
    if(!additive)
        selected_ids_.Clear();
    for(int i = min(a, b); i <= max(a, b); i++)
        if(IsSelectableNode(UiTreeNodeRef{visible_rows_[i].id}))
            selected_ids_.FindAdd(visible_rows_[i].id);
    cursor_id_ = node.id;
    NotifySelectionChange();
}

bool UiTree::IsSelectableNode(UiTreeNodeRef node) const
{
    if(!model_ || !model_->IsValid(node))
        return false;
    const UiModelItem& item = model_->Get(node);
    return item.enabled && !item.group_header;
}


Value UiTree::GetSelectionToken(UiTreeNodeRef node) const
{
    if(!model_ || !model_->IsValid(node))
        return Value();

    const UiModelItem& item = model_->Get(node);
    return IsNull(item.data) ? Value(node.id) : item.data;
}

UiTreeNodeRef UiTree::ResolveSelectionNode(const Value& token) const
{
    if(!model_ || !model_->IsValid(model_->Root()))
        return UiTreeNodeRef{-1};

    Vector<int> stack;
    stack.Add(model_->Root().id);
    while(!stack.IsEmpty()) {
        int id = stack.Top();
        stack.Drop();

        UiTreeNodeRef node{id};
        const UiModelItem& item = model_->Get(node);
        if(!IsNull(item.data) && item.data == token && IsSelectableNode(node))
            return node;

        for(int i = model_->GetChildCount(node) - 1; i >= 0; i--)
            stack.Add(model_->GetChild(node, i).id);
    }

    if(token.Is<int>()) {
        UiTreeNodeRef node{(int)token};
        return IsSelectableNode(node) ? node : UiTreeNodeRef{-1};
    }
    if(token.Is<int64>()) {
        UiTreeNodeRef node{(int)(int64)token};
        return IsSelectableNode(node) ? node : UiTreeNodeRef{-1};
    }
    return UiTreeNodeRef{-1};
}
void UiTree::NotifySelectionChange()
{
    Refresh();
    if(WhenSelection)
        WhenSelection();
}

Vector<UiTreeNodeRef> UiTree::GetDragNodes(UiTreeNodeRef primary) const
{
    Vector<UiTreeNodeRef> nodes;
    if(!model_ || !model_->IsValid(primary))
        return nodes;

    if(selection_mode_ == UITREESEL_MULTI && IsSelected(primary))
        nodes = GetSelection();
    else
        nodes.Add(primary);

    Sort(nodes, [=](const UiTreeNodeRef& a, const UiTreeNodeRef& b) {
        int ia = UiFindVisibleRowIndex(visible_rows_, a.id);
        int ib = UiFindVisibleRowIndex(visible_rows_, b.id);
        if(ia >= 0 && ib >= 0)
            return ia < ib;
        return a.id < b.id;
    });

    Vector<UiTreeNodeRef> out;
    for(int i = 0; i < nodes.GetCount(); i++) {
        UiTreeNodeRef node = nodes[i];
        bool nested = false;
        UiTreeNodeRef parent = model_->GetParent(node);
        while(model_->IsValid(parent)) {
            for(int j = 0; j < out.GetCount(); j++)
                if(out[j].id == parent.id) {
                    nested = true;
                    break;
                }
            if(nested)
                break;
            parent = model_->GetParent(parent);
        }
        if(!nested)
            out.Add(node);
    }
    return out;
}

bool UiTree::CanMoveNodes(const Vector<UiTreeNodeRef>& nodes, UiTreeNodeRef new_parent, int) const
{
    if(!model_ || !model_->IsValid(new_parent) || nodes.IsEmpty())
        return false;

    for(int i = 0; i < nodes.GetCount(); i++) {
        UiTreeNodeRef node = nodes[i];
        if(!model_->IsValid(node) || node.id == model_->Root().id || node.id == new_parent.id)
            return false;
        UiTreeNodeRef walk = new_parent;
        while(model_->IsValid(walk)) {
            if(walk.id == node.id)
                return false;
            walk = model_->GetParent(walk);
        }
    }
    return true;
}

bool UiTree::MoveNodes(const Vector<UiTreeNodeRef>& nodes, UiTreeNodeRef new_parent, int pos)
{
    if(!CanMoveNodes(nodes, new_parent, pos))
        return false;

    int insert_pos = pos < 0 ? model_->GetChildCount(new_parent) : min(max(pos, 0), model_->GetChildCount(new_parent));
    for(int i = 0; i < nodes.GetCount(); i++) {
        UiTreeNodeRef node = nodes[i];
        UiTreeNodeRef old_parent = model_->GetParent(node);
        int child_index = model_->GetChildIndex(node);
        if(old_parent.id == new_parent.id && child_index >= 0 && child_index < insert_pos)
            insert_pos--;
        if(!model_->Move(node, new_parent, insert_pos))
            return false;
        insert_pos++;
    }

    expanded_ids_.FindAdd(new_parent.id);
    selected_ids_.Clear();
    for(int i = 0; i < nodes.GetCount(); i++)
        if(model_->IsValid(nodes[i]))
            selected_ids_.FindAdd(nodes[i].id);
    anchor_id_ = nodes.IsEmpty() ? -1 : nodes[0].id;
    SetCursor(nodes.IsEmpty() ? UiTreeNodeRef{-1} : nodes[0]);
    NotifySelectionChange();
    return true;
}

UiTree::DropTarget UiTree::GetDropTarget(Point p) const
{
    DropTarget target;
    if(!model_ || !model_->IsValid(model_->Root()))
        return target;

    Rect vp = GetViewportRect();
    int row = HitTestRow(p);
    if(row < 0) {
        if(!vp.IsEmpty() && p.x >= vp.left && p.x <= vp.right) {
            target.parent_id = model_->Root().id;
            target.insert_pos = model_->GetChildCount(model_->Root());
            target.valid = true;
        }
        return target;
    }

    const VisibleRow& vr = visible_rows_[row];
    UiTreeNodeRef node{vr.id};
    if(!model_->IsValid(node))
        return target;

    if(vr.placeholder) {
        target.parent_id = node.id;
        target.insert_pos = model_->GetChildCount(node);
        target.hover_id = node.id;
        target.into = true;
        target.valid = true;
        return target;
    }

    Rect rr = GetRowRect(row);
    int quarter = max(DPI(4), rr.GetHeight() / 4);
    if(p.y <= rr.top + quarter) {
        UiTreeNodeRef parent = model_->GetParent(node);
        if(model_->IsValid(parent)) {
            target.parent_id = parent.id;
            target.insert_pos = model_->GetChildIndex(node);
            target.hover_id = node.id;
            target.valid = target.insert_pos >= 0;
        }
    }
    else if(p.y >= rr.bottom - quarter) {
        UiTreeNodeRef parent = model_->GetParent(node);
        if(model_->IsValid(parent)) {
            target.parent_id = parent.id;
            target.insert_pos = model_->GetChildIndex(node) + 1;
            target.hover_id = node.id;
            target.valid = target.insert_pos > 0;
        }
    }
    else {
        target.parent_id = node.id;
        target.insert_pos = model_->GetChildCount(node);
        target.hover_id = node.id;
        target.into = true;
        target.valid = true;
    }
    return target;
}

void UiTree::SetDropTarget(const DropTarget& target)
{
    if(drop_parent_id_ == target.parent_id && drop_insert_pos_ == target.insert_pos &&
       drop_hover_id_ == target.hover_id && drop_into_ == target.into)
        return;
    drop_parent_id_ = target.parent_id;
    drop_insert_pos_ = target.insert_pos;
    drop_hover_id_ = target.hover_id;
    drop_into_ = target.into;
    Refresh();
}

void UiTree::ClearDropTarget()
{
    if(drop_parent_id_ < 0 && drop_insert_pos_ < 0 && drop_hover_id_ < 0 && !drop_into_)
        return;
    drop_parent_id_ = -1;
    drop_insert_pos_ = -1;
    drop_hover_id_ = -1;
    drop_into_ = false;
    Refresh();
}

Image UiTree::BuildDragSample(const Vector<UiTreeNodeRef>& nodes) const
{
    String label;
    Font font = StdFont().Bold();
    if(nodes.GetCount() == 1 && model_ && model_->IsValid(nodes[0]))
        label = model_->Get(nodes[0]).text;
    else
        label = Format("%d items", nodes.GetCount());

    Size tsz = GetTextSize(label, font);
    Size sz(max(DPI(96), tsz.cx + DPI(20)), DPI(28));
    ImageDraw iw(sz);
    iw.DrawRect(sz, Color(17, 24, 39));
    iw.DrawRect(1, 1, sz.cx - 2, sz.cy - 2, White());
    DrawSmartText(iw, DPI(10), (sz.cy - font.GetHeight()) / 2,
                  max(0, sz.cx - DPI(20)), label, font, Color(17, 24, 39), 0);
    return iw;
}
void UiTree::GotFocus()
{
    Refresh();
}

void UiTree::LostFocus()
{
    if(!editing_)
        Refresh();
}
} // namespace Upp
