# ACTIVE WORK

Remote `main` is authoritative. Fetch before work/publish; never force-update `main`.
Recovery state only; Git history is implementation history.

BASE: `5948cea56d364429ba8aa8165f8bde984b0bfb75`
TASK: **UIGRAPH-EDIT-HISTORY-01 — Undo/Redo + proximity auto-connect**
TOUCHED:
- `Ui/UiGraph/UiNodeGraphBase.h`
- `Ui/UiGraph/UiNodeGraphInteractionBase.inc`
- `Ui/UiGraph/UiNodeGraphSpatial.cpp`
- `Utilities/UiGraphViewTests/InteractionState.cpp`
- `Utilities/UiNodeGraphInteractionStateTest/main.cpp`
- `examples/UiGraphDemo/UiGraphDemo.h`
- `examples/UiGraphDemo/UiGraphDemo.cpp`
- `examples/UiGraphDemo/UiGraphDemoRuntime.cpp`
- `examples/UiGraphDemo/UiGraphDemoCommands.cpp`
- `examples/UiGraphDemo/UiGraphDemo.upp`
- `docs/ACTIVE_WORK.md`
STATUS: **IMPLEMENTATION COMPLETE — PLATFORM VALIDATION PENDING**
PUBLISHED: supervisor checkpoint pending squash/merge to `main`
VALIDATION: full source/diff review + deterministic generic interaction coverage; Windows demo validation pending.

## PREVIOUS CHECKPOINTS

- `cd587fa51a8323b84820c8e041e9f819cba32a09`: measured 10k style-prep optimisation;
  platform performance validation still required.
- `5948cea56d364429ba8aa8165f8bde984b0bfb75`: circle marker placement and
  detailed connector AA consistency; platform visual validation still required.

## CURRENT IMPLEMENTATION

Generic UiNodeGraph:
- remains request-first; no hidden model undo stack;
- Ctrl+Z, Ctrl+Y and Ctrl+Shift+Z route to host-owned Undo/Redo events when installed;
- exposes a bounded `QueryPortsNear(screen_point, radius_px)` using the existing retained
  world spatial authority;
- exposes read-only prepared port screen anchors;
- proximity discovery does not bypass `UiGraphModel::ValidateConnection()`.

UiGraphDemo host history:
- bounded 64-command Undo/Redo history;
- node drag/nudge commands preserve before/after positions;
- route-edit commands preserve before/after waypoints;
- connection commands preserve the created edge and any Single-multiplicity edges replaced;
- delete commands preserve deleted nodes, original scopes and all incident/selected edges;
- Undo of node deletion restores the node and its connections in one command;
- Redo reapplies the command;
- mode changes clear history so commands can never target the wrong bound model;
- status text exposes Undo/Redo depth;
- AddNode command kind is reserved for the next authoring-palette slice.

Proximity auto-connect:
- after a committed node move, checks ports within 20 final screen pixels;
- uses retained spatial candidates, never a 10k node scan;
- orientation/type/scope/multiplicity authority is exclusively `ValidateConnection()`;
- same-node and other simultaneously moved-node candidates are ignored;
- ambiguous candidates within 4 px of the best match do not auto-select;
- a small Ui dialog defaults Enter to Yes and Escape/No to cancel;
- the dialog can enable "Always connect unambiguous compatible ports";
- silent Always mode is limited to non-replacement connections;
- Single-port replacement always remains explicit and the dialog reports how many
  existing connections will be replaced;
- accepted proximity connections use the same Undoable connection command as manual gestures.

## CONTRACTS PRESERVED

- UiGraphModel remains the sole semantic topology authority;
- Undo/Redo is host command state, not duplicated inside the model;
- connection compatibility/multiplicity rules are not duplicated in the demo;
- one retained world spatial broad phase remains authoritative;
- no 10k-wide proximity scans;
- request-first interaction remains usable by other hosts unchanged.

## WINDOWS VALIDATION NEEDED

Build/run Debug + Release:
- `UiGraphViewTests`;
- `UiNodeGraphInteractionStateTest`;
- `UiGraphDemo`.

Manual Reference-mode checks:
- delete a connected node -> Ctrl+Z restores node + incident connectors -> Redo deletes again;
- drag a node -> Ctrl+Z/Redo restores/reapplies position;
- edit a connector route -> Ctrl+Z/Redo restores/reapplies route;
- manually connect ports -> Undo removes it; Redo restores it;
- Single multiplicity replacement -> Undo restores the previous connection;
- move an unconnected compatible port within ~20 px of another compatible port:
  - confirmation appears;
  - Enter accepts;
  - No/Escape cancels;
  - Always applies only to future unambiguous non-replacement candidates;
- incompatible or ambiguous nearby ports do not connect;
- switching Reference/10k clears history and never replays against the other model.

## NEXT ACTION

Build the left authoring palette on top of this command path:
- canonical node-shape creation;
- Straight / Bezier / Orthogonal connector tools;
- visible Undo/Redo actions;
- every created node enters the existing AddNode command history.

Do not invent a second creation/connection mutation path for the palette.
