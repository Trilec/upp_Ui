// GUI-thread coordinator: navigation/session state, scan publication, model
// projections and selection policy. Worker services, native composition, layout,
// appearance, preview and file creation live in their own modules.
#include "UiFileBrowser.h"

namespace Upp
{

namespace
{
using UiFileBrowserAppearance::BrowserItem;
using UiFileBrowserAppearance::NameInk;
using UiFileBrowserAppearance::TypeInk;

// GUI-thread session data survives rebuilding controls and reopening dialogs.
struct BrowserSession
{
    Vector<String> places;
    Vector<String> recent;
    UiFileBrowser::ViewSize view_size = UiFileBrowser::ViewSize::Medium;
};
BrowserSession &Session()
{
    static BrowserSession session;
    return session;
}

bool SamePath(const String &a, const String &b)
{
#ifdef PLATFORM_WIN32
    return CompareNoCase(NormalizePath(a), NormalizePath(b)) == 0;
#else
    return NormalizePath(a) == NormalizePath(b);
#endif
}

String ShareRoot(const String &folder)
{
#ifdef PLATFORM_WIN32
    String path = NormalizePath(folder);
    if(path.StartsWith("\\\\"))
    {
        int server_end = path.Find('\\', 2);
        int share_end = server_end < 0 ? -1 : path.Find('\\', server_end + 1);
        return share_end < 0 ? path : path.Left(share_end);
    }
#endif
    return String();
}

void RememberFolder(const String &folder)
{
    auto &recent = Session().recent;
    for(int i = recent.GetCount() - 1; i >= 0; --i)
        if(SamePath(recent[i], folder))
            recent.Remove(i);
    recent.Insert(0, folder);
    if(recent.GetCount() > 8)
        recent.Trim(8);
}

} // namespace

void UiFileBrowserPlaces::LeftUp(Point p, dword flags)
{
    UiList::LeftUp(p, flags);
    auto notify = WhenNavigate;
    notify();
}

void UiFileBrowserPlaces::DragAndDrop(Point, PasteClip &clip)
{
    if(AcceptFiles(clip))
    {
        Vector<String> folders;
        for(const String &path : GetFiles(clip))
            if(DirectoryExists(path))
                folders.Add(path);
        auto notify = WhenDropFolders;
        notify(folders);
    }
}

void UiFileBrowserTable::LeftDrag(Point p, dword flags)
{
    String folder = DragFolder ? DragFolder() : String();
    if(folder.IsEmpty())
    {
        UiTable::LeftDrag(p, flags);
        return;
    }
    LeftUp(p, flags);
    Vector<String> files;
    files.Add(folder);
    VectorMap<String, ClipData> data;
    AppendFiles(data, files);
    DoDragAndDrop(data, Null, DND_COPY);
}

UiFileBrowser::~UiFileBrowser()
{
    CloseOptions();
    KillTimeCallback(1);
    KillTimeCallback(2);
    KillTimeCallback(3);
    KillTimeCallback(4);
    KillTimeCallback(5);
}

UiFileBrowser::UiFileBrowser()
{
    base_font_ = ResolveInheritedFont();
    view_size_ = Session().view_size;
    font_ = Font(base_font_).Height(base_font_.GetHeight() + DPI(2 * ((int)view_size_ - 1)));
    Add(shell_.SizePos());
    BuildUi();
    WireEvents();
    ApplyTheme();
    ApplyViewSizeLayout();
    // UiTheme revisions have no push notification; unchanged polls do no layout work.
    SetTimeCallback(-100, [this] { PollTheme(); }, 3);

    String start = GetCurrentDirectory();
    if(start.IsEmpty() || !DirectoryExists(start))
        start = GetHomeDirectory();
    SetFolderInternal(start, true);
}

void UiFileBrowser::RebuildShell()
{
    // Reparent toolbar controls deliberately; no duplicate widgets or settings.
    commands_layout_.PauseLayout().ClearItems();
    view_display_row_.PauseLayout().ClearItems();
    RebuildNavigation();
    shell_.ClearItems();
    shell_.SetGap(0).SetInset(0);
    shell_.Add(navigation_panel_).Fixed(SizePx(38));
    if(mode_ == Mode::Advanced)
    {
        new_button_.SetText("New ▾");
        commands_layout_.Add(new_button_).Fit();
        for(UiToolButton *button : {&filter_button_, &sequence_button_, &view_button_, &sort_button_})
            commands_layout_.Add(*button).Fit();
        commands_layout_.AddSpacer(1).Expand(1);
        commands_layout_.Add(item_count_).Fit();
        shell_.Add(commands_panel_).Fixed(SizePx(34));
        details_button_.SetText("Details");
        thumbnails_button_.SetText("Thumbnails");
        view_display_row_.Add(details_button_).Expand(1);
        view_display_row_.Add(thumbnails_button_).Expand(1);
    }
    else
    {
        commands_panel_.Hide();
        new_button_.SetText("");
        details_button_.SetText("");
        thumbnails_button_.SetText("");
    }
    commands_layout_.ResumeLayout();
    view_display_row_.ResumeLayout();
    shell_.Add(header_divider_).Fixed(DPI(2));
    shell_.Add(workspace_).Expand(1).MinMain(DPI(160));
    shell_.Add(scan_progress_).Fixed(DPI(2));
    shell_.Add(footer_panel_).Fixed(SizePx(48));
    RefreshModeVisuals();
    RefreshLayout();
}

void UiFileBrowser::RebuildWorkspace()
{
    workspace_.ClearItems();
    workspace_.SetGap(0).SetInset(0).SetAlignItems(UiCrossAlign::Stretch);
    workspace_.Add(places_panel_).Fixed(SizePx(205));
    if(browser_inspector_.GetCount() == 2)
        inspector_split_ = browser_inspector_.GetSplitPercent();
    browser_inspector_.Clear();
    if(mode_ == Mode::Advanced && show_inspector_.IsOn())
    {
        browser_inspector_.Horz(browser_panel_, inspector_panel_)
            .SetMinPixels(0, DPI(280))
            .SetMinPixels(1, DPI(180));
        browser_inspector_.SetSplitPercent(inspector_split_);
        workspace_.Add(browser_inspector_).Expand(1).MinMain(DPI(400));
    }
    else
        workspace_.Add(browser_panel_).Expand(1).MinMain(DPI(400));
    RebuildBrowserSurface();
    workspace_.Layout();
    RequestPreview();
}

void UiFileBrowser::RebuildBrowserSurface()
{
    browser_layout_.ClearItems();
    browser_layout_.SetGap(0).SetInset(0);
    browser_layout_.Add(browser_stack_).Expand(1);
    browser_stack_.SetActiveKey(details_view_ ? "details" : "thumbnails");
}

UiFileBrowser &UiFileBrowser::SetFolder(const String &folder, bool add_history)
{
    SetFolderInternal(folder, add_history);
    return *this;
}

void UiFileBrowser::SetFolderInternal(const String &folder, bool add_history)
{
    String normalized = NormalizePath(folder);
    if(normalized.IsEmpty())
    {
        auto notify = WhenError;
        notify("Folder does not exist: " + folder);
        return;
    }

    if(add_history)
    {
        while(history_.GetCount() - 1 > history_index_)
            history_.Drop();
        if(history_.IsEmpty() || history_.Top() != normalized)
        {
            history_.Add(normalized);
            history_index_ = history_.GetCount() - 1;
        }
    }

    bool changed = !SamePath(folder_, normalized);
    folder_ = normalized;
    if(changed)
    {
        search_.SetData("");
        restore_path_.Clear();
        restore_frame_ = Null;
    }
    ScanCurrentFolder();
    RefreshModeVisuals();
    RefreshBreadcrumbs();
    RefreshPlaces();

    auto notify = WhenFolderChanged;
    notify(folder_);
}

UiFileBrowser &UiFileBrowser::RefreshFolder()
{
    ScanCurrentFolder();
    return *this;
}

bool UiFileBrowser::EffectiveGroupSequences() const { return advanced_group_sequences_; }

void UiFileBrowser::ScanCurrentFolder()
{
    if(folder_.IsEmpty())
        return;
    if(HasSelection() && SamePath(model_->GetFolder(), folder_))
    {
        restore_path_ = (*model_)[selected_entry_].path;
        if(selected_frame_ >= 0)
            restore_frame_ = (*model_)[selected_entry_].frames[selected_frame_].number;
    }
    scanning_ = true;
    scan_error_.Clear();
    selected_entry_ = selected_frame_ = -1;
    RequestPreview(true);
    model_ = std::make_unique<UiFileBrowserModel>();
    expanded_entries_.Clear();
    RefreshProjection(false);
    open_button_.Disable();
    scan_progress_.SetIndeterminate(true);
    scan_generation_ = scanner_.Request(folder_, EffectiveGroupSequences());
    SetTimeCallback(-60, [this] { PollScan(); }, 1);
}

void UiFileBrowser::PollScan()
{
    UiFileBrowserScanner::Result result;
    if(!scanner_.Poll(result))
    {
        RefreshStatus();
        return;
    }
    if(result.generation != scan_generation_)
        return;
    KillTimeCallback(1);
    scanning_ = false;
    scan_progress_.Set(0, 1);
    scan_error_ = result.error;
    model_ = std::move(result.model);
    RefreshProjection(false);
    if(!restore_path_.IsEmpty())
    {
        for(int i = 0; i < model_->GetCount(); ++i)
            if(SamePath((*model_)[i].path, restore_path_))
            {
                int frame = -1;
                for(int f = 0; !IsNull(restore_frame_) && f < (*model_)[i].frames.GetCount(); ++f)
                    if((*model_)[i].frames[f].number == restore_frame_)
                    {
                        frame = f;
                        break;
                    }
                selected_entry_ = i;
                selected_frame_ = frame;
                SyncSelectionViews();
                RefreshInspector();
                RefreshFooter();
                break;
            }
    }
    restore_path_.Clear();
    restore_frame_ = Null;
    if(scan_error_.IsEmpty())
    {
        RememberFolder(folder_);
        RefreshPlaces();
    }
    open_button_.Enable(scan_error_.IsEmpty());
    if(!scan_error_.IsEmpty())
    {
        auto notify = WhenError;
        String error = scan_error_;
        notify(error);
    }
}

void UiFileBrowser::EditAddress(bool edit)
{
    address_stack_.SetActiveKey(edit ? "edit" : "crumbs");
    if(edit)
    {
        address_edit_.SetData(folder_);
        address_edit_.SetFocus();
        address_edit_.SelectAll();
    }
}

bool UiFileBrowser::Key(dword key, int count)
{
    if(key == K_CTRL_F)
    {
        SetOptionsOpen(true);
        search_.SetFocus();
        search_.SelectAll();
        return true;
    }
    if(key == K_CTRL_L)
    {
        EditAddress(true);
        return true;
    }
    if(key == K_ALT_LEFT)
    {
        GoBack();
        return true;
    }
    if(key == K_ALT_RIGHT)
    {
        GoForward();
        return true;
    }
    if(key == K_ALT_UP || key == K_BACKSPACE)
    {
        GoUp();
        return true;
    }
    if(key == K_F5)
    {
        RefreshFolder();
        return true;
    }
    if(key == K_ESCAPE)
    {
        if(options_open_)
        {
            CloseOptions();
            return true;
        }
        if(address_stack_.GetActiveKey() == "edit")
        {
            EditAddress(false);
            return true;
        }
        auto notify = WhenCancel;
        notify();
        return true;
    }
    return Ctrl::Key(key, count);
}

void UiFileBrowser::GoBack()
{
    if(history_index_ <= 0)
        return;
    --history_index_;
    SetFolderInternal(history_[history_index_], false);
}

void UiFileBrowser::GoForward()
{
    if(history_index_ < 0 || history_index_ + 1 >= history_.GetCount())
        return;
    ++history_index_;
    SetFolderInternal(history_[history_index_], false);
}

void UiFileBrowser::GoUp()
{
    String share_root = ShareRoot(folder_);
    if(!share_root.IsEmpty() && SamePath(folder_, share_root))
        return;
    String parent = GetFileFolder(folder_);
    if(!parent.IsEmpty() && !SamePath(parent, folder_))
        SetFolderInternal(parent, true);
}

void UiFileBrowser::GoHome()
{
    String home = GetHomeDirectory();
    if(!home.IsEmpty() && DirectoryExists(home))
        SetFolderInternal(home, true);
}

void UiFileBrowser::RefreshBreadcrumbs()
{
    breadcrumbs_.ClearItems();
    // Walk actual ancestors; this preserves drive roots and UNC share roots.
    Vector<String> ancestors;
    String path = folder_;
    String share_root = ShareRoot(folder_);
    while(!path.IsEmpty())
    {
        ancestors.Add(path);
        if(!share_root.IsEmpty() && SamePath(path, share_root))
            break;
        String parent = GetFileFolder(path);
        if(parent.IsEmpty() || SamePath(parent, path))
            break;
        path = parent;
    }
    for(int i = ancestors.GetCount() - 1; i >= 0; --i)
    {
        String label = GetFileName(ancestors[i]);
        if(label.IsEmpty())
            label = ancestors[i];
        breadcrumbs_.AddCrumb(label, ancestors[i]);
    }
    if(breadcrumbs_.GetCount())
        breadcrumbs_.SetCurrentIndex(breadcrumbs_.GetCount() - 1);
}

UiFileBrowser &UiFileBrowser::AddPlace(const String &folder)
{
    String path = NormalizePath(folder);
    if(!DirectoryExists(path))
        return *this;
    for(const String &saved : Session().places)
        if(SamePath(path, saved))
            return *this;
    Session().places.Add(path);
    RefreshPlaces();
    return *this;
}

void UiFileBrowser::RemovePlace(const String &folder)
{
    auto &places = Session().places;
    for(int i = places.GetCount() - 1; i >= 0; --i)
        if(SamePath(places[i], folder))
            places.Remove(i);
    RefreshPlaces();
}

UiFileBrowser &UiFileBrowser::AddRoot(const String &label, const String &folder)
{
    for(Root &root : roots_)
        if(SamePath(root.path, folder))
        {
            root.label = label;
            RefreshPlaces();
            return *this;
        }
    Root &root = roots_.Add();
    root.label = label;
    root.path = NormalizePath(folder);
    RefreshPlaces();
    return *this;
}

void UiFileBrowser::RefreshPlaces()
{
    UiModelUpdate update(places_.Model());
    places_.ClearModel();
    auto heading = [&](const char *text)
    {
        UiModelItem item(text);
        item.group_header = true;
        item.enabled = false;
        item.use_custom_font = true;
        item.custom_font = Font(font_).Height(max(1, font_.GetHeight() - DPI(2)));
        places_.Model().Add(item);
    };
    auto place = [&](const String &label, const String &path, Image icon)
    {
        if(!path.IsEmpty())
            places_.Model().Add(BrowserItem(label, path, icon));
    };
    place("Home", GetHomeDirectory(), ICON_DESIGN_HOME_48());
#ifdef PLATFORM_WIN32
    place("Desktop", GetDesktopFolder(), ICON_DESIGN_FOLDER_48());
    place("Documents", GetDocumentsFolder(), ICON_DESIGN_FOLDER_48());
    place("Pictures", GetPicturesFolder(), ICON_DESIGN_IMAGE_48());
#else
    for(const char *name : {"Desktop", "Documents", "Pictures"})
    {
        String path = AppendFileName(GetHomeDirectory(), name);
        if(DirectoryExists(path))
            place(name, path, ICON_DESIGN_FOLDER_48());
    }
#endif
    place("Application folder", GetExeFolder(), ICON_DESIGN_FOLDER_48());
    heading("PINNED FOLDERS");
    for(const String &path : Session().places)
        place(GetFileName(path).IsEmpty() ? path : GetFileName(path), path, ICON_DESIGN_FOLDER_48());
    UiModelItem drop_hint("  Drop here", Value(), false);
    drop_hint.use_custom_font = true;
    drop_hint.custom_font = Font(font_).Height(max(1, font_.GetHeight() - DPI(2)));
    places_.Model().Add(drop_hint);
    if(!roots_.IsEmpty())
    {
        heading("LOCATIONS");
        for(const Root &root : roots_)
            place(root.label, root.path, ICON_DESIGN_FOLDER_48());
    }
#ifdef PLATFORM_WIN32
    heading("DRIVES");
    DWORD drives = GetLogicalDrives();
    for(int i = 0; i < 26; ++i)
        if(drives & (1u << i))
        {
            String path = Format("%c:/", 'A' + i);
            place(path, path, ICON_DESIGN_FOLDER_48());
        }
#else
    heading("FILESYSTEM");
    place("Filesystem", "/", ICON_DESIGN_FOLDER_48());
#endif
    heading("RECENT");
    for(const String &path : Session().recent)
        place(GetFileName(path).IsEmpty() ? path : GetFileName(path), path, ICON_DESIGN_FOLDER_48());
    for(int i = 0; i < places_.Model().GetCount(); ++i)
        if(!IsNull(places_.Model().Get(i).data) && SamePath(AsString(places_.Model().Get(i).data), folder_))
        {
            places_.SetCursor(i);
            places_.Select(i);
            break;
        }
}

UiFileBrowser &UiFileBrowser::SetMode(Mode mode)
{
    if(mode_ == mode)
        return *this;
    CloseOptions();
    mode_ = mode;
    expanded_entries_.Clear();
    RebuildShell();
    RebuildWorkspace();
    RefreshProjection(true);
    return *this;
}

UiFileBrowser &UiFileBrowser::ShowOptions(bool show)
{
    SetOptionsOpen(show);
    return *this;
}

UiFileBrowser &UiFileBrowser::SetSequenceGrouping(bool grouped)
{
    if(advanced_group_sequences_ == grouped)
    {
        sequence_mode_.SelectByData(grouped ? 1 : 0);
        RefreshStatus();
        return *this;
    }
    advanced_group_sequences_ = grouped;
    sequence_mode_.SelectByData(grouped ? 1 : 0);
    ScanCurrentFolder();
    return *this;
}

UiFileBrowser &UiFileBrowser::ClearFilters()
{
    filters_.Clear();
    filter_selected_ = -1;
    RefreshFilterList();
    RefreshProjection(true);
    return *this;
}

UiFileBrowser &UiFileBrowser::AddFilter(const UiFileBrowserFilterRule &rule)
{
    UiFileBrowserFilterRule &dst = filters_.Add();
    dst.include = rule.include;
    dst.field = rule.field;
    dst.value = rule.value;
    RefreshFilterList();
    RefreshProjection(true);
    return *this;
}

bool UiFileBrowser::PassesBaseFileType(const UiFileBrowserEntry &entry) const
{
    int type = file_type_.HasSelection() ? (int)file_type_.GetSelectedData() : 0;
    if(type != 4 && entry.IsDirectory())
        return true;
    switch(type)
    {
    case 0:
        return entry.IsDirectory() || entry.IsSequence() || entry.kind == UiFileBrowserEntryKind::Image ||
               entry.IsVideo();
    case 1:
        return entry.IsDirectory() || entry.IsSequence();
    case 2:
        return entry.IsDirectory() || entry.kind == UiFileBrowserEntryKind::Image;
    case 3:
        return entry.IsDirectory() || entry.IsVideo();
    case 4:
        return entry.IsDirectory();
    case 5:
        return true;
    default:
        return true;
    }
}

bool UiFileBrowser::PassesProjection(const UiFileBrowserEntry &entry) const
{
    if(!PassesBaseFileType(entry))
        return false;

    String query = ToLower(TrimBoth(search_.GetData().ToString()));
    if(!query.IsEmpty())
    {
        String hay =
            ToLower(entry.name + " " + entry.friendly_name + " " + entry.pattern + " " + entry.type_label);
        if(hay.Find(query) < 0)
            return false;
    }

    if(mode_ != Mode::Advanced || !filter_enable_.IsOn())
        return true;

    if(entry.IsDirectory())
        return true;
    for(const UiFileBrowserFilterRule &rule : filters_)
    {
        bool match = UiFileBrowserModel::MatchesFilter(entry, rule);
        if(rule.include && !match)
            return false;
        if(!rule.include && match)
            return false;
    }
    return true;
}

String UiFileBrowser::EntryDisplayName(const UiFileBrowserEntry &entry) const
{
    if(!entry.IsSequence())
        return entry.name;
    if(mode_ == Mode::Simple)
        return entry.friendly_name;
    int pattern = pattern_mode_.HasSelection() ? (int)pattern_mode_.GetSelectedData() : 0;
    if(pattern == 2)
        return entry.friendly_name;
    if(pattern == 1)
    {
        String hashes;
        hashes.Cat('#', max(1, entry.padding));
        String out = entry.pattern;
        String token = String("%0") + AsString(entry.padding) + "d";
        out.Replace(token, hashes);
        return out;
    }
    return entry.pattern;
}

String UiFileBrowser::SequenceFrameRange(const UiFileBrowserEntry &entry) const
{
    if(!entry.IsSequence())
        return "—";
    return Format("%s–%s ×%d", FormatFrameNumber(entry.first, entry.padding),
                  FormatFrameNumber(entry.last, entry.padding), max(1, entry.increment));
}

String UiFileBrowser::FormatFrameNumber(int64 frame, int padding) const
{
    if(padding <= 1)
        return AsString(frame);
    return FormatIntBase(frame, 10, padding, '0');
}

void UiFileBrowser::RefreshProjection(bool keep_selection)
{
    String selected_path;
    int64 selected_number = Null;
    if(keep_selection && HasSelection())
    {
        const UiFileBrowserEntry &old = (*model_)[selected_entry_];
        selected_path = old.path;
        if(selected_frame_ >= 0 && selected_frame_ < old.frames.GetCount())
            selected_number = old.frames[selected_frame_].number;
    }

    Vector<int> visible;
    for(int i = 0; i < model_->GetCount(); ++i)
        if(PassesProjection((*model_)[i]))
            visible.Add(i);

    int sort = sort_key_.HasSelection() ? (int)sort_key_.GetSelectedData() : 0;
    int placement = folder_placement_.HasSelection() ? (int)folder_placement_.GetSelectedData() : 0;
    Sort(visible,
         [&](int ai, int bi)
         {
             const UiFileBrowserEntry &a = (*model_)[ai];
             const UiFileBrowserEntry &b = (*model_)[bi];
             if(placement != 2 && a.IsDirectory() != b.IsDirectory())
                 return placement == 0 ? a.IsDirectory() : !a.IsDirectory();
             if(sort == 2 || sort == 3)
             {
                 int cmp = SgnCompare(a.modified, b.modified);
                 if(cmp != 0)
                     return sort == 2 ? cmp > 0 : cmp < 0;
             }
             else if(sort == 4)
             {
                 if(a.size != b.size)
                     return a.size > b.size;
             }
             else
             {
                 int cmp = UiFileBrowserModel::CompareNames(a.friendly_name, b.friendly_name);
                 if(cmp != 0)
                     return sort == 1 ? cmp > 0 : cmp < 0;
             }
             return ai < bi;
         });

    rows_.Clear();
    gallery_entries_.Clear();
    for(int entry_index : visible)
    {
        gallery_entries_.Add(entry_index);
        RowRef &top = rows_.Add();
        top.entry = entry_index;
        top.frame = -1;

        const UiFileBrowserEntry &entry = (*model_)[entry_index];
        if(mode_ == Mode::Advanced && entry.IsSequence() && IsSequenceExpanded(entry_index))
        {
            for(int fi = 0; fi < entry.frames.GetCount(); ++fi)
            {
                if(!ShouldShowFrame(entry, fi))
                    continue;
                RowRef &child = rows_.Add();
                child.entry = entry_index;
                child.frame = fi;
            }
        }
    }

    syncing_ = true;
    if(details_view_)
        RefreshTable();
    else
        RefreshGallery();
    syncing_ = false;
    RefreshStatus();

    selected_entry_ = -1;
    selected_frame_ = -1;
    if(!selected_path.IsEmpty())
    {
        for(int i : visible)
        {
            if((*model_)[i].path == selected_path)
            {
                selected_entry_ = i;
                if(!IsNull(selected_number) && (*model_)[i].IsSequence())
                {
                    for(int f = 0; f < (*model_)[i].frames.GetCount(); ++f)
                        if((*model_)[i].frames[f].number == selected_number)
                        {
                            selected_frame_ = f;
                            break;
                        }
                }
                break;
            }
        }
    }
    if(selected_entry_ < 0 && !visible.IsEmpty())
        selected_entry_ = visible[0];

    SyncSelectionViews();
    RefreshInspector();
    RefreshFooter();
}

void UiFileBrowser::RefreshTable()
{
    UiModelUpdate update(table_model_);
    int cols = mode_ == Mode::Simple ? 3 : 6;
    table_model_.SetSize(rows_.GetCount(), cols);

    if(mode_ == Mode::Simple)
    {
        table_model_.SetHeader(UITABLE_COLUMN_AXIS, 0, UiTableHeader("Name"));
        table_model_.SetHeader(UITABLE_COLUMN_AXIS, 1, UiTableHeader("Type"));
        table_model_.SetHeader(UITABLE_COLUMN_AXIS, 2, UiTableHeader("Modified"));
        table_.SetColumnWidth(0, SizePx(390));
        table_.SetColumnWidth(1, SizePx(150));
        table_.SetColumnWidth(2, SizePx(145));
    }
    else
    {
        UiTableHeader disclosure_header("›");
        disclosure_header.tooltip = "Sequence members · activate an arrow to expand or collapse";
        table_model_.SetHeader(UITABLE_COLUMN_AXIS, 0, disclosure_header);
        table_model_.SetHeader(UITABLE_COLUMN_AXIS, 1, UiTableHeader("Name"));
        table_model_.SetHeader(UITABLE_COLUMN_AXIS, 2, UiTableHeader("Frames"));
        table_model_.SetHeader(UITABLE_COLUMN_AXIS, 3, UiTableHeader("Type"));
        table_model_.SetHeader(UITABLE_COLUMN_AXIS, 4, UiTableHeader("Size"));
        table_model_.SetHeader(UITABLE_COLUMN_AXIS, 5, UiTableHeader("Modified"));
        table_.SetColumnWidth(0, SizePx(18));
        table_.SetColumnWidth(1, SizePx(330));
        table_.SetColumnWidth(2, show_frames_.IsOn() ? SizePx(110) : SizePx(1));
        table_.SetColumnWidth(3, SizePx(145));
        table_.SetColumnWidth(4, show_size_.IsOn() ? SizePx(85) : SizePx(1));
        table_.SetColumnWidth(5, SizePx(145));
    }

    for(int r = 0; r < rows_.GetCount(); ++r)
    {
        const RowRef &ref = rows_[r];
        const UiFileBrowserEntry &entry = (*model_)[ref.entry];

        if(ref.frame < 0)
        {
            if(mode_ == Mode::Simple)
            {
                UiTableCell name;
                name.value = EntryDisplayName(entry);
                name.icon = EntryIcon(entry);
                name.icon_render_mode = UiIconRenderMode::PreserveColor;
                name.editable = false;
                name.ink = NameInk(entry);
                name.use_custom_ink = !IsNull(name.ink);
                UiTableCell type;
                type.value = entry.type_label;
                type.editable = false;
                type.ink = TypeInk(entry);
                type.use_custom_ink = !IsNull(type.ink);
                UiTableCell modified;
                modified.value = UiFileBrowserModel::FormatModified(entry.modified);
                modified.editable = false;
                table_model_.SetCell(r, 0, name);
                table_model_.SetCell(r, 1, type);
                table_model_.SetCell(r, 2, modified);
            }
            else
            {
                UiTableCell disclosure;
                disclosure.value = entry.IsSequence() ? (IsSequenceExpanded(ref.entry) ? "▼" : "▶") : "";
                disclosure.editable = false;
                if(entry.IsSequence())
                    disclosure.tooltip = "Double-click or press Enter to show/hide individual frames";
                UiTableCell name;
                name.value = EntryDisplayName(entry);
                name.icon = EntryIcon(entry);
                name.icon_render_mode = UiIconRenderMode::PreserveColor;
                name.editable = false;
                name.ink = NameInk(entry);
                name.use_custom_ink = !IsNull(name.ink);
                UiTableCell frames;
                frames.value = entry.IsSequence()
                                   ? SequenceFrameRange(entry)
                                   : (entry.IsVideo()                               ? String("movie")
                                      : entry.kind == UiFileBrowserEntryKind::Image ? String("1")
                                                                                    : String("—"));
                frames.editable = false;
                UiTableCell type;
                type.value = entry.type_label;
                type.editable = false;
                type.ink = TypeInk(entry);
                type.use_custom_ink = !IsNull(type.ink);
                if(entry.IsSequence() && !entry.complete && missing_badge_.IsChecked())
                {
                    type.value = Format("%s · %d missing", entry.type_label,
                                        entry.GetExpectedFrameCount() - entry.GetPresentFrameCount());
                    type.has_warning = true;
                }
                UiTableCell size;
                size.value = entry.IsDirectory() ? String("—") : UiFileBrowserModel::FormatBytes(entry.size);
                size.align = ALIGN_RIGHT;
                size.editable = false;
                UiTableCell modified;
                modified.value = UiFileBrowserModel::FormatModified(entry.modified);
                modified.editable = false;
                table_model_.SetCell(r, 0, disclosure);
                table_model_.SetCell(r, 1, name);
                table_model_.SetCell(r, 2, frames);
                table_model_.SetCell(r, 3, type);
                table_model_.SetCell(r, 4, size);
                table_model_.SetCell(r, 5, modified);
            }
        }
        else
        {
            const UiFileBrowserFrame &frame = entry.frames[ref.frame];
            UiTableCell disclosure;
            disclosure.value = "";
            disclosure.editable = false;
            UiTableCell name;
            name.value = frame.missing
                             ? Format("%s · missing frame", FormatFrameNumber(frame.number, entry.padding))
                             : "    " + GetFileName(frame.path);
            name.icon = ICON_DESIGN_IMAGE_48();
            name.editable = false;
            name.ink = NameInk(entry);
            name.use_custom_ink = !IsNull(name.ink);
            if(frame.missing)
                name.has_warning = true;
            UiTableCell frames;
            frames.value = FormatFrameNumber(frame.number, entry.padding);
            frames.editable = false;
            UiTableCell type;
            type.value = frame.missing ? String("Missing") : String("Frame");
            type.editable = false;
            type.has_warning = frame.missing;
            UiTableCell size;
            size.value = frame.missing ? String("—") : UiFileBrowserModel::FormatBytes(frame.size);
            size.align = ALIGN_RIGHT;
            size.editable = false;
            UiTableCell modified;
            modified.value = frame.missing ? String("—") : UiFileBrowserModel::FormatModified(frame.modified);
            modified.editable = false;
            table_model_.SetCell(r, 0, disclosure);
            table_model_.SetCell(r, 1, name);
            table_model_.SetCell(r, 2, frames);
            table_model_.SetCell(r, 3, type);
            table_model_.SetCell(r, 4, size);
            table_model_.SetCell(r, 5, modified);
        }
    }
}

void UiFileBrowser::RefreshGallery()
{
    UiModelUpdate update(gallery_model_);
    gallery_model_.Clear();
    for(int entry_index : gallery_entries_)
    {
        const UiFileBrowserEntry &entry = (*model_)[entry_index];
        UiModelItem item(EntryDisplayName(entry), entry_index);
        item.icon = EntryIcon(entry);
        item.icon_render_mode = UiIconRenderMode::PreserveColor;
        item.custom_ink_color = NameInk(entry);
        if(entry.IsSequence())
        {
            item.description = Format("%d / %d frames · %s", entry.GetPresentFrameCount(),
                                      entry.GetExpectedFrameCount(), entry.type_label);
            if(!entry.complete && missing_badge_.IsChecked())
            {
                item.has_metadata = true;
                item.metadata_color = TypeInk(entry);
                item.right_text =
                    Format("%d missing", entry.GetExpectedFrameCount() - entry.GetPresentFrameCount());
            }
        }
        else
            item.description =
                entry.type_label +
                (entry.IsDirectory() ? String() : " · " + UiFileBrowserModel::FormatBytes(entry.size));
        gallery_model_.Add(item);
    }
}

void UiFileBrowser::RefreshFilterList()
{
    filter_list_.ClearModel();
    for(int i = 0; i < filters_.GetCount(); ++i)
    {
        UiModelItem item(filters_[i].GetTitle(), i);
        filter_list_.Model().Add(item);
    }
    filter_count_.SetText(AsString(filters_.GetCount()));
    if(filter_selected_ >= 0 && filter_selected_ < filters_.GetCount())
        filter_list_.SetCursor(filter_selected_).Select(filter_selected_);
}

void UiFileBrowser::RefreshInspector()
{
    RequestPreview();
    RebuildInspectorDetails();
    if(!HasSelection())
    {
        preview_title_.SetText("NO SELECTION");
        preview_subtitle_.SetText("Select media to inspect");
        inspector_name_.SetText("No selection");
        for(UiLabel &value : meta_values_)
            value.SetText("—");
        coverage_label_.SetText("Frame coverage");
        coverage_.Set(0, 1);
        return;
    }

    const UiFileBrowserEntry &entry = (*model_)[selected_entry_];

    inspector_name_.SetText(selected_frame_ >= 0 ? GetFileName(GetSelection().path)
                                                 : EntryDisplayName(entry));

    meta_values_[0].SetText(entry.IsSequence()                            ? "Image sequence"
                            : entry.IsDirectory()                         ? "Folder"
                            : entry.IsVideo()                             ? "Video file"
                            : entry.kind == UiFileBrowserEntryKind::Image ? "Still image"
                                                                          : "File");
    meta_values_[1].SetText(entry.IsSequence() ? SequenceFrameRange(entry) : "—");
    meta_values_[2].SetText(entry.type_label);
    meta_values_[3].SetText(UiFileBrowserModel::FormatModified(entry.modified));
    meta_values_[4].SetText(entry.IsDirectory() ? "—" : UiFileBrowserModel::FormatBytes(entry.size));

    if(entry.IsSequence())
    {
        int present = entry.GetPresentFrameCount();
        int expected = max(1, entry.GetExpectedFrameCount());
        coverage_label_.SetText(Format("Frame coverage · %d / %d present", present, expected));
        coverage_.Set(present, expected);
        health_label_.SetText(Format("Selected sequence health · %d / %d present", present, expected));
        health_.Set(present, expected);
    }
    else
    {
        coverage_label_.SetText("Frame coverage");
        coverage_.Set(0, 1);
        health_label_.SetText("Select an image sequence");
        health_.Set(0, 1);
    }
}

void UiFileBrowser::RefreshFooter()
{
    if(!HasSelection())
    {
        footer_name_.Clear();
        name_edit_.SetData(String());
        open_button_.SetText("Open");
        return;
    }
    const UiFileBrowserEntry &entry = (*model_)[selected_entry_];
    if(selected_frame_ >= 0 && entry.IsSequence() && selected_frame_ < entry.frames.GetCount())
    {
        const UiFileBrowserFrame &frame = entry.frames[selected_frame_];
        name_edit_.SetData(frame.missing ? FormatFrameNumber(frame.number, entry.padding)
                                         : GetFileName(frame.path));
        open_button_.SetText(frame.missing ? "Missing" : "Open frame");
    }
    else
    {
        name_edit_.SetData(EntryDisplayName(entry));
        open_button_.SetText(entry.IsDirectory()  ? "Open folder"
                             : entry.IsSequence() ? "Open sequence"
                                                  : "Open");
    }
    footer_name_ = AsString(name_edit_.GetData());
}

void UiFileBrowser::RefreshStatus()
{
    item_count_.SetText(scanning_                ? Format("Scanning · %d entries", scanner_.GetScannedCount())
                        : !scan_error_.IsEmpty() ? scan_error_
                                                 : Format("%d visible / %d items",
                                                          gallery_entries_.GetCount(), model_->GetCount()));
    filter_button_.SetText(filters_.IsEmpty() ? "Filter ▾" : Format("Filter (%d) ▾", filters_.GetCount()));
    sequence_button_.Tip(EffectiveGroupSequences() ? "Sequence · grouped" : "Sequence · individual frames");
}

void UiFileBrowser::RefreshModeVisuals()
{
    mode_button_.SetChecked(mode_ == Mode::Advanced)
        .Tip(mode_ == Mode::Advanced ? "Advanced mode · click for Simple"
                                     : "Simple mode · click for Advanced");
    options_button_.SetChecked(options_open_ && options_page_ == OptionsPage::Filter);
    filter_button_.SetChecked(options_open_ && options_page_ == OptionsPage::Filter);
    sequence_button_.SetChecked(options_open_ && options_page_ == OptionsPage::Sequence);
    view_button_.SetChecked(options_open_ && options_page_ == OptionsPage::View);
    sort_button_.SetChecked(options_open_ && options_page_ == OptionsPage::Sort);
    new_button_.SetChecked(options_open_ && options_page_ == OptionsPage::NewItem);
    UiFileBrowserEntry folder_entry;
    folder_entry.kind = UiFileBrowserEntryKind::Folder;
    Color active_ink = UiFileBrowserAppearance::TypeInk(folder_entry);
    mode_button_.SetIconColor(mode_ == Mode::Advanced ? active_ink : table_.GetStyle().muted_ink, 12, 20);
    options_button_.SetIconColor(active_ink, 12, 20);
    StyleToolbar();
    String query = TrimBoth(AsString(search_.GetData()));
    options_button_.Tip(query.IsEmpty() ? "Search / filter (Ctrl+F)"
                                        : "Search / filter · active search: " + query);
    filter_enable_.Enable(mode_ == Mode::Advanced);
    filter_hint_.SetText(mode_ == Mode::Advanced ? "Rules are ordered; search filters this folder."
                                                 : "Search this folder; advanced rules are inactive.");
    details_button_.SetChecked(details_view_);
    thumbnails_button_.SetChecked(!details_view_);
    back_button_.Enable(history_index_ > 0);
    forward_button_.Enable(history_index_ >= 0 && history_index_ + 1 < history_.GetCount());
}

void UiFileBrowser::HandleTableSelection()
{
    if(syncing_)
        return;
    UiTablePos pos = table_.GetActiveCell();
    if(pos.row < 0 || pos.row >= rows_.GetCount())
        return;
    SelectEntry(rows_[pos.row].entry, rows_[pos.row].frame, false);
}

void UiFileBrowser::HandleTableAction()
{
    UiTablePos pos = table_.GetActiveCell();
    if(pos.row < 0 || pos.row >= rows_.GetCount())
        return;
    const RowRef &ref = rows_[pos.row];
    const UiFileBrowserEntry &entry = (*model_)[ref.entry];

    if(ref.frame < 0 && entry.IsDirectory())
    {
        SetFolderInternal(entry.path, true);
        return;
    }
    if(ref.frame < 0 && entry.IsSequence() && mode_ == Mode::Advanced && pos.col == 0)
    {
        ToggleSequenceExpanded(ref.entry);
        return;
    }
    AcceptSelection();
}

void UiFileBrowser::HandleGallerySelection()
{
    if(syncing_)
        return;
    int index = gallery_.GetCursor();
    if(index < 0 || index >= gallery_entries_.GetCount())
        return;
    SelectEntry(gallery_entries_[index], -1, false);
}

void UiFileBrowser::HandleGalleryAction()
{
    int index = gallery_.GetCursor();
    if(index < 0 || index >= gallery_entries_.GetCount())
        return;
    const UiFileBrowserEntry &entry = (*model_)[gallery_entries_[index]];
    if(entry.IsDirectory())
        SetFolderInternal(entry.path, true);
    else
        AcceptSelection();
}

void UiFileBrowser::HandlePlaceAction()
{
    int index = places_.GetCursor();
    if(index < 0 || index >= places_.Model().GetCount())
        return;
    String path = AsString(places_.Model().Get(index).data);
    if(!IsNull(places_.Model().Get(index).data) && !path.IsEmpty() && !SamePath(path, folder_))
        SetFolderInternal(path, true);
}

void UiFileBrowser::SelectEntry(int entry, int frame, bool sync_views)
{
    if(entry < 0 || entry >= model_->GetCount())
        return;
    selected_entry_ = entry;
    selected_frame_ = frame;
    if(sync_views)
        SyncSelectionViews();
    RefreshInspector();
    RefreshFooter();
    UiFileBrowserSelection selection = GetSelection();
    auto notify = WhenSelection;
    notify(selection);
}

void UiFileBrowser::SyncSelectionViews()
{
    if(syncing_)
        return;
    syncing_ = true;
    table_.ClearSelection();
    gallery_.ClearSelection();
    gallery_.SetCursor(-1);
    for(int r = 0; r < rows_.GetCount(); ++r)
        if(rows_[r].entry == selected_entry_ && rows_[r].frame == selected_frame_)
        {
            table_.SetActiveCell(r, mode_ == Mode::Simple ? 0 : 1);
            table_.SetSelection(UiTableRange(r, 0, r, table_model_.GetColumnCount() - 1));
            break;
        }
    for(int i = 0; i < gallery_entries_.GetCount(); ++i)
        if(gallery_entries_[i] == selected_entry_)
        {
            gallery_.SetCursor(i);
            gallery_.Select(i);
            break;
        }
    syncing_ = false;
}

bool UiFileBrowser::IsSequenceExpanded(int entry) const { return expanded_entries_.Find(entry) >= 0; }

void UiFileBrowser::ToggleSequenceExpanded(int entry)
{
    int found = expanded_entries_.Find(entry);
    if(found >= 0)
        expanded_entries_.Remove(found);
    else
        expanded_entries_.FindAdd(entry);
    RefreshProjection(true);
}

bool UiFileBrowser::ShouldShowFrame(const UiFileBrowserEntry &entry, int frame_index) const
{
    if(frame_index < 0 || frame_index >= entry.frames.GetCount())
        return false;
    int view = frame_view_.HasSelection() ? (int)frame_view_.GetSelectedData() : 0;
    if(view == 2)
        return entry.frames[frame_index].missing;
    if(view == 3)
        return frame_index == 0 || frame_index == entry.frames.GetCount() - 1;
    return true;
}

void UiFileBrowser::SetDetailsView(bool details)
{
    details_view_ = details;
    RefreshProjection(true);
    browser_stack_.SetActiveKey(details ? "details" : "thumbnails");
    RefreshModeVisuals();
}

void UiFileBrowser::SetOptionsOpen(bool open)
{
    if(open && options_open_ && options_page_ == OptionsPage::Filter)
        return;
    if(open)
        OpenOptions(OptionsPage::Filter, mode_ == Mode::Advanced ? filter_button_ : options_button_);
    else
        CloseOptions();
}

void UiFileBrowser::SetGroupSequences(bool grouped) { SetSequenceGrouping(grouped); }

// View size changes presentation without scanning or changing directory records.
UiFileBrowser &UiFileBrowser::SetViewSize(ViewSize size)
{
    if(size != ViewSize::Small && size != ViewSize::Medium && size != ViewSize::Large)
        return *this;
    view_size_ = size;
    Session().view_size = size;
    for(int i = 0; i < 3; ++i)
        view_size_buttons_[i].SetChecked((int)size == i);
    font_ = Font(base_font_).Height(max(1, base_font_.GetHeight() + DPI(2 * ((int)size - 1))));
    ApplyTheme();
    ApplyViewSizeLayout();
    RefreshProjection(true);
    return *this;
}

void UiFileBrowser::LoadSelectedFilter()
{
    filter_selected_ = filter_list_.GetCursor();
    if(filter_selected_ < 0 || filter_selected_ >= filters_.GetCount())
    {
        filter_selected_ = -1;
        return;
    }
    const UiFileBrowserFilterRule &rule = filters_[filter_selected_];
    filter_action_.SelectByData(rule.include ? 1 : 0);
    filter_field_.SelectByData((int)rule.field);
    filter_value_.SetData(rule.value);
}

void UiFileBrowser::AddFilterFromEditor()
{
    UiFileBrowserFilterRule &rule = filters_.Add();
    rule.include = (int)filter_action_.GetSelectedData() == 1;
    rule.field = (UiFileBrowserFilterField)(int)filter_field_.GetSelectedData();
    rule.value = TrimBoth(filter_value_.GetData().ToString());
    filter_selected_ = filters_.GetCount() - 1;
    RefreshFilterList();
    RefreshProjection(true);
}

void UiFileBrowser::SaveFilterFromEditor()
{
    if(filter_selected_ < 0 || filter_selected_ >= filters_.GetCount())
    {
        AddFilterFromEditor();
        return;
    }
    UiFileBrowserFilterRule &rule = filters_[filter_selected_];
    rule.include = (int)filter_action_.GetSelectedData() == 1;
    rule.field = (UiFileBrowserFilterField)(int)filter_field_.GetSelectedData();
    rule.value = TrimBoth(filter_value_.GetData().ToString());
    RefreshFilterList();
    RefreshProjection(true);
}

void UiFileBrowser::RemoveSelectedFilter()
{
    if(filter_selected_ < 0 || filter_selected_ >= filters_.GetCount())
        return;
    filters_.Remove(filter_selected_);
    filter_selected_ = min(filter_selected_, filters_.GetCount() - 1);
    RefreshFilterList();
    RefreshProjection(true);
}

void UiFileBrowser::ReorderFilter(int from, int before)
{
    if(from < 0 || from >= filters_.GetCount() || before < 0 || before > filters_.GetCount() ||
       before == from || before == from + 1)
        return;
    UiFileBrowserFilterRule moving;
    moving.include = filters_[from].include;
    moving.field = filters_[from].field;
    moving.value = filters_[from].value;
    int to = before > from ? before - 1 : before;
    filters_.Remove(from);
    filters_.Insert(to, pick(moving));
    filter_selected_ = to;
    RefreshFilterList();
    RefreshProjection(true);
}

UiFileBrowserSelection UiFileBrowser::GetSelection() const
{
    UiFileBrowserSelection out;
    if(!HasSelection())
        return out;
    const UiFileBrowserEntry &entry = (*model_)[selected_entry_];
    if(selected_frame_ >= 0 && entry.IsSequence() && selected_frame_ < entry.frames.GetCount())
    {
        const UiFileBrowserFrame &frame = entry.frames[selected_frame_];
        if(!frame.missing)
        {
            out.path = frame.path;
            out.frame = frame.number;
        }
        return out;
    }
    out.path = entry.path;
    out.directory = entry.IsDirectory();
    out.sequence = entry.IsSequence();
    return out;
}

void UiFileBrowser::SubmitName()
{
    if(scanning_)
        return;
    String value = TrimBoth(AsString(name_edit_.GetData()));
    if(value.IsEmpty())
        return;
    if(value == footer_name_ && HasSelection())
    {
        AcceptSelection();
        return;
    }
    for(int i = 0; i < model_->GetCount(); ++i)
    {
        const auto &entry = (*model_)[i];
        if(value == EntryDisplayName(entry) || value == entry.pattern || value == entry.name)
        {
            selected_entry_ = i;
            selected_frame_ = -1;
            RefreshFooter();
            AcceptSelection();
            return;
        }
    }
    String path = NormalizePath(IsFullPath(value) ? value : AppendFileName(folder_, value));
    if(DirectoryExists(path))
    {
        SetFolderInternal(path, true);
        return;
    }
    if(FileExists(path))
    {
        UiFileBrowserSelection selection;
        selection.path = path;
        auto notify = WhenAccept;
        notify(selection);
        return;
    }
    auto notify = WhenError;
    notify("File or folder not found: " + path);
}

void UiFileBrowser::AcceptSelection(bool force_sequence, bool force_frame, bool force_folder)
{
    UiFileBrowserSelection selection = GetSelection();
    if(!force_folder && !force_sequence && !force_frame && AsString(name_edit_.GetData()) != footer_name_)
    {
        SubmitName();
        return;
    }

    if(force_folder)
    {
        selection.path = folder_;
        selection.directory = true;
        selection.sequence = false;
        selection.frame = Null;
    }
    else if(force_frame && HasSelection())
    {
        const UiFileBrowserEntry &entry = (*model_)[selected_entry_];
        if(entry.IsSequence())
        {
            int frame_index = selected_frame_;
            if(frame_index < 0)
            {
                for(int i = 0; i < entry.frames.GetCount(); ++i)
                    if(!entry.frames[i].missing)
                    {
                        frame_index = i;
                        break;
                    }
            }
            if(frame_index >= 0 && frame_index < entry.frames.GetCount() &&
               !entry.frames[frame_index].missing)
            {
                selection.path = entry.frames[frame_index].path;
                selection.directory = false;
                selection.sequence = false;
                selection.frame = entry.frames[frame_index].number;
            }
        }
        else
            selection.sequence = false;
    }
    else if(force_sequence && HasSelection())
    {
        const UiFileBrowserEntry &entry = (*model_)[selected_entry_];
        if(entry.IsSequence())
        {
            selection.path = entry.path;
            selection.sequence = true;
            selection.directory = false;
        }
    }

    if(scanning_ || !scan_error_.IsEmpty() || !selection.IsValid())
        return;
    if(selection.directory && !force_folder)
    {
        SetFolderInternal(selection.path, true);
        return;
    }
    CloseOptions();
    auto notify = WhenAccept;
    notify(selection);
}

UiFileBrowserDialog::UiFileBrowserDialog()
{
    Title("Open media").Sizeable().Zoomable();
    SetRect(0, 0, DPI(1180), DPI(720));
    Add(browser_.SizePos());
    browser_.WhenAccept = [this](const UiFileBrowserSelection &selection)
    {
        selection_ = selection;
        AcceptBreak(IDOK);
    };
    browser_.WhenCancel = [this] { RejectBreak(IDCANCEL); };
}

bool UiFileBrowserDialog::Execute(const String &folder, bool discover_sequences,
                                  UiFileBrowserSelection &selection)
{
    selection_ = UiFileBrowserSelection();
    browser_.SetMode(discover_sequences ? UiFileBrowser::Mode::Advanced : UiFileBrowser::Mode::Simple);
    browser_.SetSequenceGrouping(discover_sequences);
    if(!folder.IsEmpty())
        browser_.SetFolder(folder);
    if(Run() != IDOK || !selection_.IsValid())
        return false;
    selection = selection_;
    return true;
}

} // namespace Upp
