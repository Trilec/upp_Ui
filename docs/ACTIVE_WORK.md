# ACTIVE WORK

Remote `main` is authoritative. Fetch before work/publish; never force-update it.
This is recovery state, not a replacement for Git history.

BASE: `77912445a5a1881672bb587f161d55d42a9008d8` / `main`
TASK: **UIGRAPH-NODE-COMPONENT-01A — identified Text/Icon slots and first production editor**
TOUCHED: `Ui/Ui.upp`; `Ui/UiGraph/{UiGraphNodeTemplate.h,UiNodeGraph.h,UiNodeGraphPresentation.inc,UiNodeGraphPaintRich.inc,UiNodeGraphProjection.inc}`; `Utilities/UiGraphRenderTests/{main.cpp,UiGraphRenderTests.upp,PresentationLayout.h,Components.cpp}`; `examples/UiGraphComponentStudio/`; this file and `docs/UIGRAPH_NODE_COMPONENTS.md`.
STATUS: **IMPLEMENTATION COMPLETE — PLATFORM VALIDATION PENDING** for 01A only.
PUBLISHED: the source commit containing this update; recover with `git log -1 --format=%H -- examples/UiGraphComponentStudio/main.cpp` and inspect current main.
VALIDATION: complete touched-source review and `git diff --check`; Windows/U++ builds and GUI execution NOT run in the implementation environment. Regression tests added, not claimed passing.
NEXT ACTION: Gary's focused Windows Debug gate below. Then native Micro hints and the next bounded authoring slice. The wider component contract remains PARTIAL.

## READ FIRST

1. This file.
2. `docs/UIGRAPH_NODE_COMPONENTS.md` — actual implemented API, demo and limitations.
3. `docs/UIGRAPH_NODE_COMPONENT_CONTRACT_DRAFT.md` — accepted wider direction; API sketches are not all implemented.
4. `docs/UIGRAPH_NODE_LAYOUT_ARCHITECTURE.md`.
5. `Ui/UiGraph/UiGraphNodeTemplate.h` and current complete presentation/paint/projection source.
6. `docs/08_UIGRAPH_GUIDE.md` for wider graph ownership. Its older section-layout examples must not override the current Content/Overlay contract.

The component design contract was already saved by `77912445a5a1881672bb587f161d55d42a9008d8`. Do not recreate or replace it with an older chat attachment.

## WHAT 01A IMPLEMENTS

- Named components extend the SAME fixed-capacity template slot array. Empty IDs retain the existing production feature path.
- Unique template-local IDs, validation with non-mutating failed additions, repeated Text/Icon roles, existing node-field and typed text `node.data` bindings.
- Per-component horizontal/vertical alignment, optional ink/font height, per-level Inherit/On/Off overrides and Stable/Reflow.
- Retained text/icon output, measured text-footprint bars and dot proxies in the non-Micro production renderer. Paint does not resolve bindings, measure named text or rescale named icons.
- Optional bounded component records inside `NodeGeometry.presentation`, not a second layout cache. Public snapshots have deep-copy array ownership.
- Compatible pan projects those records without allocation/re-layout. Named-component wheel scaling takes an explicit exact fallback pending a validated component-boundary reuse policy. Existing unnamed nodes retain their existing wheel path.
- `examples/UiGraphComponentStudio`: four real `UiNodeGraph` views of one model/template; independently persistent cameras; actual-level diagnostics; selected-component region/placement/alignment/proxy/flow/ink/font editor; LOD policy table; reorder; title-data editing; shape choice; authored-size expansion. Layout uses UiBoxLayout and Ui controls.

## BASELINE BREAKS FOUND AND REPAIRED

The earlier template checkpoint had NOT completed migration of all callers/build membership:

- `UiNodeGraphProjection.inc` still projected removed `body_left/body_main/body_right/center` members.
- `PresentationLayout.h` still used those retired fields and asserted the old overlay geometry.
- `Ui/Ui.upp` omitted `UiGraphNodeTemplate.cpp` and its header.

These are repaired against the accepted Content/Overlay hierarchy. No retired aliases are restored and no tests are disabled.

## RETAINED ARCHITECTURE / STRUCTURE

`NodeGeometry.presentation` remains the one evaluated layout result.

```text
Node / Safe
  Header (optional)
  Body
    Content -> Left / Main / Right
    Overlay -> Left / Main / Right
  Footer (optional)
```

Content and Overlay independently span Body. Overlay does not consume Content.
Labelled port lanes can reserve full safe sides or Body Content columns. Port IDs and semantic silhouette anchors remain graph topology, not arbitrary components.

Templates place semantic content; regions do not dictate Title/Icon/etc. roles.
LOD inclusion is distinct from the representation that fits in final pixels.
A visible bar/dot still participates. Off+Stable reserves; Off+Reflow releases its slot. Structural bands/columns remain fixed reservations in 01A.

Do not add a second per-node layout cache, per-node Ctrl trees, a runtime JSON compiler, a demo-only allocator, or an unmeasured fine-grained dependency engine.

## EXPLICITLY NOT IMPLEMENTED BY 01A

- Native physical-Micro component hints. Micro still skips rich presentation entirely; its empty rich output is labelled in the demo. Do NOT enable rich callbacks at Micro to hide this gap.
- Image pyramids, progress/field/tag/action component renderers, regional auto-collapse, proxy fade/hysteresis or general auto-font fitting.
- Drag/drop region diagrams, full Studio V4 migration, save/open JSON or production C++ export.
- User-editable LOD thresholds. The new specimen uses the existing production resolver; it does not invent a second set of thresholds.
- Optimised wheel projection of prepared named glyphs/rasters across compatible component thresholds.

The existing `UiGraphDesignMatrix` remains available. Its selector smoke is still required; the new small editor is not claimed to replace all its functionality.

## FOCUSED WINDOWS DEBUG GATE / GARY

Repo: `E:\apps\github\upp_Ui`, branch `main`.
Fetch/pull current main. Confirm the supervisor's published 01A SHA is an ancestor of tested HEAD, not necessarily identical to it. Source ancestor `f12a255441361bf2f057b74753638146067d1420` is also required.

Use the established `E:\upp-18468\umk.exe`, `CLANGx64` method and current local nests. Exact example commands and manual checks are in `docs/UIGRAPH_NODE_COMPONENTS.md`.

Debug only:
1. Build/run `Utilities/UiGraphRenderTests` (now seven suites, including Components).
2. Build `examples/UiGraphDesignMatrix`; retain its Debug selector/startup smoke.
3. Build/run `examples/UiGraphComponentStudio`; inspect actual production previews.
4. `git diff --check`.

Report exact HEAD, build/test PASS/FAIL, test summary, first real blocker, demo PID if launched, and clean worktree YES/NO.
Minor mechanical CLANG fixes may be reviewed/committed/published. Stop on architecture, ownership, template-policy or nonlocal behavioural failures; report rather than redesign.
No Release, broad suite or 10k benchmark unless this focused gate exposes a reason.

## NEXT IMPLEMENTATION BOUNDARY

After Debug validation, implement a bounded native Micro hint contract with direct-overview/zoom-out parity and no rich resolver, glyph shaping, embedded controls or image processing during paint. Keep output in the existing prepared scene and reuse the accepted template/layout authority.

Then evolve the visible editor against the SAME production component definitions: region diagrams, component add/remove/bindings, measured proxy diagnostics, and eventually C++ output. Preserve explicit camera reset and the independence of node size, camera, thresholds, inclusion and representation.

## WORKFLOW

REFRESH -> INSPECT -> IMPLEMENT -> REVIEW -> PUBLISH -> VERIFY -> VALIDATE.
Full touched files and relevant callers/headers/tests/.upp first. Small coherent changes; publish recoverable source checkpoints. Refresh main before publishing and carry only our changes when it advances. No force pushes or leftover proof/final/published branches. Remote commit/tree verification is required after the write.
