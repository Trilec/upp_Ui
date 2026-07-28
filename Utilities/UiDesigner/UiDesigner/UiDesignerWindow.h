#ifndef _Utilities_UiDesigner_UiDesigner_UiDesignerWindow_h_
#define _Utilities_UiDesigner_UiDesigner_UiDesignerWindow_h_

#include <CtrlLib/CtrlLib.h>
#include <Ui/Ui.h>
#include <Ui/UiColorPicker.h>
#include <Utilities/PropertyEditor/PropertyEditor.h>
#include <Utilities/UiDesigner/Preview/UiDesignerPreview.h>
#include <Utilities/UiDesigner/Services/UiDesignerServices.h>
#include <Utilities/UiDesigner/Theme/UiDesignerThemeGallery.h>
#include "UiDesignerInteractionOverlay.h"
#include "UiDesignerWidgets.h"
#include "UiDesignerExportDialog.h"

namespace Upp {

class UiDesignerWindow : public TopWindow {
public:
    typedef UiDesignerWindow CLASSNAME;

    UiDesignerWindow();

    UiDesignerSession& Session() { return session_; }
    const UiDesignerSession& Session() const { return session_; }
    void WriteLaunchDiagnostic();

    virtual void Layout() override;
    virtual void Close() override;

    void ApplyPreviewVirtualSize(const Size& virtual_size);
    void QueuePreviewVirtualSize(const Size& virtual_size,
                                 Function<void()> applied = Function<void()>());
    void FlushPreviewVirtualSize();
    void CancelQueuedPreviewVirtualSize();

private:
    friend class UiDesignerInteractionOverlay;
    void BuildHeader();
    void BuildDesigner();
    void BuildTheme();
    void ConnectServices();
    void ApplyThemeToShell();
    void UpdateDecorationsButton();

    void ShowDesigner();
    void ShowTheme();
    void ToggleDarkMode();
    void ActivateToolbox(const String& id);
    void SaveDocument(bool save_as = false);
    void LoadDocument();
    void ExportProject(UiDesignerExportProfile profile);

    void RefreshHierarchy();
    void RefreshInspector();
    void RefreshData();
    void RefreshBehavior();
    void RefreshThemeInspector();
    void RefreshCode();
    void RefreshDiagnostics();
    void RequestDiagnosticsRefresh();
    void RefreshStatus(const String& status);
    void PostSelectionDetailsRefresh();
    void TrackCatalogDrag(const String& type_id, Point screen);
    void FinishCatalogDrag(const String& type_id, Point screen);
    void CancelCatalogDrag();

    UiDesignerSession session_;

    UiPanel header_surface_;
    UiBoxLayout header_layout_;
    UiTitleCard brand_;
    UiSplitButton save_;
    UiSplitButton load_;
    UiSplitButton export_;
    UiLabel version_;
    UiButton designer_mode_;
    UiButton theme_mode_;
    UiDropdown theme_select_;
    UiToolButton dark_;
    UiToolButton help_;
    UiToolButton exit_;

    UiStack workspaces_;
    UiPanel designer_page_;
    UiPanel theme_page_;

    UiDesignerSideColumn designer_left_;
    UiPanel designer_center_;
    UiDesignerSideColumn designer_right_;

    UiDesignerCatalogList layouts_list_;
    UiDesignerCatalogList containers_list_;
    UiDesignerCatalogList controls_list_;
    UiDesignerCatalogList composites_list_;
    UiDesignerCatalogList presets_list_;
    UiDesignerCatalogList upp_controls_list_;

    UiDesignerPillBar aspect_pill_;
    UiToolButton portrait_;
    UiToolButton landscape_;
    UiSplitButton aspect_preset_;
    UiToolButton square_;
    UiToolButton decorations_;
    UiScrollPanel preview_scroll_;
    // UiScrollPanel needs one content child to define its scroll extent. This
    // host is intentionally unpainted; the Window canvas is the only visible
    // document surface in the center scroll viewport.
    ParentCtrl preview_workspace_;
    UiDesignerPreviewCanvas preview_canvas_;
    UiDesignerInteractionOverlay interaction_overlay_;

    UiDesignerHierarchyView hierarchy_;
    PropertyEditor inspector_;
    UiMultiEdit data_shell_;
    UiPanel data_panel_;
    UiBoxLayout data_layout_;
    UiBoxLayout data_actions_;
    UiButton data_add_;
    UiButton data_remove_;
    UiButton data_rename_;
    UiButton data_up_;
    UiButton data_down_;
    UiButton data_enable_;
    UiButton data_active_;
    UiList data_list_;
    UiListModel data_model_;
    PropertyEditor behaviors_;
    PropertyEditor overrides_;
    UiDesignerCodeView code_;
    UiPanel diagnostics_panel_;
    UiBoxLayout diagnostics_layout_;
    UiBoxLayout diagnostics_toolbar_;
    UiToolButton diagnostics_reset_;
    UiToolButton diagnostics_pause_;
    UiToolButton diagnostics_copy_;
    UiToolButton diagnostics_log_;
    UiToolButton diagnostics_timing_;
    UiMultiEdit diagnostics_shell_;

    UiPanel theme_gallery_column_;
    UiDesignerPillBar theme_gallery_pill_;
    UiToolButton theme_all_;
    UiToolButton theme_inputs_;
    UiToolButton theme_containers_;
    UiScrollPanel gallery_scroll_;
    UiPanel gallery_surface_;
    UiDesignerThemeGallery theme_gallery_;
    UiDesignerSideColumn theme_right_;
    PropertyEditor theme_inspector_;
    UiDesignerCodeView theme_code_;

    UiPanel footer_surface_;
    UiLabel footer_;

    String current_file_;
    UiDesignerExportProfile last_export_profile_ =
        UiDesignerExportProfile::CompleteCppPackage;
    bool selection_details_refresh_posted_ = false;
    bool diagnostics_refresh_posted_ = false;
    bool diagnostics_capture_paused_ = false;
    String active_catalog_drag_type_;
    bool catalog_drag_active_ = false;
    bool decorations_visible_ = true;
    bool preview_resize_posted_ = false;
    bool preview_resize_pending_ = false;
    Size pending_preview_virtual_size_;
    Function<void()> pending_preview_resize_callback_;
};

}

#endif
