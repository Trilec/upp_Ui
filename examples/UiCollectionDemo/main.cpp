#include "UiCollectionDemo.h"

using namespace Upp;

GUI_APP_MAIN
{
    UiCollectionDemo demo;
    const Vector<String>& args = CommandLine();
    if(args.GetCount() == 2 && args[0] == "--acceptance") {
        SetExitCode(demo.RunAcceptance(args[1]) ? 0 : 1);
        return;
    }
    demo.Run();
}
