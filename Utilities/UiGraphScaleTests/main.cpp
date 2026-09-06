#include <Core/Core.h>

using namespace Upp;

int RunScaleSuite();
int RunPerformanceSuite();
int RunPanProfileSuite();
int RunModelSwitchProfileSuite();

CONSOLE_APP_MAIN
{
    int failed = 0;
    failed += RunScaleSuite() != 0;
    failed += RunPerformanceSuite() != 0;
    failed += RunPanProfileSuite() != 0;
    failed += RunModelSwitchProfileSuite() != 0;

    Cout() << "\nUIGRAPH_SCALE_TESTS_SUMMARY suites=4 failed_suites="
           << failed << '\n';
    SetExitCode(failed ? 1 : 0);
}
