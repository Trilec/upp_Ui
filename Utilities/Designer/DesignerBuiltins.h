#pragma once

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    DesignerBuiltins
    ================

    Purpose
    - Public header for the DesignerBuiltins component.

    Intent
    - Define the runtime API, style contract, and integration points used by the rest of the Ui package.

    Thread context
    - GUI thread only.

    Usage
    - Include this header where the component is used or extended. Keep implementation details in the matching .cpp when present.

    Changelog
    - 2026-06: normalized the top-level header documentation.
*/

#include "DesignerRegistry.h"

// Ui Designer built-in type registration.
// Copyright (c) 2026 C Edwards (dodobar). MIT licensed, matching the Ui package.
//
// This module declares the stock toolbox/catalog entries for the designer:
// layouts, containers, splitter panes, and the first set of real Ui controls.

namespace Upp {

// Register the built-in DesignerType records into the supplied registry.
// Call once during app startup before building toolbox rows or applying templates.
void RegisterDesignerBuiltins(DesignerRegistry& registry);

}
