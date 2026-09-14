#ifndef _UiRangeSegmentsDemo_UiRangeSegmentsDemo_h_
#define _UiRangeSegmentsDemo_UiRangeSegmentsDemo_h_

#include <Ui/Ui.h>
#include <Utilities/PropertyEditor/PropertyEditor.h>
#include <Utilities/PropertyEditor/PropertyValueEditors.h>

namespace Upp {

class UiRangeSegmentsDemo : public TopWindow {
public:
    typedef UiRangeSegmentsDemo CLASSNAME;

    UiRangeSegmentsDemo();
    void Layout() override;

private:
    void BuildHeader();
    void BuildPreview();
    void BuildRightRail();
    void BuildInspector();
    void BuildOverrides();
    void BuildDataModel();
    void ConfigureEditors();
    void ConnectEvents();
    void ApplyTheme();

    void SelectPage(int page);
    void ApplyProjection();
    void ApplyDataProjection();
    bool ApplyOverrides(UiRangeSegments::Style& style) const;
    void SyncInheritedOverrides(const UiRangeSegments::Style& style);
    void SyncDataSpans();
    void ResetProperty(PropertyEditorModel& model, const String& id);
    void SetOverrideActive(const String& id, bool active);
    void ToggleTheme();
    void UpdateGeneratedCode();

    Value InspectorValue(const String& id, const Value& fallback = Value()) const;
    Value OverrideValue(const String& id, const Value& fallback = Value()) const;
    Value DataValue(const String& id, const Value& fallback = Value()) const;
    bool OverrideActive(const String& id) const;

private:
    UiTitleCard header_;
    UiBoxLayout header_actions_;
    UiToolButton theme_;
    UiToolButton help_;
    UiToolButton exit_;

    UiPanel preview_;
    UiRangeSegments ranges_;
    UiLabel caption_;

    UiPanel right_;
    UiBoxLayout tools_;
    UiToolButton inspector_mode_;
    UiToolButton overrides_mode_;
    UiToolButton data_mode_;
    UiToolButton code_mode_;
    UiStack pages_;
    UiPanel inspector_page_;
    UiPanel overrides_page_;
    UiPanel data_page_;
    UiPanel code_page_;
    PropertyEditor inspector_;
    PropertyEditor overrides_;
    PropertyEditor data_;
    UiMultiEdit code_;
    UiToolButton copy_;

    PropertyEditorFactory factory_;
    PropertyEditorModel inspector_model_;
    PropertyEditorModel override_model_;
    PropertyEditorModel data_model_;

    String generated_;
    int data_segment_count_ = 0;
    bool syncing_projection_ = false;
};

} // namespace Upp

#endif
