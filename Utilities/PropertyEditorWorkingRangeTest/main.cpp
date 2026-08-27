#include <Core/Core.h>
#include <Utilities/PropertyEditor/PropertyWorkingRangeEditors.h>

using namespace Upp;

CONSOLE_APP_MAIN
{
    int checks = 0;
    int failed = 0;
    auto Check = [&](bool condition, const char *label) {
        checks++;
        if(!condition) {
            failed++;
            Cout() << "FAIL: " << label << '\n';
        }
    };

    int lo = 0;
    int hi = 0;
    const String variant = PropertyEditorWorkingRangeVariant(0, 500);
    Check(variant == "0:500", "working range has stable encoding");
    Check(PropertyEditorParseWorkingRangeVariant(variant, lo, hi),
          "working range parses");
    Check(lo == 0 && hi == 500, "working range preserves endpoints");
    Check(!PropertyEditorParseWorkingRangeVariant("500:0", lo, hi),
          "reversed working range is rejected");
    Check(!PropertyEditorParseWorkingRangeVariant("not-a-range", lo, hi),
          "malformed working range is rejected");

    PropertyEditorFactory factory;
    RegisterPropertyEditorWorkingRangeEditors(factory);
    Check(factory.HasCustom(PropertyEditorWorkingRangeIntId()),
          "working-range editor registers");

    PropertyEditorItem item;
    item.id = "size";
    item.label = "Size";
    item.kind = PropertyEditorKind::Custom;
    item.custom_editor = PropertyEditorWorkingRangeIntId();
    item.editor_variant = PropertyEditorWorkingRangeVariant(0, 500);
    item.minimum = 0;
    item.maximum = 10000;
    item.step = 1;

    One<PropertyValueEditor> editor = factory.Create(item);
    Check((bool)editor, "factory creates working-range editor");
    if(editor) {
        editor->Configure(item);
        editor->SetEditorValue(900, false);
        Check((int)editor->GetEditorValue() == 900,
              "typed value above slider working maximum remains legal");
        editor->SetEditorValue(10000, false);
        Check((int)editor->GetEditorValue() == 10000,
              "legal maximum remains available through numeric entry");
        editor->SetEditorValue(320, false);
        Check((int)editor->GetEditorValue() == 320,
              "value inside working range remains exact");
    }

    Cout() << "PROPERTY_EDITOR_WORKING_RANGE_SUMMARY checks=" << checks
           << " failed=" << failed << '\n';
    SetExitCode(failed ? 1 : 0);
}
