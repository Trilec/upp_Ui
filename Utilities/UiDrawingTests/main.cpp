#include <Core/Core.h>

using namespace Upp;

int RunGeometryContractSuite();
int RunShapePathSuite();
int RunStyledSurfaceCacheSuite();

CONSOLE_APP_MAIN
{
    int failed = 0;
    failed += RunGeometryContractSuite() != 0;
    failed += RunShapePathSuite() != 0;
    failed += RunStyledSurfaceCacheSuite() != 0;
    Cout() << "\nUI_DRAWING_TESTS_SUMMARY suites=3 failed_suites="
           << failed << '\n';
    SetExitCode(failed ? 1 : 0);
}
