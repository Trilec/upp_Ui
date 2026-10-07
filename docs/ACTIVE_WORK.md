# ACTIVE WORK

Fetch the applicable remote branch before publishing; preserve concurrent work
and never force-push. This is a current recovery index, not a validation log.
Contracts live in the reader guides; historical checkpoints live in Git.

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
