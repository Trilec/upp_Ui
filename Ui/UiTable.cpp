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

Color UiTableFace(const StyledPalette& palette, int state, Color fallback)
{
    return palette.face[state].IsSolid() && !IsNull(palette.face[state].color)
         ? palette.face[state].color
         : fallback;
}

Color UiTableColor(Color value, Color fallback)
{
    return IsNull(value) ? fallback : value;
}

void ResolveUiTableChrome(UiTable::Style& s)
{
    const UiPanel::Style surface = UiTheme::ResolvePanel(UiPanelRole::Surface);
    const UiPanel::Style subtle = UiTheme::ResolvePanel(UiPanelRole::Subtle);
    const UiList::Style list = UiTheme::ResolveList();
    const UiThemeContext ctx = UiTheme::GetContext();
    const bool role_tuned = UiThemeDetail::IsRoleTunedPreset(ctx.preset);
    const UiThemeDetail::MinimalRoleColors standard_role =
        UiThemeDetail::MinimalRole(ctx.mode, UiRole::Standard);
    const UiThemeDetail::MinimalRoleColors accent_role =
        UiThemeDetail::MinimalRole(ctx.mode, UiRole::Accent);

    s.table_bg = UiTableFace(surface.palette, ST_NORMAL, s.table_bg);
    s.header_bg = UiTableFace(subtle.palette, ST_NORMAL, s.table_bg);
    s.header_hot_bg = UiTableFace(subtle.palette, ST_HOT, s.header_bg);
    s.row_header_bg = s.header_bg;

    s.header_ink = UiTableColor(surface.palette.ink[ST_NORMAL], s.header_ink);
    s.cell_ink = UiTableColor(surface.palette.ink[ST_NORMAL], s.cell_ink);
    s.muted_ink = UiTableColor(surface.palette.ink[ST_DISABLED], s.muted_ink);
    s.grid_color = UiTableColor(subtle.palette.frame[ST_NORMAL],
                                UiTableColor(surface.palette.frame[ST_NORMAL], s.grid_color));

    s.alternate_row_bg = UiTableFace(subtle.palette, ST_NORMAL, s.table_bg);
    s.hover_bg = UiTableFace(surface.palette, ST_HOT,
                             UiTableFace(subtle.palette, ST_HOT, s.table_bg));
    s.read_only_bg = UiTableFace(subtle.palette, ST_NORMAL, s.table_bg);

    s.selection_bg = UiTableColor(list.selected_face,
                                  UiTableFace(surface.palette, ST_PRESSED, s.table_bg));
    s.selection_border = UiTableColor(list.selected_frame,
                                      UiTableColor(list.drag_marker, s.selection_border));
    s.active_bg = s.selection_bg;
    s.active_border = UiTableColor(list.drag_marker, s.selection_border);
    s.resize_guide = s.active_border;

    // Minimal/Pill list rows intentionally permit frame-less selection. A Table
    // has a different interaction contract: the active cell and range boundary
    // need an explicit, readable edge. Resolve those domain colours from the
    // same semantic Standard/Accent role palette instead of inheriting a null
    // List row frame or a platform-dependent text colour.
    if(role_tuned) {
        s.header_ink = standard_role.ink;
        s.cell_ink = standard_role.ink;
        s.muted_ink = standard_role.ink_disabled;
        s.selection_border = accent_role.frame;
        s.active_border = accent_role.frame_pressed;
        s.resize_guide = accent_role.frame_pressed;
    }

    if(UiThemeDetail::ResolveEffectiveMode(ctx.mode) == UiThemeMode::Dark) {
        // Warning/error are semantic cell states rather than ordinary panel
        // roles. Transform their established defaults through the same dark
        // palette helper used by the central theme instead of carrying light
        // warning/error fills into a dark table.
        s.warning_bg = UiThemeDetail::ForceDarkFace(UiTable::StyleDefault().warning_bg);
        s.error_bg = UiThemeDetail::ForceDarkFace(UiTable::StyleDefault().error_bg);
    }
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

    // A default Table is an empty model view. Sample rows/columns belong in
    // demos, not in the control's hidden internal model.
    BindModel(internal_model_);
    SyncThemeStyle();
    SyncColumnWidths();
    RebuildColumnGeometry();
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
    ResolveUiTableChrome(themed_style_);
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
    if(model_ == &model)
        return *this;
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
