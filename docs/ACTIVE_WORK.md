# ACTIVE WORK

Remote `main` is authoritative. Fetch before work/publish; do not force-update `main`.
Recovery state only; Git history is implementation history.

CURRENT MAIN AT HANDOVER: `f12a255441361bf2f057b74753638146067d1420`
TASK: **UIGRAPH-NODE-LAYOUT-TEMPLATE-03 — shared templates over retained node layout**
BRANCH: `main`
STATUS: **SOURCE PUBLISHED — WINDOWS DEBUG VALIDATION + DESIGN REFINEMENT PENDING**
PUBLISHED: `f12a255441361bf2f057b74753638146067d1420` (`UIGRAPH: evaluate shared templates into retained node layout`)
NEXT ACTION: validate current source on Windows, then refine the template/content/LOD separation before building Presentation Studio V4.

## READ FIRST

1. `docs/ACTIVE_WORK.md`
2. `docs/UIGRAPH_NODE_LAYOUT_ARCHITECTURE.md`
3. `docs/08_UIGRAPH_GUIDE.md`
4. `Ui/UiGraph/UiGraphNodeTemplate.h`
5. `Ui/UiGraph/UiGraphNodeTemplate.cpp`
6. `Ui/UiGraph/UiNodeGraphPresentation.inc`

Current remote source overrides chat memory and old SHAs.

## RETAINED CACHE DECISION

Node layout is part of `NodeGeometry`; the evaluated retained layout is the cache.
There is no second per-node layout cache, runtime JSON compiler, or one-Ctrl-per-node
layout tree. Compatible live camera motion projects the retained geometry.

## CURRENT STRUCTURAL HIERARCHY

The hierarchy is structural only. It must not be confused with which semantic
feature is placed into a region:

```text
Node / Safe Area
|
+-- Header                         optional
|
+-- Body
|   |
|   +-- Content                   same Body extent
|   |   +-- Left
|   |   +-- Main
|   |   +-- Right
|   |
|   +-- Overlay                   same Body extent; does not consume Content
|       +-- Left
|       +-- Main
|       +-- Right
|
+-- Footer                         optional
```

`Content` and `Overlay` are sibling layers over the same Body. Each has independent
Left/Main/Right columns. Overlay does not reduce Content capacity.

Left/right labelled port lanes may reserve full node-safe width or be tied to the
Body Content Left/Right columns, depending on template policy. Port anchors remain
semantic geometry at the silhouette boundary.

## TEMPLATE LAYER NOW IMPLEMENTED

`UiGraphNodeTemplate.h/.cpp` provide a small shared C++ template description.
Built-in template kinds:

- Minimal
- Identity
- Summary
- Status
- Media
- Parameter
- Operator

A template owns structural defaults plus ordered slot rules. Slot rule dimensions:

- feature;
- target region;
- placement: Fill / Top / Bottom / Left / Right / Center;
- authored extent;
- LOD visibility mask;
- flow: Stable / Reflow;
- gap.

The fixed slot array is intentional: no dynamic per-node layout tree. Built-in or
custom C++ template definitions are shared; only evaluated rectangles are retained
in each prepared `NodeGeometry`.

Legacy `Standard/Centred/MediaCard` request/profile behaviour remains available when
no template is selected, so the migration is additive rather than destructive.

## CRITICAL DESIGN SEPARATION — DO NOT CONFLATE

There are three separate concerns:

1. **STRUCTURE** — Header / Body(Content+Overlay)/Footer and their columns.
2. **CONTENT PLACEMENT** — which semantic thing (Title, Subtitle, Icon, Media,
   Status, Progress, Fields, Tags, Control, etc.) occupies which structural region
   and with what placement/alignment/flow.
3. **LOD POLICY** — at Normal/LOD1/LOD2/LOD3 whether that semantic feature remains
   present, hidden, forced, inherited, and whether hiding it preserves or reflows
   its reservation.

The current source implements the structural template/slot machinery, but the
semantic feature vocabulary is still intentionally limited to production-owned
slots (`Title`, `Subtitle`, `Icon`, `Badge`, `Media`, `Description`, `Control`,
`Footer`). Domain-rich concepts such as Status, Progress, Fields, Tags and Actions
currently ride through host-painted content rather than all being first-class slot
features.

**This is the next design question.** Before expanding enums mechanically, decide
which semantic features deserve first-class reusable slot identity versus which are
content rendered inside a structural region/body mode.

Do not encode rules such as “Description always belongs in ContentMain” or “Icon
always belongs in Header”. Templates must own placement. Likewise LOD is not a
layout layer; it is policy applied to semantic slots in the chosen structural
layout.

## PRESENTATION STUDIO V4 DIRECTION

The supplied mockup direction is the current UI target:

- one selected Template / Node shape / connector style;
- four persistent Normal / LOD1 / LOD2 / LOD3 previews;
- left/middle visual diagrams for Node Region and Node Overlay;
- explicit hierarchy/visibility table;
- right-side styling/property inspector;
- `UiRangeSegments` edits LOD transition thresholds only and must not resize the
  specimen cameras;
- layout builder edits structural region + slot placement separately from LOD
  enable/disable policy;
- preview zoom remains independently user-controlled;
- production C++ template output; JSON only optional Studio/session interchange.

Likely UI mental model:
- Structure tree on rows;
- semantic feature chips attached to the relevant structural row;
- four LOD columns control visibility/flow policy;
- separate slot editor changes *where/how* a selected feature is placed.

Do not let the Studio become a full general UiDesigner.

## PERFORMANCE CONTRACT

Retain everything useful; invalidate narrowly; replay a small region where useful;
project retained layout for compatible camera changes. Do not build a fine-grained
dependency graph without measurement. Stable/Reflow already belongs to slot rules;
coarse Header/Body/Footer invalidation can be added later if profiling justifies it.

## VALIDATION / GARY ROLE

Gary validates on Windows; supervisor owns architecture and main coding.

Focused Debug gate for the current template checkpoint:

1. fetch/pull current `main`;
2. confirm `f12a255441361bf2f057b74753638146067d1420` is an ancestor;
3. build/run `UiGraphRenderTests`;
4. build `examples/UiGraphDesignMatrix`;
5. confirm selector smoke remains green;
6. quick visual smoke only;
7. `git diff --check`;
8. report first real blocker; minor mechanical CLANG fixes may be published after review.

No Release/broad suite/10k benchmark unless the focused gate exposes a shared or
performance-sensitive issue.

## GITHUB / WORKFLOW

Remote `main` is source of truth. Refresh first. Use complete touched files and the
relevant callers/tests/.upp membership. Diagnose before editing. Publish small
coherent checkpoints directly without leaving proof/final/published branches.
Review full diff and verify remote contains the published SHA.

Gary handles compile/runtime validation and minor fixups; he is not the architecture
authority.
