#include <Ui/UiList.h>
#include <Ui/UiTheme.h>

namespace Upp {

bool UiList::InlineEditor::Key(dword key, int count)
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

void UiList::InlineEditor::LostFocus()
{
    EditString::LostFocus();
    if(WhenBlur)
        WhenBlur();
}

const UiList::Style& UiList::StyleDefault()
{
    static Style s;
    ONCELOCK {
        const Color text_primary = Color(17, 24, 39);
        const Color text_muted = Color(148, 163, 184);
        const Color text_secondary = Color(100, 116, 139);

        for(int i = 0; i < 4; i++) {
            s.palette.face[i] = UiFill::Solid(White());
            s.palette.frame[i] = Color(226, 232, 240);
            s.palette.ink[i] = text_primary;
            s.palette.icon[i] = text_secondary;
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
        s.item_spacing = 0;
        s.icon_size = DPI(16);
        s.check_size = DPI(14);
        s.content_gap = DPI(6);
        s.h_padding = DPI(8);
        s.v_padding = DPI(6);
        s.row_radius = DPI(4);
        s.metadata_size = DPI(8);
        s.metadata_gap = DPI(6);
        s.right_gap = DPI(8);
        s.drag_size = DPI(14);
        s.drag_gap = DPI(6);
        s.show_icons = true;
        s.show_checks = true;
        s.show_metadata_marker = true;
        s.show_drag_handle = true;
        s.drag_side = UiAlign::RIGHT;
        s.drag_glyph = ICON_DESIGN_DRAG_INDICATOR_48();
        s.hot_as_underline = false;
        s.selected_as_underline = false;
        s.state_underline_thickness = DPI(2);

        s.ink = text_primary;
        s.disabled_ink = text_muted;
        s.muted_ink = text_secondary;
        s.hot_face = Color(245, 247, 250);
        s.hot_frame = Color(226, 232, 240);
        s.hot_ink = text_primary;
        s.selected_face = Color(232, 242, 255);
        s.selected_frame = Color(65, 167, 248);
        s.selected_ink = text_primary;
        s.separator_color = Color(226, 232, 240);
        s.row_even_face = Null;
        s.row_odd_face = Null;
        s.show_row_separator = false;
        s.row_state_frame_enabled = false;
        s.right_text_as_badge = false;
        s.badge_face = Color(241, 245, 249);
        s.badge_frame = Null;
        s.badge_ink = Color(51, 65, 85);
        s.badge_radius = DPI(999);
        s.badge_h_padding = DPI(6);
        s.metadata_default = Color(65, 167, 248);
        s.check_frame = Color(148, 163, 184);
        s.check_fill = Color(17, 24, 39);
        s.drag_marker = Color(56, 146, 255);
    }
    return s;
}

UiList::UiList()
    : style_(StyleDefault())
    , themed_style_(StyleDefault())
    , model_(&internal_model_)
{
    BackPaint();
    WantFocus();
    Add(drag_marker_);
    drag_marker_.Color(Color(56, 146, 255)).IgnoreMouse().Hide();

    inline_editor_.Hide();
    inline_editor_.WhenAccept = [=] { CommitRename(); };
    inline_editor_.WhenAbort = [=] { CancelRename(); };
    inline_editor_.WhenBlur = [=] {
        if(editing_)
            CommitRename();
    };
    Add(inline_editor_);

    SyncThemeStyle();
    BindModel(internal_model_);
    SyncModel();
}

UiList::Style& UiList::StyleEdit()
{
    if(!has_custom_style_) {
        style_ = GetEffectiveStyle();
        has_custom_style_ = true;
    }
    theme_revision_ = 0;
    return style_;
}

const UiList::Style& UiList::GetEffectiveStyle() const
{
    if(has_custom_style_)
        return style_;
    const_cast<UiList*>(this)->SyncThemeStyle();
    return themed_style_;
}

void UiList::SyncThemeStyle()
{
    if(has_custom_style_)
        return;
    uint64 revision = UiTheme::GetRevision();
    if(theme_revision_ == revision)
        return;
    themed_style_ = UiTheme::ResolveList();
    theme_revision_ = revision;
    ResetItemRenderPool();
}

UiList& UiList::SetCustomStyle(const Style& s)
{
    style_ = s;
    has_custom_style_ = true;
    OnStyleChanged();
    return *this;
}

UiList& UiList::ClearCustomStyle()
{
    if(!has_custom_style_)
        return *this;
    has_custom_style_ = false;
    style_ = StyleDefault();
    theme_revision_ = 0;
    OnStyleChanged();
    return *this;
}

void UiList::OnStyleChanged()
{
    ResetItemRenderPool();
    RefreshLayout();
    Refresh();
}

UiList& UiList::SetModel(UiListModel& model)
{
    CancelRename();
    if(model_ == &model)
        return *this;
    model_ = &model;
    BindModel(model);
    model_revision_ = -1;
    selected_.Clear();
    cursor_ = -1;
    anchor_ = -1;
    hot_ = -1;
    pressed_ = -1;
    scroll_y_ = 0;
    SyncModel();
    RefreshLayout();
    Refresh();
    return *this;
}

UiList& UiList::SetSelectionMode(UiListSelectionMode mode)
{
    if(selection_mode_ == mode)
        return *this;
    selection_mode_ = mode;
    if(selection_mode_ == UILISTSEL_SINGLE && selected_.GetCount() > 1)
        SelectSingle(cursor_);
    Refresh();
    return *this;
}

UiList& UiList::ClearSelection()
{
    if(selected_.IsEmpty())
        return *this;
    selected_.Clear();
    cursor_ = -1;
    anchor_ = -1;
    NotifySelectionChange();
    return *this;
}

UiList& UiList::Select(int index, bool additive)
{
    SyncModel();
    if(!IsSelectableIndex(index))
        return *this;
    if(selection_mode_ != UILISTSEL_MULTI || !additive)
        SelectSingle(index);
    else
        ToggleSelection(index);
    return *this;
}

UiList& UiList::SelectAll()
{
    SyncModel();
    if(selection_mode_ != UILISTSEL_MULTI || !model_)
        return *this;
    selected_.Clear();
    for(int i = 0; i < model_->GetCount(); i++)
        if(model_->Get(i).enabled && !model_->Get(i).group_header)
            selected_.FindAdd(i);
    if(selected_.GetCount() > 0) {
        cursor_ = selected_[0];
        anchor_ = cursor_;
    }
    NotifySelectionChange();
    return *this;
}

bool UiList::IsSelected(int index) const
{
    return selected_.Find(index) >= 0;
}

Vector<int> UiList::GetSelection() const
{
    Vector<int> out;
    for(int i = 0; i < selected_.GetCount(); i++)
        out.Add(selected_[i]);
    Sort(out);
    return out;
}

void UiList::SetData(const Value& v)
{
    SyncModel();

    if(IsNull(v)) {
        ClearSelection();
        return;
    }

    if(selection_mode_ == UILISTSEL_MULTI || v.Is<ValueArray>()) {
        selected_.Clear();
        if(model_) {
            Vector<int> resolved = UiResolveSequentialSelectionTokens(*model_, v,
                [=](int index) { return IsSelectableIndex(index); });
            for(int index : resolved)
                selected_.FindAdd(index);
        }

        Vector<int> selection = GetSelection();
        anchor_ = selection.IsEmpty() ? -1 : selection[0];
        cursor_ = selection.IsEmpty() ? -1 : selection.Top();
        NotifySelectionChange();
        return;
    }

    int index = ResolveSelectionIndex(v);
    if(index >= 0)
        SelectSingle(index);
    else
        ClearSelection();
}

Value UiList::GetData() const
{
    if(selection_mode_ == UILISTSEL_MULTI) {
        ValueArray values;
        Vector<int> selection = GetSelection();
        for(int i = 0; i < selection.GetCount(); i++)
            values.Add(GetSelectionToken(selection[i]));
        return values;
    }

    return selected_.GetCount() > 0 ? GetSelectionToken(selected_[0]) : Value();
}

UiList& UiList::EnableRenameOnDblClick(bool on)
{
    rename_on_dblclick_ = on;
    return *this;
}

UiList& UiList::EnableDragReorder(bool on)
{
    drag_reorder_enabled_ = on;
    if(!on)
        EndRowDrag(true);
    Refresh();
    return *this;
}

UiList& UiList::EnableInternalMutation(bool on)
{
    internal_mutation_enabled_ = on;
    return *this;
}

} // namespace Upp
