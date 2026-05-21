#pragma once

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
