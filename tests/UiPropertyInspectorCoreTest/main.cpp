#include <Ui/UiPropertyInspector/Core/UiPropertyInspectorCore.h>

using namespace Upp;

CONSOLE_APP_MAIN
{
    UiPropertyInspectorModel model;
    model.AddText("name", "Name", "Probe");
    model.AddDouble("opacity", "Opacity", 1.0).SetRange(0.0, 1.0, 0.01);
    model.AddBoolean("enabled", "Enabled", true);

    int failed = model.GetCount() == 3 ? 0 : 1;
    Cout() << "UiPropertyInspectorCoreTest: items=" << model.GetCount()
           << " structure=" << model.GetStructureRevision()
           << " value=" << model.GetValueRevision() << '\n';
    Cout() << "UI_PROPERTY_INSPECTOR_CORE_SUMMARY checks=1 failed=" << failed << '\n';
    SetExitCode(failed ? 1 : 0);
}
