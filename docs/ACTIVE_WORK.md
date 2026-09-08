# ACTIVE WORK

Remote branch is authoritative. Fetch before work/publish; never force-update main.

BASE: `cc386585d0eb5287c0f096d01a70a79e7df42fd7` / `main`
TASK: UIGRAPH-EXECUTION-CONSOLIDATION — keep 10k speed; remove competing execution paths.
BRANCH: `performance/uigraph-execution-consolidation-20260908`
TOUCHED: `Ui/UiGraph/UiNodeGraph*`, `Ui/Ui.upp`, render/pan regression tests,
`examples/UiGraphDemo`, `docs/08_UIGRAPH_GUIDE.md`, this log and validator task.
STATUS: IMPLEMENTATION COMPLETE — PLATFORM VALIDATION PENDING.
PUBLISHED: Checkpoint 1 `9d77e9e6d54400b2ee588249b14944647cb9e464`;
checkpoint 2 `866fedf0b76f50448feaf356b64b608bb7d0f6a5`.
The final source/checkpoint is the commit containing this log (resolve with
`git log -1 --format=%H -- docs/ACTIVE_WORK.md`); a commit cannot contain its own hash.
VALIDATION: git diff --check; expanded public-header comparison; 195 retained active
method-body comparisons after explicit name normalisation; Graph package/include
membership; one spatial source; one demo viewport handler/one-shot observer;
12 isolated C++ policy checks passed. Full U++/Windows build and runtime NOT RUN here.
NEXT ACTION: Run `docs/UIGRAPH_EXECUTION_CONSOLIDATION_VALIDATE.md` on the latest branch.
Do not merge into main until the platform gate passes.

## Published implementation

1. Shared inline LOD and edge admission policy; explicit paint path/fallback evidence;
   regression coverage for pan, rich-neighbour port visibility and Painter-only edges.
   Micro preflight and drawing reuse resolved edge styles.
2. Removed method/declaration alias macros and dead implementations. Geometry,
   camera, live projection and paint responsibilities now have named source parts.
   Scope-aware spatial has the sole canonical production filename. Public host API
   remains source-compatible, with additive diagnostic getters.
3. Normal status and optional diagnostics have one debounced viewport observer.
   The runtime fixture wrapper no longer replaces viewport observation. Hiding or
   disabling diagnostics does not suppress status; idle owns no repeating sampler.

No new shapes, routes, scene graph, GPU work or cache redesign. The existing raster
and style-preparation optimisations remain. File reduction is not a speed measurement.

## VALIDATED EVIDENCE BEFORE THE LAST THREE FIXES

Gary validated Debug + Release at ancestor `389359b2...`:
- UiGraphViewTests: 138/0;
- UiNodeGraphInteractionStateTest: 30/0;
- UiGraphRenderTests: 78/0;
- UiNodeGraphPresentationTest: 21/0;
- UiGraphDemo built in both configurations.

The relocation repair was then validated and published at `dc196091...`.

10k L3 style preparation improved from the old ~480–620 ms bottleneck to:
- style: 1.170 ms;
- resolve: 0.958 ms;
- scale: 0.212 ms;
- nodes: 2.341 ms;
- geometry: 3.490 ms.

The style-preparation tranche is therefore closed unless new evidence regresses it.

## Remaining release gate

The validator task covers Debug/Release Graph model, render, view, scale, pan and
presentation tests plus UiGraphDemo. Compare 10k overview/middle-pan timings on the
same machine/configuration against BASE; retain micro paint and zero rich detail/
content work. Check reference visuals, hierarchy fit/selection, scope transitions,
model switching, port circles, edge arrows and status with diagnostics on/off/hidden.

Earlier main validation also left the proximity acceptance matrix pending: Yes/Enter,
No, Escape, Always for unambiguous non-replacement candidates, explicit replacement
confirmation, and no silent connection for incompatible/ambiguous candidates.
Keep that release gate pending until its evidence is reported; do not silently mark
it passed merely because spatial code was renamed.
