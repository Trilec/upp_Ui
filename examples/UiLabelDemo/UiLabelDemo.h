#ifndef _examples_UiLabelDemo_UiLabelDemo_h_
#define _examples_UiLabelDemo_UiLabelDemo_h_

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    UiLabelDemo
    ===========

    Purpose
    - Interactive, self-contained UiLabel reference and style builder.

    Features
    - Centers the real UiLabel in a responsive preview surface.
    - Uses the production UiPropertyInspector for authored properties and explicit
      theme overrides.
    - Covers Label content, icons, sizing, typography, state palettes, frames,
      focus, shadows, highlights and optional image skin data.
    - Generates matching standalone C++ that can be selected and copied.

    Design
    - UiPropertyInspector models are the single authored state. Preview and source
      generation read the same values, avoiding parallel demo-only state.
    - Theme rows remain inherited until their override action is activated.
    - Override presentation follows docs/05_UI_PROPERTY_EDITOR_GUIDE.md;
      stable runtime ids are normalized to that human-facing layout before the
      UiPropertyInspector observes structure changes.

*/

#include <Ui/Ui.h>
#include <Ui/UiPropertyInspector/UiPropertyInspector.h>
#include <Ui/UiPropertyInspector/PropertyValueEditors.h>

namespace Upp {

class UiLabelOverrideModel : public UiPropertyInspectorModel {
public:
    // Preserve stable ids/value semantics while enforcing the canonical Label
    // group paths and concise row labels used by demos and UiDesigner.
    void StructureChanged();
};

class UiLabelDemo : public TopWindow {
public:
    typedef UiLabelDemo CLASSNAME;

    UiLabelDemo();
    void Layout() override;
    void Paint(Draw& draw) override;
    void ExportGenerated(const String& directory);

private:
    Color window_face_ = SColorFace();
    void BuildHeader();
    void BuildPreview();
    void BuildRightRail();
    void BuildInspectorModel();
    void BuildOverrideModel();
    void ConfigureEditors();
    void ConnectEvents();
    void ApplyProjection();
    void ApplyTheme();
    void UpdateOverrideSummaries();
    void UpdateGeneratedCode();
    void SelectPage(int page);
    void ToggleTheme();
    void ResetProperty(UiPropertyInspectorModel& model, const String& id);
    void SetOverrideActive(const String& id, bool active);
    bool PickImage(Value& value, Ctrl *owner);
    Image LoadImageValue(const Value& value) const;

    Value InspectorValue(const String& id) const;
    Value OverrideValue(const String& id) const;
    bool OverrideActive(const String& id) const;

    UiPropertyInspectorFactory pe_factory;
    UiPropertyInspectorModel pe_model_inspector;
    UiLabelOverrideModel pe_model_override;
    UiTitleCard tc_header;
    UiBoxLayout box_header_actions { UiDirection::H };
    UiToolButton btn_theme, btn_help, btn_exit;

    UiPanel pnl_preview;
    UiLabel lbl_preview, lbl_preview_caption;

    UiPanel pnl_right_rail;
    UiBoxLayout box_right_tools { UiDirection::H };
    UiToolButton btn_inspector_mode, btn_overrides_mode, btn_code_mode;
    UiStack stk_right_pages;
    UiPanel pnl_inspector_page, pnl_overrides_page, pnl_code_page;
    UiPropertyInspector pe_inspector, pe_overrides;
    UiMultiEdit edit_generated_code;
    UiToolButton btn_copy_code;

    String str_generated_code;
};

} // namespace Upp

#endif