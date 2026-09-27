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
