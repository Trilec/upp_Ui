#ifndef _UiFileDialogDemo_BrowserChecks_h_
#define _UiFileDialogDemo_BrowserChecks_h_

#include <Core/Core.h>

// Maintained checks carried over from the retired standalone browser demo.
Upp::String BuildBrowserFixture();
bool RunBrowserModelChecks(const Upp::String& report);
namespace Upp {
bool RunBrowserUiTests(const String& root, const String& report);
}

#endif
