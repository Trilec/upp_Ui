#include "UiRangeSegmentsDemo.h"

using namespace Upp;

GUI_APP_MAIN
{
    UiRangeSegmentsDemo demo;
    const Vector<String>& args = CommandLine();
    if(args.GetCount() == 2 && args[0] == "--export-generated") demo.ExportGenerated(args[1]);
    else demo.Run();
}
