# ACTIVE WORK

Remote `main` is authoritative. Fetch before work/publish; never force-update `main`.
Recovery state only; Git history is implementation history.

TASK: **UiGraph authoring tranche + RC compile/validation closure**
STATUS: **SOURCE FIXES PUBLISHED — WINDOWS VALIDATION PENDING**
CURRENT SOURCE CHECKPOINT: `dc196091ba1452bc7bd2091124cc4391d22503a3`

## CURRENT STATE

The left UiGraphDemo authoring rail is published and remains the intended surface:
- Undo / Redo;
- eight canonical node-shape creation tools;
- Straight / Bezier / Orthogonal route selection;
- manual connection and proximity auto-connect share `ValidateConnection()`;
- authoring controls disable in the 10k benchmark model.

Current graph work also includes:
- resolved-style/profile work;
- Circle marker clipping fix;
- detailed connector Painter AA;
- H2 proximity-port backend;
- current LOD / profiling readout.

Do not revert or bypass those descendants while validating the RC.

## BLOCKER FIX PUBLISHED

Windows validation at `389359b2da66d9be1726382fb20520b6f1b325d6` exposed:

`Vector<UiGraphPortRef>::Reserve()` -> U++ `Relocate` static assertion.

Root cause:
- `UiGraphPortRef` is an aggregate containing `UiGraphNodeRef` + `String`;
- proximity queries now store it in `Vector<UiGraphPortRef>`;
- it was neither trivially tagged nor admitted as a U++ guest type.

Repair in `dc196091ba1452bc7bd2091124cc4391d22503a3`:
- preserve the public aggregate form and all `UiGraphPortRef{node, "port"}` call sites;
- declare `is_upp_guest<UiGraphPortRef> = true`;
- relocation therefore uses the normal move constructor/destructor rather than memcpy;
- add a regression that stores a port ref, forces `Vector::Reserve` relocation, and verifies identity.

Do NOT replace this with `Moveable<UiGraphPortRef>`: that changes aggregate initialization.

## NEXT WINDOWS GATE

First prove the compile repair through an affected consumer:
- UiDesigner `Tests` Debug.

Then run current upp_Ui:
- `UiGraphModelTests`;
- `UiGraphScaleTests`;
- `UiNodeGraphPerformanceTest`;
- `UiGraphRenderTests`;
- `UiGraphViewTests`;
- `UiGraphTest`.

After automated PASS, run UiGraphDemo manual acceptance:
- Reference and 10k visual correctness;
- hierarchy / selection / interaction;
- authoring rail and Undo/Redo;
- all three route modes;
- proximity connect;
- LOD transitions and zoom readout;
- averaged profiling and idle settling;
- `micro_rasters > 0 && <= 32`;
- Reference -> 10k -> Reference -> 10k remains clean;
- capture same-machine phase timings.

## CONTRACTS

- `UiGraphPortRef` remains aggregate-initializable public API.
- No BLITZ/test workaround for production compile failures.
- Reusable defects are fixed in upp_Ui.
- Keep current H2 backend authoritative for compiled spatial implementation.
- Diagnose measured profile bottlenecks before further performance architecture changes.
