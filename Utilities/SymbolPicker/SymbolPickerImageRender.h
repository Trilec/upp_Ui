#ifndef _Utilities_SymbolPicker_SymbolPickerImageRender_h_
#define _Utilities_SymbolPicker_SymbolPickerImageRender_h_

#include "SymbolPickerCatalog.h"

namespace Upp {

Image RenderSymbolPickerIconImage(const SymbolPickerIconEntry& entry, int pixel_size, Color tint, String* error = nullptr);

}

#endif
