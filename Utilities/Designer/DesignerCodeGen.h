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

class DesignerCodeGenContext {
public:
	DesignerCodeGenContext(String& out, const DesignerRegistry& registry, const DesignerModel& model,
	                       const VectorMap<DesignerNodeId, String>& names, DesignerAppearanceMode appearance_mode)
	    : out_(out), registry_(registry), model_(model), names_(names), appearance_mode_(appearance_mode) {}

	String& Out() { return out_; }
	const String& Out() const { return out_; }
	String Var(const DesignerNode& node) const;
	Value Property(const DesignerNode& node, const String& property, const Value& fallback) const;
	bool HasProperty(const DesignerNode& node, const String& property) const;
	String CppString(const Value& value) const;
	String ColorExpr(const Value& value) const;
	String IconExpr(const Value& value) const;
	String AlignHExpr(const Value& value, const String& fallback = "Left") const;
	String AlignVExpr(const Value& value, const String& fallback = "Center") const;
	String AlignSideExpr(const Value& value, const String& fallback = "Left") const;
	DesignerAppearanceMode AppearanceMode() const { return appearance_mode_; }
	const DesignerRegistry& Registry() const { return registry_; }
	const DesignerModel& Model() const { return model_; }

private:
	String& out_;
	const DesignerRegistry& registry_;
	const DesignerModel& model_;
	const VectorMap<DesignerNodeId, String>& names_;
	DesignerAppearanceMode appearance_mode_;
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
