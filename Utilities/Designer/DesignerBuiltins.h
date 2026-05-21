#pragma once

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
