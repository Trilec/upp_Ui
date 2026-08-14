#include <Ui/UiTable.h>
#include <Ui/UiTheme.h>

namespace Upp {

namespace {

UiTable::Style MakeUiTableDefaultStyle()
{
    UiTable::Style s;
    s.palette = StyledPalette();
    s.metrics = StyledMetrics();
    s.skin = StyledSkin();
    return s;
}

}

bool UiTable::InlineEditor::Key(dword key, int count)
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

void UiTable::InlineEditor::LostFocus()
{
    EditString::LostFocus();
    if(WhenBlur)
        WhenBlur();
}

const UiTable::Style& UiTable::StyleDefault()
{
    static Style style = MakeUiTableDefaultStyle();
    return style;
}

UiTable::UiTable()
{
    model_ = &internal_model_;

    Add(hscroll_);
    Add(vscroll_);
    Add(inline_editor_);

    hscroll_.Horz();
    hscroll_.WhenScroll = [=] {
        PrepareItemRenders();
        UpdateEditorRect();
        Refresh();
    };
    vscroll_.WhenScroll = [=] {
        PrepareItemRenders();
        UpdateEditorRect();
        Refresh();
    };
    hscroll_.SetLine(DPI(40));
    vscroll_.SetLine(DPI(28));

    inline_editor_.Hide();
    inline_editor_.WhenAccept = [=] { CommitEdit(); };
    inline_editor_.WhenAbort = [=] { CancelEdit(); };
    inline_editor_.WhenBlur = [=] {
        if(editing_)
            CommitEdit();
    };

    internal_model_.SetSize(12, 6);
    for(int c = 0; c < internal_model_.GetColumnCount(); c++)
        internal_model_.SetHeader(UITABLE_COLUMN_AXIS, c, UiTableHeader(Format("Column %d", c + 1)));
    for(int r = 0; r < internal_model_.GetRowCount(); r++)
        internal_model_.SetHeader(UITABLE_ROW_AXIS, r, UiTableHeader(Format("%d", r + 1)));

    BindModel(internal_model_);
    SyncThemeStyle();
    SyncColumnWidths();
    RebuildColumnGeometry();
    active_cell_ = UiTablePos{0, 0};
    anchor_cell_ = active_cell_;
    selection_ = MakeSingleCellSelection(0, 0);
    BackPaint();
    WantFocus();
}

UiTable::Style& UiTable::StyleEdit()
{
    if(!has_custom_style_) {
        style_ = GetEffectiveStyle();
        has_custom_style_ = true;
    }
    theme_revision_ = 0;
    return style_;
}

const UiTable::Style& UiTable::GetEffectiveStyle() const
{
    if(has_custom_style_)
        return style_;
    const_cast<UiTable *>(this)->SyncThemeStyle();
    return themed_style_;
}

void UiTable::SyncThemeStyle()
{
    if(has_custom_style_)
        return;
    uint64 rev = UiTheme::GetRevision();
    if(theme_revision_ == rev)
        return;
    themed_style_ = UiTheme::ResolveTable();
    theme_revision_ = rev;
    ConfigureDefaultRenders();
    ResetRenderPools();
}

void UiTable::OnStyleChanged()
{
    ConfigureDefaultRenders();
    SyncColumnWidths();
    RebuildColumnGeometry();
    ResetRenderPools();
    SyncScrollBars();
    RefreshLayout();
    Refresh();
}

UiTable& UiTable::SetCustomStyle(const Style& s)
{
    style_ = Style(s);
    has_custom_style_ = true;
    OnStyleChanged();
    return *this;
}

UiTable& UiTable::ClearCustomStyle()
{
    has_custom_style_ = false;
    style_ = Style();
    theme_revision_ = 0;
    SyncThemeStyle();
    OnStyleChanged();
    return *this;
}

UiTable& UiTable::SetModel(UiTableModel& model)
{
    CancelEdit();
    model_ = &model;
    BindModel(model);
    model_revision_ = -1;
    active_cell_ = UiTablePos{0, 0};
    anchor_cell_ = active_cell_;
    selection_ = MakeSingleCellSelection(0, 0);
    ResetRenderPools();
    SyncModel();
    RefreshLayout();
    Refresh();
    return *this;
}

UiTable& UiTable::UseInternalModel()
{
    return SetModel(internal_model_);
}

UiTable& UiTable::EnableInternalMutation(bool on)
{
    internal_mutation_enabled_ = on;
    return *this;
}

UiTable& UiTable::ShowRowHeaders(bool on)
{
    Style& s = StyleEdit();
    if(s.show_row_headers == on)
        return *this;
    s.show_row_headers = on;
    ResetRenderPools();
    RefreshLayout();
    Refresh();
    return *this;
}

UiTable& UiTable::ShowColumnHeaders(bool on)
{
    Style& s = StyleEdit();
    if(s.show_column_headers == on)
        return *this;
    s.show_column_headers = on;
    ResetRenderPools();
    RefreshLayout();
    Refresh();
    return *this;
}

UiTable& UiTable::SetRowHeight(int px)
{
    Style& s = StyleEdit();
    int next = max(DPI(20), px);
    if(s.row_height == next)
        return *this;
    s.row_height = next;
    vscroll_.SetLine(s.row_height);
    ResetRenderPools();
    RefreshLayout();
    Refresh();
    return *this;
}

UiTable& UiTable::SetHeaderHeight(int px)
{
    Style& s = StyleEdit();
    int next = max(DPI(22), px);
    if(s.header_height == next)
        return *this;
    s.header_height = next;
    ResetRenderPools();
    RefreshLayout();
    Refresh();
    return *this;
}

UiTable& UiTable::SetRowHeaderWidth(int px)
{
    Style& s = StyleEdit();
    int next = max(DPI(28), px);
    if(s.row_header_width == next)
        return *this;
    s.row_header_width = next;
    ResetRenderPools();
    RefreshLayout();
    Refresh();
    return *this;
}

UiTable& UiTable::SetDefaultColumnWidth(int px)
{
    Style& s = StyleEdit();
    int next = max(s.min_column_width, px);
    if(s.default_column_width == next)
        return *this;
    s.default_column_width = next;
    SyncColumnWidths();
    RebuildColumnGeometry();
    ResetRenderPools();
    RefreshLayout();
    Refresh();
    return *this;
}

UiTable& UiTable::SetColumnWidth(int col, int px)
{
    SyncModel();
    if(col < 0 || col >= column_widths_.GetCount())
        return *this;
    const Style& style = GetEffectiveStyle();
    int next = clamp(px, style.min_column_width, style.max_column_width);
    if(column_widths_[col] == next)
        return *this;
    column_widths_[col] = next;
    RebuildColumnGeometry();
    ResetRenderPools();
    SyncScrollBars();
    PrepareItemRenders();
    UpdateEditorRect();
    Refresh();
    return *this;
}

int UiTable::GetColumnWidth(int col) const
{
    return col >= 0 && col < column_widths_.GetCount()
         ? column_widths_[col]
         : GetEffectiveStyle().default_column_width;
}

} // namespace Upp
