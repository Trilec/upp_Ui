# UI Timer Ownership Hardening — 2026-08-29

## Status

**IMPLEMENTATION COMPLETE — WINDOWS/U++ VALIDATION PENDING.**

Remote GitHub is authoritative. This checkpoint records the timer/animation ownership correction discovered while investigating Debug failures caused by raw `Ctrl` timer identifiers.

## Base and recovery branch

- Base `main`: `5de948d419e3802065de5e713c4904d03bcec9ba`
- Recovery branch: `agent/ui-timer-ownership-hardening`
- Reviewed source checkpoint before this bookkeeping commit: `1b9c613111dd7dcd5b2a943a2e34075b74bea4ac`

Publication must refresh `main` immediately before the non-force fast-forward and report the actual published SHA.

## Root cause

`Ctrl::SetTimeCallback(..., id)` / `Ctrl::KillTimeCallback(id)` timer identifiers are not arbitrary application handles. The affected code used invented integer identifiers, including large values such as `0x70524F47`, `8231`, `8201`, and `9000 + node.id`. These can violate the Ctrl timer-id contract and assert in Debug.

The corrected ownership rules are:

- use an owned `TimeCallback` for delayed/one-shot work;
- use `KillSet()` when replacing an outstanding callback and `Kill()` for cancellation/teardown;
- use `UiFrameTicker` for custom repeating Ui frame clocks that need one outstanding one-shot callback at a time;
- keep using the shared `Animation` package where its interpolation/lifecycle model already fits.

## Production changes

- Added `Ui/UiFrameTicker.h` and registered it in `Ui/Ui.upp`.
- `UiProgressBar`: indeterminate animation now uses owned `UiFrameTicker` scheduling.
- `UiProgressRing`: intro and indeterminate animation now use owned `UiFrameTicker` scheduling.
- `UiAccordion`: all concurrently animating sections are driven by one owned `UiFrameTicker`; duplicate raw callback scheduling is removed.
- `UiMenu`: deferred session verification now uses an owned `TimeCallback`.
- `docs/00_UPP_CODING_GUIDE.md`: records the timer-ownership rule so arbitrary Ctrl timer IDs are not reintroduced.

`UiFrameTicker` owns one `TimeCallback`, rearms only after the current frame callback returns, uses a generation counter to invalidate stale runs, and uses `Ptr<UiFrameTicker>` guards so teardown from or before a frame callback cannot rearm dead state.

## Test-harness corrections

- `Utilities/UiTableRunTests/main.cpp`: replaced `RUN_CB_ID = 8231` with owned `TimeCallback run_tc_`.
- `Utilities/UiTreeRunTests/main.cpp`: replaced `RUN_CB_ID = 8201` with owned `TimeCallback run_tc_`.
- The UiTree audit also found `SetTimeCallback(40, ..., 9000 + node.id)` in lazy loading. It now uses owned `TimeCallback lazy_load_tc_` and drains the pending lazy-load set safely.

## Animation audit

The shared `upp_animation` implementation was inspected and intentionally left unchanged. Its scheduler already owns a `TimeCallback`; its internal timer generation is lifecycle state rather than a Ctrl timer ID. Existing consumers such as `UiButton`, `UiToggle`, `UiMaskEdit`, and `UiScrollBar` therefore do not need conversion merely because this Ctrl timer-id bug was found.

`UiScrollBar` also already demonstrates the intended split: shared `Animation` instances for interpolation plus an owned `TimeCallback` for delayed collapse.

No timer scheduler was found in `UiDraw` that required a production change.

## Touched paths

- `Ui/Ui.upp`
- `Ui/UiFrameTicker.h`
- `Ui/UiProgressBar.h`
- `Ui/UiProgressBar.cpp`
- `Ui/UiProgressRing.h`
- `Ui/UiProgressRing.cpp`
- `Ui/UiAccordion.h`
- `Ui/UiAccordion.cpp`
- `Ui/UiMenu.h`
- `Ui/UiMenu.cpp`
- `Utilities/UiTableRunTests/main.cpp`
- `Utilities/UiTreeRunTests/main.cpp`
- `docs/00_UPP_CODING_GUIDE.md`
- `docs/19_UI_TIMER_OWNERSHIP_HARDENING.md`

## Validation required from Gary

Use the final published `main`, report its exact SHA first, and do not validate an older checkout.

1. Run `git diff --check` on the published tree.
2. Build the `Ui` package under CLANGx64 Debug and Release.
3. Build/run `Utilities/UiProgressBarRunTests` Debug and Release; build `examples/UiProgressBarDemo` in both configurations and exercise indeterminate start/stop plus teardown.
4. Build/run `Utilities/UiProgressRingRunTests` Debug and Release; build `examples/UiProgressRingDemo` in both configurations and exercise intro, indeterminate mode switching, hide/show, and teardown.
5. Build/run `Utilities/UiTableRunTests` Debug and Release through completion.
6. Build/run `Utilities/UiTreeRunTests` Debug and Release through completion, including the lazy-load phase.
7. Build `examples/UiAccordionDemo` Debug and Release; exercise animated open/close, rapid toggling, multiple sections, single-open policy, and destruction while idle/after animation.
8. Build `examples/UiMenuDemo` Debug and Release; exercise popup/menu-bar open-close, focus changes, submenu switching, and deferred session verification.
9. Run the existing `upp_animation` `ConsoleAnim` suite Debug and Release as a regression check. The Animation repository itself is unchanged by this task.

Any tiny obvious U++ compile/API correction may be reported with its exact diff. Any lifecycle, scheduling, behavior, or architecture change returns to the supervisor rather than being worked around in a caller.
