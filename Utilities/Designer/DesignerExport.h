#pragma once

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    DesignerExport
    ==============

    Purpose
    - Public header for the DesignerExport component.

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

struct DesignerProjectExportOptions {
	String project_name;
	String output_directory;
	String class_name;
	String source_design_filename;
	bool include_designer_appearance = false;
	bool include_design_json = true;
	bool include_readme = true;
	bool overwrite_existing = false;
	bool simulate_commit_failure = false; // test hook only; keeps rollback path verifiable.
	bool simulate_restore_failure = false; // test hook only; keeps backup-preservation verifiable.
	String umk_path;
	String build_method;
	String output_exe_path;
};

struct DesignerProjectExportResult {
	String project_name;
	String class_name;
	String package_dir;
	String upp_path;
	String main_cpp_path;
	String design_json_path;
	String readme_path;
	String error;
};

String SanitizeDesignerPackageName(String text, const String& fallback = "ExportedDesignerProject");
String SanitizeDesignerClassName(String text, const String& fallback = "GeneratedDesignerWindow");
bool ExportDesignerProject(const DesignerModel& model, const DesignerRegistry& registry,
                           const DesignerProjectExportOptions& options,
                           const String& design_json,
                           DesignerProjectExportResult& result);

}
