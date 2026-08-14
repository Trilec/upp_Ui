# ACTIVE WORK

CURRENT CODE CHECKPOINT: `1a69143bb317e7325b1160c9f747b1ffa3f38f10` — published three-line Windows compile fix on top of Gallery corrective/R2D validation state.

WINDOWS STOP-REPORT BASE: `f69fd6c1fb3259db9dd9a72e1e750ff3c0edd954` — exact HEAD Gary tested before the Dropdown compile blocker was fixed.

BASELINE MERGE: `a4f81014617dd758893e7cfc105a8a1f4ff24130` — merge of Gary's R2C demo-fix commit `1743fcbe4aee8d257015194910a9a6efc47c2726` with R2D checkpoint `ce825f8cb507fbce1b7139d9601c8c2eaf5c8f9d`.

TASK: `UI-GALLERY-CORRECTIVE + R2D-WINDOWS-VALIDATION`.

Remote GitHub is authoritative. Fetch live `main` before further work and never force-update it.

## R2C TREE + TABLE — WINDOWS ACCEPTED

Gary validated R2C on Windows/U++ CLANGx64.

- Ui Debug source compile: PASS, 82 files, 0 compile errors, 0 warnings.
- `UiModelViewPerformanceTest` Debug: **52/0**, Release: **52/0**.
- `UiTreeScaleTest` Debug: **11/0**, Release: **11/0**.
- `UiTreeDemo`: PASS for expand/collapse, navigation, multi-select paths, F2 rename/cancel and DnD; no stale lazy `Loading...` row, freeze or crash.
- `UiTableDemo`: PASS for scrolling, selection, resize and header-sort behavior; no freeze/crash.
- Four-column Table demo cannot meaningfully exercise deep horizontal overflow; deterministic R2C coverage does so with 2,000 columns.
- synthetic Enter did not trigger the Table editor in the automation driver, but no Table source defect was demonstrated.
- `git diff --check`: PASS.

R2C STATUS: **WINDOWS ACCEPTED.**

Gary's mechanical `UiTreeDemo` shared-support include fix `1743fcbe...` is already merged. `UiListDemo` still has the same pre-existing missing `../BuilderDemoSupport.h` include; apply it only when that demo is deliberately touched/built. It is not a library blocker.

## R2D DROPDOWN + MENU

Substantive R2D checkpoint: `ce825f8cb507fbce1b7139d9601c8c2eaf5c8f9d`.

Implemented architecture:
- Dropdown uses `UiListModel` as sole row authority; no parallel item mirror/conversion/sync state remains.
- `UiDropdown::Item` is only a type alias to `UiModelItem`.
- collapsed/popup Dropdown presentation uses bounded prepared `UiItemRender` instances; popup Paint does not prepare renderer geometry.
- Menu keeps authoritative `UiMenuModel` semantics and uses shared `UiItemRender` for ordinary popup content while retaining check/radio/submenu/command/session/menu-bar semantics.
- `UiDropdownMenuRenderTest` target: Debug/Release **11/0** each.
- implementation note: `docs/08_UI_MODEL_RENDERING_R2D.md`.

### Windows compile stop and published fix

At `f69fd6c1...`, Ui compilation stopped with exactly three errors in `Ui/UiDropdownPopup.cpp` because `UiDropdown::PopupWindow : TopWindow` inherited non-type `TopWindow::Style(dword)`, hiding the unqualified nested `UiDropdown::Style` type inside `PopupWindow` member scope.

Published fix: `1a69143bb317e7325b1160c9f747b1ffa3f38f10` — `UiDropdownPopup qualifies nested style in TopWindow scope`.

The diff is exactly three replacements:

```cpp
const UiDropdown::Style& style = owner->GetEffectiveStyle();
```

in:
- `PopupWindow::SyncScrollBarState()`
- `PopupWindow::GetItemContentRect()`
- `PopupWindow::Paint()`

No other source line changed in that commit. The ordinary unqualified `Style` use inside `UiDropdown` member scope remains untouched because it is not affected by `TopWindow` name hiding.

R2D STATUS: **IMPLEMENTATION COMPLETE — WINDOWS VALIDATION RESUMPTION REQUIRED.**

## GALLERY CORRECTIVE

Published corrective source includes:
- `e253f063...` — explicit selection/marquee/capture/zoom contract.
- `d125dbba...` — theme-driven Gallery viewport no longer reuses List row/nine-slice skin; Dark mode surface/accent resolve from current theme; semantic `WhenZoom`.
- `66c566c8...` — marquee fill behind tile content, frame above it, explicit selected-tile frame.
- `1d6e0656...` — Win32 capture-recursion root fix: Gallery owns marquee capture explicitly, clears ownership before release, and `CancelMode()` never calls `ReleaseCapture()`.
- `46efb6cc...` — demo listens to `WhenZoom`, so Ctrl+wheel and button zoom keep status live.
- `Utilities/UiGalleryRegressionTest` — **11 deterministic checks**.
- `docs/06_UI_MODEL_VIEW_SCALE_GUIDE.md` — accepted R2C totals and Gallery capture/theme/presentation invariants.

Gallery interaction rules:
- renderer owns ordinary item content; Gallery owns selection/marquee chrome;
- marquee fill is behind tiles so images/text are not washed out;
- marquee and selected-tile frames are explicit 2px theme-derived interaction strokes;
- theme-driven Gallery viewport uses current theme palette/metrics without retaining a List row raster skin;
- `WhenZoom(double)` fires only for an actual resolved zoom change;
- visible/overscan renderer pooling and Paint-without-layout invariants remain unchanged.

GALLERY STATUS: **IMPLEMENTATION COMPLETE — PLATFORM VALIDATION PENDING.**

## WINDOWS STATUS / TARGETS

Already accepted:
- `UiModelViewPerformanceTest`: Debug **52/0**, Release **52/0**.
- `UiTreeScaleTest`: Debug **11/0**, Release **11/0**.

Pending after code checkpoint `1a69143...`:
- Ui Debug source compile must clear the previous three `UiDropdownPopup.cpp` errors.
- `UiGalleryRegressionTest`: Debug/Release **11/0** each.
- `UiDropdownMenuRenderTest`: Debug/Release **11/0** each.
- Gallery, Dropdown and Menu runtime smoke.

## NEXT ACTION

1. Fetch live `main` and record the exact final recovery/documentation HEAD.
2. Re-run Ui Debug source compile. Stop on the first new substantive source/runtime failure.
3. Run `UiGalleryRegressionTest` Debug and Release: 11/0 each.
4. Run `UiGalleryDemo` Release and verify repeated marquee drag/release/Escape/capture-loss never stack-overflows; selected tiles and marquee are clear; Dark is genuinely dark; marquee fill does not wash out content; Ctrl+wheel zoom updates the displayed percentage and remains sensibly pointer-anchored.
5. Run `UiDropdownMenuRenderTest` Debug and Release: 11/0 each.
6. Run `UiDropdownDemo` and `UiMenuDemo` Release for popup scroll/selection/multi-check/drag reorder and Menu check/radio/submenu/keyboard/theme behavior.
7. Finish with `git diff --check` and clean `git status`.
