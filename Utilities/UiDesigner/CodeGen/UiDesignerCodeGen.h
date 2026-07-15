#ifndef _Utilities_UiDesigner_CodeGen_UiDesignerCodeGen_h_
#define _Utilities_UiDesigner_CodeGen_UiDesignerCodeGen_h_

#include <Utilities/UiDesigner/Core/UiDesignerCore.h>
#include <Utilities/UiDesigner/Catalog/UiDesignerCatalog.h>

namespace Upp {

struct UiDesignerGeneratedProject {
    String header;
    String source;
    String package;
    String json;
    Vector<String> diagnostics;
};

class UiDesignerCodeGenerator {
public:
    explicit UiDesignerCodeGenerator(const UiDesignerCatalog& catalog)
        : catalog_(catalog) {}

    UiDesignerGeneratedProject Generate(
        const UiDesignerDocument& document,
        const String& class_name = "GeneratedUiWindow") const;

    String GenerateHeader(const UiDesignerDocument& document,
                          const String& class_name) const;
    String GenerateSource(const UiDesignerDocument& document,
                          const String& class_name) const;
    String GeneratePackage(const String& package_name) const;

private:
    String MemberName(const UiDesignerNode& node) const;
    String EmitValue(const Value& value) const;
    void EmitSetup(String& out, const UiDesignerNode& node,
                   const UiDesignerControlSpec& spec) const;
    void EmitChildren(String& out, const UiDesignerDocument& document,
                      const UiDesignerNode& node) const;

    const UiDesignerCatalog& catalog_;
};

bool UiDesignerWriteGeneratedProject(
    const String& folder, const String& package_name,
    const UiDesignerGeneratedProject& project, String& error);

}

#endif
