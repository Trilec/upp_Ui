#include "UiGraphDemo.h"

using namespace Upp;

GUI_APP_MAIN
{
    UiGraphDemo demo;
    InstallUiGraphDemoRuntime(demo);
    for(const String& argument : CommandLine())
        if(argument.StartsWith("--export-code=")) {
            SaveFile(argument.Mid(14), demo.GenerateUsageCode());
            return;
        }
        else if(argument.StartsWith("--export-topology=")) {
            SaveFile(argument.Mid(18), demo.GenerateUsageCode(true));
            return;
        }
        else if(argument.StartsWith("--export-authored-topology=")) {
            SaveFile(argument.Mid(27), demo.GenerateUsageCode(true, true));
            return;
        }
    demo.Run();
}
