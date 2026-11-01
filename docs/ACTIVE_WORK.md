# ACTIVE WORK
Fetch the applicable remote branch before publishing; preserve concurrent work
and never force-push. This is a current recovery index, not a validation log.
Contracts live in the reader guides; historical checkpoints live in Git.
## Accordion measurement and body containment — 2026-10-23
TASK: fix growth and clipped content exposed by shadow-heavy Designer specimens. TOUCHED: UiAccordion measurement/body layout; UiControlTests accordion suite.
STATUS: filling children measure intrinsically; content uses styled body insets.
VALIDATION: all 15 control suites pass in Release and Debug without BLITZ; new accordion suite 66/0 each. Designer theme suite 2178/0 and native Brutalist checks pass.
BOUNDARY: no preset shadow suppression or Designer measurement workaround.
## Package cleanup — 2026-10-23
TASK: move production inspector/graph packages under Ui and validation under tests.
TOUCHED: package paths/API, demos, Designer, validation runners, inventory and docs.
STATUS: UiPropertyInspector has one public identity and one full adapter registration.
BOUNDARY: Core remains headless; duplicate cache/theme launchers folded into aggregates.
VALIDATION: 45 demos + Designer Release; 39 test packages Release; 8 inspector Debug BLITZ.
45 automatic runs pass; 2 harnesses build-only; 4 headers no-BLITZ; Designer 89/0 + 59/0.
NEXT: owner review; the existing Designer Services/UiFonts architecture boundary remains separate.
## Shared themes and showcase — 2026-10-23
TASK: Ui/Themes recipe modules and one preset catalogue shared with Designer.
TOUCHED: theme recipes/facade, UiThemeDemo, Designer catalogue/context and docs.
STATUS: eight presets incl. Olive; compact paired Light/Dark specimens, header roles/category buttons.
VALIDATION: UiThemeDemo and Designer Release builds; 32 native traversals; document 59/0.
BOUNDARY: preset IDs and Minimal default unchanged; Olive compact field metrics retained.
NEXT: owner review in bin/windows-x64/UiThemeDemo.exe; no all-controls certification.
## UiDoc foundation — 2026-10-11
BASE: Ui `b09b06f`; foundation published in `ecadb12` on main.
TASK: audit/harden the reusable document foundation.
TOUCHED: model/history/callback/layout/input; focused Doc tests and docs.
STATUS: implemented; findings and reproducible checks in [audit](UIDOC_AUDIT.md).
VALIDATION: all eight Doc suites pass in Release BLITZ and Debug without BLITZ.
BOUNDARY: application-specific authoring controls are maintained outside Ui.
NEXT: native authoring, capture/focus/resize and platform/font/print acceptance.
## RangeSegments cards — 2026-10-10
PUBLISHED: recover with `git log -1 -- Ui/UiRangeSegmentsGeometry.cpp`.
VALIDATION: Debug/Release range 363/0 and native smoke 73/0; demo 46/0;
11 unchanged exports 470/0; 27 usage recipes/header/skill checks pass.
STATUS: shared scalar model supports mirrored H/V cards; binaries refreshed.
NEXT: hands-on pointer/resize acceptance remains separate from focused raster checks.
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
TASK: softer Accent/Alert idle, hover and drag; one UiPropertyInspector registration path.
TOUCHED: splitter resolver/paint/demo; UiPropertyInspector adapters/demos; release metadata.
STATUS: focused validation complete; no whole-library release certification.
VALIDATION: 25 affected demos build; Release BLITZ and Debug no-BLITZ splitter
224/0 each; selectors pass; generated C++ 40/0; UiPropertyInspector 158/0 + 44/0 +
143/0; theme 1092/0 + 13/0; inventory, reader links and diff checks pass.
PUBLISHED: containing commit; recover with `git log -1 -- Ui/UiSplitter.cpp`.
NEXT: use bin/windows-x64/UiSplitterDemo.exe for the current reference; continue open gates below.
## Shared project fonts — 2026-10-07
PUBLISHED: Ui `2cc2df0`; UiDesigner `416d6fe`.
CONTRACT: [shared fonts and acceptance](PROJECT_FONTS.md).
VALIDATION: Ui 83/0 in Release BLITZ and Debug no-BLITZ; Designer fonts 43/0,
assistant 318/0, UiPropertyInspector 143/0, theme 1105/0; generated application 9/0.
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
## Shared browser and micro picker — 2026-10-27
CONTRACT: optional Ui/UiFileBrowser; core UiColorPickerMicro; demos UiFileDialogDemo/UiColorPickerDemo; Ui/UiFileBrowser/README.md and docs/COLOR_PICKER_MICRO.md.
VALIDATION: heading renderer measures bold glyphs/theme padding; Release BLITZ + Debug no-BLITZ each pass 383 native checks, model 25/0; captures/commands/hashes in build/file-browser-headings-2026-10-27. Physical DPI/cross-platform/100k acceptance remains separate.
## Compact media controls — 2026-10-07
Playback Style adds per-command icon sizes, combined transport inset and cache-line thickness.
Probe defaults to a neutral selected outline; geometry and sampling contracts remain intact.
Debug/Release: media demo 53 self-test + 576 selector PASS; two standalone headers and six generated examples compile/run.
Guide: MEDIA_CONTROLS.md. Evidence: upp_cineview/build/skip-correction-shared; viewer native 98 PASS.
