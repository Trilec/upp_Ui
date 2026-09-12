# ACTIVE WORK

Remote main is authoritative. Fetch before work/publish; do not force-update main.

BASE: `40ff7c974934b077616f2e5cd85fdd8bb7e57c8b` / main
TASK: UIGRAPH-PRESENTATION-LAYOUT-01 — shared prepared node presentation contract
STATUS: IMPLEMENTATION COMPLETE — PLATFORM VALIDATION PENDING
BRANCH: main
PUBLISHED: This checkpoint is the commit containing this recovery record.
NEXT ACTION: Gary runs the focused Debug gate below; Curt inspects Reference content.

## TOUCHED

- Ui/Ui.upp
- Ui/UiGraph/UiNodeGraph.h
- Ui/UiGraph/UiNodeGraph.cpp
- Ui/UiGraph/UiNodeGraphPresentation.inc (new exact allocation owner)
- Ui/UiGraph/UiNodeGraphGeometry.inc
- Ui/UiGraph/UiNodeGraphLod.h
- Ui/UiGraph/UiNodeGraphPaintRich.inc
- Ui/UiGraph/UiNodeGraphCore.inc
- Ui/UiGraph/UiNodeGraphProjection.inc
- Ui/UiGraph/UiNodeGraphInteraction.cpp
- examples/UiGraphDemo/UiGraphDemoData.cpp
- Utilities/UiGraphRenderTests/PresentationLayout.h (new shared structural tests)
- Utilities/UiGraphRenderTests/{Presentation,DetailLod,RenderLod}.cpp
- Utilities/UiGraphRenderTests/UiGraphRenderTests.upp
- Utilities/UiNodeGraphPresentationTest/main.cpp
- Utilities/UiNodeGraphDetailLodTest/main.cpp
- Utilities/UiNodeGraphRenderLodTest/main.cpp
- docs/08_UIGRAPH_GUIDE.md
- docs/UIGRAPH_PRESENTATION_AUDIT.md
- docs/ACTIVE_WORK.md

## IMPLEMENTED

One UiGraphNodePresentation in NodeGeometry replaces independent content/title/
control rectangles. It owns shape-safe capacity, header/body parents, disjoint
text/icon/media/badge/control/footer leaves, port lanes, level and show flags.
Exact preparation resolves Standard/Centred/MediaCard requests. Hidden slots are
reserved; Normal enlargement preserves composition. Font/icon sizing is linear.
The production outline bounds stock content, including resolved Rectangle radius.
Insufficient capacity is reported with fits=false rather than overlapping leaves.

WhenResolveNodePresentation supplies bounded authored slot requests. Hosts call
InvalidateNodePresentation after changing captures; batches defer until EndBatch.
GetNodePresentation reads already prepared results, including inside content paint.
WhenPaintNodeContent now receives the media slot, not the former whole-content
rectangle. Graph clips it away from stock text/controls/port lanes. Demo badge/media
callbacks exercise this contract; guessed title lanes were removed from fixtures.

Live projection copies/projects the result, rejecting presentation/control visibility
boundaries. Reusable pan performs no layout. Micro geometry skips the new resolver
and retains empty rich allocations; micro projection skips rich rectangle work.
No spatial authority, endpoint/route semantics, micro cache or backend was replaced.

Native controls need real minimum-size capacity and Normal eligibility. Reduced
presentation retains their reservation but hides the live child. Old tests updated
to this deliberate contract; dense-shape control fixtures now author enough space.

## VALIDATION

Source review and git diff --check: PASS.
Linux G++ C++17 syntax-only checks (VIRTUALGUI headers): PASS for changed graph/
interaction, aggregate Presentation/DetailLod/RenderLod, standalone Presentation,
and demo data translation units. No linking or GUI runtime pass is claimed.
Headers: ultimatepp b3a6106a7a0daf642a0d6740ac0e7015f08875bb;
Animation 4a01b6f4e2a9f122ea1a93457b62a4054d01f970.
Windows UMK/CLANGx64 and GUI are unavailable in the implementation environment.

Required Windows gate — DEBUG ONLY:
1. Build/run UiGraphRenderTests.
2. Build/run UiNodeGraphPresentationTest.
3. Build/run UiGraphViewTests (projection and batch completion changed).
4. Build UiGraphDemo; launch and leave running for Curt.
Expect zero failures, including UIGRAPH_EXECUTION_PATH_SUMMARY checks=8 failed=0.
Check Reference images/tags/title separation, selected ports unchanged, attached
controls at useful size, and a brief Reference/10k middle-pan smoke check.
No Release or exhaustive timing matrix is required for this checkpoint.
Report exact tested SHA, toolchain, command/results and failures. Minor build/test
fix-ups are allowed; stop for architectural changes or performance regressions.

## DELIBERATE LIMITATIONS / NEXT

No Rectangle Design matrix slice yet. First validate production layout, then add
four rows using the same production renderer/result and extend to all eight shapes.
No automatic content reflow, maximal polygon packing or native-control proxy/scaling
engine. Media/badge/footer content is host-owned. Custom painted shapes must honor
their declared content capacity. Collapse and transfer animation remain deferred.
Existing diagnostic L0-L4 and Micro/Rich names are unchanged; the new numerical
presentation levels are separate, and configured zoom gates still limit visibility.
Earlier port/arrow fixes are retained. Their historical Windows evidence must not
be mistaken for runtime validation of this new layout checkpoint.
