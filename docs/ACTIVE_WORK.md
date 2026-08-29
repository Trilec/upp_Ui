# ACTIVE WORK

This file is intentionally short. It is a crash/recovery checkpoint, not project history.
Remote GitHub is authoritative. Refresh `main` before work/publication, preserve unrelated concurrent advances, and never force-update `main`.
Use Git history and the dedicated docs below when older implementation detail is needed.

## CURRENT REPOSITORY STATE

Authoritative branch: `main`

Last compacted from remote HEAD:

`2ff28da2b147810eef4beab17316e5bc9c1eefac`
`PROPERTYEDITOR: preserve double working-range values`

That SHA is only the compaction checkpoint. Always fetch current `main` again before acting.

## RECENT LIVE AREAS

### 1. Timer ownership hardening

Published baseline includes:

`1dc620cb9741aa52b5bb9301c44c89cabc05166a`
`UI-TIMER: harden control timer ownership and test scheduling`

Affected areas:
- `UiFrameTicker`
- `UiProgressBar`
- `UiProgressRing`
- `UiAccordion`
- `UiMenu`
- `UiTableRunTests`
- `UiTreeRunTests`

`upp_animation` was audited and intentionally not redesigned.

DO NOT REGRESS:
- `Ctrl::SetTimeCallback(..., int id)` ids are byte offsets, not arbitrary magic IDs.
- Normal Ui code should use owned `TimeCallback`, `UiFrameTicker`, or the shared `Animation` system as appropriate.
- Do not reintroduce large numeric/FourCC-style Ctrl callback ids.

Detailed rationale/validation: `docs/19_UI_TIMER_OWNERSHIP_HARDENING.md`

### 2. UiProgressRing

Current published cap contract includes:

`9d6d8240f197f9d3227bd564429f1c7290db0c73`
`UIPROGRESSRING: make cap roundness thickness-relative`

Current contract:
- continuous Painter arc; no segmented progress geometry;
- optional angular gradient uses the same continuous arc geometry;
- independent unused-track colour;
- `SetCapRoundness(0..100)` / `GetCapRoundness()`;
- default cap roundness = `100`;
- `0` = flat; `100` = true semicircle derived from current stroke thickness;
- intermediate values keep a central flat face while proportional rounded corners grow;
- old independent pixel `cap_radius` API is retired.

Focused test:
- `Utilities/UiProgressRingRunTests`
- expected current summary: `UIPROGRESSRING_SUMMARY checks=51 failed=0`

Visual demo:
- `examples/UiProgressRingDemo`

Detailed contract: `docs/18_UIPROGRESSRING.md`

### 3. UiGraph R9/R9.1 + PropertyEditor working ranges

Recent published work includes Graph regression recovery plus PropertyEditor double working-range support.
Latest repository HEAD at compaction is the PropertyEditor preservation fix listed above.

DO NOT REGRESS:
- Graph Inspector X/Y must not return to million-unit scrub sliders.
- Numeric entry remains authoritative.
- working-range slider bounds follow the current Graph viewport/overscan policy;
- wheel editing uses normal world-unit stepping;
- application-authored Graph model coordinates remain unrestricted by the demo Inspector policy;
- repeated Reference -> 10k -> Reference switching must remain responsive;
- do not hide generic Graph renderer issues inside demo-only coordinate policy.

Relevant validation packages:
- `Utilities/PropertyEditorWorkingRangeTest`
- `Utilities/UiNodeGraphModelSwitchProfileTest`
- `Utilities/UiNodeGraphPanProfileTest`
- `Utilities/UiNodeGraphScaleTest`
- `examples/UiGraphDemo`

Detailed current Graph contract: `docs/21_UIGRAPH_EXTREME_COORDINATE_HARDENING.md`

### 4. Recent theme fallback correction

Published main also includes the dark-theme fallback correction for PropertyEditor/Table:

`45caf4b7e32d451dd8bb3139a582f8201ba09d6a`
`Fix dark theme fallback for PropertyEditor and Table`

Do not restore hard white/default table surfaces when semantic dark-theme surfaces are available.

## RECOVERY / VALIDATION STATUS

Do not infer Windows acceptance merely from a published commit. If a later validation report exists, use that evidence; otherwise treat the recent timer/ring/Graph/PropertyEditor changes above as requiring the normal Windows/U++ verification before declaring them closed.

Minimum recovery validation after a crash or uncertain checkout:

1. Fetch current remote `main`; record exact HEAD.
2. Confirm the worktree contains no unintended local changes.
3. Check the configured `.var` / assembly and use the repository's normal U++ toolchain.
4. Run `git diff --check`.
5. Build the `Ui` package in the required Debug/Release configurations.
6. Run only the focused packages relevant to the active defect/change rather than replaying old historical tranches.

For the most recent timer/ring work, the focused Windows set is:
- `Utilities/UiProgressBarRunTests`
- `Utilities/UiProgressRingRunTests`
- `Utilities/UiTableRunTests`
- `Utilities/UiTreeRunTests`
- `examples/UiProgressBarDemo`
- `examples/UiProgressRingDemo`
- `examples/UiAccordionDemo`
- `examples/UiMenuDemo`
- `upp_animation` `ConsoleAnim` regression suite

For current Graph/PropertyEditor work, use the packages listed in section 3 and retain timing/profile output as evidence rather than imposing machine-dependent speed thresholds.

## CONTINUATION RULE

When resuming:
- recover from current remote `main`, not this SHA or chat memory;
- inspect the complete touched dependency slice before editing;
- diagnose root cause first;
- make the smallest coherent change;
- review the full diff and package membership;
- publish in recoverable checkpoints and verify remote HEAD afterwards.

Older completed work is intentionally omitted from this file. Git history and the dedicated docs are the source for it.
