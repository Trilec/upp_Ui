# ACTIVE WORK

TASK: `UI-GALLERY-CORRECTIVE + R2D-WINDOWS-VALIDATION`.

Remote GitHub is authoritative. Fetch live `main` before further work and never force-update it.

BASELINE MERGE: `a4f81014617dd758893e7cfc105a8a1f4ff24130` — merged R2C demo fix plus R2D implementation checkpoint.

## ACCEPTED BASELINE

R2C Tree + Table is Windows accepted:
- Ui Debug source compile: PASS, 82 files, 0 compile errors, 0 warnings.
- `UiModelViewPerformanceTest`: Debug **52/0**, Release **52/0**.
- `UiTreeScaleTest`: Debug **11/0**, Release **11/0**.
- Tree and Table focused smoke: PASS; no freeze/crash.

Gary's `UiTreeDemo` shared-support include fix `1743fcbe...` is merged. `UiListDemo` still has the same tiny pre-existing `../BuilderDemoSupport.h` include gap; Gary may fix it mechanically when that demo is deliberately built.

## R2D DROPDOWN + MENU

Substantive R2D checkpoint: `ce825f8cb507fbce1b7139d9601c8c2eaf5c8f9d`.

Architecture remains:
- Dropdown uses one authoritative `UiListModel`; no parallel item mirror/sync state.
- collapsed/popup Dropdown content uses bounded prepared `UiItemRender` instances.
- Menu keeps authoritative `UiMenuModel` semantics and uses shared rendering for ordinary popup content while retaining check/radio/submenu/command/session/menu-bar behavior.
- `UiDropdownMenuRenderTest` target: Debug/Release **11/0** each.

Windows initially found three `UiDropdownPopup.cpp` type-name errors because `PopupWindow : TopWindow` inherited non-type `TopWindow::Style(dword)`, hiding unqualified `UiDropdown::Style` inside popup member scope. Published mechanical source fix:
- `1a69143bb317e7325b1160c9f747b1ffa3f38f10` — exactly three `const UiDropdown::Style&` qualifications.

Gary subsequently confirmed at `d78f16134c14d5eeb6eabd60abb9b9c4d1dc5303`:
- Ui Debug compile PASS, 0 warnings, 0 compile errors.
- the three Dropdown errors are gone.

R2D STATUS: **IMPLEMENTATION COMPLETE — WINDOWS RUNTIME/TEST VALIDATION PENDING.**

## GALLERY CAPTURE + MARQUEE CORRECTIVE

Already published and retained:
- explicit Gallery-owned selection/marquee interaction frame;
- marquee fill painted behind item content and frame painted above it;
- `WhenZoom(double)` semantic notification;
- Win32 capture-recursion root fix: Gallery tracks marquee capture ownership, clears ownership before normal release, and `CancelMode()` never calls `ReleaseCapture()`.

The former stack overflow path was:
`CancelMode -> ReleaseCapture -> CancelMode -> ...`
Current source removes that recursive ownership path; no depth guard/workaround was introduced.

## WINDOWS DARK-THEME STOP REPORT

Gary retested exact HEAD `d78f16134c14d5eeb6eabd60abb9b9c4d1dc5303` with clean tree:
- Ui Debug compile: PASS, 0 warnings/errors.
- `UiGalleryRegressionTest` Debug: **11 checks, 1 fail**.
- `UiGalleryRegressionTest` Release: **11 checks, 1 fail**.
- sole failure: `Dark theme resolves Gallery viewport surface to a dark palette face`.
- all other ten Gallery corrective checks passed.
- Gary correctly stopped before Gallery runtime/R2D runtime tests.

User screenshot confirmed the broader visual symptom: while Dark mode was active, Gallery viewport/tiles, List region and showcase header/margins still appeared light.

### Root cause

This was not a missing hardcoded dark RGB.

1. The Minimal List role intentionally allows a transparent normal row face (`UiFill::None()` / lightweight row surface).
2. Gallery copied that List palette, enabled its viewport face, but when the face remained `NONE` it fell through to platform `SColorPaper()` semantics; therefore the standalone Gallery remained white.
3. The showcase root used transparent `UiBoxLayout` directly over the platform `TopWindow`, so the intentionally transparent List region also exposed white.
4. `UiGalleryDemo` called style-mutating `UiGroupPanel::SetHeaderMode()` / `SetInset()` during construction, which entered `StyleEdit()` and froze the then-current Light style, preventing later theme switching from restyling the header.

### Published dark/theme corrections

- `250e62cb67c2f67e861733e7d9ff185912daf30e` — Gallery keeps List-derived content palette semantics but, when a face is intentionally `NONE`, resolves the standalone viewport face through the current theme's `UiPanel` Surface role. No hardcoded dark palette; explicit solid/image List faces remain authoritative. Gallery still drops List row skin for the viewport.
- `393113ade55e814c98e7a7dbee74b426624d1712` — Gallery showcase removes redundant style-mutating GroupPanel setters and puts its transparent layouts on a theme-aware `UiPanel` root surface. Header, margins/gaps and transparent List region can now remain theme-live.

## GALLERY ZOOM-TEXT CORRECTIVE

The original acceptance also found that Gallery tiles zoomed while text stayed full-size.

Published:
- `3be0fdef6ef599a38d73a14afe6be6c15898093e` — `UiItemRenderImage` stores layout-resolved fonts.
- `fef9cd6fda28d989428cc3d39bac26e2e61e4b36` — vertical image-tile presentation uses a bounded 0/1/2/3-step font reduction ladder based on the allocated content rectangle. Normal-size tiles keep the theme font; compact tiles step down to a DPI-aware minimum. Horizontal List rendering is unchanged. Explicit custom model title fonts are preserved. Font sizing happens only in renderer `Layout()` and Paint consumes cached fonts.
- `a071b808164c2ca206d6a5b6440cf2277e16ccdc` — scale guide records theme-surface, style-freezing and zoom-font rules plus the 10/11 Windows stop evidence.

GALLERY STATUS: **CORRECTIVE IMPLEMENTATION COMPLETE — WINDOWS REVALIDATION PENDING.**

## CURRENT WINDOWS TARGETS

Already accepted:
- `UiModelViewPerformanceTest`: Debug **52/0**, Release **52/0**.
- `UiTreeScaleTest`: Debug **11/0**, Release **11/0**.

Revalidate at final remote HEAD:
- Ui Debug source compile: PASS required.
- `UiGalleryRegressionTest`: expected Debug **11/0**, Release **11/0**.
- `UiDropdownMenuRenderTest`: expected Debug **11/0**, Release **11/0**.
- Gallery/Dropdown/Menu focused runtime smoke.

## GARY SCOPE DURING VALIDATION

Gary does **not** need to stop for trivial mechanical defects. He may repair and locally commit a tiny obvious issue such as:
- missing include;
- required type qualification/name-shadow fix;
- obvious spelling/identifier typo;
- similarly mechanical build-only correction with no design/behavior choice.

He should record the exact change/commit and continue validation. Stop and return to implementation for substantive rendering, model ownership, interaction, state, lifecycle, performance or architecture defects.

## NEXT ACTION

1. Fetch live `main`, record exact HEAD, require clean start.
2. Build Ui Debug CLANGx64.
3. Run `UiGalleryRegressionTest` Debug/Release: expected **11/0** each.
4. Run `UiGalleryDemo` Release and verify:
   - repeated marquee drag/release/Escape/edge-autoscroll never stack-overflows;
   - selected tile and marquee frames remain obvious;
   - marquee fill does not wash over images/text;
   - Light -> Dark -> Light changes header, root/margins, List region and Gallery viewport/tile area coherently;
   - Ctrl+wheel and +/- zoom both update status and keep a sensible anchor;
   - at reduced tile zoom, theme text steps down discretely/boundedly rather than remaining full-size; 100% keeps normal text size;
   - 10,000-item scrolling/First/Last remain responsive.
5. Run `UiDropdownMenuRenderTest` Debug/Release: expected **11/0** each.
6. Run Dropdown/Menu demos for popup scrolling, selection, multi-check/reorder, check/radio/submenus, keyboard and theme behavior.
7. Finish with `git diff --check` and report final status/local mechanical commits if any.
