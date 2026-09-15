# ACTIVE WORK

Remote `main` is authoritative. Fetch before work/publish; do not force-update `main`.
Recovery state only; Git history is implementation history.

BASE: `57f26b7297442a9e0718e179550daa9de1c7a18e` / main
TASK: **UIGRAPH-LOD-POLICY-EDITOR-01 — interactive production matrix authoring**
STATUS: **SOURCE CHECKPOINT PUBLISHED — DEBUG VALIDATION PENDING**
BRANCH: `main`
PUBLISHED: `57f26b7297442a9e0718e179550daa9de1c7a18e`
NEXT ACTION: one focused Debug build/launch of `examples/UiGraphDesignMatrix`; fix compile/API mistakes only, then visually tune the editor shell if needed.

## ACCEPTED FOUNDATION

Eddie's presentation work remains authoritative:
- one prepared `UiGraphNodePresentation` owns layout/visibility/capacity;
- Standard / Centred / MediaCard production profiles;
- bounded request callback, explicit invalidation and read-only getter;
- rich paint consumes prepared regions;
- live projection reuses prepared regions;
- Micro preparation skips rich presentation callbacks and preserves the 10k fast path;
- full eight-shape structural matrix validation and measured Windows text-line correction at `2ca63ac5c8665d1aabadd0c5de541e23a0f69c62`.

The production renderer/LOD selection has NOT been silently rewritten by this editor checkpoint.
The editor authors proposed policy and renders it through the existing production graph so mismatches remain visible.

## CURRENT EDITOR CHECKPOINT

`examples/UiGraphDesignMatrix` is now a compact LOD policy editor based on the approved matrix concept.

Shell / viewing:
- Label-demo style `UiTitleCard` header;
- light/dark theme toggle;
- compact toolbar + threshold editor + legend + fixed LOD rail;
- horizontally scrollable eight-shape matrix;
- concise cell metadata instead of the previous multi-line documentation blocks.

Presentation templates:
1. Minimal
2. Compact
3. Standard
4. Status Card
5. Media Card
6. Parameter Node

The templates are intentionally separate from LOD. Each template owns editable per-shape/per-LOD requested feature state.

Authored-size scenarios:
- 260 x 170 capacity stress;
- 360 x 240 reference;
- 480 x 360 spacious.

Port topology scenarios:
- no ports;
- 1 input / 1 output;
- 3 inputs / 2 outputs;
- 4 inputs / 4 outputs.

The cells use real semantic ports plus real incoming/outgoing production edges from off-screen helper nodes so dense port behaviour is inspectable rather than implied.

## FEATURE TAGS

Each shape/LOD cell has clickable requested-feature tags:
- `TLE` title
- `SUB` subtitle
- `ICO` icon
- `BGE` badge
- `MED` media
- `DES` description
- `FOOT` footer
- `PLAB` port labels
- `CONT` native control

Tag state is diagnostic:
- green = requested and production currently shows it;
- red = deliberately disabled by authored policy;
- amber = requested, but current production LOD/capacity suppresses it.

This is deliberate. The matrix is an authoring/diagnostic tool, not a second renderer.
A request that production cannot currently satisfy remains visible as amber rather than being faked by the demo.

## LOD THRESHOLD EDITOR

The editor uses the production `UiRangeSegments` control.

Default proposed boundaries are:
- Normal: 56%
- LOD 1: 33%
- LOD 2: 13%
- below 13%: LOD 3

The range bar visually runs Normal -> LOD 1 -> LOD 2 -> LOD 3.
Boundary labels show the descending percentages from the HTML design concept.

Thresholds can be authored:
- globally per presentation template; or
- as an explicit per-shape override.

The selected shape can copy:
- thresholds only;
- feature policy only;
- all settings from another shape.

`Apply to all` propagates the current threshold policy.
`Reset shape` restores template defaults for the selected shape.

For this checkpoint, thresholds define the representative camera samples used by the matrix rows:
- Normal row: 1.00x;
- LOD 1 row: just below the authored Normal threshold;
- LOD 2 row: just below the authored LOD 1 threshold;
- LOD 3 row: just below the authored LOD 2 threshold.

The cell metadata still reports the ACTUAL production `UiGraphPresentationLevel`.
This lets the user see where proposed policy and today's production thresholds differ before promoting policy into core source.

## EXPORT / IMPORT

The editor owns a versioned JSON policy document (`schema_version: 1`).

It persists:
- active presentation template;
- authored-size scenario;
- port topology preset;
- all six template policies;
- global thresholds for every template;
- per-shape threshold overrides;
- all eight shapes;
- all four LOD feature policies;
- explicit boolean feature intent using human-readable names.

Actions:
- `Copy JSON` copies the complete pretty JSON document;
- `Export...` writes `uigraph_lod_policy.json` (or chosen path);
- `Import...` validates schema/catalogue shape before applying it.

This JSON is the durable bridge for later promotion of accepted matrix decisions into production defaults, tests, another editor session, or AI-assisted source changes.

## DELIBERATE BOUNDARY

This checkpoint does NOT yet make exported thresholds/features globally authoritative inside `UiNodeGraph`.
That is intentional until Curt has used the editor to decide what the desired policies actually are.

Requested features are projected into existing production inputs where possible (authored title/subtitle/icon/description, presentation badge/media/footer requests, port-label style, attached native control). Production remains free to suppress them according to its current level/capacity rules; amber tags expose those gaps.

This avoids changing stable renderer semantics before the editor has produced real design evidence.

Template reservation metrics (for example richer parameter/control-slot recipes) remain the next policy-schema extension if the interactive matrix proves they are needed. Do not duplicate arbitrary application data into `UiGraphModel`.

## FAST WINDOWS GATE

DEBUG ONLY.

1. Build `examples/UiGraphDesignMatrix`.
2. If it compiles, launch it and leave it running for Curt.
3. Smoke only:
   - light/dark toggle;
   - drag the three LOD boundaries;
   - switch Global / Per-shape threshold scope;
   - click one feature chip and confirm green/red/amber feedback changes;
   - select Media Card and Parameter Node;
   - select 1x1 and 3x2 port presets;
   - Copy JSON produces readable `schema_version: 1` policy;
   - Export then Import one JSON file and confirm the editor restores.
4. Run `UiGraphRenderTests` Debug only if the demo source required a production/API fix.
5. `git diff --check` PASS.

STOP on the first real compile/API failure and report it. Do not run Release or the full aggregate suite for this editor checkpoint.
