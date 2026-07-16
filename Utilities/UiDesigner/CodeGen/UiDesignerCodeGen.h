#ifndef _Utilities_UiDesigner_CodeGen_UiDesignerCodeGen_h_
#define _Utilities_UiDesigner_CodeGen_UiDesignerCodeGen_h_

#include <Utilities/UiDesigner/Core/UiDesignerCore.h>
#include <Utilities/UiDesigner/Catalog/UiDesignerCatalog.h>

namespace Upp {

enum class UiDesignerExportProfile : byte {
    CompleteCppPackage = 0,
    ComponentOnly,
    ProjectJson,
    DocumentJson,
    ThemeJson,
};

enum class UiDesignerOverwritePolicy : byte {
    RefuseExisting = 0,
    ReplaceGenerated,
    ReplaceAll,
};

struct UiDesignerCodeGenerationOptions : Moveable<UiDesignerCodeGenerationOptions> {
    String package_name = "GeneratedUi";
    String class_name = "GeneratedUiWindow";
    String namespace_name = "Upp";
    String appearance_mode = "ThemeFirst";
    bool include_source_design = true;
    bool include_theme = true;
};

struct UiDesignerGeneratedFile : Moveable<UiDesignerGeneratedFile> {
    String relative_path;
    String content;
    bool generator_owned = true;
    bool required = true;
};

struct UiDesignerGeneratedProject {
    // Backward-compatible aliases for the generated base class.
    String header;
    String source;
    String package;
    String json;

    String generated_header;
    String generated_source;
    String user_header;
    String user_source;
    String main_source;
    Vector<UiDesignerGeneratedFile> files;
    Vector<String> diagnostics;

    const UiDesignerGeneratedFile* FindFile(const String& path) const;
    bool IsValid() const { return diagnostics.IsEmpty(); }
};

struct UiDesignerExportWriteOptions : Moveable<UiDesignerExportWriteOptions> {
    UiDesignerOverwritePolicy overwrite =
        UiDesignerOverwritePolicy::ReplaceGenerated;
    bool preserve_user_files = true;
};

class UiDesignerCodeGenerator {
public:
    explicit UiDesignerCodeGenerator(const UiDesignerCatalog& catalog)
        : catalog_(catalog) {}

    UiDesignerGeneratedProject Generate(
        const UiDesignerDocument& document,
        const String& class_name = "GeneratedUiWindow") const;
    UiDesignerGeneratedProject Generate(
        const UiDesignerDocument& document,
        const UiDesignerCodeGenerationOptions& options) const;

    String GenerateHeader(const UiDesignerDocument& document,
                          const String& class_name) const;
    String GenerateSource(const UiDesignerDocument& document,
                          const String& class_name) const;
    String GeneratePackage(const String& package_name) const;

private:
    String MemberName(const UiDesignerNode& node) const;
    String EmitValue(const Value& value) const;
    String EmitColor(Color color) const;
    String QualifiedClass(const UiDesignerCodeGenerationOptions& options,
                          const String& suffix = String()) const;
    void EmitSetup(String& out, const UiDesignerNode& node,
                   const UiDesignerControlSpec& spec) const;
    void EmitChildren(String& out, const UiDesignerDocument& document,
                      const UiDesignerNode& node) const;
    void EmitSpacer(String& out, const UiDesignerNode& spacer,
                    const UiDesignerNode& parent) const;
    void EmitBinding(String& out, const UiDesignerDocument& document,
                     const UiDesignerNode& node,
                     const UiDesignerActionBinding& binding) const;
    Vector<String> CollectHandlers(const UiDesignerDocument& document) const;

    const UiDesignerCatalog& catalog_;
};

bool UiDesignerValidateCppIdentifier(const String& value, String& error);
bool UiDesignerValidateGenerationOptions(
    const UiDesignerCodeGenerationOptions& options, String& error);

bool UiDesignerWriteGeneratedProject(
    const String& folder, const String& package_name,
    const UiDesignerGeneratedProject& project, String& error);
bool UiDesignerWriteGeneratedProject(
    const String& folder, const UiDesignerGeneratedProject& project,
    const UiDesignerExportWriteOptions& options,
    Vector<String>& written_files, String& error);

}

#endif
