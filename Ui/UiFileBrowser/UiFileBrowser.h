/*
    UiFileBrowser — embeddable native file and image-sequence selector.

    Composes navigation, session pins/recent folders, host locations,
    grouped sequences, filtering, detail/tile views, bounded image/text previews
    and basic metadata. New creates folders, text, C++ pairs and bounded starter
    images. Host preview providers add formats without dependencies.
    Filesystem enumeration/grouping runs on a cancellable worker; controls and
    callbacks stay on the GUI thread. There is no imaging/playback dependency.

    The scan model is authoritative. Read-only table/gallery models are derived
    projections and outlive their views. Closing cancels pending scan generations;
    queued workers own their state independently of this control's lifetime.

    The host owns UiTheme, Body/Code typography and StdFont. The browser follows
    their changes, retaining
    structural choices (square surrounding panels, compact layout, thin dividers).
    SetFont supplies an explicit font; UseThemeFont restores host inheritance.
    Optional theme buttons request a host change rather than changing global state.

    Typical use: add browser.SizePos(), call SetFolder(path), handle WhenAccept
    and WhenCancel. These callbacks may close/destroy the host synchronously.
    UiFileBrowserDialog supplies modal Open/Cancel policy for the same control.
*/
#ifndef _UiFileBrowser_UiFileBrowser_h_
#define _UiFileBrowser_UiFileBrowser_h_

#include "UiFileBrowserAppearance.h"
#include "UiFileBrowserCreate.h"
#include "UiFileBrowserPreview.h"
#include "UiFileBrowserScanner.h"
#include <Ui/Ui.h>

namespace Upp
{

class UiFileBrowserPlaces : public UiList
{
public:
    Event<> WhenNavigate;
    Event<const Vector<String> &> WhenDropFolders;
    void LeftUp(Point p, dword flags) override;
    void DragAndDrop(Point p, PasteClip &clip) override;
};

class UiFileBrowserTable : public UiTable
{
public:
    Function<String()> DragFolder;
    void LeftDrag(Point p, dword flags) override;
};

// One owned popup hosts the existing option controls. A child dropdown may
// take focus without dismissing its parent; outside focus and Escape close it.
class UiFileBrowserOptionsPopup : public Ctrl
{
public:
    Event<> WhenDismiss;
    void DeactivateBy(Ctrl *next) override;
    bool Key(dword key, int count) override;
};

struct UiFileBrowserSelection : Moveable<UiFileBrowserSelection>
{
    String path;
    bool directory = false;
    bool sequence = false;
    int64 frame = Null;

    bool IsValid() const { return !path.IsEmpty(); }
};

class UiFileBrowser : public Ctrl
{
public:
    typedef UiFileBrowser CLASSNAME;

    enum class Mode : byte
    {
        Simple,
        Advanced
    };
    enum class ViewSize : byte
    {
        Small,
        Medium,
        Large
    };

    UiFileBrowser();
    ~UiFileBrowser();

    UiFileBrowser &SetFolder(const String &folder, bool add_history = true);
    UiFileBrowser &RefreshFolder();
    String GetFolder() const { return folder_; }
    bool IsScanning() const { return scanning_; }
    int GetEntryCount() const { return model_->GetCount(); }
    UiFileBrowser &AddPlace(const String &folder);
    UiFileBrowser &AddRoot(const String &label, const String &folder);
    // Compatibility with early CineView hosts; roots are generic locations.
    UiFileBrowser &AddProductionRoot(const String &label, const String &folder)
    {
        return AddRoot(label, folder);
    }
    void RemovePlace(const String &folder);
    bool Key(dword key, int count) override;

    UiFileBrowser &SetMode(Mode mode);
    Mode GetMode() const { return mode_; }
    UiFileBrowser &SetViewSize(ViewSize size);
    ViewSize GetViewSize() const { return view_size_; }
    // Legacy entry point for the compact Search/Filter popup (on an open browser).
    UiFileBrowser &ShowOptions(bool show);
    // Optional worker-side decoder. False delegates to JPEG/PNG/text defaults.
    UiFileBrowser &SetPreviewProvider(UiFileBrowserPreviewProvider provider);
    UiFileBrowser &SetSequenceGrouping(bool grouped);
    bool IsSequenceGrouping() const { return advanced_group_sequences_; }

    UiFileBrowserSelection GetSelection() const;
    bool HasSelection() const
    {
        return !scanning_ && selected_entry_ >= 0 && selected_entry_ < model_->GetCount();
    }

    UiFileBrowser &ClearFilters();
    UiFileBrowser &AddFilter(const UiFileBrowserFilterRule &rule);
    int GetFilterCount() const { return filters_.GetCount(); }

    void ApplyTheme();
    UiFileBrowser &SetFont(const Font &font);
    UiFileBrowser &UseThemeFont();
    UiFileBrowser &ShowThemeButton(bool show = true);
    Font GetFont() const { return font_; }

    Event<const UiFileBrowserSelection &> WhenAccept;
    Event<> WhenCancel;
    Event<const String &> WhenFolderChanged;
    Event<const UiFileBrowserSelection &> WhenSelection;
    Event<const String &> WhenError;
    Event<UiThemeMode> WhenThemeRequested;

private:
    friend struct UiFileBrowserTest;
    struct RowRef : Moveable<RowRef>
    {
        int entry = -1;
        int frame = -1;
    };

    void BuildUi();
    void BuildNavigation();
    void RebuildNavigation();
    void BuildOptions();
    void BuildWorkspace();
    void BuildFooter();
    void WireEvents();

    void RebuildShell();
    void RebuildWorkspace();
    void RebuildBrowserSurface();

    void ScanCurrentFolder();
    void PollScan();
    void EditAddress(bool edit);
    void SubmitName();
    bool EffectiveGroupSequences() const;
    void SetFolderInternal(const String &folder, bool add_history);
    void GoBack();
    void GoForward();
    void GoUp();
    void GoHome();
    void RefreshBreadcrumbs();
    void RefreshPlaces();

    void RefreshProjection(bool keep_selection = true);
    void RefreshTable();
    void RefreshGallery();
    void RefreshFilterList();
    void RefreshInspector();
    void RebuildInspectorDetails();
    void RequestPreview(bool force = false);
    void PollPreview();
    void RefreshFooter();
    void RefreshStatus();
    void RefreshModeVisuals();

    bool PassesProjection(const UiFileBrowserEntry &entry) const;
    bool PassesBaseFileType(const UiFileBrowserEntry &entry) const;
    String EntryDisplayName(const UiFileBrowserEntry &entry) const;
    String SequenceFrameRange(const UiFileBrowserEntry &entry) const;
    String FormatFrameNumber(int64 frame, int padding) const;
    Image EntryIcon(const UiFileBrowserEntry &entry) const;

    void HandleTableSelection();
    void HandleTableAction();
    void HandleGallerySelection();
    void HandleGalleryAction();
    void HandlePlaceAction();
    void SelectEntry(int entry, int frame = -1, bool sync_views = true);
    void SyncSelectionViews();

    void ToggleSequenceExpanded(int entry);
    bool IsSequenceExpanded(int entry) const;
    bool ShouldShowFrame(const UiFileBrowserEntry &entry, int frame_index) const;

    void SetDetailsView(bool details);
    void SetOptionsOpen(bool open);
    enum class OptionsPage
    {
        Filter,
        Sequence,
        View,
        Sort,
        NewItem
    };
    void OpenOptions(OptionsPage page, Ctrl &anchor);
    void CloseOptions();
    void PositionOptions();
    void CreateItem();
    void ConfigureCreation();
    void RefreshCreationColour();
    void ChooseCreationColour();
    void CloseCreationColour();
    void PositionCreationColour();
    void SetGroupSequences(bool grouped);
    int SizePx(int logical_pixels) const;
    void ApplyViewSizeLayout();

    void LoadSelectedFilter();
    void AddFilterFromEditor();
    void SaveFilterFromEditor();
    void RemoveSelectedFilter();
    void ReorderFilter(int from, int before);

    void AcceptSelection(bool force_sequence = false, bool force_frame = false, bool force_folder = false);
    void RequestThemeChange();
    static Font ResolveInheritedFont();
    void PollTheme();
    void StyleToolbar();

private:
    Mode mode_ = Mode::Advanced;
    ViewSize view_size_ = ViewSize::Medium;
    Font base_font_, font_;
    bool font_override_ = false;
    bool show_theme_button_ = false;
    uint64 theme_revision_ = 0;
    bool options_open_ = false;
    OptionsPage options_page_ = OptionsPage::Filter;
    Ptr<Ctrl> options_anchor_;
    bool details_view_ = true;
    bool advanced_group_sequences_ = true;
    bool syncing_ = false;

    String folder_;
    Vector<String> history_;
    int history_index_ = -1;

    std::unique_ptr<UiFileBrowserModel> model_ = std::make_unique<UiFileBrowserModel>();
    UiFileBrowserScanner scanner_;
    UiFileBrowserPreviewWorker preview_worker_;
    UiFileBrowserPreviewProvider preview_provider_;
    uint64 preview_generation_ = 0;
    String preview_path_;
    uint64 scan_generation_ = 0;
    bool scanning_ = false;
    String scan_error_, restore_path_;
    String footer_name_;
    int64 restore_frame_ = Null;
    struct Root : Moveable<Root>
    {
        String label, path;
    };
    Vector<Root> roots_;
    Vector<UiFileBrowserFilterRule> filters_;
    int filter_selected_ = -1;
    Vector<RowRef> rows_;
    Vector<int> gallery_entries_;
    Index<int> expanded_entries_;

    int selected_entry_ = -1;
    int selected_frame_ = -1;

    UiBoxLayout shell_{UiDirection::V};

    UiToolButton mode_button_;
    UiBoxLayout view_size_layout_{UiDirection::H};
    UiToolButton view_size_buttons_[3];
    UiToolButton theme_button_;

    UiPanel navigation_panel_;
    UiBoxLayout navigation_layout_{UiDirection::H};
    UiToolButton back_button_, forward_button_, up_button_, home_button_;
    UiBreadcrumbs breadcrumbs_;
    UiStack address_stack_;
    UiLineEdit address_edit_;
    UiToolButton address_button_;
    UiLineEdit search_;
    UiToolButton details_button_, thumbnails_button_;
    UiToolButton options_button_;
    UiToolButton refresh_button_;
    UiPanel header_divider_;

    UiPanel commands_panel_;
    UiBoxLayout commands_layout_{UiDirection::H};
    UiToolButton filter_button_, sequence_button_, view_button_, sort_button_, new_button_;
    UiFileBrowserOptionsPopup options_popup_;
    UiBoxLayout options_layout_{UiDirection::V};
    UiFileBrowserOptionsPopup colour_popup_;
    UiColorPickerMicro colour_micro_;

    UiPanel filter_group_;
    UiBoxLayout filter_layout_{UiDirection::V};
    UiBoxLayout filter_header_{UiDirection::H};
    UiLabel filter_title_, filter_count_;
    UiToggle filter_enable_;
    UiLabel filter_enable_label_;
    UiBoxLayout filter_enable_row_{UiDirection::H};
    UiButton filter_add_button_, filter_clear_button_;
    UiList filter_list_;
    UiBoxLayout filter_editor_{UiDirection::H};
    UiDropdown filter_action_, filter_field_;
    UiLineEdit filter_value_;
    UiButton filter_save_button_, filter_delete_button_;
    UiLabel filter_hint_;

    UiPanel sequence_group_;
    UiBoxLayout sequence_layout_{UiDirection::V};
    UiLabel sequence_title_;
    UiDropdown sequence_mode_, frame_view_, pattern_mode_;
    UiCheckBox missing_badge_;
    UiLabel health_label_;
    UiProgressBar health_;

    UiPanel view_group_;
    UiBoxLayout view_layout_{UiDirection::V};
    UiLabel view_title_;
    UiToggle show_size_, show_frames_, show_inspector_;
    UiLabel size_label_, frames_label_, inspector_label_;
    UiBoxLayout size_row_{UiDirection::H}, frames_row_{UiDirection::H}, inspector_row_{UiDirection::H};
    UiBoxLayout view_display_row_{UiDirection::H};

    UiPanel sort_group_;
    UiBoxLayout sort_layout_{UiDirection::V};
    UiLabel sort_title_;
    UiDropdown sort_key_, folder_placement_;

    UiPanel new_item_group_;
    UiBoxLayout new_item_layout_{UiDirection::V};
    UiLabel new_item_title_, new_item_error_;
    UiLineEdit new_item_name_;
    UiButton create_item_button_;
    UiDropdown new_item_kind_, new_image_fill_;
    UiLineEdit new_image_width_, new_image_height_, new_image_colour_;
    UiLabel new_image_size_label_, new_image_fill_label_, new_image_multiply_;
    UiButton new_image_swatch_;
    UiBoxLayout new_image_size_row_{UiDirection::H}, new_image_fill_row_{UiDirection::H};

    UiBoxLayout workspace_{UiDirection::H};

    UiPanel places_panel_;
    UiBoxLayout places_layout_{UiDirection::V};
    UiLabel places_title_;
    UiFileBrowserPlaces places_;
    UiBoxLayout places_header_{UiDirection::H};
    UiToolButton pin_button_, unpin_button_;

    UiSplitter browser_inspector_;
    double inspector_split_ = 70;
    UiPanel browser_panel_;
    UiBoxLayout browser_layout_{UiDirection::V};
    UiLabel item_count_;
    UiStack browser_stack_;
    UiTableModel table_model_;
    UiFileBrowserTable table_;
    UiListModel gallery_model_;
    UiGallery gallery_;
    UiProgressBar scan_progress_;

    UiPanel inspector_panel_;
    UiBoxLayout inspector_layout_{UiDirection::V};
    UiPanel preview_panel_;
    UiBoxLayout preview_layout_{UiDirection::V};
    UiBoxLayout preview_message_{UiDirection::V};
    UiStack preview_content_;
    UiLabel preview_image_, preview_text_;
    UiScrollPanel preview_text_scroll_;
    UiLabel preview_title_, preview_subtitle_, inspector_name_;
    Size preview_image_size_{0, 0};
    UiGridLayout inspector_meta_;
    UiLabel meta_labels_[6];
    UiLabel meta_values_[6];
    UiLabel coverage_label_;
    UiProgressBar coverage_;

    UiPanel footer_panel_;
    UiBoxLayout footer_layout_{UiDirection::H};
    UiLabel name_label_, type_label_;
    UiLineEdit name_edit_;
    UiDropdown file_type_;
    UiButton cancel_button_;
    UiSplitButton open_button_;
};

class UiFileBrowserDialog : public TopWindow
{
public:
    typedef UiFileBrowserDialog CLASSNAME;

    UiFileBrowserDialog();
    bool Execute(const String &folder, bool discover_sequences, UiFileBrowserSelection &selection);
    UiFileBrowserDialog &SetPreviewProvider(UiFileBrowserPreviewProvider provider)
    {
        browser_.SetPreviewProvider(std::move(provider));
        return *this;
    }

private:
    friend struct UiFileBrowserTest;
    UiFileBrowser browser_;
    UiFileBrowserSelection selection_;
};

} // namespace Upp

#endif
