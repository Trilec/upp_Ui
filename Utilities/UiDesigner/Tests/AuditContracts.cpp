#include <Utilities/PropertyEditor/PropertyEditor.h>
#include <Utilities/UiDesigner/UiDesigner/UiDesignerWidgets.h>

#include <type_traits>

namespace Upp {

static_assert(std::is_base_of<UiDesignerSideColumn,
                              UiDesignerInspectorColumn>::value,
              "Inspector columns must preserve the shared side-column interaction");
static_assert(std::is_same<decltype(((PropertyEditorItem *)nullptr)
                                       ->SetInlineEditor()),
                           PropertyEditorItem&>::value,
              "Inline editor opt-in must remain a fluent property contract");

INITBLOCK {
    PropertyEditorItem item;
    item.SetInlineEditor();
    ASSERT(item.inline_editor);

    ASSERT(UiDesignerStyleMetrics::InspectorNormalWidth() == DPI(324));
    ASSERT(UiDesignerStyleMetrics::InspectorMediumWidth() == DPI(364));
    ASSERT(UiDesignerStyleMetrics::InspectorWideWidth() == DPI(404));
    ASSERT(UiDesignerStyleMetrics::PanelNormalWidth() == DPI(250));
}

}
