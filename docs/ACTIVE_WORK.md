# ACTIVE WORK

Remote GitHub is authoritative. Never force-update `main`. Work directly from refreshed `main`; preserve unrelated concurrent advances.

## CURRENT SUPERVISORY STATE — 2026-08-18

STATUS: **UIGRAPH R2 AUTOMATED WINDOWS GATES PASS; MANUAL ACCEPTANCE STOPPED ON SELECTION-CHROME RENDERING; FOUR-CONTROL SOURCE WORK COMPLETE — WINDOWS VALIDATION PENDING.**

Detailed prior history is preserved in:
- `docs/ACTIVE_WORK_ARCHIVE_PRE_FOUR_CONTROL_2026-08-17.md`
- `docs/ACTIVE_WORK_UI_OVERRIDE_ROLLOUT.md`

## ACTIVE TASK — UI-NODEGRAPH-SPATIAL-INTERACTION-R2

Source checkpoints:
- `ef3d518a1a1ddd7edca1c4e5857433fc6f36d527` — final R2 spatial interaction / marquee / batch code.
- `56ab7bdec916b1b022873ee58f0465bbc24d2af9` — R2 completion documentation checkpoint used for the first Windows acceptance run.
- `13337c98b98b26898c0c5fb5c1eddd0fac1088ce` — Gary's allowed mechanical Windows fix: explicit integer casts for `StyledState` bounds in `UiGraphDemo.cpp`.

Core R2 architecture remains accepted at source level:
- retained world-space spatial hash is the sole broad-phase index;
- pointer hit-testing queries tiny world-space neighborhoods and exact-tests only local candidates;
- marquee uses a Windows-style thin blue frame plus translucent blue fill;
- local marquee preview uses retained spatial cells and does not commit selection until release;
- large marquee preview defers candidate hints after the 256-cell threshold and resolves once on release;
- repaint is damage-bounded;
- `BeginBatchUpdate()` / `EndBatchUpdate()` coalesce UiNodeGraph retained-index/geometry response around authoritative `Model()` mutation;
- local batch movement updates touched nodes and unique incident edges without a full spatial rebuild;
- ordinary nodes remain painted virtual objects; semantic topology remains solely in `UiGraphModel`;
- canonical `Model()/SetModel()/UseInternalModel()/ClearModel()` ownership semantics remain unchanged.

## WINDOWS EVIDENCE — 2026-08-18

Starting acceptance HEAD: `56ab7bdec916b1b022873ee58f0465bbc24d2af9`.

Automated results reported by Gary:
- `Utilities/UiGraphTest` Debug: **90/90 passed**.
- `Utilities/UiGraphTest` Release: **90/90 passed**.
- `Utilities/UiNodeGraphScaleTest` Debug: **UINODEGRAPH_SCALE_SUMMARY checks=49 failed=0**.
- `Utilities/UiNodeGraphScaleTest` Release: **UINODEGRAPH_SCALE_SUMMARY checks=49 failed=0**.
- `Utilities/UiDataModelsTest` Debug: **Checks 7535 / Fails 0 / PASS**.
- `examples/UiGraphDemo` Release: built successfully.
- `examples/UiGraphDemo` Debug: built and launched responsively after the mechanical cast fix.
- `git diff --check`: PASS.
- final reported worktree after the mechanical fix: clean.

Mechanical build fix:
- commit `13337c98b98b26898c0c5fb5c1eddd0fac1088ce`;
- `GraphDemoProjectState()` now casts `ST_NORMAL` / `ST_DISABLED` to `int` for `minmax` type consistency;
- no architecture or test contract changed.

### MANUAL STOP / CURRENT BLOCKER

Gary correctly stopped the GUI acceptance at the first substantive failure:
- the initially selected Rectangle rendered a dark/black committed-selection outline instead of the required consistent blue approximately 2px shape-following selection chrome.

Because this was a stop condition, the remaining manual checks were not run:
- marquee appearance/performance;
- 10k navigation and adaptive large-marquee behavior;
- batch drag/key/delete smoke;
- PropertyEditor live preview / Cancel;
- middle-pan regression;
- theme smoke;
- Reference -> 10k -> Reference retention.

### SOURCE DIAGNOSIS TO CONTINUE FROM

The resolved Graph selection colour is semantic blue (`selection_box_frame` resolves from the Standard role accent path), so the reported black stroke is not explained by the intended theme value.

The Graph paint path currently consumes several `ImageBuffer`s while their `BufferPainter` objects are still alive and without an explicit `Finish()` first. U++ documents `BufferPainter::Finish()` as the operation that guarantees scheduled painting is complete; the destructor invokes it automatically. The affected Graph buffers include:
- marquee alpha tile (`bp`);
- edge buffer (`ep`);
- node-details buffer (`np`);
- transient marquee-preview buffer (`pp`);
- committed selection buffer (`sp`).

NEXT CORRECTIVE:
1. explicitly call `Finish()` before each of those Painter-backed buffers is converted/drawn;
2. add focused rendered-image coverage for a selected Rectangle so the expected blue selection chrome reaches the final image, rather than only asserting selection state;
3. rebuild/re-run the focused Windows Graph gates and resume manual acceptance from selection chrome onward;
4. if the rendered selection is still wrong after correct BufferPainter finalization, inspect the selected-state frame/overlay composition next rather than changing spatial/model architecture.

No Windows PASS is claimed for the manual Graph acceptance yet.

## BRANCH STATE

Repository policy is main-only for ordinary work. Gary is currently pruning obsolete temporary branches. The intended steady state is local/remote `main` only.

## OTHER ACTIVE WORK

### Four-control override normalization

Status: **SOURCE IMPLEMENTATION COMPLETE — WINDOWS VALIDATION PENDING.**

Authoritative detail and exact gates are in `docs/ACTIVE_WORK_UI_OVERRIDE_ROLLOUT.md`.

Key checkpoints:
- UiList renderer authority: `d0579b8753748ca765710f6c29805d2859ddf6aa`.
- UiList striped-row persistence: `97d1531192f712365cddea1f9390a1a031e01836`.
- UiDesigner List/Edit adapters: `c27f499c8d51ad73037d9a60481bb73d870d38a7`.
- UiDesigner Dropdown/Accordion adapters: `ec02f1cbcc040f70ad55e656b98ec64640142cec`.

### UiLabel

Accepted reference implementation. Continue using the accepted UiLabel PropertyEditor grammar and shared FillRecipe conventions; do not introduce a parallel schema dialect.

## NEXT

1. Complete the narrow Graph BufferPainter/selection corrective above.
2. Resume the stopped Graph manual Windows acceptance from selection chrome onward.
3. Finish obsolete branch pruning so `main` is the only working branch.
4. Run the four-control Windows gates in `docs/ACTIVE_WORK_UI_OVERRIDE_ROLLOUT.md`.
