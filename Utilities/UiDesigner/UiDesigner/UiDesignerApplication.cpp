#include "UiDesignerApplication.h"

namespace Upp {

void UiDesignerApplication::Run()
{
    UiDesignerWindow window;
    window.OpenMain();
    window.WriteLaunchDiagnostic();
    window.Run();
}

}
