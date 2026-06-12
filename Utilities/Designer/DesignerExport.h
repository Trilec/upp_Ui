#pragma once

#include "DesignerModel.h"
#include "DesignerRegistry.h"

namespace Upp {

struct DesignerCodeGenOptions {
	String class_name = "GeneratedDesignerWindow";
	bool emit_designer_appearance = false;
	bool emit_export_header = false;
	String source_design_filename;
	String package_name;
	String umk_path;
	String exported_package_path;
	String build_method;
	String output_exe_path;
};

struct DesignerProjectExportOptions {
	String project_name;
	String output_directory;
	String class_name;
	String source_design_filename;
	bool include_designer_appearance = false;
	bool include_design_json = true;
	bool include_readme = true;
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
