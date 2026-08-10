# UiDoc v2 redesign status

Branch: `document-redesign`

## Recovery point

Last verified build-relevant Core checkpoint before staged editor sources:
`c27f02ad41592f20ab1f781470b798c6413f5c8b` — `UiDoc v2: notify views when core clears`

Current staged editor-source branch head when this note was introduced:
`057cb4aa49041dcfd15730ec814745d26597fafa` — `UiDoc v2: checkpoint editor commands`

The commits after `c27f02a` currently add source-only editor units that are not yet members of `Ui/Ui.upp`; therefore the active package still builds the V1 `Ui/UiDoc.cpp`/`Ui/UiDoc.h` while v2 is assembled.

## Authoritative v2 Core already published

- concrete non-visual `UiDocCore`
- sparse text style runs
- semantic sparse blocks
- arbitrary document/block/annotation/resource/embed metadata
- deterministic revisioned transactions and position mapping
- sparse undo/redo history
- comments/annotations, resources, anchors and generic embeds
- canonical typed rich table model
- versioned native `.uidoc` persistence
- monotonic revision and model-change notifications
- headless Core test package including a 100,000-line sparse-style case

## Final source organisation decision

UiDoc is a subsystem and will live under `Ui/UiDoc/`, similar to `Ui/UiColorPicker/`.

Do not keep the temporary highly granular editor split as the final architecture. Consolidate to approximately:

- `UiDocCore.h`
- `UiDocCore.cpp` — core state, transactions, API and typed table operations
- `UiDocCoreJson.cpp` — native persistence only
- `UiDoc.h`
- `UiDoc.cpp` — Ctrl lifecycle, Core facade, editing/search/clipboard/commands
- `UiDocLayout.cpp` — paragraph, table/image layout, viewport geometry and hit testing
- `UiDocPaint.cpp` — document, embeds, selections, comments/gutters/caret painting
- `UiDocInput.cpp` — keyboard/mouse/navigation/table-cell interaction

This split is by real responsibility, not by file-size target. Avoid additional public classes or hierarchy unless a real second consumer requires one.

## Staged source already published but not package-active

Temporary root-level checkpoint files:
- `Ui/UiDocParagraphLayout.cpp`
- `Ui/UiDocGeometry.cpp`
- `Ui/UiDocCommands.cpp`

These are recovery checkpoints only. Their reviewed logic should be folded into the consolidated `Ui/UiDoc/` files and the temporary root-level files deleted when the v2 package switch is made.

## Next exact steps

1. Reconstruct the latest branch files from GitHub before every edit.
2. Consolidate the local v2 editor implementation into the eight-file `Ui/UiDoc/` layout above.
3. Move the published Core implementation into that folder without changing its logical contract.
4. Update includes in `Ui/Ui.h`, `Ui/UiTheme.h`, tests/demo and package membership.
5. Atomically switch `Ui/Ui.upp` from V1 root `UiDoc.*` to the new folder and remove temporary/root V1 UiDoc sources from package membership.
6. Statically inspect declarations/definitions, package membership, include direction and full diff.
7. Publish the editor switch promptly before compile work.
8. Then compile/fix with Windows validator help; never weaken Core/model tests.
9. After the editor is coherent, redesign `examples/UiDocDemo` into the Word-like application showcase with real `.uidoc` New/Open/Save/Save As, Home/Insert/Review/View surfaces and representative rich content.
10. Remove this status file before merge unless it is deliberately converted into permanent maintainer documentation.

## Important constraints

- Remote GitHub branch is source of truth.
- Reconstruct exact touched files locally; no blind whole-file edits against remembered content.
- Publish small recoverable checkpoints because transport timeouts are frequent.
- No V1 backward-compatibility requirement for UiDoc API/document format.
- Keep public concepts small: `UiDocCore` + `UiDoc`; use composition, not a deep inheritance hierarchy.
- Core owns logical document meaning; UiDoc owns U++ pixel layout, input and painting.
- Agent/MCP support comes through deterministic Core query/mutation/revision APIs, not MCP code inside the control.
