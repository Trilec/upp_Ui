#include <Core/Core.h>

using namespace Upp;

int RunUiGraphCoreSuite();
int RunUiGraphHierarchySuite();

CONSOLE_APP_MAIN
{
    int failed = 0;
    failed += RunUiGraphCoreSuite() != 0;
    failed += RunUiGraphHierarchySuite() != 0;

    Cout() << "\nUIGRAPH_MODEL_TESTS_SUMMARY suites=2 failed_suites="
           << failed << '\n';
    SetExitCode(failed ? 1 : 0);
}
