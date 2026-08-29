#include <Core/Core.h>
#include <Utilities/PropertyEditor/PropertyWorkingRangeEditors.h>

#include <cmath>

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
    Check(variant == "0:500", "working range has stable integer encoding");
    Check(PropertyEditorParseWorkingRangeVariant(variant, lo, hi),
          "integer working range parses");
    Check(lo == 0 && hi == 500, "integer working range preserves endpoints");
    Check(!PropertyEditorParseWorkingRangeVariant("500:0", lo, hi),
          "reversed integer working range is rejected");
    Check(!PropertyEditorParseWorkingRangeVariant("not-a-range", lo, hi),
          "malformed integer working range is rejected");

    double dlo = 0.0;
    double dhi = 0.0;
    const String double_variant = PropertyEditorWorkingRangeVariant(-125.5, 640.25);
    Check(PropertyEditorParseWorkingRangeVariant(double_variant, dlo, dhi),
          "double working range parses");
    Check(std::fabs(dlo + 125.5) < 1e-12 && std::fabs(dhi - 640.25) < 1e-12,
          "double working range preserves endpoints");
    Check(!PropertyEditorParseWorkingRangeVariant("12.5:-3.0", dlo, dhi),
          "reversed double working range is rejected");
    Check(!PropertyEditorParseWorkingRangeVariant("nan:20", dlo, dhi),
          "non-finite double working range is rejected");

    PropertyEditorFactory factory;
    RegisterPropertyEditorWorkingRangeEditors(factory);
    Check(factory.HasCustom(PropertyEditorWorkingRangeIntId()),
          "integer working-range editor registers");
    Check(factory.HasCustom(PropertyEditorWorkingRangeDoubleId()),
          "double working-range editor registers");

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
    Check((bool)editor, "factory creates integer working-range editor");
    if(editor) {
        editor->Configure(item);
        editor->SetEditorValue(900, false);
        Check((int)editor->GetEditorValue() == 900,
              "integer typed value above slider working maximum remains legal");
        editor->SetEditorValue(10000, false);
        Check((int)editor->GetEditorValue() == 10000,
              "integer legal maximum remains available through numeric entry");
        editor->SetEditorValue(320, false);
        Check((int)editor->GetEditorValue() == 320,
              "integer value inside working range remains exact");
    }

    PropertyEditorItem double_item;
    double_item.id = "position";
    double_item.label = "Position";
    double_item.kind = PropertyEditorKind::Custom;
    double_item.custom_editor = PropertyEditorWorkingRangeDoubleId();
    double_item.editor_variant = PropertyEditorWorkingRangeVariant(-250.0, 750.0);
    double_item.minimum = -1000000.0;
    double_item.maximum = 1000000.0;
    double_item.step = 1.0;
    double_item.decimals = 2;

    One<PropertyValueEditor> double_editor = factory.Create(double_item);
    Check((bool)double_editor, "factory creates double working-range editor");
    if(double_editor) {
        double_editor->Configure(double_item);
        double_editor->SetEditorValue(900.5, false);
        Check(std::fabs((double)double_editor->GetEditorValue() - 900.5) < 1e-12,
              "double typed value above slider working maximum remains legal");
        double_editor->SetEditorValue(-723000.25, false);
        Check(std::fabs((double)double_editor->GetEditorValue() + 723000.25) < 1e-12,
              "double numeric entry preserves legal values outside the working slider range");
        double_editor->SetEditorValue(320.75, false);
        Check(std::fabs((double)double_editor->GetEditorValue() - 320.75) < 1e-12,
              "double value inside working range remains exact");
    }

    Cout() << "PROPERTY_EDITOR_WORKING_RANGE_SUMMARY checks=" << checks
           << " failed=" << failed << '\n';
    SetExitCode(failed ? 1 : 0);
}
