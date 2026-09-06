#include <Core/Core.h>

using namespace Upp;

int RunButtonSuite();
int RunColorMatrixSuite();
int RunDateTimeSuite();
int RunGroupPanelSuite();
int RunMatrixSelectorSuite();
int RunProgressBarSuite();
int RunProgressRingSuite();
int RunChartRingSuite();
int RunRangeSliderEditSuite();
int RunRangeSliderSuite();
int RunSliderSuite();
int RunStackSuite();
int RunTabSuite();

CONSOLE_APP_MAIN
{
    int failed = 0;
    failed += RunButtonSuite() != 0;
    failed += RunColorMatrixSuite() != 0;
    failed += RunDateTimeSuite() != 0;
    failed += RunGroupPanelSuite() != 0;
    failed += RunMatrixSelectorSuite() != 0;
    failed += RunProgressBarSuite() != 0;
    failed += RunProgressRingSuite() != 0;
    failed += RunChartRingSuite() != 0;
    failed += RunRangeSliderEditSuite() != 0;
    failed += RunRangeSliderSuite() != 0;
    failed += RunSliderSuite() != 0;
    failed += RunStackSuite() != 0;
    failed += RunTabSuite() != 0;

    Cout() << "\nUI_CONTROL_TESTS_SUMMARY suites=13 failed_suites="
           << failed << '\n';
    SetExitCode(failed ? 1 : 0);
}
