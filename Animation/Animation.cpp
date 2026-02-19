// Animation/Animation.cpp
//
// U++ Animation scheduler and state machine.
// - Drives per-frame updates for UI animations (position, color, opacity, etc.)
// - Designer-friendly easing via cubic-Bézier (CSS-style control points)
// - Looping, yoyo, delays, pause/resume, and lifecycle callbacks
// - Single-threaded: runs entirely on the UI thread via TimeCallback
//
// Invariants / behavior:
// - Never deletes animation state while iterating the active list.
//   (Removals are deferred to end-of-frame; Cancel/Stop is safe inside ticks.)
// - Progress() returns [0..1]. After Stop(): 1.0. After forced KillFor(): 0.0.
// - Easing callables are tiny lambdas; no global singletons or leaks.
//
// Changelog:
// 2025-08-19 — initial test pass / cleaned comments / cache helper
// 2025-08-21 — runtime Tick(n, max_ms_per_tick) manual driver (no #define)
// 2025-08-27 — yoyo fix, naming clarity, optimized scheduler
// 2025-08-27 — adjusted yoyo cycle count in Play, catch all exceptions
// 2025-09-07 — constexpr Bézier presets; move FPS config into Scheduler
//              defer removals to end-of-frame (fix access violation on Cancel/Stop),
//              KillFor(): Progress=0.0 semantics, no globals
// 2025-09-11 — headless deterministic console probe; owned re-entrant spawns;
//              explicit shutdown order (ClearPool → Animation::Finalize());
//              eliminated exit-time AV/leak; added L23–L26 tests & docs.
// 2025-09-13 — Reset(): abort + re-prime staging so the same instance can be
//              reused immediately. Tightened semantics (Pause reversible,
//              Cancel destructive, Reset clean slate). All setters and
//              operator()(...) now EnsureStaging_() to avoid null deref after
//              Cancel(). Added L27/L28 tests.
//
// Note: file banner path reflects package directory (Animation/).

#include "Animation.h"

using namespace Upp;

/*==================== Scheduler ====================*/
namespace {

struct Scheduler {
    // Singleton accessor (U++-style; safe for shutdown + memdiag).
    static Scheduler& Inst() { return Single<Scheduler>(); }

    // State
    Vector<Animation::State*> active; // Owns State* pointers
    TimeCallback ticker;              // Timer for frame updates
    bool running = false;             // Scheduler active state
    int  timer_id = 0;                // Timer identifier for validation
    int64 manual_last_now = 0;        // Monotonic time for manual ticking
    bool sweeping = false;            // true while RunFrame() iterates 'active'

    // Frame pacing
    int fps     = 60;
    int step_ms = 1000 / 60;

    // Change FPS safely while running: re-arms timer with new step.
    void SetFPSInternal(int f) {
        fps     = clamp(f, 1, 240);
        step_ms = max(1, 1000 / fps);
        if (running) {
            Stop();   // kills timer, bumps timer_id
            Start();  // re-arms with new step_ms
        }
    }
    int GetFPSInternal() const { return fps; }

    // Stop timer if there is nothing to advance (all paused or dying).
    void MaybeStopIfAllPaused() {
        for (Animation::State* s : active)
            if (s && !s->paused && !s->dying)
                return; // at least one needs ticking
        Stop();
    }

    // Ensure timer runs if there is something to advance.
    void EnsureRunningIfAnyUnpaused() {
        for (Animation::State* s : active)
            if (s && !s->paused && !s->dying) { Start(); return; }
        // all paused: nothing to do
    }

    // Destroy a state if present.
    void DeleteState(Animation::State* s) { if (s) delete s; }

    // Finalize: stop and purge all animations; detach back-pointers first.
    void Finalize() {
        running = false;
        ++timer_id;
        ticker.Kill();

        // Phase 1: break Animation ↔ State links so Animations don't keep live_.
        for (Animation::State* s : active) {
            if (!s) continue;
            if (s->anim) {
                Animation* a = s->anim;
                double snap = a->Progress();          // snapshot forward progress
                a->_OnStateRemovedCancel(snap);       // sets live_ = nullptr; cache
                s->anim = nullptr;                    // break back-pointer
            }
        }
        // Phase 2: delete states and clear.
        for (Animation::State* s : active)
            DeleteState(s);
        active.Clear();
        manual_last_now = 0;
    }

    // Start/stop timer loop.
    void Start() {
        if (running) return;
        running = true;
        int current_id = ++timer_id;
        ticker.Set(step_ms, callback1(this, &Scheduler::TickTimer, current_id));
    }
    void Stop() {
        if (!running) return;
        running = false;
        ++timer_id; // invalidate queued ticks
        ticker.Kill();
    }

    // Add/remove active states.
    void Add(Animation::State* s) {
        active.Add(s);
        Start();
    }
    void Remove(Animation::State* st) {
        if (!st) return;
        if (sweeping) { // never mutate 'active' mid-iteration
            st->dying = true;
            return;
        }
        for (int i = 0; i < active.GetCount(); ++i) {
            if (active[i] == st) {
                DeleteState(active[i]);
                active.Remove(i);
                break;
            }
        }
        if (active.IsEmpty())
            Stop();
    }

    // Kill all animations for a given Ctrl or dead owners; Progress=0.0.
    void KillFor(Ctrl* c) {
        for (int i = active.GetCount() - 1; i >= 0; --i) {
            Animation::State* s = active[i];
            if (!s || !s->owner || s->owner == c) {
                if (s && s->anim) {
                    Animation* a = s->anim;
                    a->_OnStateRemovedCancel(0.0); // clears a->live_, Progress=0
                    s->anim = nullptr;
                }
                if (s) s->dying = true; // defer delete to next sweep
            }
        }
        // Defer actual free to RunFrame(); avoids re-entrancy.
    }

    // Advance all active animations to 'now'; sweep dead states after iteration.
    void RunFrame(int64 now) {
        sweeping = true;
        Vector<int> to_remove;

        for (int i = 0; i < active.GetCount(); ++i) {
            Animation::State* s = active[i];
            bool cont = true;

            if (!s || s->dying) {
                cont = false;
            } else {
                try {
                    cont = s->Step(now);
                } catch (...) {
                    Cerr() << "Exception in Animation::State::Step\n";
                    cont = false;
                }
            }

            if (!cont) {
                if (s && s->anim) {
                    Animation* a = s->anim;
                    if (!s->owner) a->_OnStateRemovedCancel(0.0); // owner died → abort
                    else           a->_OnStateRemovedFinish();    // natural finish
                    s->anim = nullptr;
                }
                to_remove.Add(i);
            }
        }

        sweeping = false;

        // Delete after iteration to keep iteration stable.
        for (int k = to_remove.GetCount() - 1; k >= 0; --k) {
            DeleteState(active[to_remove[k]]);
            active.Remove(to_remove[k]);
        }

        if (active.IsEmpty())
            Stop();
    }

    // Timer-driven frame updates.
    void TickTimer(int current_id) {
        if (current_id != timer_id || !running) return;
        RunFrame(msecs());
        if (!active.IsEmpty())
            ticker.Set(step_ms, callback1(this, &Scheduler::TickTimer, current_id));
    }

    // One manual tick for tests; clamps dt if requested.
    void TickManualOnce(int max_ms_per_tick) {
        int64 wall_now = msecs();
        if (manual_last_now == 0)
            manual_last_now = wall_now;

        int64 dt = wall_now - manual_last_now;
        if (max_ms_per_tick > 0 && dt > max_ms_per_tick)
            dt = max_ms_per_tick;
        if (dt < 0) dt = 0; // guard against clock skew

        manual_last_now += dt;
        RunFrame(manual_last_now);
    }
};

} // namespace

/*==================== Animation::State::Step ====================
  Advance time within the current leg, compute eased value, invoke callbacks,
  and handle loop/yoyo bookkeeping. Returns true to keep scheduling. */
bool Animation::State::Step(int64 now)
{
    if (!owner) return false;   // owner died
    if (paused) return true;    // stay scheduled, do not advance

    const int64 local = now - start_ms + elapsed_ms;
    if (local < spec.delay_ms)
        return true;            // still in delay window

    const int dur = max(1, spec.duration_ms);
    double leg_progress = double(local - spec.delay_ms) / dur;
    leg_progress = clamp(leg_progress, 0.0, 1.0);

    // Adjust for yoyo direction.
    double t = reverse ? (1.0 - leg_progress) : leg_progress;

    // Apply easing.
    const double e = spec.easing ? spec.easing(t) : t;

    // Callbacks.
    if (spec.on_update) spec.on_update(e);
    if (spec.tick && !spec.tick(e))
        return false;           // user requested stop → treated as finish/cancel

    // Leg finished?
    if (leg_progress >= 1.0) {
        if (spec.yoyo) {
            reverse = !reverse;
            if (!reverse) { // finished a forward+reverse cycle
                if (spec.loop_count >= 0 && --cycles <= 0) {
                    if (spec.on_finish) spec.on_finish();
                    return false;       // natural finish
                }
            }
            start_ms = now;             // next leg
            elapsed_ms = 0;
        } else {
            if (spec.loop_count >= 0 && --cycles <= 0) {
                if (spec.on_finish) spec.on_finish();
                return false;           // natural finish
            }
            start_ms = now;             // next loop
            elapsed_ms = 0;
        }
    }
    return true;
}

/*==================== Animation implementation ====================*/

// Construct an Animation bound to 'owner'. Initializes empty staging config.
Animation::Animation(Ctrl& owner)
    : owner_(&owner)
{
    staging_box_.Create();
    staging_ = ~staging_box_;
}

// Destructor: detach safely if a run is still live.
// We use the internal unscheduler so we don't duplicate cleanup logic.
// This is a *silent* detach (no on_cancel); last_spec_ remains intact for Replay().
Animation::~Animation()
{
    if (live_)
        _Unschedule(false); // silent
}


#define RET(e) do { e; return *this; } while (0)

/*---------------- Staging setters (lazily re-prime if null) ----------------*/

Animation& Animation::Duration(int ms)                    { EnsureStaging_(); RET(staging_->duration_ms = ms); }
Animation& Animation::Ease(const Easing::Fn& fn)          { EnsureStaging_(); RET(staging_->easing = fn); }
Animation& Animation::Ease(Easing::Fn&& fn)               { EnsureStaging_(); RET(staging_->easing = pick(fn)); }
Animation& Animation::Loop(int n)                         { EnsureStaging_(); RET(staging_->loop_count = n); }
Animation& Animation::Yoyo(bool b)                        { EnsureStaging_(); RET(staging_->yoyo = b); }
Animation& Animation::Delay(int ms)                       { EnsureStaging_(); RET(staging_->delay_ms = ms); }

Animation& Animation::OnStart(const Event<>& cb)         { EnsureStaging_(); RET(staging_->on_start  = cb); }
Animation& Animation::OnStart(Event<>&& cb)              { EnsureStaging_(); RET(staging_->on_start  = pick(cb)); }

Animation& Animation::OnFinish(const Event<>& cb)        { EnsureStaging_(); RET(staging_->on_finish = cb); }
Animation& Animation::OnFinish(Event<>&& cb)             { EnsureStaging_(); RET(staging_->on_finish = pick(cb)); }

Animation& Animation::OnCancel(const Event<>& cb)        { EnsureStaging_(); RET(staging_->on_cancel = cb); }
Animation& Animation::OnCancel(Event<>&& cb)             { EnsureStaging_(); RET(staging_->on_cancel = pick(cb)); }

Animation& Animation::OnUpdate(const Event<double>& c)   { EnsureStaging_(); RET(staging_->on_update = c); }
Animation& Animation::OnUpdate(Event<double>&& c)        { EnsureStaging_(); RET(staging_->on_update = pick(c)); }

Animation& Animation::operator()(const Function<bool(double)>& f) { EnsureStaging_(); RET(staging_->tick = f); }
Animation& Animation::operator()(Function<bool(double)>&& f)      { EnsureStaging_(); RET(staging_->tick = pick(f)); }

#undef RET

/*---------------- Control methods ----------------*/

// _Unschedule(): common detach path used by ~Animation/Cancel/Reset/Replay.
// - Detaches from the live State and removes it from the scheduler safely.
// - If fire_cancel==true, invokes on_cancel on the current spec.
// - Always snapshots forward time progress into Progress() cache.
// - Keeps last_spec_ intact for future Replay().
void Animation::_Unschedule(bool fire_cancel)
{
    if (!live_)
        return;

    if (fire_cancel && live_->spec.on_cancel)
        live_->spec.on_cancel();

    const double p = Progress(); // forward-time snapshot for caching

    Animation::State* st = live_;
    live_ = nullptr;            // detach first (avoid re-entrancy surprises)
    if (st->anim) st->anim = nullptr;

    Scheduler::Inst().Remove(st); // deferred-safe removal via scheduler
    _OnStateRemovedCancel(p);     // Progress() cache ← snapshot
}


// EnsureStaging_(): lazily create a fresh staging config if missing.
// Needed after Play()/Cancel()/Stop() so setters always have a target.
void Animation::EnsureStaging_() {
    if (staging_)
        return;
    staging_box_.Create();
    staging_ = ~staging_box_;
    *staging_ = Staging(); // default config
}

// Reset(): silent abort + prime a fresh staging + Progress() ← 0.
// last_spec_ is intentionally *kept*, so Replay() still works after Reset().
void Animation::Reset()
{
    _Unschedule(false); // silent (no on_cancel)
    EnsureStaging_();   // user can immediately reconfigure
    progress_cache_ = 0.0;
}


// Play(): commit the *current staging* if present; otherwise reuse last_spec_.
// - If staging_ exists → commit it (preferred).
// - Else if we have a cached last_spec_ → rehydrate staging from it and play.
// - Else → no-op (we never run with accidental defaults).
void Animation::Play()
{
    // If there is no fresh staging, try to reuse the last committed spec.
    if (!staging_) {
        if (!have_last_spec_)
            return; // nothing to run yet
        staging_box_.Create();
        staging_ = ~staging_box_;
        *staging_ = *~last_spec_box_; // copy the cached spec back to staging
    }

    // Build a live State from the staged config.
    live_ = new State;
    live_->anim  = this;
    live_->owner = owner_;
    live_->spec  = pick(*staging_);   // consume staging (move/pick)
    staging_ = nullptr;               // staging consumed

    // Cache the just-committed spec so Replay() can re-run it later.
    last_spec_box_.Create();
    *~last_spec_box_ = live_->spec;
    have_last_spec_  = true;

    // Initialize runtime bookkeeping and schedule.
     progress_cache_ = 0.0;
    live_->start_ms = msecs();
    live_->cycles   = (live_->spec.loop_count < 0)
                    ? INT_MAX
                    : (live_->spec.yoyo ? (live_->spec.loop_count + 1) / 2
                                        :  live_->spec.loop_count);

    if (live_->spec.on_start) live_->spec.on_start();
    Scheduler::Inst().Add(live_);
}


// Replay(): (re)start using the last committed spec.
// Behavior:
//  - If there is fresh staging (user just set setters), prefer that by calling Play().
//    If a run is active, we silently interrupt it (no on_cancel) for smooth UX.
//  - Else, if we have a cached last_spec_, rehydrate staging from it and Play().
//  - Else, no-op (there has never been a committed run).
void Animation::Replay()
{
    if (staging_) {
        if (live_) _Unschedule(false); // silent interrupt
        Play();
        return;
    }

    if (!have_last_spec_)
        return; // nothing to replay yet

    if (live_) _Unschedule(false); // silent interrupt if currently running

    // Rehydrate staging from the cached spec, then Play().
    staging_box_.Create();
    staging_ = ~staging_box_;
    *staging_ = *~last_spec_box_;
    Play();
}



bool Animation::HasReplay() const
{
    return have_last_spec_;
}

// Pause(): reversible freeze; accumulates elapsed_ms and stops time advancement.
// Scheduler may stop ticking if everything is paused.
void Animation::Pause()
{
    if (live_ && !live_->paused) {
        live_->elapsed_ms += msecs() - live_->start_ms;
        live_->paused = true;
        Scheduler::Inst().MaybeStopIfAllPaused();
    }
}

// Resume(): continue after Pause(); re-arms scheduler if needed.
void Animation::Resume()
{
    if (live_ && live_->paused) {
        live_->start_ms = msecs();
        live_->paused = false;
        Scheduler::Inst().EnsureRunningIfAnyUnpaused();
    }
}

// Stop(): complete the animation immediately (Progress=1.0). Fires final tick
// and on_finish, then unschedules and frees state.
void Animation::Stop()
{
    if (!live_) return;

    // Deliver final callbacks at boundary values.
    if (live_->spec.tick)      live_->spec.tick(live_->reverse ? 0.0 : 1.0);
    if (live_->spec.on_finish) live_->spec.on_finish();

    Animation::State* st = live_;
    _OnStateRemovedFinish();        // Progress ← 1.0; live_ ← nullptr

    if (st->anim) st->anim = nullptr;
    Scheduler::Inst().Remove(st);
}

// Cancel(): abort the current run, fire on_cancel, keep last_spec_ for Replay().
// Also preserves a forward progress snapshot so Progress() stays meaningful.
void Animation::Cancel()
{
    _Unschedule(true); // fire on_cancel
}


// IsPlaying(): true if a live state exists and is not paused.
bool Animation::IsPlaying() const
{
    return live_ && !live_->paused;
}

// IsPaused(): true if a live state exists and is paused.
bool Animation::IsPaused() const
{
    return live_ && live_->paused;
}

// Progress(): normalized *time* progress in [0..1], independent of easing.
// Uses cached value when no run is live.
double Animation::Progress() const
{
    if (!live_) return progress_cache_;
    int64 run = live_->elapsed_ms + (live_->paused ? 0 : (msecs() - live_->start_ms));
    run = max<int64>(0, run - live_->spec.delay_ms);
    return clamp(double(run) / max(1, live_->spec.duration_ms), 0.0, 1.0);
}

/*---------------- Manual ticking (tests/diagnostics) ----------------*/

// Tick(): advance scheduler by n frames; optionally clamp each dt.
void Animation::Tick(int n, int max_ms_per_tick)
{
    if (n <= 0) return;
    Scheduler& s = Scheduler::Inst();
    for (int i = 0; i < n; ++i)
        s.TickManualOnce(max_ms_per_tick);
}

/*---------------- FPS control ----------------*/

// SetFPS(): change target FPS; re-arms the timer loop if running.
void Animation::SetFPS(int fps) {
    Scheduler::Inst().SetFPSInternal(fps);
}

// GetFPS(): read current target FPS.
int Animation::GetFPS() {
    return Scheduler::Inst().GetFPSInternal();
}

/*---------------- Global helpers ----------------*/

// KillAllFor(): abort all animations for the given Ctrl; Progress=0.0.
void Animation::KillAllFor(Ctrl& c)
{
    Scheduler::Inst().KillFor(&c);
}

// Finalize(): stop scheduler; free all states; sever back-pointers safely.
void Animation::Finalize()
{
    Scheduler::Inst().Finalize();
}

/*---------------- Scheduler → Animation hooks ----------------*/

// Finish path: cache 1.0 and clear live_.
void Animation::_OnStateRemovedFinish() {
    progress_cache_ = 1.0;
    live_ = nullptr;
}

// Cancel/kill path: cache provided forward progress snapshot and clear live_.
void Animation::_OnStateRemovedCancel(double p) {
    progress_cache_ = clamp(p, 0.0, 1.0);
    live_ = nullptr;
}
