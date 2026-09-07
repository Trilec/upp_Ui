# ACTIVE WORK

Remote `main` is authoritative. Fetch before work/publish; never force-update `main`.
Recovery state only; Git history is implementation history.

## CURRENT
TASK: **Final upp_Ui completion acceptance**
STATUS: **ADVANCED REVIEW MUST-FIX TRANCHE PUBLISHED — WINDOWS VALIDATION PENDING**
SOURCE_HEAD: `2349a5ebbad24cf37ec6eb4a6f54bdf310aa5fc9`

The architecture audit F1-F10 remains accepted. A focused senior review of the final 10k
micro-rendering / exact-preparation path returned `CHANGE NOW`; its required bounded source
tranche has now been implemented without changing the accepted architecture.

## COMPLETED AFTER ADVANCED REVIEW

Micro raster:
- scratch integer path reset now preserves Vector capacity;
- one conservative miter/AA stroke pad is shared by cached raster extent and micro paint bounds;
- padded max-axis/byte eligibility is checked before ImageBuffer construction;
- failed/oversized raster attempts are not counted as admitted variants;
- diagnostics distinguish admitted variants, successful cached draws and direct fallbacks;
- resolved-style cache identity still uses exact retained local path coordinates + paint state;
- focused regressions cover:
  - real cached reuse under WhenResolveNodeStyle,
  - oversized short-wide direct fallback before allocation,
  - 6 px sharp triangle miter extent,
  - clipped dirty repaint of that extent,
  - fractional-radius path identity,
  - resolver-driven paint identity changes.

Exact preparation:
- phase evidence now separates reset, spatial, query, sort/order, node, edge, style,
  silhouette and anchor costs;
- node sort records resolve/store node pointer + z-order once instead of model lookups
  inside the comparator and again during construction;
- four micro side-port index vectors are reusable scratch instead of per-node allocations;
- identical exact local projected-micro silhouettes reuse a bounded 64-entry preparation-local
  path cache and are translated into each retained NodeGeometry;
- anchor semantics remain unchanged and eager; exact settle/programmatic APIs remain synchronous.

Optional ideas from the review (demand-driven anchors, retained-map storage reuse, typed overview
bins, wider callback-cache contracts) remain deferred until the new phase evidence justifies them.

## LAST WINDOWS EVIDENCE

At `0968129f882e5ffc8bf65a3ed87fdfc5f3f5a357`:
- Debug + Release PASS;
- BLITZ aggregate packages PASS;
- PropertyEditor PASS;
- 10k idle / Live profiling idle / Reference <-> 10k switches PASS;
- 10k Paint ~561.6 ms; node/surface ~554.4 ms; geometry ~622.1 ms; edge ~1.4 ms;
- prior node/surface baseline was ~838.4 ms;
- completion remained FAIL only because `micro_rasters=0`.

The resolved-style cache eligibility bug causing `micro_rasters=0` was fixed afterward at
`29ad41007a446935c2ffcb1b72e74370f9c39dc6`.

## FINAL WINDOWS GATE

Fetch current `main` and validate Debug + Release:
- PropertyEditorTests
- PropertyEditorOverrideCommitTest
- UiGraphScaleTests
- UiNodeGraphPerformanceTest
- UiGraphRenderTests
- UiGraphViewTests
- UiGraphTest
- all nine aggregate hygiene packages.

Required performance evidence from UiGraphDemo Reference + 10k:
- `micro_rasters > 0 && <= 32`;
- `cached_draws > 0`;
- record `direct_fallbacks`;
- record current/avg paint, node, geometry and edge timings;
- record preparation phases:
  reset / spatial / query / sort / nodes / style / silhouette / anchors / edges;
- record path-cache hits/misses;
- verify zoom and LOD diagnostics update across thresholds;
- verify route handle cutoff at 0.55 and other displayed feature transitions;
- 10k node/surface remains materially below the old ~838 ms baseline;
- 10k exact geometry result is evaluated from its phase breakdown rather than one aggregate
  number alone;
- 10k idle and Live-profiling idle settle;
- Reference -> 10k -> Reference -> 10k remains clean.

Manual PropertyEditor:
- inactive override value click enables immediately;
- first wheel edits value;
- wheel outside active value surface scrolls normally;
- explicit override toggle remains independent;
- row states remain visually distinct.

Final hygiene:
- `git diff --check` PASS;
- worktree clean.

If one preparation phase remains materially dominant, return the phase evidence before proposing
another optimisation. Do not broaden architecture without measured cause.

## CONTRACTS TO PRESERVE
- explicit generated curves target 0.35 final-device-pixel positional error inside the supported
  `TessellationStatus` envelope;
- direct Draw/native Painter first; shared exact raster cache for stable repeated AA;
- dense Graph may use `UiGeometry` directly;
- semantic positions never depend on tessellation vertex index;
- one retained world broad phase remains authoritative;
- diagnostics are observer-only; no periodic idle profiler clock;
- static views eventually become idle.

## BRANCH STATE
Single authoritative branch: `main`.

## CANONICAL DOCS
`00` Coding · `01` Controls · `02` Theme · `03` Model · `04` Demo ·
`05` PropertyEditor · `06` Large-scale Views & LOD · `07` Drawing & Geometry ·
`08` UiGraph · `09` UiDoc
