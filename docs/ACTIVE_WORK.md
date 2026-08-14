# ACTIVE WORK

TASK: `UI-GALLERY-CORRECTIVE + R2D-WINDOWS-VALIDATION`.

Remote GitHub is authoritative. Fetch live `main` before further work and never force-update it.

BASELINE MERGE: `a4f81014617dd758893e7cfc105a8a1f4ff24130` — merged R2C demo fix plus R2D implementation checkpoint.

## ACCEPTED BASELINE

R2C Tree + Table is Windows accepted:
- Ui Debug source compile: PASS.
- `UiModelViewPerformanceTest`: Debug **52/0**, Release **52/0**.
- `UiTreeScaleTest`: Debug **11/0**, Release **11/0**.
- Tree and Table focused smoke: PASS; no freeze/crash.

## GALLERY CORRECTIVE — WINDOWS ACCEPTED

Gary validated exact source/recovery HEAD `e2a98d4be4d998e70f27bea5d01ec6bbade415e4` on Windows/U++ CLANGx64:
- Ui Debug: **0 compile errors, 0 warnings**, 83 files.
- `UiGalleryRegressionTest` Debug: **11/0**.
- `UiGalleryRegressionTest` Release: **11/0**.
- the former Dark viewport invariant now passes.
- Light -> Dark -> Light changed header, Gallery surface and margins to genuinely dark values and back without stale light surfaces.
- repeated marquee drag/release, Escape, edge autoscroll and long drags: no capture recursion, stack overflow or crash.
- Dark selection/marquee frames remain visible and content stays legible.
- +/- and Ctrl+wheel zoom both update the status, remain sensibly anchored, and compact vertical image-tile text steps down in bounded discrete sizes; 100% returns to normal text size.
- First/Last, deep scrolling and 10,000-item interaction remained responsive/stable.

Relevant published corrective checkpoints remain:
- `250e62cb...` — transparent List-derived Gallery face resolves through theme Panel Surface role.
- `393113ad...` — demo root/header remain theme-live.
- `3be0fdef...` + `fef9cd6f...` — cached layout-resolved image-render fonts and bounded vertical tile font ladder.

GALLERY STATUS: **WINDOWS ACCEPTED.**

## R2D DROPDOWN + MENU

Substantive architecture checkpoint: `ce825f8cb507fbce1b7139d9601c8c2eaf5c8f9d`.

Architecture:
- Dropdown uses one authoritative `UiListModel`; no parallel item mirror/sync state.
- collapsed/popup Dropdown content uses bounded prepared `UiItemRender` instances.
- Menu keeps authoritative `UiMenuModel` semantics and uses shared rendering for ordinary popup content while retaining check/radio/submenu/command/session/menu-bar behavior.
- `UiDropdownMenuRenderTest` target remains Debug/Release **11/0** each.

Windows already confirmed the earlier three `UiDropdownPopup.cpp` type-name errors were fixed by `1a69143bb317e7325b1160c9f747b1ffa3f38f10`; Ui Debug compiled cleanly at `e2a98d4...`.

### R2D renderer-test stop report

At exact HEAD `e2a98d4be4d998e70f27bea5d01ec6bbade415e4`:
- `UiDropdownMenuRenderTest` Debug: **11 checks, 1 fail**.
- Release: **11 checks, 1 fail**.
- sole failing assertion: `Dropdown renderer prototype can be replaced without changing model state`.
- the other ten R2D checks passed.
- Dropdown/Menu demo smoke was correctly deferred.

Review showed no production Dropdown/model-state defect. The original test incorrectly combined its semantic invariant with this scheduling assertion:

`GetLastRenderLayoutCount() == 1` after `SetItemRender(image); drop.Layout();`

`SetItemRender()` calls `RefreshLayout()`. A live U++ control may service that requested renderer layout before the following explicit `Layout()` call. That later no-op call can therefore correctly report zero new layouts. Forcing production code to lay out twice would weaken the renderer reuse contract merely to satisfy test scheduling.

Published correction:
- `ffc0ddb4402dcd7331ca2be3a60c631ea946d9de` — removes the caller-visible layout-turn assumption and verifies the durable invariants instead: exactly one collapsed renderer, custom renderer style survives cloning, selected row/text remain unchanged, and authoritative model content remains unchanged.
- `dc355d98a612cac878c6da7d38b910a302dc1eaf` — records the layout/scheduling rule in `08_UI_MODEL_RENDERING_R2D.md`.

Production `UiDropdown` code was intentionally not changed.

R2D STATUS: **TEST FALSE-NEGATIVE CORRECTED — WINDOWS REVALIDATION REQUIRED.**

## LOCAL HYGIENE NOTE

Gary reported one pre-existing local-only working-tree change:
`examples/UiGalleryDemo/UiGalleryDemo.upp` had a blank line at EOF, causing `git diff --check` to fail.

This change was not published and is unrelated to the source correction. Before final hygiene validation, revert that local file to remote `main` unless it contains intentional local work.

Gary may continue to repair tiny obvious mechanical build issues locally and record the commit. Stop only for substantive rendering, model ownership, interaction, state, lifecycle, performance or architecture defects.

## NEXT ACTION

1. Fetch/pull live `main`; record exact HEAD.
2. Remove/revert the stray local-only `UiGalleryDemo.upp` blank-line change so final hygiene is meaningful.
3. Rebuild only if needed after pulling; previous Ui Debug source build at the Gallery gate was clean.
4. Re-run `UiDropdownMenuRenderTest` Debug and Release: expected **11/0** each.
5. If that gate passes, run `UiDropdownDemo` Release smoke: popup open/close, scrolling, selection, multi-check, reorder where enabled, Light/Dark; no freeze/crash.
6. Run `UiMenuDemo` Release smoke: popup/submenus, check/radio, keyboard navigation, activation, Light/Dark; no freeze/crash.
7. Finish with `git diff --check` and clean `git status --short`.

Do not rerun the already accepted Gallery runtime stress unless a new shared-renderer change unexpectedly touches Gallery behavior.
