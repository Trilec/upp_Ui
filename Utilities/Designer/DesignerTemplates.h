#pragma once

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    DesignerTemplates
    =================

    Purpose
    - Public header for the DesignerTemplates component.

    Intent
    - Define the runtime API, style contract, and integration points used by the rest of the Ui package.

    Thread context
    - GUI thread only.

    Usage
    - Include this header where the component is used or extended. Keep implementation details in the matching .cpp when present.

    Changelog
    - 2026-06: normalized the top-level header documentation.
*/

#include "DesignerBuiltins.h"

// Ui Designer starter templates.
// Copyright (c) 2026 C Edwards (dodobar). MIT licensed, matching the Ui package.
//
// Templates are small model seeds used to explore common layout patterns quickly.
// They should remain model-first examples rather than hand-built preview widgets.

namespace Upp {

// Replace the supplied model with the named starter layout.
// Returns false for unknown ids; caller is responsible for refreshing views.
bool ApplyDesignerTemplate(DesignerModel& model, const DesignerRegistry& registry, const String& id);

}
