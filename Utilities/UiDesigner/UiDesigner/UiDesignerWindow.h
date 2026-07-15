#ifndef _Utilities_UiDesigner_UiDesigner_UiDesignerWindow_h_
#define _Utilities_UiDesigner_UiDesigner_UiDesignerWindow_h_

#include <CtrlLib/CtrlLib.h>
#include <Ui/Ui.h>
#include <Ui/UiColorPicker.h>
#include <Utilities/PropertyEditor/PropertyEditor.h>
#include <Utilities/UiDesigner/Services/UiDesignerServices.h>
#include <Utilities/UiDesigner/Theme/UiDesignerThemeGallery.h>
#include "UiDesignerWidgets.h"

namespace Upp {

class UiDesignerWindow : public TopWindow {
public:
    typedef UiDesignerWindow CLASSNAME;

    UiDesignerWindow();

    UiDesignerSession& Session() { return session_; }

    virtual void Layout() override;
    virtual bool Key(dword key, int count) override;
    virtual void Close() override;

private:
    void BuildHeader();
    void BuildDesigner();
    void BuildTheme();
    void ConnectServices();

    void ShowDesigner();
    void ShowTheme();
    void RefreshHierarchy();
    void RefreshInspector();
    void RefreshThemeInspector();
    void RefreshCode();
    void RefreshStatus(const String& text = String());

    void ActivateToolbox(const String& id);
    void SaveDocument(bool save_as);
    void LoadDocument();
    void ExportProject();
    void ToggleDarkMode();
    void ApplyThemeToShell();

    UiDesignerSession session_;

    UiPanel header_surface_;
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

    UiStack workspaces_;
    ParentCtrl designer_page_;
    ParentCtrl theme_page_;

    UiDesignerSideColumn designer_left_;
    UiPanel designer_center_;
    UiDesignerPillBar aspect_pill_;
    UiToolButton portrait_;
    UiToolButton landscape_;
    UiSplitButton aspect_preset_;
    UiToolButton square_;
    UiScrollPanel preview_scroll_;
    UiPanel preview_surface_;
    UiDesignerPreviewCanvas preview_canvas_;
    UiDesignerSideColumn designer_right_;

    UiDesignerCatalogList presets_list_;
    UiDesignerCatalogList layouts_list_;
    UiDesignerCatalogList containers_list_;
    UiDesignerCatalogList controls_list_;
    UiDesignerCatalogList composites_list_;
    UiDesignerCatalogList upp_controls_list_;

    UiDesignerHierarchyView hierarchy_;
    PropertyEditor inspector_;
    PropertyEditor overrides_;
    PropertyEditorModel overrides_model_;
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
    PropertyEditorModel theme_model_;
    UiDesignerCodeView theme_code_;

    UiPanel footer_surface_;
    UiLabel footer_;

    String current_path_;
};

}

#endif
