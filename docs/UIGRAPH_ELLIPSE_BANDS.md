# UiGraph ellipse bands — production and authoring contract

## Using the workspace

New Media families enable **Fit ellipse Header/Footer**. Choose Ellipse to see the
production Header and Footer move outward where capacity permits. The diagram and
node preview use the same retained rectangles; this is not a guide-only offset.
Use Fit if the current camera clips the specimen; changing the band policy never
resets the camera or resizes the node.

In **Template / Layout > Regions**, edit:
- **Fit ellipse Header/Footer**: enabled/disabled layout policy.
- **Band width (%)**: 20..100 percent of the conservative band width, not node width.
  Narrower bands can use space nearer the top/bottom. Their heights are unchanged.

Edit Base to change the inherited family, or explicitly Create layout override to
change one shape. Component typography and band policy belong to layout, not the
independently inherited node-appearance section. Changing shape alone never creates
an override. The existing property transaction provides Undo and preserves ordinary
inspector selection/scroll; band controls do not reconstruct the model per toggle.

Old v1 saved families retain conservative geometry. Enable the checkbox explicitly
in Base or a detached shape. Save/Open retains the policy and width using strict
workspace schema v2; generated C++ emits the same fields. New-family defaults are
not applied to imported documents. Unknown/missing/invalid fields reject the candidate
without replacing the current family. See UIGRAPH_WORKSPACE_AUTHORING.md.

## Runtime capacity

```cpp
UiGraphNodeTemplate t;
t.ellipse_bands = true;
t.ellipse_band_width_percent = 80;
```

All slots must be identified components when this policy is enabled. Only non-Micro
Ellipse/Circle geometry uses the independent bands. Header/Footer preserve authored
height; each may narrow and move outward if its entire rectangle fits the actual
silhouette and avoids labelled port reservations. When the proposed move cannot fit,
that band keeps its conservative position. Other shapes and physical Micro retain
conservative capacity. This is bounded band fitting, not general shape packing or
arbitrary widget positioning. No extra per-node cache or paint-time solver exists.

The original presentation.safe is NOT inflated. It stays a contained conservative
rectangle. Header/Footer alone may extend beyond it. Body can reclaim newly free
vertical space while staying inside safe and between the bands. Evaluated rectangles
remain in NodeGeometry.presentation and use its existing camera projection.

Production paint and preview picking both use NodeComponentClip. Header/Footer clip
to their validated region, other components retain safe clipping. Synthetic snapshots
without a band fall back to safe. Legacy content hooks remain conservative; unnamed
legacy slots cannot enable bands. Thin proxy picking retains its slot-clipped tolerance.

Top/bottom labelled reservations block movement of the corresponding band; side
reservations remain protected. Port anchors, IDs and connections are unchanged.
The complete V8 four-side Body-only/Full-edge shared post-port Content/Overlay
contract is still outstanding. This band policy does not claim to implement it.

## Media text and code-pane corrections included in this gate

03B's font-height fitting was insufficient on its own: a preceding natural Subtitle
could already consume the minimum line space required by Title Fill. 03D2 resolves
rich inputs once and protects later readable minima within the SAME region before
allocating flexible natural rows. Fixed extents, authored header/node sizes, LOD masks
and readability floors are preserved. Paint consumes the prepared result. Micro does
not run the font-fit pass. A genuinely overfull header still reports limited capacity.

The old failing Media assertions remain. New integration tests repeat the enlarged
Subtitle/two-icons case on Rectangle and Ellipse with bands explicitly DISABLED and
then enabled. Thus independent bands cannot conceal failure of the text correction.
Failure logs include shape, policy, component ID, header/slot/content rectangles and
font metrics. Do not lower readability floors or delete assertions to obtain a pass.

03D1 moved the rail pages into one non-flow ParentCtrl host because UiBoxLayout
re-shows participating children. Code no longer shares height with an empty inspector.
The selected mode has a persistent contrasting checked face/frame/icon; this is not
mouse focus. The zoom caption also no longer treats its literal x as a format token.
Startup and integration tests exercise repeated mode changes and layout passes.

## Source checkpoints and validation boundary

- 35a2cf8: exclusive full-height rail, selected mode style and zoom caption.
- 08f09ba: protected readable row capacity; prior failing checks retained.
- 645a6b7: production bands, independent clip and EllipseBands render suite.
- e5a1f19: strict v2 persistence, v1 migration and generated C++ fields.
- 03D5 (commit containing this page): inspector, new Media defaults, shared picking,
  and WorkspaceBandTests integration. Recover exact SHA from ACTIVE_WORK/git log.

EllipseBands.cpp is RenderTests suite nine. It checks entire-band containment,
unchanged safe/camera/node size, paint outside the old clip, pan projection, policy
rejection, unchanged Rectangle, protected port reservations and Micro. WorkspaceTests
cover migration/export. WorkspaceBandTests covers actual inspector/preview integration.
These new source tests await Windows execution. The latest reported gate is still
FAIL at 1fa8793 (native view45/2), not a PASS for these later changes.

## Gary — one accumulated Debug validation task

Repository: Trilec/upp_Ui, E:\apps\github\upp_Ui, branch main.
Require the supervisor's latest 03D5 SHA as an ancestor, not exact HEAD equality.
Read ACTIVE_WORK and this page. Preserve unrelated work; stop on a dirty checkout.
Fetch and fast-forward current main using the established workflow.

```powershell
Set-Location E:\apps\github\upp_Ui
powershell -NoProfile -ExecutionPolicy Bypass `
    -File .\scripts\ValidateUiGraphWorkspace.ps1 `
    -RequiredAncestor '<latest published 03D5 SHA>' -Launch
if ($LASTEXITCODE) { throw 'Workspace gate failed; inspect evidence logs' }
```

Use established U++18468/CLANGx64, Debug only. Require all RenderTests (now nine
suites), WorkspaceTests, generated Ui/CtrlLib-only C++, native view, BAND_UI and
startup summaries to pass with positive counts. Do not impose the old export hash:
the fixture now contains explicit enabled/disabled band policies and shape widths.
The runner must propagate any native nonzero exit. The original Media tests are
not optional; a failure stops the gate before normal demo/manual acceptance.

After automated PASS, manually check:
1. Fresh Media at 1:1: two Left icons, Subtitle18/Bold, Asset name20 on Rectangle
   and Ellipse. Both text rows must be readable where minima fit. Narrow width
   elides; genuinely insufficient height reports a proxy/no-room result.
2. Ellipse toggle/width in Base: red/green guides AND production content move
   outward. Selection in the newly used outer header area updates the inspector;
   Delete/Undo still acts on the component, not node topology. Test Fit as needed.
3. Inherited shape rejects edits until explicitly detached. Save/Open preserves
   independent band widths and style; old v1 files stay conservative until enabled.
   Generated C++ contains the same ellipse fields and compiles unchanged.
4. Repeated Inspector/Template/Style/Code switching and resize: selected mode remains
   obvious after focus moves. Code occupies the full remaining rail without a blank
   PropertyEditor/filter. Return to inspector and confirm existing scroll behavior.

Minor mechanical CLANG fixes may be reviewed/documented/published and retested on
current main. Stop for architecture or ownership failures. Do not weaken tests,
force-push, add a rich-Micro fallback, run retired DesignMatrix, Release or a broad
benchmark. Report exact HEAD, ancestry, all summaries, first issue, manual findings,
screenshot/executable evidence, evidence directory, demo PID, diff check, clean
worktree and any fix SHA. Held-button Escape DND remains separately unverified.
