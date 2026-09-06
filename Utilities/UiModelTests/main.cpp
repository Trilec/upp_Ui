#include <Core/Core.h>

using namespace Upp;

int RunDataModelsSuite();
int RunModelBindingSuite();
int RunModelMutationSuite();

CONSOLE_APP_MAIN
{
    int failed = 0;
    failed += RunDataModelsSuite() != 0;
    failed += RunModelBindingSuite() != 0;
    failed += RunModelMutationSuite() != 0;

    Cout() << "\nUI_MODEL_TESTS_SUMMARY suites=3 failed_suites="
           << failed << '\n';
    SetExitCode(failed ? 1 : 0);
}
