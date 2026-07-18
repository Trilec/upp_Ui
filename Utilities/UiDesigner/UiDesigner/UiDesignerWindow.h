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

private:
    friend class UiDesignerInteractionOverlay;
    void BuildHeader();
    void BuildDesigner();
    void BuildTheme();
    void ConnectServices();
    void ApplyThemeToShell();

    void ShowDesigner();
    void ShowTheme();
    void ToggleDarkMode();
    void ActivateToolbox(const String& id);
    void SaveDocument(bool save_as = false);
    void LoadDocument();
    void ExportProject(UiDesignerExportProfile profile);

    void RefreshHierarchy();
    void RefreshInspector();
    void RefreshBehavior();
    void RefreshThemeInspector();
    void RefreshCode();
    void RefreshStatus(const String& status);
    void PostSelectionDetailsRefresh();

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
    UiScrollPanel preview_scroll_;
    // UiScrollPanel needs one content child to define its scroll extent. This
    // host is intentionally unpainted; the Window canvas is the only visible
    // document surface in the center scroll viewport.
    ParentCtrl preview_workspace_;
    UiDesignerPreviewCanvas preview_canvas_;
    UiDesignerInteractionOverlay interaction_overlay_;

    UiDesignerHierarchyView hierarchy_;
    PropertyEditor inspector_;
    PropertyEditor behaviors_;
    PropertyEditor overrides_;
    UiDesignerCodeView code_;

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
};

}

#endif
