#include "UiButtonDemo.h"

using namespace Upp;

GUI_APP_MAIN
{
    UiButtonDemo demo;
    const Vector<String>& args = CommandLine();
    if(args.GetCount() == 2 && args[0] == "--export-generated") demo.ExportGenerated(args[1]);
    else if(args.GetCount()==2 && args[0]=="--selector-test") {
        demo.Open(); Ctrl::ProcessEvents(); bool passed=demo.TestSelectors(args[1]); demo.Close(); SetExitCode(passed?0:1);
    }
    else if(args.GetCount()==2 && args[0]=="--render") { demo.Open(); Ctrl::ProcessEvents(); demo.RenderFamilies(args[1]); demo.Close(); }
    else demo.Run();
}
