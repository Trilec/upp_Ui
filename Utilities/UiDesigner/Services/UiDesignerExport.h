#ifndef _Utilities_UiDesigner_Services_UiDesignerExport_h_
#define _Utilities_UiDesigner_Services_UiDesignerExport_h_

#include <Utilities/UiDesigner/CodeGen/UiDesignerCodeGen.h>
#include <Utilities/UiDesigner/ThemeCore/UiDesignerTheme.h>

namespace Upp {

struct UiDesignerExportRequest : Moveable<UiDesignerExportRequest> {
    UiDesignerExportProfile profile =
        UiDesignerExportProfile::CompleteCppPackage;
    String destination;
    UiDesignerCodeGenerationOptions generation;
    UiDesignerExportWriteOptions write;
};

struct UiDesignerExportResult : Moveable<UiDesignerExportResult> {
    Vector<String> inventory;
    Vector<String> written_files;
    Vector<String> preserved_files;
    String diagnostic;
    bool success = false;
};

class UiDesignerExportService {
public:
    explicit UiDesignerExportService(const UiDesignerCatalog& catalog)
        : catalog_(catalog) {}

    UiDesignerExportResult Preview(
        const UiDesignerDocument& document,
        const UiDesignerThemeDocument& theme,
        const UiDesignerExportRequest& request) const;
    UiDesignerExportResult Execute(
        const UiDesignerDocument& document,
        const UiDesignerThemeDocument& theme,
        const UiDesignerExportRequest& request) const;

    Value BuildProjectValue(
        const UiDesignerDocument& document,
        const UiDesignerThemeDocument& theme,
        const UiDesignerCodeGenerationOptions& generation) const;

private:
    UiDesignerGeneratedProject BuildCppProject(
        const UiDesignerDocument& document,
        const UiDesignerThemeDocument& theme,
        const UiDesignerExportRequest& request,
        String& error) const;
    bool WriteSingleFileAtomic(const String& path, const String& content,
                               UiDesignerOverwritePolicy overwrite,
                               String& error) const;
    String ResolveJsonPath(const UiDesignerExportRequest& request,
                           const String& default_name) const;

    const UiDesignerCatalog& catalog_;
};

}

#endif
