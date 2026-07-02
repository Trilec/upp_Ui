#pragma once

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    DesignerCodeGen
    ===============

    Purpose
    - Public header for the DesignerCodeGen component.

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

// Ui Designer generated-code exporter.
// Copyright (c) 2026 C Edwards (dodobar). MIT licensed, matching the Ui package.
//
// Codegen reads only DesignerModel plus registry metadata. It should emit
// theme-first U++ code that recreates the layout without relying on designer
// preview-only state.

namespace Upp {

enum class DesignerAppearanceMode {
	ExactDesign,
	ThemeFirst
};

struct DesignerCodeGenOptions {
	String class_name = "GeneratedDesignerWindow";
	DesignerAppearanceMode appearance_mode = DesignerAppearanceMode::ThemeFirst;
	bool emit_export_header = false;
	String designer_version;
	String source_design_filename;
	String package_name;
	String umk_path;
	String exported_package_path;
	String build_method;
	String output_exe_path;
};

// Produce a standalone GUI window class and entry point from the current model.
// ExactDesign emits explicit appearance helpers and applies them back into the
// generated controls. ThemeFirst keeps theme helpers out of the output and
// leaves appearance resolution to runtime theme defaults.
String GenerateDesignerCode(const DesignerModel& model, const DesignerRegistry& registry,
                            const DesignerCodeGenOptions& options);

String GenerateDesignerCode(const DesignerModel& model, const DesignerRegistry& registry,
                            const String& class_name = "GeneratedDesignerWindow",
                            DesignerAppearanceMode appearance_mode = DesignerAppearanceMode::ThemeFirst);

// Compatibility wrapper for older call sites. Prefer the explicit appearance
// mode overload above for new code.
String GenerateDesignerCode(const DesignerModel& model, const DesignerRegistry& registry,
                            const String& class_name, bool emit_designer_appearance);

}
