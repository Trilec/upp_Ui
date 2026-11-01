#include <Core/Core.h>

using namespace Upp;

int RunThemeStructureSuite();
int RunThemeSurfaceSuite();

CONSOLE_APP_MAIN
{
    int failed = 0;
    failed += RunThemeStructureSuite() != 0;
    failed += RunThemeSurfaceSuite() != 0;

    Cout() << "\nUI_THEME_TESTS_SUMMARY suites=2 failed_suites="
           << failed << '\n';
    SetExitCode(failed ? 1 : 0);
}
