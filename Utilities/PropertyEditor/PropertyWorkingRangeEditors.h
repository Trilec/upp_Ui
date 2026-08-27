#ifndef _Utilities_PropertyEditor_PropertyWorkingRangeEditors_h_
#define _Utilities_PropertyEditor_PropertyWorkingRangeEditors_h_

#include "PropertyValueEditors.h"

namespace Upp {

// Stable custom-editor id for a numeric integer property whose typed value uses
// item.minimum/item.maximum while the slider uses a smaller working range.
const char *PropertyEditorWorkingRangeIntId();

// Encodes/decodes the slider working range carried by editor_variant. Legal
// numeric bounds remain in PropertyEditorItem::minimum/maximum.
String PropertyEditorWorkingRangeVariant(int minimum, int maximum);
bool PropertyEditorParseWorkingRangeVariant(const String& variant,
                                            int& minimum, int& maximum);

void RegisterPropertyEditorWorkingRangeEditors(PropertyEditorFactory& factory);

}

#endif
