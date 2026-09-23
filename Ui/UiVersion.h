#ifndef _Ui_UiVersion_h_
#define _Ui_UiVersion_h_

/*
    Ui release identity
    ===================
    Author: C Edwards (dodobar)
    License: Apache License 2.0 (see LICENSE).

    One version for the Ui library, not a separate counter for each control.
    An RC identifier does not certify platform/visual acceptance. Independently
    versioned sibling packages and serialized-data schemas retain their versions.
    No GUI dependency; safe to include in build and diagnostic tools.
*/
#define UPP_UI_VERSION "1.0.0-rc.1"

namespace Upp {
inline const char* UiGetVersion() { return UPP_UI_VERSION; }
}

#endif
