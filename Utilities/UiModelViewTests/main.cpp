#include <Core/Core.h>

using namespace Upp;

int RunModelViewPerformanceSuite();
int RunGalleryRegressionSuite();
int RunTreeScaleSuite();
int RunListStyleSuite();
int RunDropdownMenuRenderSuite();

CONSOLE_APP_MAIN
{
    int failed = 0;
    failed += RunModelViewPerformanceSuite() != 0;
    failed += RunGalleryRegressionSuite() != 0;
    failed += RunTreeScaleSuite() != 0;
    failed += RunListStyleSuite() != 0;
    failed += RunDropdownMenuRenderSuite() != 0;

    Cout() << "\nUI_MODEL_VIEW_TESTS_SUMMARY suites=5 failed_suites="
           << failed << '\n';
    SetExitCode(failed ? 1 : 0);
}
