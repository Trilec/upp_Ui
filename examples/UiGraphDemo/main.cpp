#include "UiGraphDemo.h"

using namespace Upp;

GUI_APP_MAIN
{
    UiGraphDemo demo;
    InstallUiGraphDemoRuntime(demo);
    demo.Run();
}
