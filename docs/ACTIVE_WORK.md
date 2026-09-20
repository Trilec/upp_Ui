# ACTIVE WORK

Remote main is authoritative. Fetch before editing/publishing; never force-push.

BASE: `e02bbd0845f154e417f242088db93cf1f75da446` / main
TASK: **UIGRAPH-NODE-WORKSPACE-03C — fast/light audit, camera admission and matched 10k evidence**
TOUCHED: `Ui/UiGraph/UiNodeGraphProjection.inc`; `Utilities/UiGraphScaleTests/{main.cpp,UiGraphScaleTests.upp,ComponentProfile.cpp}`; `docs/{ACTIVE_WORK,UIGRAPH_PERFORMANCE_AUDIT,UIGRAPH_NODE_LAYOUT_ARCHITECTURE}.md`.
STATUS: **IMPLEMENTATION COMPLETE — PLATFORM VALIDATION PENDING** for the bounded source correction/profile. **10k named-component performance is NOT yet accepted.**
PUBLISHED: the commit containing this record; recover via `git log -1 -- docs/UIGRAPH_PERFORMANCE_AUDIT.md` and fresh remote main.
VALIDATION: pinned complete touched existing source reconstructed with matching Git blob hashes; full local diff review and git diff --check PASS. U++/Windows compilation, new scale profile and timing comparisons NOT run here.
NEXT ACTION: Gary runs current workspace Debug correctness gate plus focused ScaleTests --pan-profile / --components and UiNodeGraphLiveViewTest. Read UIGRAPH_PERFORMANCE_AUDIT. Hold optional shape-aware bands until that evidence is reviewed.

## 03C — audit findings and correction

The architecture remains bounded/shared, but named component records currently
force exact wheel scaling even for Micro hints. Legacy 10k pan results cannot be
used to certify generated component templates. Hover and overview model updates
also have broader invalidation paths; timing Paint alone misses input preparation.

Fixed: defer the existing immutable camera-baseline copy until projection has
passed coverage/LOD/component/control admission. Rejected frames no longer clone
the prepared scene before exact fallback. Accepted frames still project from the
immutable baseline; no guard removal, new cache, layout, LOD or paint change.

Added: --components diagnostic mode in existing UiGraphScaleTests. Matched legacy
and registered text/icon/progress fixtures use 10k nodes, 9,900 edges, eight shapes,
compact/card sizes and near/mid/overview. Reports input-event and Paint timings,
rebuild/population/backend counts, first event, median/coarse p95. Timings remain
MEASURE_ONLY; structural assertions fail normally. --pan-profile runs the original
legacy contract alone; no arguments still run the original four suites.

Open performance concerns are prioritised in UIGRAPH_PERFORMANCE_AUDIT: exact
component wheel rebuild, repeated registry validation, Micro preparation/payload
cost, state/image resources, hover/overview invalidation. Do not start a generic
component class hierarchy or solver to address them. Measure the affected path.

## Existing 03B source still awaits Windows validation

`e02bbd0845f154e417f242088db93cf1f75da446`: focused component Delete/Undo, improved
thin-preview picking, bounded readable-height fitting for single-line Ellipsis
text and actual representation/reason summaries. See UIGRAPH_WORKSPACE_03B.md.
No retired DesignMatrix or shape-aware-band implementation is included.

## Latest Windows evidence — Gary's 03A report ONLY

Tested `8f9a49a8367f0dd9d95d1085582e9348c5aa91db`; ancestry PASS. Debug RenderTests,
WorkspaceTests, unchanged generated C++ (Ui/CtrlLib only), Component Studio PASS.
Render: 8 suites/0; Workspace: 33 checks/0; native views: 27/0; startup: 1/0.
Diff check PASS, worktree clean, no fix commits. Manual icon/typography/inspector
scroll and inherited-scope checks passed. Held-button Escape during native DND
remains unverified. Those results validate neither 03B nor 03C or 10k components.
Evidence directory supplied by Gary:
`C:\Users\admin\AppData\Local\Temp\UiGraphWorkspace-validation\20260920-203041-8f9a49a8367f-6ea403`.

## Read first / source boundaries

1. This file and `docs/UIGRAPH_PERFORMANCE_AUDIT.md`.
2. `docs/UIGRAPH_WORKSPACE_03B.md`, `docs/UIGRAPH_WORKSPACE_V8_AUDIT.md`.
3. `docs/UIGRAPH_NODE_LAYOUT_ARCHITECTURE.md`, `docs/UIGRAPH_WORKSPACE_RUNTIME.md`.
4. `docs/UIGRAPH_NODE_WORKSPACE.md`, `docs/UIGRAPH_WORKSPACE_AUTHORING.md`.
5. Current complete source/callers/tests, not remembered snippets.

NodeGeometry.presentation is the sole evaluated layout authority; the existing
immutable camera snapshot physically copies evaluated data, not another allocator.
Content and Overlay are sibling layers. Templates are bounded shared C++ data;
JSON/history/PropertyEditor are authoring only. Ordinary components have no Ctrl
tree; real controls stay host-managed and useful-size-only. Camera changes never
resize authored nodes. Port identity/topology remains model-owned.

## Remaining work (not implemented by 03C)

Template-driven diagram inventory even for missing/unallocated/LOD-hidden items;
complete V8 Body-only/Full-edge post-port interior shared by both layers and zone
preview controls; optional independently fitted shape bands; malformed zoom label,
palette glyphs/chip packing; threshold undo/compact-size lifecycle; full save/open
and physical DND follow-through. Roles already exist; improve discoverability,
not a second role framework. No new arbitrary live-control/accordion authoring.

## Focused validation

This user explicitly requested a performance audit: that supplies the focused
reason to run the two 10k diagnostic modes, not an unrelated broad matrix.
Use established U++ 18468 / CLANGx64 / Debug. Require latest supervisor SHA as an
ancestor of current HEAD. Existing workspace runner plus standalone retained-camera
test and ScaleTests --pan-profile / --components. A passing timing-free invariant
summary does NOT establish 10k response; report the actual event/paint distributions.

Stop on real compile/assertion/contract failures. Minor mechanical CLANG fixes
may be reviewed, documented, published after refreshing main, and retested.
No guard removal, test weakening, silent toolchain changes, geometry redesign or
rich-Micro fallback. Preserve user edits/running work. Report exact HEAD/ancestry,
summaries, measured workload counts/timings, CPU/DPI, first blocker, manual 03B
findings, evidence directory, clean tree and any fix SHA.
