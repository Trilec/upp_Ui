/*
    Cancellable catalogue worker. Requests supersede older generations; Poll
    transfers only the current completed snapshot. The registered U++ thread
    owns shared state, never a control, so closing a dialog need not wait for a
    blocked filesystem call. This owner is intentionally non-copyable.
*/
#ifndef _UiFileBrowser_UiFileBrowserScanner_h_
#define _UiFileBrowser_UiFileBrowserScanner_h_

#include "UiFileBrowserModel.h"
#include <memory>

namespace Upp
{

// Worker-owned filesystem state. No Ctrl, callbacks into UI, or GUI locks.
class UiFileBrowserScanner
{
public:
    struct Result
    {
        uint64 generation = 0;
        std::unique_ptr<UiFileBrowserModel> model;
        String error;
    };
    UiFileBrowserScanner();
    ~UiFileBrowserScanner();
    UiFileBrowserScanner(const UiFileBrowserScanner &) = delete;
    UiFileBrowserScanner &operator=(const UiFileBrowserScanner &) = delete;
    uint64 Request(const String &folder, bool grouped);
    bool Poll(Result &result);
    int GetScannedCount() const;

private:
    struct State;
    std::shared_ptr<State> state_;
};

} // namespace Upp
#endif
