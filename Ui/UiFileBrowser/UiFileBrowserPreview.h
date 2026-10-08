/* Bounded, selection-driven preview service. The generic browser owns JPEG/PNG
   and ten-line text previews; a host provider adds formats without an imaging
   dependency. Providers run on the worker: capture owned services, never Ctrl.
   Requests supersede old generations; worker state can outlive a closed dialog. */
#ifndef _UiFileBrowser_Preview_h_
#define _UiFileBrowser_Preview_h_
#include <CtrlLib/CtrlLib.h>
#include <functional>
#include <memory>
namespace Upp
{
struct UiFileBrowserPreviewRequest
{
    String path;
    Size max_size = Size(512, 320);
    int64 image_file_bytes = 64LL * 1024 * 1024;
    int64 image_pixels = 64LL * 1024 * 1024;
};
struct UiFileBrowserPreviewData
{
    enum class Type
    {
        Message,
        Image,
        Text
    };
    Type type = Type::Message;
    Image image;
    Size image_size{0, 0}; // Original dimensions, separate from the thumbnail.
    String text, info;
};
using UiFileBrowserPreviewCancel = std::function<bool()>;
// Return false to use the built-in provider; true means handled, including error.
using UiFileBrowserPreviewProvider = std::function<bool(
    const UiFileBrowserPreviewRequest &, UiFileBrowserPreviewData &, const UiFileBrowserPreviewCancel &)>;
UiFileBrowserPreviewData LoadFileBrowserPreview(const UiFileBrowserPreviewRequest &request,
                                                const UiFileBrowserPreviewCancel &cancelled);
class UiFileBrowserPreviewWorker
{
public:
    struct Result
    {
        uint64 generation = 0;
        UiFileBrowserPreviewData data;
    };
    UiFileBrowserPreviewWorker();
    ~UiFileBrowserPreviewWorker();
    UiFileBrowserPreviewWorker(const UiFileBrowserPreviewWorker &) = delete;
    UiFileBrowserPreviewWorker &operator=(const UiFileBrowserPreviewWorker &) = delete;
    uint64 Request(UiFileBrowserPreviewRequest request, UiFileBrowserPreviewProvider provider = {});
    void Cancel();
    bool Poll(Result &result);

private:
    struct State;
    std::shared_ptr<State> state_;
};
} // namespace Upp
#endif
