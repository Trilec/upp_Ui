#ifndef _Utilities_UiDesigner_Core_UiDesignerSerialization_h_
#define _Utilities_UiDesigner_Core_UiDesignerSerialization_h_

#include "UiDesignerDocument.h"

namespace Upp {

Value UiDesignerDocumentToValue(const UiDesignerDocument& document);
bool UiDesignerDocumentFromValue(const Value& value, UiDesignerDocument& document,
                                 String& error);
String UiDesignerSerialize(const UiDesignerDocument& document, bool pretty = true);
bool UiDesignerDeserialize(const String& json, UiDesignerDocument& document,
                           String& error);
bool UiDesignerSaveFile(const String& path, const UiDesignerDocument& document,
                        String& error);
bool UiDesignerLoadFile(const String& path, UiDesignerDocument& document,
                        String& error);

}

#endif
