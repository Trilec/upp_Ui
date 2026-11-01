#ifndef _Ui_UiPropertyInspector_PropertyWorkingRangeEditors_h_
#define _Ui_UiPropertyInspector_PropertyWorkingRangeEditors_h_

#include "PropertyValueEditors.h"

namespace Upp {

// Stable custom-editor ids for numeric properties whose typed value uses
// item.minimum/item.maximum while the slider uses a smaller working range.
const char *UiPropertyInspectorWorkingRangeIntId();
const char *UiPropertyInspectorWorkingRangeDoubleId();

// Encodes/decodes the slider working range carried by editor_variant. Legal
// numeric bounds remain in UiPropertyInspectorItem::minimum/maximum.
String UiPropertyInspectorWorkingRangeVariant(int minimum, int maximum);
bool UiPropertyInspectorParseWorkingRangeVariant(const String& variant,
                                            int& minimum, int& maximum);
String UiPropertyInspectorWorkingRangeVariant(double minimum, double maximum);
bool UiPropertyInspectorParseWorkingRangeVariant(const String& variant,
                                            double& minimum, double& maximum);

void RegisterUiPropertyInspectorWorkingRangeEditors(UiPropertyInspectorFactory& factory);

}

#endif
