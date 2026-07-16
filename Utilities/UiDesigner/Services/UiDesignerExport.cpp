#include "UiDesignerExport.h"

namespace Upp {

static String ExportProfileName(UiDesignerExportProfile profile)
{
    switch(profile) {
    case UiDesignerExportProfile::CompleteCppPackage: return "Complete C++ package";
    case UiDesignerExportProfile::ComponentOnly: return "C++ component";
    case UiDesignerExportProfile::ProjectJson: return "UiDesigner project JSON";
    case UiDesignerExportProfile::DocumentJson: return "Document JSON";
    case UiDesignerExportProfile::ThemeJson: return "Theme JSON";
    }
    return "Export";
}

Value UiDesignerExportService::BuildProjectValue(
    const UiDesignerDocument& document,
    const UiDesignerThemeDocument& theme,
    const UiDesignerCodeGenerationOptions& generation) const
{
    ValueMap options;
    options.Set("package_name", generation.package_name);
    options.Set("class_name", generation.class_name);
    options.Set("namespace", generation.namespace_name);
    options.Set("appearance_mode", generation.appearance_mode);
    options.Set("include_source_design", generation.include_source_design);
    options.Set("include_theme", generation.include_theme);

    ValueMap project;
    project.Set("format", "upp-ui-designer-project");
    project.Set("schema", 1);
    project.Set("document", UiDesignerDocumentToValue(document));
    project.Set("theme", theme.Get().ToValue());
    project.Set("generation", options);
    return project;
}

String UiDesignerExportService::ResolveJsonPath(
    const UiDesignerExportRequest& request,
    const String& default_name) const
{
    if(request.destination.IsEmpty())
        return String();
    const String extension = GetFileExt(request.destination);
    return extension.IsEmpty()
        ? AppendFileName(request.destination, default_name)
        : request.destination;
}

bool UiDesignerExportService::WriteSingleFileAtomic(
    const String& path, const String& content,
    UiDesignerOverwritePolicy overwrite, String& error) const
{
    if(path.IsEmpty()) {
        error = "Export path is empty";
        return false;
    }
    const bool existed = FileExists(path);
    if(existed && overwrite == UiDesignerOverwritePolicy::RefuseExisting) {
        error = "Export target already exists: " + path;
        return false;
    }
    const String folder = GetFileFolder(path);
    if(!folder.IsEmpty() && !DirectoryExists(folder) &&
       !RealizeDirectory(folder)) {
        error = "Unable to create export folder: " + folder;
        return false;
    }

    const String suffix = AsString(Uuid::Create());
    const String temporary = path + ".uidesigner-tmp-" + suffix;
    const String backup = path + ".uidesigner-backup-" + suffix;
    if(!SaveFile(temporary, content)) {
        error = "Unable to write temporary export: " + temporary;
        return false;
    }
    if(existed && !FileCopy(path, backup)) {
        FileDelete(temporary);
        error = "Unable to back up export target: " + path;
        return false;
    }

    bool published = false;
    if((!existed || FileDelete(path)) && FileMove(temporary, path))
        published = true;

    if(!published) {
        FileDelete(temporary);
        if(FileExists(path))
            FileDelete(path);
        bool restored = true;
        if(existed)
            restored = FileExists(backup) && FileCopy(backup, path);
        FileDelete(backup);
        error = restored
            ? "Unable to publish export; previous file restored: " + path
            : "Unable to publish export and restore previous file: " + path;
        return false;
    }

    FileDelete(backup);
    error.Clear();
    return true;
}

UiDesignerGeneratedProject UiDesignerExportService::BuildCppProject(
    const UiDesignerDocument& document,
    const UiDesignerThemeDocument& theme,
    const UiDesignerExportRequest& request,
    String& error) const
{
    UiDesignerCodeGenerator generator(catalog_);
    UiDesignerGeneratedProject project =
        generator.Generate(document, request.generation);
    if(!project.IsValid()) {
        error = project.diagnostics.IsEmpty()
            ? "Code generation failed"
            : Join(project.diagnostics, "\n");
        return project;
    }

    if(request.profile == UiDesignerExportProfile::ComponentOnly) {
        for(int i = project.files.GetCount() - 1; i >= 0; i--) {
            const String& path = project.files[i].relative_path;
            if(path == "main.cpp" || path.EndsWith(".upp") ||
               path == "design.json")
                project.files.Remove(i);
        }
    }
    else {
        UiDesignerGeneratedFile& metadata = project.files.Add();
        metadata.relative_path = "project.uidesign.json";
        metadata.content = AsJSON(
            BuildProjectValue(document, theme, request.generation), true);
        metadata.generator_owned = true;
    }

    if(request.generation.include_theme) {
        UiDesignerGeneratedFile& theme_file = project.files.Add();
        theme_file.relative_path = "theme.json";
        theme_file.content = theme.Serialize(true);
        theme_file.generator_owned = true;
    }
    error.Clear();
    return project;
}

UiDesignerExportResult UiDesignerExportService::Preview(
    const UiDesignerDocument& document,
    const UiDesignerThemeDocument& theme,
    const UiDesignerExportRequest& request) const
{
    UiDesignerExportResult result;
    if(request.destination.IsEmpty()) {
        result.diagnostic = "Export destination is empty";
        return result;
    }

    if(request.profile == UiDesignerExportProfile::CompleteCppPackage ||
       request.profile == UiDesignerExportProfile::ComponentOnly) {
        String error;
        UiDesignerGeneratedProject project =
            BuildCppProject(document, theme, request, error);
        if(!error.IsEmpty()) {
            result.diagnostic = error;
            return result;
        }
        for(const UiDesignerGeneratedFile& file : project.files) {
            const String path = AppendFileName(request.destination,
                                               file.relative_path);
            result.inventory.Add(path);
            if(FileExists(path) && !file.generator_owned &&
               request.write.preserve_user_files)
                result.preserved_files.Add(path);
        }
    }
    else {
        const String default_name =
            request.profile == UiDesignerExportProfile::ProjectJson
                ? "project.uidesign.json"
                : request.profile == UiDesignerExportProfile::DocumentJson
                    ? "document.uidesign.json" : "theme.json";
        result.inventory.Add(ResolveJsonPath(request, default_name));
    }
    result.success = true;
    result.diagnostic = ExportProfileName(request.profile);
    return result;
}

UiDesignerExportResult UiDesignerExportService::Execute(
    const UiDesignerDocument& document,
    const UiDesignerThemeDocument& theme,
    const UiDesignerExportRequest& request) const
{
    UiDesignerExportResult result = Preview(document, theme, request);
    if(!result.success)
        return result;

    String error;
    if(request.profile == UiDesignerExportProfile::CompleteCppPackage ||
       request.profile == UiDesignerExportProfile::ComponentOnly) {
        UiDesignerGeneratedProject project =
            BuildCppProject(document, theme, request, error);
        if(error.IsEmpty()) {
            Vector<String> written;
            if(!UiDesignerWriteGeneratedProject(
                    request.destination, project, request.write,
                    written, error)) {
                result.success = false;
                result.diagnostic = error;
                return result;
            }
            result.written_files = pick(written);
        }
    }
    else {
        String path;
        String content;
        if(request.profile == UiDesignerExportProfile::ProjectJson) {
            path = ResolveJsonPath(request, "project.uidesign.json");
            content = AsJSON(
                BuildProjectValue(document, theme, request.generation), true);
        }
        else if(request.profile == UiDesignerExportProfile::DocumentJson) {
            path = ResolveJsonPath(request, "document.uidesign.json");
            content = UiDesignerSerialize(document, true);
        }
        else {
            path = ResolveJsonPath(request, "theme.json");
            content = theme.Serialize(true);
        }
        if(!WriteSingleFileAtomic(path, content,
                                  request.write.overwrite, error)) {
            result.success = false;
            result.diagnostic = error;
            return result;
        }
        result.written_files.Add(path);
    }

    result.success = true;
    result.diagnostic = ExportProfileName(request.profile) + " completed";
    return result;
}

}
