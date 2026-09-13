# ACTIVE WORK

Remote `main` is authoritative. Fetch before work/publish; do not force-update `main`.
Recovery state only; Git history is implementation history.

BASE: `2ca63ac5c8665d1aabadd0c5de541e23a0f69c62` / main
TASK: **UIGRAPH-PRESENTATION-CLOSE-01 — full production matrix / release closure**
STATUS: **IMPLEMENTATION + LOCAL DEBUG VALIDATION COMPLETE — FINAL VISUAL SMOKE ONLY**
BRANCH: `main`
PUBLISHED: `2ca63ac5c8665d1aabadd0c5de541e23a0f69c62`
NEXT ACTION: brief visual smoke of the full Design Matrix, then close UiGraph presentation and clean stale supervisor branches.

## IMPLEMENTED

Accepted shared presentation contract from `7798f5f9ee6d9c4c6da6a962cd380da00c63d2da` remains authoritative:
- one prepared `UiGraphNodePresentation` owns regions, level, visibility and capacity;
- Standard / Centred / MediaCard profiles;
- bounded request callback, explicit invalidation and read-only getter;
- rich paint consumes prepared regions;
- live projection projects prepared regions without layout work;
- Micro preparation skips the rich presentation callback and preserves the fast path.

Final matrix work at `2ca63ac5c8665d1aabadd0c5de541e23a0f69c62` adds:
- all eight canonical shapes: Rectangle, Ellipse, Diamond, Triangle, Hexagon, Cloud, Document, Database;
- Normal / LOD 1 / LOD 2 / LOD 3 production rows;
- shared Standard / Centred / MediaCard profile selection;
- Compact / Reference / Spacious authored-size scenarios;
- native-control capacity scenario;
- 1.0x / 1.5x Normal enlargement comparison;
- actual prepared level, projected size, capacity and visible-feature evidence per cell;
- explicit `not assessed` rich-capacity status for Micro cells.

The Windows visual pass exposed one real production issue: presentation text slots
were based on requested font height rather than the rendered Windows line box.
`UiNodeGraphPresentation.inc` now reserves measured final-pixel line height for
visible stock text, and single-line descriptions ellipsize within their prepared slot.
No presentation thresholds were changed because matrix evidence did not justify it.

## LOCAL WINDOWS VALIDATION

Eddie validated on the local Windows/CLANGx64 environment:
- full structural sweep: 8 shapes x 3 profiles x 11 real projected sizes;
- all shape/profile combinations reach all four presentation levels;
- allocated regions remain contained and disjoint;
- compact 260x170 capacity-stress cases remain structurally safe;
- Normal enlargement checks at 1.5x and 2.0x preserve composition/alignment;
- measured Windows line boxes fit their prepared slots;
- reusable pan performs no presentation/layout rebuild;
- Micro invalidation/preparation skips the rich presentation callback;
- presentation suites: 85 checks PASS after the line-box correction;
- execution-path coverage remains 8/8;
- all six aggregate Debug suites reported PASS;
- Debug Design Matrix built and launched successfully;
- first visual pass confirmed expected capacity limits in compact Ellipse, Diamond and Triangle cells and separate port-label lanes;
- corrected 1.5x enlargement view showed complete text lines after the measured-line-height fix.

Eddie was interrupted while finishing the final profile/native-control visual sweep.
No architectural or threshold blocker was found.

## ACCEPTED POLICY / LIMITS

- Native controls are real only at useful interaction sizes; reduced presentation hides them rather than pretending to scale internal fonts/padding/theme.
- Capacity-limited shapes are reported honestly; content is not overlapped to force a fit.
- Micro cells do not claim rich-content capacity.
- Transfer activity remains a separate deferred feature and is not a presentation-release blocker.
- Collapse remains deferred.
- The single-translation-unit `.inc` implementation organisation is retained; converting suitable fragments to ordinary `.cpp` files is optional future hygiene only if compile-time/navigation evidence justifies it.
- Painted control summaries and transition hysteresis/fades remain optional future work only if real application evidence requires them.

## FINAL ACCEPTANCE — KEEP IT SMALL

One brief Debug visual smoke is sufficient; do not repeat Eddie's full validation:
1. Launch `examples/UiGraphDesignMatrix`.
2. Check Standard / Centred / MediaCard selectors.
3. Check Compact / Reference / Spacious authored sizes.
4. Check the native-control scenario at useful and reduced sizes.
5. Check 1.0x / 1.5x enlargement for clipping or ownership/alignment changes.
6. Confirm there is no obvious overlap/clipping across the eight-shape matrix.

If that smoke is satisfactory, mark **UIGRAPH PRESENTATION COMPLETE**.
No Release build, exhaustive benchmark or another architecture audit is required.
After closure, prune the stale merged `supervisor/...` branches.
