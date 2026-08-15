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

    BindModel(internal_model_);
    SyncThemeStyle();
    ConfigureDefaultItemRender();
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
    ConfigureDefaultItemRender();
    ResetRenderPools();
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
    SyncThemeStyle();
    OnStyleChanged();
    return *this;
}

void UiTree::OnStyleChanged()
{
    ConfigureDefaultItemRender();
    ResetRenderPools();
    UpdateAttachedCtrls();
    RefreshLayout();
    Refresh();
}

UiTree& UiTree::SetModel(UiTreeModel& model)
{
    if(model_ == &model)
        return *this;
    CancelRename();
    model_ = &model;
    BindModel(model);
    model_revision_ = -1;
    expanded_ids_.Clear();
    selected_ids_.Clear();
    if(model_->IsValid(model_->Root()))
        expanded_ids_.FindAdd(model_->Root().id);
    cursor_id_ = anchor_id_ = hot_id_ = -1;
    scroll_y_ = 0;
    ResetRenderPools();
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
    cursor_id_ = anchor_id_ = -1;
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
        if(!visible_rows_[i].placeholder && IsSelectableNode(UiTreeNodeRef{visible_rows_[i].id}))
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
        if(!visible_rows_[i].placeholder && selected_ids_.Find(id) >= 0)
            out.Add(UiTreeNodeRef{id});
    }
    for(int i = 0; i < selected_ids_.GetCount(); i++) {
        int id = selected_ids_[i];
        if(FindVisibleRow(id) < 0 && model_ && model_->IsValid(UiTreeNodeRef{id}))
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
    return selected_ids_.GetCount() > 0
         ? GetSelectionToken(UiTreeNodeRef{selected_ids_[0]}) : Value();
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

UiTree& UiTree::EnableInternalMutation(bool on)
{
    internal_mutation_enabled_ = on;
    return *this;
}

UiTree& UiTree::SetGlyphStyle(UiTreeGlyphStyle style)
{
    StyleEdit().glyph_style = style;
    OnStyleChanged();
    return *this;
}

UiTree& UiTree::SetGlyphImages(const Image& collapsed, const Image& expanded,
                               UiIconRenderMode render_mode)
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
    RebuildVisibleRows();
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
    loading_ids_.RemoveKey(node.id);
    UiModelItem item = model_->Get(node);
    item.lazy_loaded = loaded;
    if(loaded)
        item.lazy_children = false;
    model_->Set(node, item);
    return *this;
}

UiTree& UiTree::SetColumnWidths(const Vector<int>& widths)
{
    column_widths_ = clone(widths);
    ResetRenderPools();
    UpdateAttachedCtrls();
    RefreshLayout();
    Refresh();
    return *this;
}

UiTree& UiTree::ClearColumnWidths()
{
    column_widths_.Clear();
    ResetRenderPools();
    UpdateAttachedCtrls();
    RefreshLayout();
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

Vector<UiTreeNodeRef> UiTree::GetExpandedNodes() const
{
    Vector<UiTreeNodeRef> out;
    for(int id : expanded_ids_)
        if(model_ && model_->IsValid(UiTreeNodeRef{id}))
            out.Add(UiTreeNodeRef{id});
    return out;
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

} // namespace Upp
