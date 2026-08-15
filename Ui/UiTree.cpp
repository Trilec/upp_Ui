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

namespace {

int ClampMin(int value, int minimum)
{
    return max(minimum, value);
}

UiItemRenderStyle MakeTreeDefaultRenderStyle(const UiTree::Style& tree)
{
    UiItemRenderBasic base;
    UiItemRenderStyle out = base.GetStyle();
    out.palette = tree.palette;
    out.metrics = tree.metrics;
    out.skin = tree.skin;
    out.metrics.face_enabled = false;
    out.metrics.frame_enabled = false;
    out.metrics.focus_enabled = false;
    out.metrics.shadow.enabled = false;
    out.metrics.content_margin = Rect(0, 0, 0, 0);
    out.show_face = false;
    out.show_image = false;
    out.show_icon = tree.show_icons;
    out.show_subtitle = false;
    out.show_description = false;
    out.show_right_text = true;
    out.show_metadata = false;
    out.title_font = tree.font;
    out.subtitle_font = tree.font;
    out.description_font = tree.font;
    out.right_font = tree.font;
    out.icon_size = tree.icon_size;
    out.content_gap = tree.content_gap;
    out.text_gap = DPI(1);
    out.muted_ink = tree.disabled_ink;
    return out;
}

}

const UiTree::Style& UiTree::StyleDefault()
{
    static Style style;
    ONCELOCK {
        for(int i = 0; i < 4; i++) {
            style.palette.face[i] = UiFill::Solid(SColorPaper());
            style.palette.frame[i] = SColorShadow();
            style.palette.ink[i] = SColorText();
            style.palette.icon[i] = SColorText();
        }
        style.metrics.face_enabled = true;
        style.metrics.frame_enabled = false;
        style.metrics.frame_width = 1;
        style.metrics.radius = 0;
        style.metrics.content_margin = Rect(0, 0, 0, 0);
        style.metrics.focus_enabled = false;
        style.metrics.shadow.enabled = false;
        style.skin = StyledSkin();
        style.font = StdFont();
        style.row_height = DPI(24);
        style.indent_px = DPI(16);
        style.glyph_size = DPI(10);
        style.icon_size = DPI(16);
        style.content_gap = DPI(6);
        style.item_spacing = 0;
        style.h_padding = DPI(8);
        style.v_padding = DPI(6);
        style.row_radius = DPI(4);
        style.branch_hit_extra = DPI(10);
        style.metadata_size = DPI(8);
        style.metadata_gap = DPI(6);
        style.accessory_gap = DPI(8);
        style.show_icons = true;
        style.show_connector_lines = false;
        style.show_metadata_marker = true;
        style.glyph_style = UITREEGLYPH_CHEVRON;
        style.icon_render_mode = UiIconRenderMode::MonoTint;
        style.ink = SColorText();
        style.disabled_ink = SColorDisabled();
        style.hot_face = Color(241, 245, 249);
        style.hot_frame = Color(226, 232, 240);
        style.hot_ink = SColorText();
        style.selected_face = Color(232, 242, 255);
        style.selected_frame = Color(65, 167, 248);
        style.selected_ink = SColorText();
        style.line_color = Color(203, 213, 225);
        style.glyph_color = Color(100, 116, 139);
        style.glyph_hot_color = Color(71, 85, 105);
        style.glyph_selected_color = Color(37, 99, 235);
    }
    return style;
}

UiTree::UiTree()
{
    model_ = &internal_model_;
    Add(inline_editor_);
    inline_editor_.Hide();
    inline_editor_.WhenAccept = [=] { CommitRename(); };
    inline_editor_.WhenAbort = [=] { CancelRename(); };
    inline_editor_.WhenBlur = [=] { CommitRename(); };
    BindModel(internal_model_);
    if(model_->IsValid(model_->Root()))
        expanded_ids_.FindAdd(model_->Root().id);
    SyncThemeStyle();
    ConfigureDefaultItemRender();
    SyncModel();
    BackPaint();
    WantFocus();
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

UiTree& UiTree::SetGlyphStyle(UiTreeGlyphStyle style)
{
    Style& s = StyleEdit();
    if(s.glyph_style == style)
        return *this;
    s.glyph_style = style;
    Refresh();
    return *this;
}

UiTree& UiTree::SetGlyphImages(const Image& collapsed, const Image& expanded, UiIconRenderMode render_mode)
{
    Style& s = StyleEdit();
    s.collapsed_icon = collapsed;
    s.expanded_icon = expanded;
    s.icon_render_mode = render_mode;
    Refresh();
    return *this;
}

UiTree& UiTree::EnableDragDrop(bool on)
{
    if(dnd_enabled_ == on)
        return *this;
    dnd_enabled_ = on;
    if(!dnd_enabled_)
        ClearDropTarget();
    Refresh();
    return *this;
}

UiTree& UiTree::EnableInternalMutation(bool on)
{
    internal_mutation_enabled_ = on;
    return *this;
}

UiTree& UiTree::ShowConnectorLines(bool on)
{
    Style& s = StyleEdit();
    if(s.show_connector_lines == on)
        return *this;
    s.show_connector_lines = on;
    Refresh();
    return *this;
}

UiTree& UiTree::ShowMetadataMarker(bool on)
{
    Style& s = StyleEdit();
    if(s.show_metadata_marker == on)
        return *this;
    s.show_metadata_marker = on;
    Refresh();
    return *this;
}

UiTree& UiTree::EnableRenameOnDblClick(bool on)
{
    rename_on_dblclick_ = on;
    if(!on)
        CancelRename();
    return *this;
}

UiTree& UiTree::SetColumnWidths(const Vector<int>& widths)
{
    column_widths_.Clear();
    column_widths_.Reserve(widths.GetCount());
    for(int i = 0; i < widths.GetCount(); i++)
        column_widths_.Add(max(DPI(24), widths[i]));
    ResetRenderPools();
    RefreshLayout();
    Refresh();
    return *this;
}

UiTree& UiTree::ClearColumnWidths()
{
    if(column_widths_.IsEmpty())
        return *this;
    column_widths_.Clear();
    ResetRenderPools();
    RefreshLayout();
    Refresh();
    return *this;
}

} // namespace Upp
