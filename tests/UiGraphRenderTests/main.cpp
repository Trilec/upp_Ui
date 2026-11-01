#include <Core/Core.h>

using namespace Upp;

int RunDetailLodSuite();
int RunOverviewLodSuite();
int RunRenderLodSuite();
int RunPatternedPaintSuite();
int RunPresentationSuite();
int RunExecutionPathSuite();
int RunComponentSuite();
int RunWorkspaceComponentSuite();
int RunEllipseBandSuite();

CONSOLE_APP_MAIN
{
    int failed = 0;
    failed += RunDetailLodSuite() != 0;
    failed += RunOverviewLodSuite() != 0;
    failed += RunRenderLodSuite() != 0;
    failed += RunPatternedPaintSuite() != 0;
    failed += RunPresentationSuite() != 0;
    failed += RunExecutionPathSuite() != 0;
    failed += RunComponentSuite() != 0;
    failed += RunWorkspaceComponentSuite() != 0;
    failed += RunEllipseBandSuite() != 0;

    Cout() << "\nUIGRAPH_RENDER_TESTS_SUMMARY suites=9 failed_suites="
           << failed << '\n';
    SetExitCode(failed ? 1 : 0);
}

