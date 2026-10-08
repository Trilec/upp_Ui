#ifndef _UiFileBrowser_Native_h_
#define _UiFileBrowser_Native_h_
// Private Win32 codec ownership shared by preview and starter-image creation.
// Public browser headers do not expose COM or require Windows codec headers.
#ifdef PLATFORM_WIN32
#include <wincodec.h>
namespace Upp
{
namespace UiFileBrowserNative
{
template <class T> struct ComOwner
{
    T *value = nullptr;
    ComOwner() = default;
    ComOwner(const ComOwner &) = delete;
    ComOwner &operator=(const ComOwner &) = delete;
    ~ComOwner()
    {
        if(value)
            value->Release();
    }
    T **Out() { return &value; }
    T *operator->() const { return value; }
};
class ComApartment
{
public:
    explicit ComApartment(DWORD mode) : status_(CoInitializeEx(nullptr, mode)) {}
    ~ComApartment()
    {
        if(SUCCEEDED(status_))
            CoUninitialize();
    }
    bool IsReady() const { return SUCCEEDED(status_) || status_ == RPC_E_CHANGED_MODE; }
    ComApartment(const ComApartment &) = delete;
    ComApartment &operator=(const ComApartment &) = delete;

private:
    HRESULT status_;
};
} // namespace UiFileBrowserNative
} // namespace Upp
#endif
#endif
