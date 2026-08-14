# ACTIVE WORK

CURRENT REMOTE HEAD: `1a69143bb317e7325b1160c9f747b1ffa3f38f10` — Gallery corrective/R2D validation continuation checkpoint. Parent `f69fd6c1fb3259db9dd9a72e1e750ff3c0edd954` is the exact Windows stop-report HEAD; `1a69143...` is the published three-line Dropdown compile fix.

CURRENT BASELINE MERGE: `a4f81014617dd758893e7cfc105a8a1f4ff24130` — merge of Gary's R2C demo-fix commit `1743fcbe4aee8d257015194910a9a6efc47c2726` with the published R2D checkpoint `ce825f8cb507fbce1b7139d9601c8c2eaf5c8f9d`.

CURRENT TASK: `UI-GALLERY-CORRECTIVE + R2D-WINDOWS-VALIDATION`.

Remote GitHub is authoritative. Never force-update `main`.

## R2C TREE + TABLE — WINDOWS ACCEPTED

Gary validated published R2C source at `a0b07fad20c034dd5096335af591a971f9857f0a` on Windows/U++ CLANGx64.

Acceptance evidence:
- Ui package Debug source compile: **PASS**, 82 files, 0 compile errors, 0 warnings. Final undefined `WinMain` link message is the expected attempt to link the Ui library package as an executable, not a source failure.
- `Utilities/UiModelViewPerformanceTest` Debug: **Checks: 52, Fails: 0**, exit 0.
- `Utilities/UiModelViewPerformanceTest` Release: **Checks: 52, Fails: 0**, exit 0.
- `Utilities/UiTreeScaleTest` Debug: **Checks: 11, Fails: 0**, exit 0.
- `Utilities/UiTreeScaleTest` Release: **Checks: 11, Fails: 0**, exit 0.
- `UiTreeDemo` smoke: PASS — expand/collapse, arrow/Home/End navigation, multi-select paths, F2 rename/cancel, DnD path; no freeze/crash and no stale lazy `Loading...` row.
- `UiTableDemo` smoke: PASS — vertical scrolling, active-cell/range selection, column resize and header-sort behavior; headers/cells remain stable; no freeze/crash.
- Table demo has only about 560 px of four-column content, so it cannot meaningfully exercise deep horizontal overflow. This is a demo limitation, not a Table scale failure; deterministic R2C coverage exercises 2,000 columns.
- automated synthetic Enter did not trigger the Table editor in the smoke driver, but the normal `LeftDouble`/Enter -> `BeginEdit` -> `CommitEdit`/`CancelEdit` implementation is present and no runtime defect was demonstrated. Do not invent a Table source fix from this automation limitation.
- `UiTableRunTests`-style GUI runners wait for a window and were correctly not used as console pass/fail gates.
- `git diff --check`: PASS.

R2C status: **WINDOWS ACCEPTED.**

The previously stale 50-check documentation is corrected: the authoritative combined R2C `UiModelViewPerformanceTest` total is **52** because the final Table integration contains two additional renderer checks.

### R2C demo include reconciliation

Gary found `UiTreeDemo` used shared `DemoToggleRow` / `DemoSliderRow` / `DemoColorRow` types without including the shared builder-demo header. His exact mechanical fix:

```cpp
#include <Ui/Ui.h>
#include "../BuilderDemoSupport.h"
```

was committed as `1743fcbe4aee8d257015194910a9a6efc47c2726` and is now already in authoritative `main` through merge `a4f81014617dd758893e7cfc105a8a1f4ff24130`. Do not reapply it.

`examples/UiListDemo/main.cpp` has the same pre-existing missing include and remains a tiny demo-hygiene follow-up. The available GitHub write path replaces the entire large file, so this session deliberately did not risk a blind whole-file rewrite for one include. Safe mechanical fix when that demo is next touched:

```cpp
#include <Ui/Ui.h>
#include "../BuilderDemoSupport.h"
```

No library/runtime architecture depends on this outstanding demo include.

## R2D DROPDOWN + MENU — IMPLEMENTATION COMPLETE, PLATFORM VALIDATION PENDING

R2D substantive checkpoint: `ce825f8cb507fbce1b7139d9601c8c2eaf5c8f9d`.

Implemented state:
- Dropdown uses `UiListModel` as the sole row authority; no parallel `items_` mirror/conversion/sync path remains.
- `UiDropdown::Item` is only a direct type alias to `UiModelItem`, not a second item class or state store.
- Dropdown collapsed/popup item presentation uses bounded prepared `UiItemRender` instances; popup Paint does not prepare renderer geometry.
- Menu keeps authoritative `UiMenuModel` semantics and uses shared `UiItemRender` only for ordinary popup icon/title/description/right-content presentation.
- Menu retains check/radio, submenu, command, separator, popup stack/session and menu-bar semantics.
- `Utilities/UiDropdownMenuRenderTest`: expected Windows Debug/Release **Checks: 11, Fails: 0**; still pending platform execution.
- public implementation note: `docs/08_UI_MODEL_RENDERING_R2D.md`.

### R2D Windows stop report and compile fix

Gary started the combined Gallery corrective + R2D validation at exact published HEAD `f69fd6c1fb3259db9dd9a72e1e750ff3c0edd954`, with a clean working tree and no local work at risk.

The Ui package stopped with exactly three compile errors in `Ui/UiDropdownPopup.cpp`:

```text
UiDropdownPopup.cpp:336:11: error: must use 'struct' tag to refer to type 'Style' in this scope
UiDropdownPopup.cpp:383:11: error: must use 'struct' tag to refer to type 'Style' in this scope
UiDropdownPopup.cpp:478:11: error: must use 'struct' tag to refer to type 'Style' in this scope
```

Root cause was confirmed rather than guessed: `UiDropdown::PopupWindow` derives from `TopWindow`, whose non-type member `TopWindow& Style(dword)` hides the unqualified nested `UiDropdown::Style` type name inside `PopupWindow` member-function scope on Windows/Clang.

Published fix: `1a69143bb317e7325b1160c9f747b1ffa3f38f10` — **`UiDropdownPopup qualifies nested style in TopWindow scope`**.

The commit changes exactly three lines and nothing else:

```cpp
const UiDropdown::Style& style = owner->GetEffectiveStyle();
```

in:
- `UiDropdown::PopupWindow::SyncScrollBarState()`
- `UiDropdown::PopupWindow::GetItemContentRect()`
- `UiDropdown::PopupWindow::Paint()`

The ordinary unqualified `const Style&` inside `UiDropdown` members remains unchanged because it is not in `TopWindow` scope. GitHub commit review confirms the published diff is exactly the three intended qualifications.

Gary correctly stopped before running any blocked tests/demos. At the stop-report HEAD:
- `git diff --check`: PASS.
- `git status --short`: clean.
- no local commits/source edits/pushes.

R2D validation status: **COMPILE BLOCKER FIX PUBLISHED — WINDOWS REVALIDATION REQUIRED.**

## GALLERY CORRECTIVE — PUBLISHED SOURCE

The Gallery defects found during R2A/R2B Windows acceptance have now been corrected in source on top of merged `main`.

Published corrective checkpoints:
- `e253f0630145d13bdd226f303633637e9e078dd8` — explicit Gallery interaction-style/capture/zoom contract (`selection_frame`, stronger marquee frame, `WhenZoom`, marquee capture ownership).
- `d125dbba3ecdaea7f22ce7a1d4adb1a7c5930b7c` — theme-driven Gallery viewport no longer reuses List row/nine-slice skin; Dark mode derives viewport face plus interaction accent from current theme; semantic zoom emits `WhenZoom` only on actual change.
- `66c566c8e6925e3a2c95b9364eccbfb2413d0ad0` — marquee fill moved behind tile content; marquee frame drawn on top; selected tiles receive an explicit Gallery-owned interaction frame.
- `1d6e0656361469d268417c2ab8ee0434b3720ecf` — Win32 capture-recursion root fix: Gallery tracks owned marquee capture, clears ownership before release, and `CancelMode()` never calls `ReleaseCapture()`.
- `46efb6cc7c0b84f48427cd452a711dc0b98f036b` — Gallery demo status listens to `WhenZoom`, so Ctrl+wheel and button zoom keep the displayed percentage live.
- `dc0ccc1b4b6c4d3fc00132d1b0ae4da2a6512977` + `a90be27de3eedabee3e2c19a703392a0e018b818` + `dab52ffa3d4e1d751d9065488f5a6b0367c42010` + `1c6f0729f2dd2ca74b18d19f7a584d66586f252a` — focused `Utilities/UiGalleryRegressionTest` package and 11-check deterministic corrective suite.
- `da2ba754a4f6c8d3a64f6fe37e466e4fe1b20471` — public scale guide updated with the accepted R2C totals and Gallery capture/theme/presentation invariants.

### Gallery capture rule

The former failure was:

`CancelMode -> ReleaseCapture -> CancelMode -> ...`

on Win32 capture teardown. Current rule:
- only an active Gallery marquee may own capture;
- ownership is cleared before a normal marquee release;
- a re-entrant `CancelMode()` ends/restores marquee state without releasing capture;
- `CancelMode()` itself never calls `ReleaseCapture()`.

This removes the recursive path instead of adding a depth guard or swallowing the crash.

### Gallery presentation rule

- Gallery owns selection/marquee interaction chrome; the renderer owns ordinary item content.
- marquee fill is behind tiles, so it remains visible in gaps without washing over images/text;
- selection and marquee frames are explicit 2px theme-derived interaction strokes;
- theme-driven Gallery viewport uses current List palette/metrics but a neutral Gallery surface skin, preventing a stale light row raster from surviving a Dark-mode switch;
- `WhenZoom(double)` is the semantic zoom notification; no-op zoom changes do not emit it;
- existing visible/overscan renderer-pool scale behavior remains intact and Paint still performs no renderer layout.

`Utilities/UiGalleryRegressionTest` contains **11 checks** covering bounded pooling, explicit selection/marquee frames, zoom notification/no-op behavior, Dark viewport styling, Paint-without-relayout and passive non-marquee `CancelMode()` behavior.

Gallery corrective status: **IMPLEMENTATION COMPLETE — PLATFORM VALIDATION PENDING.**

## CURRENT DETERMINISTIC WINDOWS STATUS / TARGETS

Already accepted:
- `UiModelViewPerformanceTest`: Debug **52/0**, Release **52/0**.
- `UiTreeScaleTest`: Debug **11/0**, Release **11/0**.

Pending Windows execution after `1a69143...`:
- Ui Debug source compile must clear the previous three Dropdown errors.
- `UiGalleryRegressionTest`: expected Debug/Release **Checks: 11, Fails: 0**.
- `UiDropdownMenuRenderTest`: expected Debug/Release **Checks: 11, Fails: 0**.

## CURRENT TOUCHED CORRECTIVE SLICE

Gallery: `Ui/UiGallery.h`, `Ui/UiGallery.cpp`, `Ui/UiGalleryPaint.cpp`, `Ui/UiGalleryInteraction.cpp`, `examples/UiGalleryDemo/main.cpp`, `Utilities/UiGalleryRegressionTest/*`, `docs/06_UI_MODEL_VIEW_SCALE_GUIDE.md`.

R2D Windows compile fix: `Ui/UiDropdownPopup.cpp` only, exactly three type qualifications.

Recovery state: `docs/ACTIVE_WORK.md`.

## NEXT ACTION

Resume Windows validation at exact final remote `main` (currently `1a69143bb317e7325b1160c9f747b1ffa3f38f10` before this ACTIVE_WORK checkpoint):
1. Refresh/fetch and record the exact new final HEAD after this documentation commit.
2. Re-run Ui Debug source compile first. The three `UiDropdownPopup.cpp` `Style` errors must be gone. Stop on the first new substantive compile/runtime failure.
3. Run `UiGalleryRegressionTest` Debug and Release: 11/0 each.
4. Build/run `UiGalleryDemo` Release: marquee drag/release/Escape/capture-loss must never stack-overflow; selected tiles and marquee frame must remain clearly visible; marquee fill must not wash over tile content; Dark mode must give a genuinely dark Gallery surface; Ctrl+wheel zoom must update the displayed zoom percentage and retain a sensible pointer anchor.
5. Run `UiDropdownMenuRenderTest` Debug and Release: 11/0 each.
6. Build/run `UiDropdownDemo` and `UiMenuDemo` Release for popup scrolling/selection/multi-check/drag reorder and Menu check/radio/submenu/keyboard/theme behavior.
7. `git diff --check` and final clean status.
8. Apply the one-line `UiListDemo` `BuilderDemoSupport.h` include only when that demo is next deliberately built/touched; it is not a blocker for this resumed validation.
