#pragma once

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    DesignerSerialization
    =====================

    Purpose
    - Public header for the DesignerSerialization component.

    Intent
    - Define the runtime API, style contract, and integration points used by the rest of the Ui package.

    Thread context
    - GUI thread only.

    Usage
    - Include this header where the component is used or extended. Keep implementation details in the matching .cpp when present.

    Changelog
    - 2026-06: normalized the top-level header documentation.
*/

#include "DesignerModel.h"
#include "DesignerRegistry.h"

namespace Upp {

String StoreDesignerModelJson(const DesignerModel& model);
bool LoadDesignerModelJson(DesignerModel& model, const DesignerRegistry& registry,
                           const String& json, String& error, Vector<String>* notes = nullptr);

}
