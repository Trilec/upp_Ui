// Small, bounded starter-file operations, independent of browser controls.
// Names are single leaves; files are created exclusively. Failed operations
// remove only their own uncommitted files (including half of a C++ pair).
#ifndef _UiFileBrowser_Create_h_
#define _UiFileBrowser_Create_h_
#include <CtrlLib/CtrlLib.h>
namespace Upp
{
enum class UiFileBrowserCreateKind
{
    Folder,
    Text,
    CppPair,
    Png,
    Jpeg
};
struct UiFileBrowserCreateRequest
{
    UiFileBrowserCreateKind kind = UiFileBrowserCreateKind::Folder;
    String name;
    Size size{1920, 1080};
    Color colour{208, 208, 208};
    bool gradient = false; // Vertical ramp from the chosen colour to white.
};
bool CreateFileBrowserItem(const String &folder, const UiFileBrowserCreateRequest &request,
                           String &created_path, String &error);
} // namespace Upp
#endif
