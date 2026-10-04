# Ui controls audit — 27 September 2026

## Result and scope

**Not a clean all-controls release sign-off.** The catalogue covers all 50 inventoried
Ui controls, but source review and focused native probes found shared-control defects.
The existing document geometry suite also has one failing assertion.

This is a first-pass source/API, ownership, theme, measurement and interaction audit
of all 50 public control entries, with targeted implementation inspection and the
native tests listed below. It is not a line-by-line review of every implementation,
a visual certification of every theme/state, or a generated-code/platform release pass.
The release inventory's more demanding `source_review` acceptance flags remain pending.

Source base: `7a3816e607eb5b563bbda7bcf78f3dbe54b43525`, with local documentation changes.
Windows; U++ at `E:/upp-18468`; CLANGx64; Release; BLITZ requested (packages marked
`noblitz` retain that policy). This audit did not change production control code.

## Findings to repair

### F01 — High: callbacks are not consistently safe when the caller rebuilds the UI

A user event may remove or destroy the originating control. Several implementations
then access members, emit another member event, or keep an invalid item index.
Source-confirmed examples:

- [UiButton::Activate_](../Ui/UiButton.cpp): WhenPush followed by WhenAction.
- [UiCheckBox::LeftDown](../Ui/UiCheckBox.cpp): action followed by EndIndicatorPress.
- [UiDropdown::ApplySelectionInternal](../Ui/UiDropdown.cpp): WhenSelect followed by
  fresh model/index reads. Clearing/rebinding the model in WhenSelect is also unsafe.
- [UiSplitButton::OpenPopupInternal](../Ui/UiSplitButton.cpp): reads state after WhenOpen.
- [UiSlider](../Ui/UiSlider.cpp), [UiRangeSlider](../Ui/UiRangeSlider.cpp),
  [UiDateTime](../Ui/UiDateTime.cpp): Changing followed by Action without lifetime checks.
- [UiScrollPanel](../Ui/UiScrollPanel.cpp): ApplyScroll emits WhenScroll; Layout,
  MouseWheel and SetScrollPos continue accessing the panel afterwards.
- [UiTab::LeftDown](../Ui/UiTab.cpp): WhenClose before Remove(index), and active-tab
  notifications before later indexed reads; a callback can change the tab list.
- [UiMenu::ActivateItem](../Ui/UiMenu.cpp): request/session callbacks precede more work;
  creating a Ptr only after EndSession cannot protect destruction during EndSession.
- [UiDoc::OnCoreChange](../Ui/UiDoc/UiDoc.cpp): WhenMapped followed by WhenChange.
- [UiNodeGraph::SetModel](../Ui/UiGraph/UiNodeGraphModelBinding.inc): WhenSelection
  followed by more member access.
- [PropertyEditor::ActivateRow](../Utilities/PropertyEditor/PropertyEditorInteraction.cpp):
  override/commit callbacks followed by transaction and repaint work.

Related paths occur in Breadcrumbs, Stack, Accordion, Splitter, RangeSliderEdit,
ColorMatrix's modal editor, BezierCurveEditor and the shared text engine.
This finding is from source inspection; destructive-callback crash reproduction was
not performed. Repair entire dispatch chains, copy event/value data before dispatch,
and use lifetime guards before subsequent work. The RangeSegments interaction code
already provides examples. Add delete/rebuild/rebind/reorder callback tests.

### F02 — High: layout registries can outlive their child relationship

[UiAbsoluteLayout](../Ui/UiAbsoluteLayout.cpp), [UiBoxLayout](../Ui/UiBoxLayout.cpp)
and [UiGridLayout](../Ui/UiGridLayout.cpp) retain raw child pointers without a
ChildRemoved reconciliation path. After reparenting, the old layout must stop
measuring or positioning that child. After destruction, dereferencing the retained
pointer can access freed memory. Grid's optional ValidateItems is not lifetime safety.

**Reproduced:** add a child at `(10,20,30,40)` to AbsoluteLayout, reparent it, set its
new rectangle to `(1,2,3,4)`, then call the old layout's Layout(). Its rectangle becomes
`(10,20,30,40)` again. No destruction was needed for this reproduction.
QuadSplitter's raw pane-slot registry needs the same detach audit. DirectContentHost,
GroupPanel, TitleCard and Stack show stronger observer/removal patterns to follow.

### F03 — Medium: shared edit notifications and modifier handling

[UiBaseEdit::Key](../Ui/UiBaseEdit.cpp) calls RemoveSelection(), which emits WhenChange,
then emits it again for Delete/Backspace. Replacing selected text also publishes an
intermediate deletion before insertion. Cut/Paste keyboard branches add notifications
around helpers that already notify. This affects the edit family and inspector editors.

**Reproduced:** `UiLineEdit("abcd")`, select `[0,2)`, send Delete: **two** changes for
one edit. Emit once after the complete operation; keep undo and preview boundaries clear.
Also, switching on `key & ~K_SHIFT` makes the explicit Shift+Delete/Shift+Insert cases
unreachable as written. Verify cut/paste shortcuts without consuming the modifier first.

### F04 — Medium: integer spin overflow occurs before clamping

[UiIntEdit::OnSpinUp/OnSpinDown](../Ui/UiIntEdit.cpp) add/subtract step in `int` before
range checks. **Reproduced:** range `[0, INT_MAX]`, value INT_MAX, step1, Up -> **0**.
Use checked/wider arithmetic before clamping/wrapping; cover both ends, large steps,
Null and loop mode. Float/non-finite validation merits corresponding boundary tests.

### F05 — Medium: oversized scrollbar page gives an invalid position

[UiScrollBar::SetRange/SetPos](../Ui/UiScrollBar.cpp) use `max_ - page_` as the upper
bound even when it is below `min_`. **Reproduced:** SetRange(10,20,100) -> position
**-80**, expected10. Normalize the effective upper position consistently in setters,
geometry and pointer conversion; cover empty ranges and a page larger than the range.

### F06 — Medium: SliderEdit ignores a minimum set on only one axis

[UiSliderEdit::GetMinSize](../Ui/UiSliderEdit.cpp) only uses the user minimum when
both dimensions are positive. **Reproduced:** SetMinSize(Size(0,360)) -> **(248,30)**
on this machine. Merge each requested minimum with the natural size independently,
as RangeSliderEdit does. Add width-only, height-only, reset and both-axis cases.

### F07 — Medium: Bézier editor bypasses its resolved theme

[UiBezierCurveEditor](../Ui/UiBezierCurveEditor.cpp) resolves the current theme in
GetStyle(), but Paint, hit testing and coordinate transforms read `style_` directly.
**Reproduced:** Light/Dark resolved axis colours differ, but the rendered 200x100
images are byte-identical. ClearCustomStyle likewise cannot restore theme-driven
painting reliably. Use one effective style consistently while preserving explicit
flip settings. UiBezierCurveField embeds this editor and inherits the problem.

### F08 — Medium: document metadata geometry test fails

[UiDocGeometryTest](../Utilities/UiDocGeometryTest/main.cpp) reports **34 checks,
1 failure**: “expanded metadata edit immediately remeasures the visible reference card”.
The metadata payload/state assertions pass, but the expected extra layout height does
not. Root cause is not yet isolated between measurement/invalidation and the test's
height expectation; do not label it fixed or weaken the assertion without investigation.
The other six document targets pass.

### F09 — Fixed documentation/tooling issue: UMK assembly argument

The local UMK does not load GitHubOut.var when supplied as its assembly argument;
it reports a missing package. GETTING_STARTED and the development skill showed that
command, and ValidateUiRelease passed it too. They now use comma-separated nests and
an explicit output-cache directory; the release runner parses its supplied .var,
including generated header-probe assemblies. Native audit builds verified that command
form. The runner's parser self-test passes12 checks; its clean-checkout release gate
was not bypassed or claimed as run.

### F10 — Follow-up: capture cancellation and visual coverage

Slider, RangeSlider, BezierCurveEditor, Tab and Splitter have drag state but no
control-specific CancelMode override. Several other controls clear pressed state on
LostFocus but need capture-interruption tests. Check hide/disable, popup opening,
window deactivation and capture theft without a matching LeftUp. This is a source
coverage concern, not a reproduced failure in this audit.

A full native visual matrix remains open: Minimal/Pill/Linear/Solid, Light/Dark,
normal/hover/focus/selected/disabled, small bounds and DPI scaling. Include rounded
collection viewports, dropdown/caret alignment and the colour picker/PropertyEditor.
No whole-library visual PASS is inferred from theme resolver tests.

## Control-by-control register

“Source review” below means targeted inspection of the public contract and relevant
implementation paths. Passing a named suite covers its assertions, not every API.
All controls have current catalogue/header entries. Shared findings apply through
inheritance/composition even where a row has no separate dedicated test.

| Control | Runtime evidence this audit | Review notes |
|---|---|---|
| [UiLabel](../Ui/UiLabel.h) | Control tests (icon sizing) | Text/icon measurement and selection/access-key paths inspected; full text selection and wrapping visuals remain manual. |
| [UiButton](../Ui/UiButton.h) | Control tests | Interaction insets/icon size contracts pass; Activate_ dispatches WhenPush then WhenAction without a lifetime guard (F01). |
| [UiToolButton](../Ui/UiToolButton.h) | Control tests | Inherits button activation risk F01; separate compact minimum baseline is tested. |
| [UiSplitButton](../Ui/UiSplitButton.h) | Control tests, shared button checks | Close selection path guards lifetime; opening path reads state after WhenOpen without a guard (F01). |
| [UiCheckBox](../Ui/UiCheckBox.h) | Theme/shared contracts only | Indicator measurement honours user minimum; LeftDown calls EndIndicatorPress after a user action (F01). |
| [UiRadioButton](../Ui/UiRadioButton.h) | Theme/shared contracts only | Sibling exclusivity and shared indicator geometry inspected; full group keyboard behaviour not exercised. |
| [UiToggle](../Ui/UiToggle.h) | Theme/shared contracts only | Animation uses lifetime observers; pressed-state/capture cancellation deserves targeted interaction coverage (F10). |
| [UiBreadcrumbs](../Ui/UiBreadcrumbs.h) | No dedicated run | Natural text/icon extent inspected; LeftDown refreshes after WhenAction (F01). |
| [UiLineEdit](../Ui/UiLineEdit.h) | Shared edit probe | Selected Delete emits two notifications through UiBaseEdit (F03); Enter dispatch inspected. |
| [UiIntEdit](../Ui/UiIntEdit.h) | Boundary probe | Stepping above INT_MAX produced 0 instead of clamping (F04); inherits shared edit risks. |
| [UiFloatEdit](../Ui/UiFloatEdit.h) | Composite/shared tests | Null/range/spin paths inspected; inherits shared edit risks; finite/extreme arithmetic needs dedicated cases. |
| [UiPasswordEdit](../Ui/UiPasswordEdit.h) | Shared edit review | Visibility flank/callback and inherited masking inspected; clipboard policy and theme-switch visuals need dedicated cases. |
| [UiMultiEdit](../Ui/UiMultiEdit.h) | Shared edit review | Shared edit risks F01/F03; minimum size overwrites the base result using stored style fields—geometry/theme consistency needs coverage. |
| [UiMaskEdit](../Ui/UiMaskEdit.h) | Shared edit review | Formatter path continues after base Key callbacks (F01); modifier/selection behaviour needs dedicated tests. |
| [UiSlider](../Ui/UiSlider.h) | Control tests | Value/geometry contracts pass; sequential Changing/Action notifications are unguarded (F01); capture interruption F10. |
| [UiRangeSlider](../Ui/UiRangeSlider.h) | Control tests | Range/bounds geometry contracts pass; same callback and capture-interruption concerns as slider (F01/F10). |
| [UiSliderEdit](../Ui/UiSliderEdit.h) | Minimum-size probe | Height-only minimum ignored (F06); composed slider/edit event paths need reentrancy coverage. |
| [UiRangeSliderEdit](../Ui/UiRangeSliderEdit.h) | Control tests | Per-axis minimum implementation is stronger than SliderEdit; field commit continues after Changing/Action (F01). |
| [UiRangeSegments](../Ui/UiRangeSegments.h) | RangeSegments tests | 60 checks pass; interaction uses copied events and lifetime guards, useful reference for F01/F10 repairs. |
| [UiScrollBar](../Ui/UiScrollBar.h) | Oversized-page probe | Page greater than range gives out-of-range position (F05); SetPos refreshes after scroll callback (F01). |
| [UiProgressBar](../Ui/UiProgressBar.h) | Control tests | Value, geometry, indeterminate and style lifecycle pass; owned animation lifecycle inspected. |
| [UiProgressRing](../Ui/UiProgressRing.h) | Control tests | 60 checks pass; ticker stops when hidden/closed; raster cache and role resolution covered. |
| [UiChartRing](../Ui/UiChartRing.h) | Control tests | 39 checks pass; chart data, segment geometry, role/text and raster cache covered. |
| [UiMatrixSelector](../Ui/UiMatrixSelector.h) | Control tests | 91 checks cover presets, selection, geometry, pairs, keys and styles; capture interruption still needs explicit cases. |
| [UiColorMatrix](../Ui/UiColorMatrix.h) | Control tests | Values/slots/geometry pass; modal EditColors captures owning control across user callbacks (F01). |
| [UiDateTime](../Ui/UiDateTime.h) | Control tests | 25 checks pass; parsing/null/range/presentation covered; sequential Changing/Action is unguarded (F01). |
| [UiColorPicker](../Ui/UiColorPicker/UiColorPicker.h) | Control tests via ColorMatrix | Eight-slot contract covered; complete modal/live preview, cancel and all pages not visually exercised. |
| [UiDropdown](../Ui/UiDropdown.h) | Dropdown interaction, model-view tests | Popup-close dispatch is guarded; Select reads current model after WhenSelect (F01), unlike close path. |
| [UiMenu](../Ui/UiMenu.h) | Menu interaction, model-view tests | 26 native interaction checks pass; ActivateItem continues after action request/session callbacks (F01). |
| [UiPanel](../Ui/UiPanel.h) | Shared geometry contracts | Child measurement and theme resolution inspected; catalogue corrected: ordinary child surface, not enforced single-root host. |
| [UiDirectContentHost](../Ui/UiDirectContentHost.h) | Release smoke: no captured summary | Ptr child and parent/cycle checks inspected; do not count smoke exit alone as tested acceptance. |
| [UiGroupPanel](../Ui/UiGroupPanel.h) | Control tests | 87 checks cover slots, placement, measurement and serialization; ChildRemoved clears borrowed slots. |
| [UiTitleCard](../Ui/UiTitleCard.h) | Source review | ChildRemoved clears content cell; theme/layout/action implementation inspected; comprehensive style and small-size visuals remain open. |
| [UiStack](../Ui/UiStack.h) | Control tests | 11 measurement checks pass; Ptr pages and ChildRemoved exist; page-event chains remain reentrant risks (F01). |
| [UiAccordion](../Ui/UiAccordion.h) | Source review | Owned section controls and animation shutdown inspected; Remove invokes callback before more policy/layout work (F01). |
| [UiScrollPanel](../Ui/UiScrollPanel.h) | Source review; smoke output unconfirmed | Dedicated viewport clips children; ApplyScroll invokes user callback while callers continue touching members (F01). |
| [UiTab](../Ui/UiTab.h) | Control tests | 11 measurement checks pass; close callback precedes indexed Remove, and selection callback precedes indexed drag setup (F01/F10). |
| [UiSplitter](../Ui/UiSplitter.h) | Source review | Measures current children; LeftUp refreshes after WhenSplitFinish (F01); cancellation coverage F10. |
| [UiQuadSplitter](../Ui/UiQuadSplitter.h) | Source review | Composes three splitters; external pane removal leaves slot registry raw pointers (F02 follow-up). |
| [UiAbsoluteLayout](../Ui/UiAbsoluteLayout.h) | Reparenting probe | Old parent still repositions detached child (F02); raw registry also exposes lifetime risk. |
| [UiGridLayout](../Ui/UiGridLayout.h) | Source review, shared geometry tests | Raw item pointers and no ChildRemoved reconciliation (F02); ValidateItems detects wrong parent but cannot safely inspect a freed pointer. |
| [UiBoxLayout](../Ui/UiBoxLayout.h) | Source review, shared geometry tests | Raw item pointers used by measurement and placement without detach reconciliation (F02). |
| [UiList](../Ui/UiList.h) | Model-view tests | Fit now follows row count; style, remapping, rendering and model contracts pass; retain callback-reentrancy review work. |
| [UiTree](../Ui/UiTree.h) | Model-view tests; interactive harness build | Visible-row navigation/model/scale source inspected; interactive RunTests was not completed automatically. |
| [UiTable](../Ui/UiTable.h) | Model/binding tests; interactive harness build | Selection/edit/model paths inspected; interactive RunTests not completed; rounded header/data viewport visuals still need review. |
| [UiGallery](../Ui/UiGallery.h) | Model-view tests | Regression and model-view coverage passes; selection/marquee cancellation and model notification paths inspected. |
| [UiDoc](../Ui/UiDoc/UiDoc.h) | Seven document targets | Geometry target fails one expanded-metadata height assertion (F08); six other document targets pass. Notification chain risk F01. |
| [UiBezierCurveEditor](../Ui/UiBezierCurveEditor.h) | Light/Dark raster probe | Resolved palette changes but rendered pixels are identical: Paint/hit geometry bypass GetStyle (F07). Callback/capture risks F01/F10. |
| [UiBezierCurveField](../Ui/UiBezierCurveField.h) | Source review | Composes editor, formula and copy button; inherits curve appearance issue F07; no dedicated current example in inventory. |
| [UiNodeGraph](../Ui/UiGraph/UiNodeGraph.h) | Graph model/view/render aggregates | 18 suites pass; model switch continues after selection notification (F01). Full graph scale/profile/platform acceptance was not rerun. |

## Build/run evidence

All 22 targets below compiled. Eighteen completed with passing test summaries; one
failed, one exited without a captured summary, and two are interactive harnesses.
Interactive processes launched by this audit were stopped at the 60-second timeout;
existing user applications were not closed. An interactive timeout is not a control failure.

| Target (under Utilities/) | Result | Captured evidence |
|---|---|---|
| UiControlTests | Pass | 13 suites; 552 checks; zero failed suites |
| UiReleaseSmoke | Unconfirmed run | Exit 0; stdout summary absent. No check count accepted. |
| UiModelTests | Pass | Checks: 7535; Checks: 55, Fails: 0; Checks: 31, Fails: 0; UI_MODEL_TESTS_SUMMARY suites=3 failed_suites=0 |
| UiModelViewTests | Pass | Checks: 52, Fails: 0; Checks: 11, Fails: 0; Checks: 12, Fails: 0; UILIST_STYLE_CONTRACT_SUMMARY checks=15 failed=0; Checks: 11, Fails: 0; UI_MODEL_VIEW_TESTS_SUMMARY suites=5 failed_suites=0 |
| UiDrawingTests | Pass | UI_GEOMETRY_CONTRACT_SUMMARY checks=28 failed=0; UI_SHAPE_PATH_SUMMARY checks=27 failed=0; UI_STYLED_SURFACE_CACHE_SUMMARY checks=16 failed=0; UI_DRAWING_TESTS_SUMMARY suites=3 failed_suites=0 |
| UiThemeTests | Pass | UI_THEME_STRUCTURE_SUMMARY checks=1092 failed=0; Checks: 13, Fails: 0; UI_THEME_TESTS_SUMMARY suites=2 failed_suites=0 |
| UiRangeSegmentsRunTests | Pass | UIRANGESEGMENTS_SUMMARY checks=60 failed=0 |
| UiTreeRunTests | Build only; interactive | Run() window needs user-driven test actions; no completed test result. |
| UiTableRunTests | Build only; interactive | Run() window needs user-driven test actions; no completed test result. |
| UiDropdownInteractionTest | Pass | Checks: 4, Fails: 0 |
| UiMenuInteractionTest | Pass | Checks: 26, Fails: 0 |
| UiDocCoreTest | Pass | SUMMARY passed=84 failed=0 total=84 |
| UiDocGeometryTest | Fail | [FAIL] expanded metadata edit immediately remeasures the visible reference card; UIDOC_GEOMETRY_SUMMARY checks=34 failed=1 |
| UiDocInteractionTest | Pass | UIDOC_INTERACTION_SUMMARY checks=41 failed=0 |
| UiDocModelTest | Pass | Checks: 151; UIDOC_MODEL_SUMMARY cases=17 case_pass=17 case_fail=0 checks=151 failed=0 |
| UiDocImageTest | Pass | UIDOC_IMAGE_SUMMARY checks=75 failed=0 |
| UiDocMetadataTest | Pass | UIDOC_METADATA_SUMMARY checks=47 failed=0 |
| UiDocModelBindingTest | Pass | UIDOC_MODEL_BINDING_SUMMARY checks=22 failed=0 |
| UiGraphModelTests | Pass | UIGRAPH_HIERARCHY_SUMMARY checks=49 failed=0; UIGRAPH_MODEL_TESTS_SUMMARY suites=2 failed_suites=0 |
| UiGraphViewTests | Pass | UINODEGRAPH_CANONICAL_SHAPE_SUMMARY checks=14 failed=0; UINODEGRAPH_HIERARCHY_VIEW_SUMMARY checks=30 failed=0; UINODEGRAPH_INTERACTION_STATE_SUMMARY checks=29 failed=0; UINODEGRAPH_SELECTION_MODIFIER_SUMMARY checks=13 failed=0; UINODEGRAPH_ROUTE_EDIT_SUMMARY checks=25 failed=0; UINODEGRAPH_DRAG_DAMAGE_SUMMARY checks=8 failed=0; UINODEGRAPH_LIVE_VIEW_SUMMARY checks=19 failed=0; UIGRAPH_VIEW_TESTS_SUMMARY suites=7 failed_suites=0 |
| UiGraphRenderTests | Pass | UINODEGRAPH_DETAIL_LOD_SUMMARY checks=19 failed=0; UINODEGRAPH_OVERVIEW_LOD_SUMMARY checks=12 failed=0; UINODEGRAPH_RENDER_LOD_SUMMARY checks=16 failed=0; UINODEGRAPH_PATTERNED_PAINT_SUMMARY checks=10 failed=0; UINODEGRAPH_PRESENTATION_SUMMARY checks=87 failed=0; UIGRAPH_EXECUTION_PATH_SUMMARY checks=8 failed=0; UIGRAPH_COMPONENT_SUMMARY checks=21 failed=0; UIGRAPH_WORKSPACE_COMPONENT_SUMMARY checks=24 failed=0; UIGRAPH_ELLIPSE_BANDS_SUMMARY checks=16 failed=0; UIGRAPH_RENDER_TESTS_SUMMARY suites=9 failed_suites=0 |
| PropertyEditorTests | Pass | 143 checks; 0 failures |

PropertyEditor source review also checked model-binding generations, inline editor
ownership and the boolean override/commit path. Its 143-check suite passes; callback
reentrancy from F01 remains a separate issue. Other optional/platform utility packages
and PropertyEditor specialised suites are outside this first pass.

Raw build/run logs and the six-case native probe are retained locally in
`E:/apps/github/upp_uidesigner/build/control-audit/`; the control aggregate logs are
`build/control-audit-build.log` and `build/control-audit-run.log` in that checkout.
These are audit evidence, not the distributed Designer executable.

## Repair order and remaining acceptance

1. Fix shared callback dispatch and detached-child registries first; add destruction,
   model mutation and reparenting tests at the owning control layer.
2. Repair the reproduced numeric, scroll-range, edit-notification, minimum-size and
   Bezier-theme defects with focused regressions. Investigate the document failure.
3. Exercise interrupted capture and run the native visual matrix, including the
   interactive tree/table harnesses. Then rerun affected Debug/Release tests and the
   clean-checkout release runner (headers, relevant BLITZ/non-BLITZ and examples).

Documentation cleanup retained historical acceptance notes, corrected the Panel
contract and UMK instructions, added missing recent contracts, and linked the skills.
All 50 catalogue controls match the inventory. No tests, examples or historical evidence
were deleted merely because their names looked old. No production-code defects listed
above were repaired by this audit, and no commit or publication was performed.

## Gallery reuse audit — 4 October 2026

Scope: all six UiGallery implementation/header files, shared UiItemRender and
UiListModel seams, Gallery demo and the focused binding/mutation/scale suites.
Reviewed on `b86e598` plus the uncommitted changes from this audit; U++ 18468,
CLANGx64, Windows. This section supersedes the earlier Gallery row only; it does
not certify other controls or close the Graph development gates.

The architecture is appropriate for reuse: one active semantic model, owned
renderer prototypes, visible/overscan-only lightweight renderers, arithmetic
uniform geometry, preparation outside Paint, host-owned lazy assets and no item
Ctrl population. Model switches do not copy datasets. Explicit external-model
lifetime and GUI-thread notification contracts remain part of the public API.

Repairs implemented:

- Release surplus renderers and their image handles on viewport/range shrink and
  empty models. Cyclic slots retain overlapping data; a one-row scroll rebinds
  only the entering row when the prepared range size is unchanged.
- Reconcile UI_MODEL_UPDATE selection over its changed range rather than scanning
  all selected records on every local presentation edit. Uniform geometry stays
  intact; prepared data invalidation remains bounded to the renderer pool.
- Map dirty paint bounds directly to grid row/column candidates and respect
  Draw::IsPainting, so a tile-sized damage region paints one tile.
- Complete state/refresh work before user callbacks. Guard continuation with a
  destruction-aware Ctrl observer and a structural serial. Model replacement,
  structural edits and destruction cancel pending input; lazy Touch notifications
  can update assets during scrolling without suppressing cursor selection.
- Capture the opening background press; cancel marquee without recursive capture
  teardown. Escape restores the original cursor as well as selection. Structural
  edits restore old indices before remapping them into the edited model.
- Enforce single-selection when binding ValueArray tokens; Home/End skip disabled
  items/group headers. Reject Null/non-finite zoom and unrepresentable tile sizes.
- Fix the demo's member declaration order so its borrowed model outlives views.
  Share the same Gallery regression implementation between the standalone and
  consolidated targets, eliminating the duplicate corrective test body.

Validation on the final edited sources:

| Check | Result |
| --- | --- |
| UiModelViewTests Debug, non-BLITZ | 5 suites / 0 failed; includes 100,000-item Gallery reachability and bounded renderer/paint tests |
| Gallery regressions, Debug non-BLITZ and Release BLITZ | 29 checks / 0 failed in each configuration |
| UiModelTests Debug, non-BLITZ | 3 suites / 0 failed; data models 7,535 checks, binding 55, mutation 31 |
| UiGalleryDemo Debug BLITZ | Build passes |
| git diff --check | Pass |

Build/test logs and executable artifacts are under `build/GalleryAudit_*` in this
checkout. Source-only hygiene does not establish native visual acceptance. The
Computer Use demo launch timed out waiting for app approval, and re-observation
confirmed no demo window opened; no visual/physical interaction PASS is claimed.

Remaining finish gates: inspect Light/Dark/Light, resize, focus/disabled states,
Ctrl/Shift marquee, mouse-up outside the viewport and Escape while held at
representative DPI. UiGalleryDemo remains a List/Gallery shared-model showcase;
it does not yet provide the full Inspector/Theme Overrides/Data/Code experience
or generated-code acceptance required by the demo guide. The Designer catalogue
is outside this checkout, so Designer authoring/export is not certified here.

Scale limits are explicit: uniform tiles, 32-bit pixel extent saturation at
INT_MAX, GUI-thread mutations, and models that outlive binding. Explicit Select
All/token resolution/structural changes may be O(N); marquee selection work grows
with the intersected selection. No unlimited-coordinate, zero-risk or completed
Graph claim follows from this Gallery audit. The control is suitable for reuse
within these documented contracts; overall finish status remains pending the
native/demo gates above.

## Collection designer and shared presentation audit — 4 October 2026

This follow-up replaces the separate UiListDemo/UiGalleryDemo packages with
`examples/UiCollectionDemo`. Both previews bind the same production UiListModel;
Inspector, Style, Data and Code use PropertyEditor and a projection of the active
record. The projection is not another collection model. The catalogue and release
inventory now point both controls to this example. Historical evidence above is
retained as evidence of the earlier state.

The old comparison exposed a reusable layout defect: vertical Expand used the
collection's full natural height and did not shrink a deficit. That made the
viewport, renderer population and wheel distance much larger than the visible
window. UiBoxLayout now shrinks expanding children to explicit minimums before
distributing spare space. List gains a production scrollbar, releases surplus
renderers, retains overlapping records with cyclic slots and respects paint
damage. Fit/Fixed allocation is preserved. Native header icons are explicitly
16 DPI-scaled pixels and page icons 17, matching UiLabelDemo.

Validation:

- UiModelViewTests: five suites pass, including the new 100,000-record Expand
  viewport regression and existing List/Gallery scale tests.
- UiStackMeasureTest and UiTabMeasureTest: 11 checks each, zero failures, Debug
  non-BLITZ.
- UiCollectionDemo Debug BLITZ builds; native acceptance: 12 checks, zero failures.
- Five generated standalone sources (default, configured, overrides, Gallery,
  List) compile unchanged, including quoted Data text. Generated configuration
  deliberately leaves dataset size and image loading to the host.
- Repeated synthetic wheel preparation: 100 iterations, about 5.1 ms List and
  3.5 ms Gallery total, pools 17/27 in the acceptance viewport. This excludes
  painting and physical input latency; it is not a frame-rate claim.
- The corrected executable was opened on the visible desktop and its icon sizes
  inspected; native List wheel input advanced the bounded viewport. Dark-theme
  inspection exposed a light window background behind pale header buttons. The
  demo now paints its resolved theme surface and tints title media; the polished
  build passes the same 12 acceptance checks. Final physical Dark-theme recheck
  yielded to active user interaction. Logs/artifacts: `build/CollectionAcceptance*`,
  `build/Collection_*Measure*`, `build/UiCollectionDemo*`.

### Can Graph, Gallery and List share node families?

Yes, at the presentation layer. They do not currently have one unified family
authority. No Graph schema or evaluator was changed by this audit.

| Existing seam | What it already provides | Boundary to resolve |
| --- | --- | --- |
| UiItemRender and UiMakeItemRenderData | List/Gallery share model-to-render data, cloneable lightweight prototypes and viewport pools | No Graph family/template adapter exists |
| UiMediaCardRender | List/Gallery can use the same media card; live UiMediaCard and its renderer share preparation/painting | Graph's Media family uses its separate component evaluator |
| UiGraphNodeTemplate and slot rules | Validated bounded recipes, stable component IDs, data-key bindings, component styles and multiple families | Names/types and evaluator input depend on Graph nodes/styles |
| UiGraphNodeComponent preparation | Text, icons, images, progress, fields, tags and actions become prepared output outside Paint | Preparation takes UiGraphNode/UiGraphNodeStyle; Graph owns region allocation and painting |
| UiGraphNodePresentation | Retained output drives rendering and authoring picking | Shape-safe bands, port reservations, camera zoom and LOD are Graph-specific |

Evidence: `Ui/UiItemRender.h`, `Ui/UiListRender.cpp`,
`Ui/UiGalleryRender.cpp`, `Ui/UiMediaCard.h/.cpp`,
`Ui/UiGraph/UiGraphNodeComponent.h/.cpp`, `Ui/UiGraph/UiNodeGraph.h`,
and the production template/presentation contracts in guides 08 and 09.
Graph Media and MediaCard also implement image fit separately; the shared
UiMediaFit utility is used by MediaCard, but not this Graph component path.
Changing this requires parity checks for cropping, alignment and rounding.

Recommended extraction, in dependency order:

1. Extract a view-independent, bounded presentation recipe, component data
   resolver and prepared paint output from the existing Graph evaluator. Keep
   existing public Graph templates through a compatibility adapter. Avoid copying
   its region allocator into a second renderer implementation.
2. Retain Graph's shape/port allocation, camera, LOD, topology and selection.
   Provide ordinary rectangular allocation for collection rows/tiles; family
   intent is shared, while the allocated footprint differs with available space.
3. Adapt UiModelItem/UiItemRenderData at preparation time into the same resolver.
   Do not populate a parallel UiGraphModel or convert all N records. Arbitrary
   typed fields remain in the existing Value data payload with explicit bindings.
4. Put default families and their styling in one registry/recipe authority used
   by all adapters. Extension authors provide shared recipes or bounded painted
   components; per-record heavyweight Ctrl instances are not the default path.
   Map selection/focus/disabled states explicitly and reserve List chrome lanes.
5. Add a UiItemRender implementation backed by the shared evaluator, then
   consolidate MediaCard only where its richer tag/action contracts are preserved.
   Action identity/hit routing must remain view-owned and destruction-safe.

Required acceptance before replacing the current engines: Graph rendering and
picking parity at every LOD/shape; Media Contain/Cover parity; Light/Dark and
selected/disabled/focus states; custom family registration and missing/invalid
field behavior; generated-code compatibility; collection dirty paint and pool
shrink; one-row overlap retention; local model Touch; 100,000-record bounded
preparation; no image decode/font fitting/data callbacks during Paint; interrupted
actions and model replacement/destruction. Template/theme changes invalidate only
affected prepared state. Prepared resources remain viewport-bounded and image
loading stays host-owned.

The architecture makes this feasible, but performance equivalence has not been
measured for a Graph-backed collection renderer because none exists yet. This is
an extraction proposal, not a certification of unified nodes or a 100%-finished
Graph. The remaining native Gallery capture/DPI matrix from the earlier section
and a full List callback audit are still open.

### Shared node foundation: detailed follow-up

This source audit refines the extraction proposal above. The target is one
content contract and one design/evaluator/drawing authority, not permanent
translation between independently maintained Graph and collection node systems.
Compatibility forwarding is a migration detail. No production API, serialization
or rendering implementation is changed by this follow-up.

**Base class versus shared values.** A lightweight rendering interface already
exists: UiItemRender explicitly names later Graph node content as an intended
consumer. Reuse/evolve that interface rather than introduce a second renderer
hierarchy. Common semantic content and authored designs should be plain value
types, separate from transient prepared geometry. Graph can contain/reference
common content and add graph identity, position, ports and connections. List and
Gallery add sequence identity and their own placement/interaction. Composition
is the recommended default for these records; deriving a Graph control from a
Gallery/List control would couple unrelated lifetimes and interaction. A simple
non-virtual data base could also work, but alone would not unify drawing, binding,
serialization or notifications. Preserve U++ relocation/copy contracts explicitly.

Confirmed differences that a common contract must resolve:

| Concern | Current source behavior | Proposed single authority |
| --- | --- | --- |
| Main text | UiModelItem.text maps to UiItemRenderData.title; Graph stores title directly | One content title, with old spelling retained only for compatibility |
| Subtitle | Render data and Graph have it; UiModelItem does not | Common optional subtitle, without inventing a second payload key |
| Media | Collection data has image; named Graph Image components require a literal or data-key Image | Common image/icon fields and explicit custom-field binding precedence |
| Arbitrary fields | Both use Value data; Graph named binding expects ValueMap | Retain opaque host data; validate ValueMap only for a component that requests named fields |
| Styling | Graph has its own role enum/style plus slot styles; collections use UiRole/UiItemRenderStyle | Common presentation roles, component styling and family recipe; Graph-only port styles remain extra |
| Identity | Graph refs, collection keys and action values have different meanings | Content identity separate from view/topology identity and action identity |
| Designs | Graph registered templates are held per UiNodeGraph; Workspace owns a Family with Base/shape overrides | One explicitly owned, shareable family/design catalogue used by the hosts and authoring tools |
| Interaction | UiItemRenderHit has an action Value; Graph Actions currently paint string chips | Shared component/subitem hit IDs and action tokens; each view owns dispatch/capture |

Important limits from the source: UiGraphNodeComponentKind is a closed enum.
Users can register new compositions of existing components, but arbitrary new
component kinds are not currently a plug-in registration API. Graph Actions
currently accept arrays of strings and prepare boxes/text; UiGraphNodeComponentItem
has no action token. WhenNodeAction is a node double-click event, not per-chip
action routing. Therefore custom components and actionable chips need an explicit
extension/interaction contract before promising them across views.

**One record in several views.** A common struct alone removes vocabulary drift,
but placing copies of it into two independently edited models would still diverge.
For simultaneous Graph/List/Gallery editing, retain one host-owned authoritative
content record and let topology/sequence entries refer to it by stable identity.
Content revisions notify each bound view; each prepares only its affected visible
records. This can be an access path into the host's existing model, rather than a
mandatory new datastore. Prepared snapshots may retain String/Value/Image handles,
as the existing renderer does, but those are disposable rendering state, not
another editable content model. Value sharing alone must not be treated as a
mutable shared-record/notification mechanism. Define removal, model replacement
and borrowed-reference lifetime before implementing this access path.

**Shared design does not mean fixed pixel bounds.** The same family should retain
component identities, bindings, order, alignment and styling. A 300-pixel Graph
node and a 100-pixel Gallery tile may legitimately wrap/hide content according to
that same recipe. A narrow List row need not acquire a separately authored List
family. The host supplies allocated bounds, exclusions and scale/detail policy.
Shared content-region allocation then prepares identical drawing/picking output
for equivalent inputs. Graph supplies shape-safe regions and port exclusions;
List supplies its chrome reservations; Gallery usually supplies a rectangle.
Silhouette math should use existing Ui shape/geometry primitives where practical,
while preserving Graph's dense-scene paths and Micro budgets.

Concrete implementation slices, with checkpoints:

1. Establish the common content/style/state contract by evolving the current
   render data vocabulary. Preserve legacy Graph field access and its explicit
   stream order; do not serialize a new base blindly. Test old Graph model load,
   missing fields and image/custom-key precedence. This is where any migration
   field forwarding belongs.
2. Extract the existing named component resolver, prepared types and painter
   from Graph-specific inputs. Let Graph and a UiItemRender implementation call
   those same functions. Keep Graph legacy callbacks and Micro behavior working.
   Verify Media and Identity families in both hosts with identical input/bounds.
3. Extract content region allocation from BuildNodePresentation, with host-owned
   shape/port/chrome constraints as inputs. Promote the current template
   validation and registration/prepared-resource logic into the shared design
   catalogue. Move Workspace Base/shape inheritance and generated code to that
   authority instead of duplicating a Gallery design editor.
4. Bind both hosts to the same authoritative content access path and revisions.
   Demonstrate an edit made through Gallery updating Graph and List without
   copying datasets or refreshing every record. Exercise replacement/removal and
   callback destruction. Selection and camera/scroll remain independent.
5. Add registered painted component extensions only after the built-in path has
   parity. Require bounded preparation, stable hits and no Ctrl per record.
   Consolidate Basic/Image/MediaCard code incrementally; preserve MediaCard tag
   semantics and live-control behavior before retiring duplicate implementation.

Main footprint opportunities are GraphNodeComponent resolution/painting,
BuildNodePresentation content allocation, shared family authoring/validation,
Basic/Image/MediaCard text/media layout and separate media-fit calculations.
Topology, spatial queries, ports, scrolling and selection remain useful distinct
code. Moving files/names alone saves no implementation; quantify removed duplicate
logic only after parity permits deleting the old paths. Do not promise a line
count or speedup from this source audit.

Later consumers are already evidenced by SetItemRender/SetCellRender APIs in
UiTree, UiTable and UiDropdown. They can opt into the shared renderer without
having to turn ordinary cells or menu rows into rich nodes. Live MediaCard is
another natural consumer of the common prepare/paint functions. The shared core
must not depend on UiNodeGraph, UiGraphWorkspace or PropertyEditor; authoring sits
above it. Keep preparation bounded by visible records and recipe component limits,
cache immutable recipe resources once, and avoid a virtual call/lookup per painted
primitive. Final performance acceptance must measure Graph and collections,
including cold media preparation, warm pan/scroll and local content/design edits.

Additional reviewed sources: UiDataModels.h; UiItemRender.cpp/Data.cpp;
UiGraphModel.h/.cpp; UiGraphNodeTemplate.h; UiNodeGraphTemplates.cpp;
UiNodeGraphPresentation.inc; UiGraphNodeComponentPaint.cpp;
UiNodeGraphInteraction.cpp; UiMediaFit.h; UiGraphWorkspace.h; UiTree.h,
UiTable.h and UiDropdown.h. This follow-up is source/design evidence only; no
shared-engine executable exists yet and no new runtime PASS is claimed.

## Initial demo estate and test/artifact scan — 4 October 2026

User-authorized direction: redesign the maintained demos around the corrected
MediaCard shell, combine genuine control families, preserve public feature/style
coverage and generate clean standalone usage code. This is active migration work,
not an acceptance claim for the entire estate. Explicit repeated shell code is
acceptable; no new mandatory DemoBase/framework is planned.

Source inventory started with 50 controls and 31 mapped packages. UiTag and
UiMediaCard were present in the catalogue but missing from the machine inventory;
they are now included, giving 52 controls and 33 mapped packages. All mapped
headers exist. UiFontSelectorDemo is a font utility, not a public UiFontSelector
control; it must not create a fictitious control row. The untracked media-control
work underway elsewhere in this checkout is preserved and needs its own final
inventory reconciliation.

| Canonical package group | Source findings and disposition |
| --- | --- |
| UiLabelDemo, UiButtonDemo, UiRangeSegmentsDemo | Closest existing explicit PropertyEditor shells. Retain behavior; align exposed backdrop and verify page feedback/native matrix during redesign. |
| UiMediaCardDemo, UiTagDemo, UiCollectionDemo, UiProgressRingDemo, UiChartRingDemo | Shell/theme cleanup implemented. MediaCard and Tag lacked native palette bridge and PropertyEditor palette updates; both Rings had the same omission. Apply custom styles from the current theme, paint the continuous window surface, use compact left-aligned page icons and visible selected feedback. Five Debug BLITZ builds pass. MediaCard inspected in native Light and Dark; other final native matrices remain open. |
| UiCheckBoxDemo, UiRadioButtonDemo, UiToggleDemo, UiEditDemo, UiSliderDemo, UiTabDemo, UiDropdownDemo | Production PropertyEditor exists, but header lacks Help and rail uses expanded Properties/Code buttons. Preserve Usage/Current changes/Full explicit generation modes while adding canonical pages. Existing authored styles must be audited for unconditional light-color snapshots before claiming reset/theme inheritance. |
| UiAccordionDemo, UiBreadcrumbsDemo, UiColorPickerDemo, UiIntFloatDemo, UiMatrixSelectorDemo, UiMenuDemo, UiScrollBarDemo, UiScrollPanelDemo, UiSplitterDemo, UiTableDemo, UiTitleCardDemo, UiTreeDemo | Depend on BuilderDemoSupport and/or DemoPropertyRows. Re-author explicit self-contained shells and production PropertyEditor; preserve model editing, popup, layout and generated-code capabilities before removing helpers. |
| UiPanelDemo | Bespoke builder and property rows. Keep Panel/GroupPanel teaching coverage; migrate shell/editor and generated projection. |
| UiDateTimeDemo, UiProgressBarDemo, UiSplitButtonDemo | Need dedicated canonical editor/code/page review and redesign; existing snippets/showcases do not establish complete API coverage. |
| UiGraphDemo | Specialist topology/model demo remains canonical; preserve authoring, diagnostics and generation behavior while aligning shell. Existing source loads image samples from tests/Images; move demo asset ownership out of test-fixture paths during self-containment work. |
| UiDocDemo | Specialist document interaction harness. Retain unique editing coverage; add a canonical design/code shell without erasing document behavior. |

Ten inventory rows lack mapped examples: UiSliderEdit, UiRangeSliderEdit,
UiColorMatrix, UiDirectContentHost, UiStack, UiAbsoluteLayout, UiGridLayout,
UiBoxLayout, UiBezierCurveEditor and UiBezierCurveField. Layout/nonvisual helpers
may share an explicit composition example rather than receive artificial empty
style panels. Using a control in a shell is not proof its features are showcased.
UiToolButton's current UiLabelDemo mapping also needs actual feature coverage
review; toolbar presence alone does not qualify.

Consolidation decisions/checkpoints:

- List/Gallery: one UiCollectionDemo replaces separate packages; removal and
  catalogue/inventory routing already implemented, including generated-code gates.
- Line/Password/Mask/MultiEdit: UiEditDemo is the family authority. The four old
  separate packages remain retirement candidates until matching their useful
  examples/generation with the redesigned family demo. Do not blindly delete them.
- Slider/RangeSlider: keep one family demo; assess adding SliderEdit/RangeSliderEdit
  there so all four concrete public types have meaningful configuration and code.
- Panel/GroupPanel and Splitter/QuadSplitter already share family examples; retain
  their concrete-type coverage. A layout composition family can cover Box/Grid/
  Absolute/Stack and document DirectContentHost where appropriate.
- MatrixSelector/ColorMatrix and Bezier editor/field are candidate families to
  review at their actual API/style seams. ChartRing's series semantics, topology
  authoring and document editing are not generic collection/demo duplicates.
- UiGraphComponentStudio and UiGraphHierarchyDemo are specialist authoring/
  topology examples; UiRenderBenchmarkDemo is benchmark evidence. UiThemeDemo,
  UiFontSelectorDemo and UiOsFileDialogDemo are theme/font/platform utilities.
  UiLabelGeneratedSmoke is generated-code infrastructure. Preserve or explicitly
  relocate these rather than presenting them as competing canonical builders.
- UiGraphDesignMatrix contains only a retirement README, no executable package;
  its former implementation is in Git history. Empty List/Gallery directories
  after source deletion are not active demos.

Test source comparison covered nine aggregate packages and their 49 suite sources.
After normalizing whitespace and the console entry-point/exit wrapper, 38
standalone bodies match aggregate bodies. This supports sharing implementation,
not throwing away regression coverage. Gallery already uses one shared suite;
the updated ModelViewPerformance standalone differs from the aggregate and must
be reconciled before retirement.

| Aggregate | Matching standalone implementation candidates |
| --- | --- |
| UiControlTests | UiButtonInteractionContractTest; UiChartRingRunTests; UiColorMatrixRunTests; UiDateTimeRunTests; UiGroupPanelRunTests; UiMatrixSelectorRunTests; UiProgressBarRunTests; UiProgressRingRunTests; UiRangeSliderEditRunTests; UiRangeSliderRunTests; UiSliderRunTests; UiStackMeasureTest; UiTabMeasureTest |
| UiDrawingTests | UiGeometryContractTest; UiShapePathTest |
| UiThemeTests | UiThemeSurfaceRegressionTest |
| UiModelTests | UiDataModelsTest; UiModelBindingContractTest; UiModelMutationContractTest |
| UiModelViewTests | UiDropdownMenuRenderTest; UiListStyleContractTest; UiTreeScaleTest |
| UiGraphModelTests | UiGraphHierarchyTest |
| UiGraphViewTests | UiNodeGraphCanonicalShapeTest; UiNodeGraphDragDamageTest; UiNodeGraphHierarchyViewTest; UiNodeGraphInteractionStateTest; UiNodeGraphLiveViewTest; UiNodeGraphRouteEditTest; UiNodeGraphSelectionModifierTest |
| UiGraphRenderTests | UiNodeGraphDetailLodTest; UiNodeGraphOverviewLodTest; UiNodeGraphPatternedPaintTest; UiNodeGraphRenderLodTest |
| UiGraphScaleTests | UiNodeGraphModelSwitchProfileTest; UiNodeGraphPanProfileTest; UiNodeGraphPerformanceTest; UiNodeGraphScaleTest |

There are 78 tracked test/probe/smoke .upp targets. Other suites include unique
PropertyEditor transactions/adapters, MediaCard/Tag, Doc models/geometry/input,
Menu/Dropdown interaction, shape/theme contracts and native/platform behavior.
Their purpose and runner references must be reviewed before consolidation.
Focused targets may remain tiny entry points linking aggregate suite sources;
having one implementation is more important than minimizing useful launch names.

Artifact findings: no tracked .log/.exe/.pdb/.obj files. .gitignore already excludes
build output, logs, caches and local scripts. At inspection, build held about
15.56 GB (14.49 GiB), including executable/acceptance evidence and generated smoke
sources. It is disposable output, but running demos and in-progress evidence must
be accounted for before scoped deletion. No blanket deletion was performed by
this audit. tests/Images contains real Graph fixture/demo references, not merely
unreferenced screenshots. Root Snapshot_*.jpg and icon.png are documentation/
project assets, distinct from output logs.

Acceptance for every replacement: declared direct dependencies; meaningful source
header and subtitle; Theme/Help/Exit order; selected left-aligned Inspector/
Overrides/optional Data/Code icons; bounds-safe live preview; Light/Dark/Light and
hidden-page palettes; all consumed style states/features and reset/inheritance;
same semantic model for Data; actual unchanged default/changed/override generated
C++ compiled for every concrete family type; normal close and idle behavior.
Source token scans locate candidates only: they cannot certify style completeness
or generated-code accuracy. This initial scan is superseded by the implementation
checkpoint below; the complete native interaction matrix remains open.

## Demo implementation checkpoint — 4 October 2026

The accepted MediaCard/Collection shell now guides the maintained builders:
matching rounded preview and inspector containers, slightly tinted Light/Dark
surfaces, transparent inner pages, icon-only left-aligned page actions, and
adjacent Theme/Help/red Exit at the top right. Each demo owns its shell directly;
shared demo helper dependencies have been removed. Compilation is recorded
separately from full native acceptance.

The release inventory maps 55 controls to 37 canonical demos and retains five
specialist examples: document authoring, graph components, graph hierarchy,
theme comparison and font selection. New meaningful coverage includes slider
editors, layout families, Bezier field/editor, ColorMatrix, ToolButton and native
file dialogs. GroupPanel and QuadSplitter have actual selected concrete branches.
ColorProbe and PlaybackBar share UiMediaControlsDemo, while generating only the
selected control's reusable usage. RangeSlider is available in UiSliderDemo via
its preview selector, alongside SliderEdit and RangeSliderEdit. Button, SplitButton
and ToolButton share UiButtonDemo with selected concrete-type generation.

Ten obsolete packages are retired: List/Gallery singles, the four separate
edit variants, SliderEdit, SplitButton and ToolButton singles, and the
RenderBenchmark demo explicitly removed by the user.
Document authoring remains UiDocEditorDemo because its file/ribbon/review workflow
is distinct from the focused UiDoc control builder. Regression suites and image
fixtures are preserved. The Utilities checkpoint below records the completed
retirement of duplicate standalone packages and their preserved aggregate coverage.

`examples/build_demos.py` builds only the maintained inventory, keeps objects,
logs and staged outputs in `build/demos`, and promotes successful executables to
`bin/windows-x64`. It does not erase that folder. The user moved earlier finished
executables to this platform directory during the cleanup. The former approximately
15.56 GB build output was cleaned before the fresh builds.

Generated examples use public APIs, concrete selected types and explicit lifetime
order. The modern form and media builders emit reusable ParentCtrl examples with
owned members and visible placement, rather than GUI entry points that configure
objects and immediately exit. Layout exports omit unused children. Actual exports
are compiled unchanged inside independent harnesses; runtime checks verify model
binding, initialization, layout and selected concrete types where applicable.
Graph offers a concise usage recipe and a separate authored-topology export;
connections use the model-returned node references, escaped strings are preserved,
and custom style exports include font traits and shadow parameters. Graph demo
images are embedded package assets, independent of test fixture paths.

RangeSegments implementation hygiene: geometry/hit testing lives in
`UiRangeSegmentsGeometry.cpp`; all drawing, palette resolution and raster caching
live in `UiRangeSegmentsPaint.cpp`. The former PaintParts/PaintView fragments were
consolidated without changing the control API or rendering/cache policy. This is
source organization, not a claimed performance improvement.

Earlier build checkpoint: all 45 then-maintained packages built successfully with CLANGx64
Release/BLITZ and were promoted to `bin/windows-x64`; `build/demos/build_report.json`
contains each result. All 55 inventory mappings resolve to existing headers and
maintained package manifests. No published executable predates its demo sources.

Earlier generated-code evidence covered 122 recipes before the final family
consolidations: 53 forms/media, 28
container/numeric, 27 specialist/layout and 14 Graph/font/dialog/Collection.
Every recipe compiled unchanged inside its acceptance harness. Constructor,
model, layout and teardown checks passed where applicable; the four native-dialog
functions were compiled without opening modal OS dialogs. Evidence resides in
`build/agents/forms`, `build/agents/containers`, `build/agents/special` and
`build/RootGenerated`. MediaControls passes 48 regression checks; Collection
passes 12 acceptance checks, and GraphComponentStudio's view checks exit zero.
RangeSegments Debug/non-BLITZ and Release/BLITZ regressions each report 60 checks
and zero failures. The captured-drag studio regressions described below exercise
the newly fixed cancellation paths.

Native offscreen MediaControls Light/Dark/code renders were inspected. A full
per-demo live Light/Dark/Light, resizing, keyboard/focus, disabled/selected and
DPI sweep remains open: Computer Use approval timed out during this session.
Native file-dialog modal interaction has not been exercised by generated-code
compilation. These build results do not certify every library control as finished
or eliminate all possible integration defects.

## Utilities consolidation checkpoint — 4 October 2026

The user authorized removal of obsolete and duplicated Utilities test packages.
Current source comparison confirmed all 38 candidates in the table above still
match their aggregate implementations after normalizing only whitespace outside
string/character literals, the console entry point and the exit wrapper. Each
retained source is present in its aggregate manifest and its suite is invoked by
the aggregate driver. The 38 standalone directories and their 76 copied source/
manifest files were removed, totaling 339,335 bytes. No retained aggregate source
file changed; SHA256 checks before and after retirement confirm that.

The nine authoritative packages built with CLANGx64 Release/BLITZ and all 48
no-argument suites passed with zero failures. UiGraphScaleTests --components also
passed its opt-in suite (76 checks, zero failures), exercising the 49th retained
suite implementation. Profile timings are measurement data, not a performance
acceptance claim. Build/runtime logs, literal-preserving comparison metadata and
retained-source hashes are in build/agents/special; finished test executables
remain there rather than in the demo bin folder.

The machine inventory records every retired package's replacement, retained
source and suite entry in retired_test_packages. Current behavior-test routes and
audit source links point to retained packages/files. Utilities/README.md lists the
maintained aggregate entry points and the purpose of remaining support tools.
There are 46 Utilities package manifests after this cleanup.

Focused RangeSegments, shared-source Gallery, PropertyEditor callback lifecycle,
working-range/numeric, Doc, platform and other unique tests remain. Different
ModelViewPerformance, theme-structure, styled-cache and graph-presentation bodies
were not discarded based on a filename resemblance. PropertyEditor and its
headless core are production dependencies. Their two capability demos, headless
probe, icon authoring/export pipeline and rendering benchmark have distinct
current purposes and remain; no obsolete non-test tool was proved safe to retire.

Slider family consolidation also replaces UiSliderEditDemo with UiSliderDemo:
four compact buttons select Slider, RangeSlider, SliderEdit or RangeSliderEdit,
with exactly one preview and selected concrete-type generation. The old package
was removed and both catalogue/inventory routes updated. All four family-selector
checks (Slider, Panel/GroupPanel, Bezier and layouts) passed. Twelve actual slider
default/authored/override exports compiled unchanged against CtrlLib and Ui, and
the twelve unchanged generated control bodies passed construction/teardown in a
separate harness. These checks do not replace a complete live theme/DPI review.


## Final reported defect fixes and family policy — 4 October 2026

The user's studio crash was a real stack overflow (Windows exception c00000fd):
RangeSegments released capture inside CancelMode, while U++ invokes CancelMode
before clearing capture. CancelMode now only clears transient state; explicit
CancelDrag releases capture for Escape, disable/hide/close and model-reset paths.
The real studio view regression obtains native capture and exercises drag updates,
release, Escape, disable, hide, model resync and framework capture cancellation.
It exits successfully. Geometry/measurement and drawing now have two clear source
files; the source split itself does not claim a rendering speedup.

The user's MediaControls access violation (c0000005) exposed synchronous editor
reconstruction during Choice preview/commit. PropertyEditor now guards the whole
model and host callback dispatch, copies borrowed payloads, checks binding/lifetime
and defers structural rebuilding until callbacks unwind, including nested native
event pumps. Bounded Integer and SliderInt reuse the numeric number/slider editor
while preserving their schema kind, minimum, maximum and step. Unbounded integer
values retain their full domain. The focused lifecycle/numeric gate and four
existing PropertyEditor suites pass 511 checks in total with zero failures.

All multi-control builders use compact top selector buttons and one active preview:
Button/SplitButton/ToolButton, Slider/RangeSlider/SliderEdit/RangeSliderEdit,
Line/Password/Mask/MultiEdit, Probe/Playback/HDR, Int/Float, Panel/GroupPanel,
Splitter/QuadSplitter, Bezier Field/Editor and layouts. Collection defaults to List
and retains explicit List/Gallery/Compare buttons. Its single-view exported recipes
now declare and configure only the selected view. Native repeated selector checks
pass for Media (576), Edit (320), Button including popup cycles (340), and the
other family builders. These are application-driven native checks, not physical
keyboard/mouse or DPI certification.

The document authoring demo's ribbon and review children, including hidden pages,
now refresh their Light/Dark semantic palettes. Twelve native snapshots cover
three ribbons, two review pages and both themes; representative images were
inspected. Numeric demo properties have meaningful authored bounds: preview
width/height generally at most 1000, radii at most 60, small counts retain their
small domains, and live indices/pages follow the actual model ranges.

Final shipment verification: all 42 maintained demos (37 canonical builders and
five specialist workflows) build successfully. `bin/windows-x64` contains exactly
those 42 executables, with ten superseded demo packages absent. All 55 control
catalogue mappings resolve to existing headers and maintained package manifests.
The final fresh generated-code gate passes 126 recipes: 50 forms, 28 containers,
33 specialist/slider and 15 Graph/font/native-dialog/Collection recipes, with
zero compile or initialization/teardown failures. Actual emitted source is used;
native file-dialog execution remains an interactive workflow.

The nine aggregate test runners pass 48 default suites, plus GraphScale's
76 component checks. All 38 retired standalone test routes retain their coverage
in existing aggregate source files. The shipped Studio, Slider, Panel, Bezier,
Layout, IntFloat and Splitter native acceptance commands exit successfully after
the final full build. Build reports, logs, generated sources and test executables
remain under `build`; finished demo executables alone belong in `bin/windows-x64`.
