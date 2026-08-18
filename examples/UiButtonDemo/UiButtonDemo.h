#ifndef _examples_UiButtonDemo_UiButtonDemo_h_
#define _examples_UiButtonDemo_UiButtonDemo_h_

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    UiButtonDemo
    ============

    Purpose
    - Interactive, self-contained UiButton reference and style builder.

    Features
    - Uses the same demo shell and production PropertyEditor architecture as
      UiLabelDemo.
    - Demonstrates the real UiButton content, icon, alignment, checkable,
      focus, sizing and underline APIs.
    - Exposes the live themed appearance as explicit opt-in local overrides,
      grouped by the canonical PropertyEditor override layout.
    - Generates copyable U++ from the same authored PropertyEditor state.

    Design
    - PropertyEditor models are the single authored configuration state.
    - The preview is the real UiButton and retains normal mouse/keyboard
      interaction.
    - Theme rows remain inherited until their override action is enabled.
    - No UiDesigner or demo-only property-row framework is used.
*/

#include <Ui/Ui.h>
#include <Utilities/PropertyEditor/PropertyEditor.h>
#include <Utilities/PropertyEditor/PropertyValueEditors.h>

namespace Upp {

class UiButtonDemo : public TopWindow {
public:
    typedef UiButtonDemo CLASSNAME;

    UiButtonDemo();
    void Layout() override;

private:
    void BuildHeader();
    void BuildPreview();
    void BuildRightRail();
    void BuildInspectorModel();
    void BuildOverrideModel();
    void ConfigureEditors();
    void ConnectEvents();

    void ApplyProjection();
    void ApplyTheme();
    void ConfigureModeButton(UiToolButton& button);
    void UpdateOverrideSummaries();
    void UpdateGeneratedCode();
    void UpdateStatus();
    void SelectPage(int page);
    void ToggleTheme();

    void ResetProperty(PropertyEditorModel& model, const String& id);
    void SetOverrideActive(const String& id, bool active);
    bool PickImage(Value& value, Ctrl *owner);
    Image LoadImageValue(const Value& value) const;

    Value InspectorValue(const String& id) const;
    Value OverrideValue(const String& id) const;
    bool OverrideActive(const String& id) const;

    UiTitleCard tc_header;
    UiBoxLayout box_header_actions { UiDirection::H };
    UiToolButton btn_theme, btn_help, btn_exit;

    UiPanel pnl_preview;
    UiButton btn_preview;
    UiLabel lbl_preview_caption, lbl_status;

    UiPanel pnl_right_rail;
    UiBoxLayout box_right_tools { UiDirection::H };
    UiToolButton btn_inspector_mode, btn_overrides_mode, btn_code_mode;
    UiStack stk_right_pages;
    UiPanel pnl_inspector_page, pnl_overrides_page, pnl_code_page;
    PropertyEditor pe_inspector, pe_overrides;
    UiMultiEdit edit_generated_code;
    UiToolButton btn_copy_code;

    PropertyEditorFactory pe_factory;
    PropertyEditorModel pe_model_inspector;
    PropertyEditorModel pe_model_override;
    String str_generated_code;
    int activation_count = 0;
};

} // namespace Upp

#endif
