#include "WorkspaceWindow.h"
using namespace Upp;
GUI_APP_MAIN
{
    bool test_only = false;
    for(const String& arg : CommandLine()) if(arg == "--view-tests") test_only = true;
    bool run_tests = test_only;
#ifdef _DEBUG
    run_tests = true;
#endif
    if(run_tests) {
        String error;
        if(!GraphWorkspace::RunWorkspaceViewTests(error)) {
            LOG("UIGRAPH_WORKSPACE_VIEW_FAILURE: " << error);
            SetExitCode(1);
            return;
        }
        if(!GraphWorkspace::RunWorkspaceBandTests(error)) {
            LOG("UIGRAPH_WORKSPACE_BAND_UI_FAILURE: " << error);
            SetExitCode(1);
            return;
        }
        if(!GraphWorkspace::RunWorkspaceOverlayTests(error)) {
            LOG("UIGRAPH_WORKSPACE_OVERLAY_FAILURE: " << error);
            SetExitCode(1);
            return;
        }
    }
    if(!test_only) GraphWorkspace::NodeWorkspace().Run();
}
