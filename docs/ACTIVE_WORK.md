# ACTIVE WORK

Remote `main` is authoritative. Fetch before work/publish; never force-update `main`.
Recovery state only; Git history is implementation history.

BASE: `e283523c5a77f0da85a6442877685c819ab4e34d`
TASK: **UIGRAPH-ARROW-VOCAB-01 — complete the core endpoint-marker set**
BRANCH: `main`
STATUS: **PUBLISHED — WINDOWS VALIDATION PENDING**

TOUCHED:
- `Ui/UiGraph/UiGraphModel.h`
- `Ui/UiGraph/UiNodeGraphCore.inc`
- `Utilities/UiGraphModelTests/UiGraphCore.cpp`
- `Utilities/UiGraphTest/main.cpp`
- `Utilities/UiGraphRenderTests/Presentation.cpp`
- `Utilities/UiNodeGraphPresentationTest/main.cpp`
- `examples/UiGraphDemo/UiGraphDemo.cpp`
- `examples/UiGraphDemo/UiGraphDemoData.cpp`
- `docs/08_UIGRAPH_GUIDE.md`
- `docs/ACTIVE_WORK.md`

## ACCEPTED BASELINE

Eddie's execution consolidation remains authoritative:
- one explicit implementation owner per UiNodeGraph responsibility;
- one world spatial authority;
- immutable live camera projection + exact settle;
- Micro/Rich paint backends with shared admission/LOD policy;
- no recovery/H2 execution aliases.

Port/route-handle raster correction:
- source fix: `3ecd53a3c25cc32d4a8266299d844c1ef4717c2e`;
- Gary's first Debug run compiled but exposed an over-strict 7x7 hollow-centre test;
- test-only refinement: `bb70fb940cf07627b381152daebb12e798a63e5c`;
- full Windows validation still needs to resume from that checkpoint.

## ARROW VOCABULARY

Core authored endpoint markers are now:
- None;
- Open;
- Triangle;
- Tee;
- Square;
- Circle;
- Diamond.

Existing wire values are frozen:
- None=1;
- Triangle=2;
- Open=3;
- Circle=4;
- Diamond=5.

New values are append-only:
- Tee=6;
- Square=7.

Tee:
- transverse terminal bar;
- sits immediately on the connector side of the semantic endpoint.

Square:
- filled terminal block;
- forward face is tangent to the semantic endpoint.

Circle/Diamond/Triangle/Open behavior is unchanged.
Complex marker combinations from larger graph libraries are intentionally not copied.

Filled/hollow marker state is a separate possible future style dimension; do not
multiply the shape enum into duplicate filled/hollow variants.

## DEMO / TEST COVERAGE

UiGraphDemo:
- Inspector Arrow choice exposes Tee and Square;
- parser/name/generated-code path supports both;
- reference fixture cycles Open/Triangle/Tee/Square/Circle/Diamond.

Model tests:
- preserve established marker wire values;
- verify Tee/Square appended values;
- serialize/deserialize Square as a new authored value.

Presentation tests:
- Tee renders a transverse terminal bar rather than a filled block;
- Square renders a filled tangent terminal block;
- existing Circle/AA presentation checks remain.

PUBLISHED: `e283523c5a77f0da85a6442877685c819ab4e34d`

## WINDOWS GATE

Build/run Debug + Release:
- `UiGraphModelTests`;
- `UiGraphTest`;
- `UiGraphRenderTests`;
- `UiNodeGraphPresentationTest`;
- `UiGraphDemo`.

Required:
- all automated checks PASS;
- radius-3/radius-4/radius-6 port-ring checks PASS after `bb70fb9...`;
- visually inspect Tee and Square at normal/detail scale;
- verify Open/Triangle/Circle/Diamond are unchanged;
- inspector can switch an edge among all seven non-Inherit choices;
- generated C++ reports `UiGraphArrowStyle::Tee` / `Square` correctly;
- no 10k regression: dense fixture still authors `arrow=None`;
- `git diff --check` PASS.

## NEXT ACTION

After the focused ring + arrow visual gate:
1. implement the shared prepared node-content layout/profile contract;
2. build the 8-shape x Normal/LOD1/LOD2/LOD3 Design matrix from that same contract;
3. transfer activity is a separate later slice;
4. collapse remains deferred.

Do not alter execution/spatial/micro architecture for presentation work.
