#pragma once

#include "DesignerModel.h"
#include "DesignerRegistry.h"
#include "DesignerExport.h"

// Ui Designer generated-code exporter.
// Copyright (c) 2026 C Edwards (dodobar). MIT licensed, matching the Ui package.
//
// Codegen reads only DesignerModel plus registry metadata. It should emit
// theme-first U++ code that recreates the layout without relying on designer
// preview-only state.

namespace Upp {

// Produce a standalone GUI window class and entry point from the current model.
// Appearance metadata is omitted by default and only emitted when explicitly
// requested, so generated examples stay aligned with the active Ui theme.
String GenerateDesignerCode(const DesignerModel& model, const DesignerRegistry& registry,
                            const DesignerCodeGenOptions& options);

String GenerateDesignerCode(const DesignerModel& model, const DesignerRegistry& registry,
                            const String& class_name = "GeneratedDesignerWindow",
                            bool emit_designer_appearance = false);

}
