#ifndef _Ui_UiFrameTicker_h_
#define _Ui_UiFrameTicker_h_

#include <CtrlCore/CtrlCore.h>

namespace Upp {

// UiFrameTicker owns exactly one one-shot TimeCallback and rearms it only after
// the current frame callback returns. This keeps custom UI frame clocks out of
// Ctrl's integer timer-id API, whose ids are byte offsets inside Ctrl rather
// than arbitrary application identifiers.
//
// Start() is intentionally idempotent while running. Stop()/Restart() bump a
// generation so a stale callback cannot re-arm an obsolete run. The Pte/Ptr
// guards also make scheduling safe if the owning control destroys this member
// before or from inside the frame callback.
class UiFrameTicker : public Pte<UiFrameTicker> {
public:
    UiFrameTicker() = default;
    ~UiFrameTicker() { Stop(); }

    UiFrameTicker(const UiFrameTicker&) = delete;
    UiFrameTicker& operator=(const UiFrameTicker&) = delete;

    bool IsRunning() const { return running_; }
    int  GetInterval() const { return interval_ms_; }

    void Start(int interval_ms, Function<void()> cb)
    {
        if(running_)
            return;

        interval_ms_ = max(1, interval_ms);
        callback_ = pick(cb);
        if(!callback_)
            return;

        running_ = true;
        const uint64 generation = ++generation_;
        Arm_(generation);
    }

    void Restart(int interval_ms, Function<void()> cb)
    {
        Stop();
        Start(interval_ms, pick(cb));
    }

    void Stop()
    {
        running_ = false;
        ++generation_;
        timer_.Kill();
        callback_ = Function<void()>();
    }

private:
    void Arm_(uint64 generation)
    {
        Ptr<UiFrameTicker> self = this;
        timer_.KillSet(interval_ms_, [self, generation] {
            if(self)
                self->Tick_(generation);
        });
    }

    void Tick_(uint64 generation)
    {
        if(!running_ || generation != generation_)
            return;

        Ptr<UiFrameTicker> alive = this;
        Function<void()> cb = callback_;
        if(cb)
            cb();

        if(!alive)
            return;
        if(running_ && generation == generation_)
            Arm_(generation);
    }

    TimeCallback     timer_;
    Function<void()> callback_;
    int              interval_ms_ = 16;
    uint64           generation_ = 0;
    bool             running_ = false;
};

} // namespace Upp

#endif
