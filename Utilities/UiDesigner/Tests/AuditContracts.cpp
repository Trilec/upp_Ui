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
static_assert(UiDesignerStyleMetrics::PanelNormalPixels == 250,
              "The left catalog profile must remain unchanged");
static_assert(UiDesignerStyleMetrics::InspectorNormalPixels == 324 &&
              UiDesignerStyleMetrics::InspectorMediumPixels == 364 &&
              UiDesignerStyleMetrics::InspectorWidePixels == 404,
              "Right Inspector widths must remain three forty-pixel steps");

}
