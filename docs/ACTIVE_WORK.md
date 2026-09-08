# ACTIVE WORK

Remote `main` is authoritative. Fetch before work/publish; never force-update `main`.
Recovery state only; Git history is implementation history.

BASE: `7985c868fa0e77c9fd7e81eb03a410a6689448c8`
TASK: **UIGRAPH-AUTHOR-PALETTE-01 — left authoring rail**
TOUCHED:
- `examples/UiGraphDemo/UiGraphDemo.h`
- `examples/UiGraphDemo/UiGraphDemo.cpp`
- `examples/UiGraphDemo/UiGraphDemoRuntime.cpp`
- `examples/UiGraphDemo/UiGraphDemoCommands.cpp`
- `examples/UiGraphDemo/UiGraphDemoAuthoring.cpp`
- `examples/UiGraphDemo/UiGraphDemo.upp`
- `Ui/UiGraph/UiNodeGraphSpatialH2.cpp`
- `docs/ACTIVE_WORK.md`
STATUS: **PUBLISHED — WINDOWS VALIDATION PENDING**
PUBLISHED: `7985c868fa0e77c9fd7e81eb03a410a6689448c8`
VALIDATION: source/diff/package-membership review complete; Windows Debug/Release + GUI/runtime validation pending.

## PUBLISHED CHECKPOINTS IN THIS SERIES

- `cd587fa51a8323b84820c8e041e9f819cba32a09`
  - split 10k style preparation into resolver vs metric scaling;
  - cache deterministic 10k demo preset resolution by role + preset.
- `5948cea56d364429ba8aa8165f8bde984b0bfb75`
  - fixed Circle marker clipping;
  - unified detailed connector AA on Painter while preserving overview Direct Draw.
- `9456272d73d79348a37c7ac56fe4fc89d39d48d4`
  - added host-owned Undo/Redo command history;
  - added validated post-drag proximity auto-connect.
- `9efe164ea3725c4579e7600943fc01da17d7efc9`
  - hotfix: mirrored new proximity methods into the H2 spatial backend actually compiled by `Ui.upp`.
    The pre-H2 spatial source remains synchronized recovery source.

## CURRENT AUTHORING RAIL

UiGraphDemo now has a compact left rail built entirely from upp_Ui controls.

History:
- visible Undo and Redo tool buttons;
- Ctrl+Z, Ctrl+Y and Ctrl+Shift+Z remain available;
- buttons enable/disable from the existing command stacks.

Canonical node creation:
- Rectangle;
- Ellipse;
- Diamond;
- Triangle;
- Hexagon;
- Cloud;
- Document;
- Database.

Each node tool:
- uses a small antialiased shape icon;
- creates one canonical UiGraphNode at the current viewport centre;
- gives it Flow input/output ports;
- records creation through the existing AddNode history command;
- selects the new node for immediate Inspector editing.

Connector authoring:
- Straight;
- Bezier;
- Orthogonal route tools;
- Bezier is the default;
- the selected route is applied to both manual port drags and proximity auto-connect;
- route tools do not create a second connection path: `ValidateConnection()` remains authoritative.

Scale-mode safety:
- the authoring rail is disabled in the 10k benchmark model;
- history is already cleared on Reference/10k model switches.

Layout:
- left authoring rail + central Graph + existing right Inspector/Style/Code/Diagnostics rail;
- no node/edge child-Ctrl scaling changes were made to UiNodeGraph itself.

## VALIDATION NEEDED

Build Debug + Release:
- `UiGraphViewTests`;
- `UiNodeGraphInteractionStateTest`;
- `UiGraphRenderTests`;
- `UiNodeGraphPresentationTest`;
- `UiGraphDemo`.

Manual Reference-mode:
1. Create each of the eight node shapes and confirm canonical appearance.
2. Undo/Redo node creation from both buttons and keyboard.
3. Choose Straight, Bezier and Orthogonal, then drag output -> compatible input and confirm route.
4. Delete a newly created connected node; Undo restores node + connectors; Redo removes them.
5. Exercise 20 px proximity connect and confirmation/Always behavior.
6. Confirm incompatible/ambiguous ports do not connect.
7. Switch to 10k: authoring controls disabled, benchmark remains responsive.
8. Return to Reference: history is empty and authoring controls re-enable.
9. Recheck Circle marker and detailed connector AA.

Performance:
- capture 10k L3 `style / resolve / scale` after `cd587fa...`;
- compare style/node preparation against the prior ~480–620 ms style baseline;
- verify micro_rasters/cached_draws/path-cache evidence remains healthy;
- verify Live profiling becomes idle.

## NEXT ACTION

Run the consolidated Windows validation above.
If the new style split still shows one dominant subphase, return that evidence before another
performance optimisation. Do not reopen spatial/raster architecture without measured cause.
