#include "UiOsFileDialog.h"
#include <CtrlLib/CtrlLib.h>

using namespace Upp;

GUI_APP_MAIN
{
    StdLogSetup(LOG_FILE|LOG_COUT);

    UiOsFileDialog dlg;
    dlg.SetMode(UiOsFileDialog::Mode::OpenFiles)
       .SetTitle("Open files")
       .AddFilter("Images", "*.png;*.jpg;*.jpeg;*.bmp")
       .AddFilter("All files", "*.*");

    if(dlg.Execute()) {
        String out;
        for(const String& path : dlg.GetPaths())
            out << path << "\n";
        PromptOK(out.IsEmpty() ? "No files selected" : out);
    }
    else {
        PromptOK("Cancelled or backend unavailable.");
    }
}