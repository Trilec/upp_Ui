/*
    File-browser appearance vocabulary.

    Semantic file colours stay stable across application presets, with separate
    Light/Dark palettes. Surrounding chrome, typography and button geometry come
    from the host's Ui theme. This module owns the small reusable glyph set and
    its bounded colour cache; it never decodes media or touches the filesystem.
    Functions are GUI-thread only because they inspect the active Ui theme.
*/
#ifndef _UiFileBrowser_UiFileBrowserAppearance_h_
#define _UiFileBrowser_UiFileBrowserAppearance_h_

#include "UiFileBrowserModel.h"
#include <Ui/Ui.h>

namespace Upp
{
namespace UiFileBrowserAppearance
{

Color TypeInk(const UiFileBrowserEntry &entry);
Color NameInk(const UiFileBrowserEntry &entry);
Image FileIcon(const UiFileBrowserEntry &entry);
UiModelItem BrowserItem(const String &text, const Value &data, const Image &icon = Image());

// Optional font choice for standalone hosts. The control itself inherits StdFont.
Font ModernFont();

} // namespace UiFileBrowserAppearance
} // namespace Upp
#endif
