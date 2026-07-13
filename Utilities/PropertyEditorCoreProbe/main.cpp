#include <Utilities/PropertyEditorCore/PropertyEditorCore.h>

using namespace Upp;

CONSOLE_APP_MAIN
{
    PropertyEditorModel model;
    model.AddText("name", "Name", "Probe");
    model.AddDouble("opacity", "Opacity", 1.0).SetRange(0.0, 1.0, 0.01);
    model.AddBoolean("enabled", "Enabled", true);

    Cout() << "PropertyEditorCoreProbe: items=" << model.GetCount()
           << " structure=" << model.GetStructureRevision()
           << " value=" << model.GetValueRevision() << '\n';
}
