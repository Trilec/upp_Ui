# ACTIVE WORK

Remote GitHub is authoritative. Never force-update `main`. Work directly from refreshed `main`; do not create temporary feature branches for ordinary control normalization.

## CURRENT SUPERVISORY STATE — 2026-08-17

STATUS: **UIGRAPH SPATIAL INTERACTION R2 IN PROGRESS — MARQUEE PREVIEW PUBLISHED; BATCH COALESCING NEXT; WINDOWS VALIDATION PENDING; UI LABEL REFERENCE ACCEPTED; FOUR-CONTROL SOURCE WORK COMPLETE.**

### UiGraph spatial-interaction continuation — 2026-08-17

BASE: `5ff17bdf2d365fbf95d172690f1c1f7fc30e0e1a` (`main` at this continuation refresh).

TASK: `UI-NODEGRAPH-SPATIAL-INTERACTION-R2` — finish the retained spatial-hash interaction path with transient marquee feedback and coalesced view/spatial updates for multi-node graph mutations.

PUBLISHED:

- `f85751adb4620776bdb34e0128fb035ee3ed5a06` — transient marquee candidate cache. Marquee movement queries the existing world-space hash, keeps semantic selection unchanged, damages only changed preview nodes/rectangle, and mouse-up commits the cached IDs without a second spatial query.

STATUS: **PARTIAL — first R2 slice published and verified; visual preview overlay + UiNodeGraph batch-update scope remain in implementation.**

TOUCHED SO FAR:

- `Ui/UiGraph/UiNodeGraph.h`
- `Ui/UiGraph/UiNodeGraphInteraction.cpp`
- `Utilities/UiNodeGraphScaleTest/main.cpp`
- `docs/ACTIVE_WORK.md`

DETERMINISTIC CONTRACT SO FAR:

- `UiNodeGraphScaleTest` expected total is now **45 checks / 0 failures** before the forthcoming batch tests are added;
- zoom-0.5 marquee movement may query spatial cells for transient preview IDs but must not rebuild prepared geometry or the spatial index;
- preview candidates remain locally bounded and are not semantic selection until mouse-up;
- mouse-up commits the cached preview rather than querying the graph again.

NEXT ACTION:

1. paint preview-only nodes with a lightweight shape-path overlay while keeping committed selection as the independent 2px overlay;
2. add nested `UiNodeGraph::BeginBatchUpdate()/EndBatchUpdate()` coalescing for authoritative `graph.Model()` mutations and route internal multi-node move/delete operations through it;
3. extend the 10k test to prove batch mutations cause no spatial/geometry work until outer commit, no full spatial rebuild, and one prepared-geometry pass;
4. update this recovery log again and only then hand the final documented `main` descendant to Gary.

The previous long-form convergence/Label/UiGraph record is preserved verbatim in `docs/ACTIVE_WORK_ARCHIVE_PRE_FOUR_CONTROL_2026-08-17.md`. Task-specific four-control detail is retained in `docs/ACTIVE_WORK_UI_OVERRIDE_ROLLOUT.md`.

### Authoritative published checkpoints before this commit

- UiLabel reference and PropertyEditor ordering are accepted; latest published Label demo styling checkpoint recorded in the archived log is `8a016656651fb929c08ac1c2f801a6b8c2f2ab77`.
- UiGraph scale/source-review work is already promoted into `main`; its source checkpoint is recorded in the archived log. Windows focused scale validation remains pending.
- UiList runtime style-authority + renderer contract: `d0579b8753748ca765710f6c29805d2859ddf6aa`.
- UiList striped-row style persistence: `97d1531192f712365cddea1f9390a1a031e01836`.
- cross-repository `Trilec/upp_uidesigner` List + Edit adapter checkpoint: `c27f499c8d51ad73037d9a60481bb73d870d38a7`.
- cross-repository `Trilec/upp_uidesigner` Dropdown + Accordion checkpoint/current accepted source head: `ec02f1cbcc040f70ad55e656b98ec64640142cec`.

## ACTIVE TASK — FOUR-CONTROL OVERRIDE NORMALIZATION

TASK: `UI-LIST-EDIT-DROPDOWN-ACCORDION-OVERRIDE-NORMALIZATION`.

SOURCE RESULT:

- `UiList` built-in rows now project the owning List style instead of independently resolving presentation from global theme state; explicit custom item renderers retain renderer ownership.
- `UiList::Style::striped_rows` is serialized and covered by the focused style contract test.
- Designer has dedicated normalized adapters for UiList, UiBaseEdit, UiDropdown and UiAccordion; existing authored field ids are preserved where they existed.
- Face fields promoted from plain Color to FillRecipe accept legacy Color values as Solid recipes rather than losing authored data.
- Dropdown preserves collapsed-control ownership separately from nested `Popup/*` domains.
- Accordion preserves outer chrome separately from composed `Header/*`, `Body/*`, Section, Behaviour and Animation domains.
- resource-backed Skin/custom-image editing remains intentionally deferred until Designer theme adapters receive document-resource resolution; do not add raw path workarounds.
- four Ui demos use one local config/model authority each. The large legacy List/Dropdown builders remain in their old `main.cpp` files for reference but are no longer package entrypoints; `NormalizedDemo.cpp` is compiled instead.
- demo ownership sections contain live runtime-backed fields; no placeholder/empty style section is intentionally exposed.

TOUCHED BY FINAL DEMO CHECKPOINT:

- `examples/UiListDemo/NormalizedDemo.cpp`
- `examples/UiListDemo/UiListDemo.upp`
- `examples/UiLineEditDemo/main.cpp`
- `examples/UiDropdownDemo/NormalizedDemo.cpp`
- `examples/UiDropdownDemo/UiDropdownDemo.upp`
- `examples/UiAccordionDemo/main.cpp`
- `docs/ACTIVE_WORK.md`
- `docs/ACTIVE_WORK_UI_OVERRIDE_ROLLOUT.md`
- `docs/ACTIVE_WORK_ARCHIVE_PRE_FOUR_CONTROL_2026-08-17.md` (preserved prior ACTIVE_WORK blob)

## WINDOWS VALIDATION REQUIRED — FOUR-CONTROL WORK

No Windows/U++ PASS is claimed for the new four-control work yet.

`Trilec/upp_Ui` on the exact final `main` HEAD:

1. Build/run `Utilities/UiListStyleContractTest` Debug + Release; require `UILIST_STYLE_CONTRACT_SUMMARY checks=13 failed=0`.
2. Build/run `Utilities/UiModelViewPerformanceTest` Debug + Release; require 52/0 and unchanged second List `Layout()` reporting zero renderer relayouts.
3. Build `examples/UiListDemo`, `examples/UiLineEditDemo`, `examples/UiDropdownDemo`, and `examples/UiAccordionDemo` Debug + Release with zero compile errors; report warnings.
4. Launch all four demos. Confirm every visible ownership section contains working controls and changes affect the live preview without crashes/corruption.
5. UiList smoke: viewport chrome remains distinct from `Rows/*`; model rows, badges, metadata/checks and drag settings remain functional.
6. UiLineEdit smoke: Editing, Underline, Whitespace, Focus, Shadow and Highlight settings affect the live edit as applicable.
7. UiDropdown smoke: collapsed control remains usable; open popup after changing Popup/Items/Marker/Badge settings and verify the nested style domains visibly remain popup-owned.
8. UiAccordion smoke: headers/bodies remain composed separately; section open policy, chevron, line, drag and animation remain functional.
9. Run `git diff --check` and confirm clean `git status --short`.

`Trilec/upp_uidesigner` at `ec02f1cbcc040f70ad55e656b98ec64640142cec` unless remote `main` has legitimately advanced:

1. Build/run `tests/ListEditThemeAdapterTest` Debug + Release; require emitted zero-failure summary.
2. Build/run `tests/DropdownAccordionThemeAdapterTest` Debug + Release; require emitted zero-failure summary.
3. Build UiDesigner Debug + Release and launch it.
4. Select UiList, UiBaseEdit-family control, UiDropdown and UiAccordion; verify normalized group ownership/order and that Face rows use FillRecipe where specified.
5. Verify authored QuadGradient recipes survive Inspector refresh/reselection and generated C++ for the covered Face fields.
6. Confirm legacy Color-backed Face values resolve as Solid FillRecipe instead of None.
7. Confirm no fake resource-path Skin editor appears.

If a substantive ownership, model, rendering, persistence or adapter-contract failure appears, return it to implementation. Do not restore retired accessors, duplicate state, weaken tests or broaden unrelated architecture during Windows validation.

## UIGRAPH CORRECTIVE — SOURCE IMPLEMENTED

BASE: `67bb841c267b983fcec470eeef9a33f5349c185c` (`main` at corrective refresh).

TASK: `UI-NODEGRAPH-CORRECTIVE-R1` — close Windows acceptance findings for live style editing, selection presentation, double-click semantics, marquee performance and UiLabel-consistent PropertyEditor structure.

TOUCHED:

- `Ui/UiGraph/UiNodeGraph.cpp`
- `Ui/UiGraph/UiNodeGraph.h`
- `Ui/UiGraph/UiNodeGraphInteraction.cpp`
- `Ui/UiGraph/UiNodeGraphSpatial.cpp`
- `Utilities/UiNodeGraphScaleTest/main.cpp`
- `examples/UiGraphDemo/UiGraphDemo.cpp`
- `examples/UiGraphDemo/UiGraphDemo.h`
- `examples/UiGraphDemo/UiGraphDemoData.cpp`
- `docs/ACTIVE_WORK.md`

PUBLISHED:

- `426004a1e6dc7838e5ddfed9b4da6a1d9564022b` — damage-bounded marquee movement and double-click sole-selection semantics.
- `0eebe8f4ad466749899bacbfd220aac834642770` — marquee release selects through the retained world-space spatial index instead of walking prepared/all model geometry.
- `838c7164ca32edd735eb5d5866f0024dc3cb6ad7` — first corrective recovery log checkpoint.
- `b43f9243d2f2b545048c498409483055f6f57f75` — dirty-region/spatial paint candidate pass, dirty-sized Painter buffers, modern translucent marquee and one antialiased 2px shape-independent selection overlay.
- `c4fddbbc04a8631597e75c6257cf96bfe5554a57` / `22c3a06b1e4f9017725ee5222921ffd7aea0b096` — demo state-preview and authored FillRecipe support state.
- `966b83256dad491d6018b4eb3d055ed28acf0aba` — UiGraphDemo style page normalized to shared FillRecipe Face values and UiLabel-style group/id grammar; selected-node state row previews live without overwriting authored state.
- `02b104f043105c63733755c0ee39c730b5a95305` — softer deterministic demo-only role palette; production semantic theme roles unchanged.
- `6e8d06e40ab082e4705e6ccb478c0711fd145b90` — scale test updated for dirty/spatial paint visits, zero-work marquee drag at zoom 0.5, spatial selection on release and sole double-click selection.
- `f1d65afa53ba0893ca6af631e71e97ae25ada0fe` — corrective recovery checkpoint before final spatial interaction hardening.
- `0706515349298eda51332cee61192f185216847b` — private spatial node/port/edge interaction hit helpers declared.
- `314b05121d44cabc5bf255a2895303b79d8de705` — pointer hit helpers query tiny world-space regions through the retained spatial hash, then exact-test only local prepared geometry.
- `14882ae3f7f3aa17aef72d19096a61a451051b01` — mouse-down, hover, double-click and connection-target discovery use spatial interaction hit queries.
- `3dab24cefdc64650fd83d99781c31a508bbc79f6` — deterministic scale coverage for local pointer neighbourhoods at zoom 0.5.

SOURCE RESULT:

- marquee drag is overlay-only: it performs no selection query and no prepared-geometry/spatial rebuild on pointer movement;
- mouse-up converts the marquee to world space and queries the same retained spatial cells used by viewport culling, so selection resolution scales with covered cells rather than total graph size;
- old/new marquee damage is refreshed locally instead of invalidating the full control;
- Graph paint further narrows viewport-prepared geometry through the current dirty rectangle and the same spatial index; antialiased edge/node buffers are allocated to the dirty rectangle rather than the whole viewport;
- selected standard nodes receive one final antialiased 2px outline built from their actual shape path; rectangular focus chrome is not stacked on selected nodes;
- double-click clears prior node/edge selection, selects only the clicked node, then fires `WhenNodeAction`;
- UiGraphDemo Face state rows use the shared `PropertyEditorKind::FillRecipe`; Frame/Ink/Header remain Color because those are the current runtime field types;
- the selected node paint-projects whichever Normal/Hot/Selected/Disabled style row is being inspected so live colour/fill edits are visibly testable while the 2px selection chrome remains independent;
- authored Face recipes for local custom styles are retained by style token/state so a QuadGradient recipe survives PropertyEditor refresh/reselection instead of being reverse-engineered from the generated image;
- PropertyEditor groups follow the accepted Label grammar: Face, Frame, Ink, Header, Typography, Content Margin, Focus, Shadow, Highlight, Ports, with state rows labelled only Normal/Hot/Selected/Disabled;
- the demo palette is intentionally softer/pastel and theme-aware without changing global UiTheme role colours;
- viewport status exposes logical nodes/edges, prepared nodes/edges, candidate nodes/edges and zoom;
- the retained world-space spatial hash remains the sole broad-phase index; no quadtree/R-tree/BVH was added;
- live pointer interactions now query a small screen-radius converted to world space, use `QuerySpatial`, and exact-test only the returned node/port/edge candidates;
- node drag preview still leaves the authoritative spatial index unchanged until the model commit; ordinary node mutations update only the affected node spatial record and incident edge records.

DETERMINISTIC SCALE CONTRACT:

- `Utilities/UiNodeGraphScaleTest` now expects **44 checks / 0 failures** at the R1 baseline; R2 raises this as documented above;
- 10,000 nodes / 19,800 bounded-neighbour edges remain the fixture;
- paint visit counts are required to be <= bounded prepared counts because dirty-region spatial painting can visit less than the full prepared viewport;
- at zoom 0.5, repeated marquee MouseMove must leave both geometry and spatial build serials unchanged;
- marquee mouse-up must produce a bounded local selection while leaving spatial build serial unchanged;
- double-click must reduce an existing multi-selection to the double-clicked node only;
- zoom-0.5 hover must resolve a tiny spatial pointer neighbourhood (`node hit candidates < 24`, `port hit candidates < 32`) without rebuilding the spatial index.

VALIDATION: source-reviewed architecture; **no Windows/U++ PASS claimed yet**. The prior acceptance remains STOPPED until Gary reruns the focused graph gates on the final documented R2 `main` descendant.

BOUNDARY / DEFERRED BY DESIGN AT R1:

- no second spatial tree or hover-region cache is added; the current hash query is intentionally the simplest broad phase until profiling demonstrates otherwise;
- the prior no-batch deferral is superseded by active R2: a UiNodeGraph view-side coalescing scope is now explicitly in implementation, while shared UiDataModelBase notification semantics remain unchanged;
- public `HitTestNode/Port/Edge` remain compatibility helpers over the bounded prepared viewport; production live mouse interaction uses the spatial helpers directly;
- custom/non-rectangular image-backed Face recipes and deeper prepared-geometry style micro-invalidation are not required to close the reported Windows blockers and remain separate renderer enhancements.

## NEXT

1. Finish the R2 visual-preview and batch-coalescing slice, update this log, then run the active UiGraph Windows gate before treating UiGraph acceptance as closed.
2. Complete the four-control Windows gates above and source-review any genuinely mechanical platform correction before publishing it.
3. Remove stale temporary/Label branches only after comparing each tip to current `main`; the accidental `DO_NOT_USE` branch contains no useful work and is safe to delete.
4. Continue remaining control normalization from the same documented convention rather than introducing a new parallel schema framework.