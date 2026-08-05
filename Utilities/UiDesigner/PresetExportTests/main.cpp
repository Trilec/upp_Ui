#include <Core/Core.h>
#include <Utilities/UiDesigner/Services/UiDesignerServices.h>

using namespace Upp;

static bool ExportPreset(const String& preset, const String& package_name,
                         const String& destination, String& error)
{
    UiDesignerSession session;
    session.NewDocument(preset);

    UiDesignerExportRequest request;
    request.profile = UiDesignerExportProfile::CompleteCppPackage;
    request.destination = destination;
    request.generation.package_name = package_name;
    request.generation.class_name = package_name + "Window";
    request.generation.namespace_name = "Upp";
    request.generation.include_source_design = true;
    request.generation.include_theme = true;
    request.write.overwrite = UiDesignerOverwritePolicy::ReplaceAll;
    request.write.preserve_user_files = false;

    UiDesignerExportService service(session.Catalog());
    UiDesignerExportResult result = service.Execute(
        session.Document(), session.Theme(), request);
    if(!result.success) {
        error = preset + ": " + result.diagnostic;
        return false;
    }

    const String upp = AppendFileName(destination, package_name + ".upp");
    const String generated_h = AppendFileName(
        destination, package_name + "Window.generated.h");
    const String generated_cpp = AppendFileName(
        destination, package_name + "Window.generated.cpp");
    if(!FileExists(upp) || !FileExists(generated_h) || !FileExists(generated_cpp)) {
        error = preset + ": generated package is incomplete";
        return false;
    }

    Cout() << "EXPORTED " << preset << " " << destination << '\n';
    error.Clear();
    return true;
}

CONSOLE_APP_MAIN
{
    const Vector<String>& args = CommandLine();
    if(args.GetCount() != 2 || args[0] != "--export-all") {
        Cout() << "Usage: UiDesignerPresetExportTests --export-all <folder>\n";
        SetExitCode(2);
        return;
    }

    const String root = NormalizePath(args[1]);
    DeleteFolderDeep(root);
    RealizeDirectory(root);

    struct Fixture {
        const char *preset;
        const char *package_name;
    };
    const Fixture fixtures[] = {
        {"blank", "BlankFormGenerated"},
        {"three_pane", "ThreePaneGenerated"},
        {"dialog", "DialogGenerated"},
    };

    String error;
    for(const Fixture& fixture : fixtures) {
        const String destination = AppendFileName(root, fixture.package_name);
        RealizeDirectory(destination);
        if(!ExportPreset(fixture.preset, fixture.package_name,
                         destination, error)) {
            Cout() << "FAIL: " << error << '\n';
            SetExitCode(1);
            return;
        }
    }

    Cout() << "UiDesignerPresetExportTests: 3 creation presets exported\n";
}
