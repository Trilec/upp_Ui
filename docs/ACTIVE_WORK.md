# ACTIVE WORK

Fetch the applicable remote branch before publishing; preserve concurrent work
and never force-push. This is a current recovery index, not a validation log.
Contracts live in the reader guides; historical checkpoints live in Git.

## RangeSegments cards — 2026-10-10
BASE: Ui `2e8c1c4` / `main`.
TASK: title/subtitle cards, mirrored span percentages, upright vertical stacks.
TOUCHED: RangeSegments model binding/style/geometry/paint; canonical demo; usage/skills.
STATUS: implemented on the existing scalar model; compact defaults remain supported.
VALIDATION: Release BLITZ and Debug no-BLITZ range 363/0 each; native smoke 73/0 each.
Standalone public header Debug/no-BLITZ compiles; demo editor/state checks 46/0.
11 actual generated exports compile unchanged and pass Light/Dark 470/0, including
fractional scalar round-trips. All 27 documented usage recipes compile; skill hashes match.
Native raster checks cover handle shapes/cache reuse, small bounds and legacy streams;
H/V mirrored previews inspected. Range demo and GraphComponentStudio rebuilt in bin/windows-x64.
PUBLISHED: containing main commit; recover with `git log -1 -- Ui/UiRangeSegmentsGeometry.cpp`.
NEXT: use the Range/Cards demo for hands-on pointer/resize acceptance; broader gates below remain open.

## Frame Accent and authoring coverage - 2026-10-09
BASE: Ui `3adee3c` / `main`; Designer `416d6fe` / `main`.
TASK: shared curved Top/Bottom/Left/Right accents; Designer and real demo exports.
TOUCHED: StyledMetrics/drawing; GroupPanel, Tag, Graph and RangeSegments;
32 maintained demos; Designer theme adapters/schema/skills; control usage docs.
STATUS: implemented; focused validation recorded below, broader visual gates stay open.
VALIDATION: Release controls 14/0 suites, including accent 1023/0; theme 2/0 suites.
Debug no-BLITZ accent 1023/0 and five isolated public headers compile; 32 demos build.
Designer Release and Debug no-BLITZ 2120/0 each; generated C++ 12/0.
Designer checks include immediate visual-dependent field visibility and Undo.
27 unchanged demo exports compile, 136/0 runtime checks; 57 catalogue entries
covered by 26 compiled usage snippets; reader links and skill snapshots checked.
PUBLISHED: containing main commits; recover with `git log -1 -- Ui/UiDraw.cpp`
and Designer `git log -1 -- UiDesigner/Theme/UiDesignerFrameAccentThemeCommon.h`.
NEXT: native interaction acceptance remains separate from pixel/build checks;
use bin/windows-x64 demos and Designer bin/UiDesigner.exe for inspection.

## Splitter and repository polish — 2026-10-07
BASE: `2cc2df0` / `codex/uitag-hardening`.
TASK: softer Accent/Alert idle, hover and drag; one PropertyEditor registration path.
TOUCHED: splitter resolver/paint/demo; PropertyEditor adapters/demos; release metadata.
STATUS: focused validation complete; no whole-library release certification.
VALIDATION: 25 affected demos build; Release BLITZ and Debug no-BLITZ splitter
224/0 each; selectors pass; generated C++ 40/0; PropertyEditor 158/0 + 44/0 +
143/0; theme 1092/0 + 13/0; inventory, reader links and diff checks pass.
PUBLISHED: containing commit; recover with `git log -1 -- Ui/UiSplitter.cpp`.
NEXT: use bin/windows-x64/UiSplitterDemo.exe for the current reference; continue open gates below.

## Shared project fonts — 2026-10-07
PUBLISHED: Ui `2cc2df0`; UiDesigner `416d6fe`.
CONTRACT: [shared fonts and acceptance](PROJECT_FONTS.md).
VALIDATION: Ui 83/0 in Release BLITZ and Debug no-BLITZ; Designer fonts 43/0,
assistant 318/0, PropertyEditor 143/0, theme 1105/0; generated application 9/0.
Designer export-theme regression 53/0; full Designer Release application built.
NEXT: Font Picker reuses catalogue/revision; other adapters, shaping and real
multi-monitor DPI transitions remain separate acceptance.

## Demo and all-controls audit
BASELINE: consistent self-contained shell and family selectors; splitter defaults
published at `11d0698`. [Coverage register](../tests/ui_release_inventory.json)
drives maintained demos and records coverage routes; build output stays in build.
STATUS: publication slices are not full-library visual/API acceptance.
NEXT: remaining control/role/state/generated-code gates; [audit](../tests/CONTROL_AUDIT.md).
UiTag's native visual/theme/image-fill acceptance remains open.

## UiTab theme boundary
PUBLISHED: `8114269`; active caps/strip fills include explicit None.
STATUS: newer native UiTabThemePaintTest + Designer ThemeStudioRoleTest/open checks
remain the acceptance boundary; source review alone does not close them.

## UiGraph workspace boundary
APP: UiGraphComponentStudio; the obsolete DesignMatrix stub is removed.
STATUS: 03E1/03E2 Overlay/content underlay changes still need their native gate.
PUBLISHED: recover with `git log -1 -- examples/UiGraphComponentStudio/WorkspaceOverlayTests.cpp`.
The earlier 03D gate at `57e8d38` does not validate later source additions.
NEXT: existing ValidateUiGraphWorkspace Debug/launch gate and positive Overlay summary;
held-button Escape/DND, diagram inventory, post-port zones, undo and compact lifecycle.
Preserve one presentation geometry authority, immutable camera baseline, independent
Content/Overlay and native component/Micro budgets. See [Graph Development](09_UIGRAPH_DEVELOPMENT.md).

## Shared browser and micro picker — 2026-10-07
MIGRATION: UiColorPickerMicro is core Ui; Ui/UiFileBrowser is optional.
Maintained examples: UiColorPickerDemo (family) and UiFileBrowserDemo.
Contracts: Ui/UiColorPickerMicro.md and Ui/UiFileBrowser/README.md.
Focused Windows checks recorded under upp_cineview/build; no clean release gate or cross-platform/100k acceptance claimed.
Windows: browser 25 model / 164 native PASS (Debug/Release + BLITZ); picker
family/native captures, nine headers/generated C++, Ui smoke 73/0 and ranges
60/0 PASS in Debug/Release; CineView core/media/native + EXR provider PASS.

## Compact media controls — 2026-10-07
Playback Style adds per-command icon sizes, combined transport inset and cache-line thickness.
Probe defaults to a neutral selected outline; geometry and sampling contracts remain intact.
Debug/Release: media demo 53 self-test + 576 selector PASS; two standalone headers and six generated examples compile/run.
Guide: MEDIA_CONTROLS.md. Evidence: upp_cineview/build/skip-correction-shared; viewer native 98 PASS.
