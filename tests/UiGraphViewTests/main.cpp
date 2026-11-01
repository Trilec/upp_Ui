#include <Core/Core.h>

using namespace Upp;

int RunCanonicalShapeSuite();
int RunHierarchyViewSuite();
int RunInteractionStateSuite();
int RunSelectionModifierSuite();
int RunRouteEditSuite();
int RunDragDamageSuite();
int RunLiveViewSuite();

CONSOLE_APP_MAIN
{
    int failed = 0;
    failed += RunCanonicalShapeSuite() != 0;
    failed += RunHierarchyViewSuite() != 0;
    failed += RunInteractionStateSuite() != 0;
    failed += RunSelectionModifierSuite() != 0;
    failed += RunRouteEditSuite() != 0;
    failed += RunDragDamageSuite() != 0;
    failed += RunLiveViewSuite() != 0;

    Cout() << "\nUIGRAPH_VIEW_TESTS_SUMMARY suites=7 failed_suites="
           << failed << '\n';
    SetExitCode(failed ? 1 : 0);
}
