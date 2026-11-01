#include <Core/Core.h>

using namespace Upp;

int RunScaleSuite();
int RunPerformanceSuite();
int RunPanProfileSuite();
int RunModelSwitchProfileSuite();
int RunComponentProfileSuite();

CONSOLE_APP_MAIN
{
    // Explicit focused gates; the no-argument four-suite contract is unchanged.
    const auto& args = CommandLine();
    if(args.GetCount() == 1 && (args[0] == "--components" || args[0] == "--pan-profile")) {
        int result = args[0] == "--components" ? RunComponentProfileSuite() : RunPanProfileSuite();
        SetExitCode(result ? 1 : 0);
        return;
    }
    if(!args.IsEmpty()) {
        Cout() << "Usage: UiGraphScaleTests [--components|--pan-profile]\n";
        SetExitCode(2);
        return;
    }
    int failed = 0;
    failed += RunScaleSuite() != 0;
    failed += RunPerformanceSuite() != 0;
    failed += RunPanProfileSuite() != 0;
    failed += RunModelSwitchProfileSuite() != 0;

    Cout() << "\nUIGRAPH_SCALE_TESTS_SUMMARY suites=4 failed_suites="
           << failed << '\n';
    SetExitCode(failed ? 1 : 0);
}
