#include "UiFileBrowserScanner.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>

namespace Upp
{

struct UiFileBrowserScanner::State
{
    std::mutex mutex;
    std::condition_variable wake;
    std::atomic<uint64> generation{0};
    std::atomic<int> scanned{0};
    bool stop = false;
    bool pending = false;
    String folder;
    bool grouped = true;
    Result ready;
};

UiFileBrowserScanner::UiFileBrowserScanner() : state_(std::make_shared<State>())
{
    // A blocked OS enumeration may outlive a closed dialog. It owns only State,
    // and will exit at the next OS return; closing the GUI never waits on it.
    Thread::Start(
        [state = state_]
        {
            for(;;)
            {
                String folder;
                bool grouped;
                uint64 generation;
                {
                    std::unique_lock<std::mutex> lock(state->mutex);
                    while(!state->stop && !state->pending && !Thread::IsShutdownThreads())
                        state->wake.wait_for(lock, std::chrono::milliseconds(100));
                    if(state->stop || Thread::IsShutdownThreads())
                        return;
                    folder = state->folder;
                    grouped = state->grouped;
                    generation = state->generation;
                    state->pending = false;
                }
                Result result;
                result.generation = generation;
                result.model = std::make_unique<UiFileBrowserModel>();
                auto cancelled = [&]
                { return state->generation.load() != generation || Thread::IsShutdownThreads(); };
                try
                {
                    result.model->Scan(folder, grouped, result.error, cancelled,
                                       [&](int count)
                                       {
                                           if(!cancelled())
                                               state->scanned = count;
                                       });
                }
                catch(const std::exception &e)
                {
                    result.error = String("Scan failed: ") + e.what();
                }
                catch(...)
                {
                    result.error = "Scan failed";
                }
                std::lock_guard<std::mutex> lock(state->mutex);
                if(state->stop)
                    return;
                if(!cancelled())
                    state->ready = std::move(result);
            }
        });
}

UiFileBrowserScanner::~UiFileBrowserScanner()
{
    {
        std::lock_guard<std::mutex> lock(state_->mutex);
        state_->stop = true;
        ++state_->generation;
    }
    state_->wake.notify_one();
}

uint64 UiFileBrowserScanner::Request(const String &folder, bool grouped)
{
    uint64 generation;
    {
        std::lock_guard<std::mutex> lock(state_->mutex);
        generation = ++state_->generation;
        state_->folder = folder;
        state_->grouped = grouped;
        state_->pending = true;
        state_->ready = Result();
        state_->scanned = 0;
    }
    state_->wake.notify_one();
    return generation;
}

bool UiFileBrowserScanner::Poll(Result &result)
{
    std::lock_guard<std::mutex> lock(state_->mutex);
    if(!state_->ready.model)
        return false;
    result = std::move(state_->ready);
    return true;
}

int UiFileBrowserScanner::GetScannedCount() const { return state_->scanned.load(); }

} // namespace Upp
