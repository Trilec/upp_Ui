#include "UiThemeDemo.h"

GUI_APP_MAIN
{
    UiThemeDemo demo;
    const Vector<String>& args = CommandLine();
    if(args.GetCount() == 2 && args[0] == "--check") {
        demo.Open(); Ctrl::ProcessEvents();
        SetExitCode(demo.CheckPresets(args[1]) ? 0 : 1); demo.Close();
    }
    else if(args.GetCount() == 2 && args[0] == "--capture") {
        demo.Open(); Ctrl::ProcessEvents(); demo.Capture(args[1]); demo.Close();
    }
    else demo.Run();
}
