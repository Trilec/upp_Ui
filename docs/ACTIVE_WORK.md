# ACTIVE WORK

Remote main is authoritative. Fetch before work/publish; do not force-update main.

BASE: `81bd7b53cfbf531bc2e2e406f7677186aaa23bb0` / main
TASK: UIGRAPH-DESIGN-MATRIX-RECT-01 — Rectangle four-row production presentation proof
STATUS: PUBLISHED — PLATFORM VALIDATION PENDING
BRANCH: main
PUBLISHED: `81bd7b53cfbf531bc2e2e406f7677186aaa23bb0`
NEXT ACTION: quick Debug-only build/launch of the new design-matrix example, then expand the proven row mechanism to the remaining seven canonical shapes.

## ACCEPTED FOUNDATION

Eddie's shared presentation contract at `7798f5f9ee6d9c4c6da6a962cd380da00c63d2da`
is the source of truth:
- one prepared `UiGraphNodePresentation` owns regions, level, visibility and capacity;
- Standard/Centred/MediaCard profiles;
- bounded request callback + explicit invalidation + read-only getter;
- rich paint consumes prepared regions;
- live camera projection projects prepared regions without layout work;
- micro preparation skips the presentation callback and preserves the 10k fast path.

No UiNodeGraph production source is changed by this checkpoint.

## TOUCHED

- examples/UiGraphDesignMatrix/UiGraphDesignMatrix.upp (new)
- examples/UiGraphDesignMatrix/main.cpp (new)
- docs/ACTIVE_WORK.md

## RECTANGLE PROOF

A dedicated design-matrix example now shows the SAME authored Rectangle through four
independent production UiNodeGraph views. The rows differ only by camera zoom:

- Normal: 1.00x
- LOD 1: 0.55x
- LOD 2: 0.32x
- LOD 3: 0.13x

The authored node is identical in all four rows:
- Rectangle silhouette;
- title + subtitle + description;
- icon;
- input/output ports and labels;
- MediaCard presentation request;
- badge, media minimum and footer reservations.

Each row uses normal UiNodeGraph preparation/paint. There is no preview-only renderer,
no copied layout logic and no hand-authored presentation rectangles.

The content callback consumes `GetNodePresentation()` and paints only the prepared
badge/media/footer slots. Row readouts report:
- expected row;
- actual prepared presentation level;
- current zoom;
- projected node size;
- capacity (`fits` / capacity-limited);
- currently visible features.

Rows display `[OK]` when the actual prepared level matches the intended matrix row.
The views remain zoomable for inspection; Reset restores the canonical four zooms.

This structure is intentionally ready for the full matrix: later checkpoints add the
other seven canonical shapes to the same four production rows instead of inventing a
new matrix renderer.

## PLATFORM GATE — KEEP IT FAST

DEBUG ONLY.

1. Build `examples/UiGraphDesignMatrix`.
2. Launch it and leave it running for Curt.
3. Confirm all four left-side row readouts say `[OK]` at Reset state.
4. Confirm the visible progression is sensible:
   - Normal shows the richest authored composition;
   - LOD 1 simplifies without moving to an unrelated composition;
   - LOD 2 keeps only the useful reduced identity/detail;
   - LOD 3 is the cheapest silhouette identity.
5. `git diff --check` PASS.

No Release build.
No full test matrix.
No 10k benchmark for this demo-only checkpoint.

If the example fails to compile, fix only a local example/API-usage mistake. Stop and
report if validation suggests a production presentation-contract change is required.

## NEXT

After this Rectangle proof is visually accepted:
1. expand the same four rows to Rectangle / Ellipse / Diamond / Triangle / Hexagon /
   Cloud / Document / Database;
2. label each cell with actual projected size/capacity evidence;
3. include one enlarged Normal sample proving authored composition is preserved above
   1:1;
4. use the matrix to tune presentation thresholds/profile defaults only where evidence
   shows a real capacity problem.

Transfer animation remains a separate later slice. Collapse remains deferred.
