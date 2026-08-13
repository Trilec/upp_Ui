# ACTIVE WORK

BASE: `91d431637a6619b503e548f297375a252cbb8603` — Gary's completed Windows compile/test baseline on `main`.

TASK: `UI-GRAPH-BUILD-R1` — final Windows/runtime acceptance of the reusable `UiGraphModel` / `UiNodeGraph` implementation now integrated into the `Ui` package.

STAGING REFERENCE: `Trilec/upp_agentflow@76c217d743b7b9f79b4c9e921fe86e7cb7b1f1a7`

PROMOTION: `4e837325b3ea6cff39d218bf8ee2bc60aa52a81a` merged UiGraph onto current `main`; the obsolete `UiGraphBuild` branch has since been deleted locally and remotely.

WINDOWS FIX BASELINE: `91d431637a6619b503e548f297375a252cbb8603` fixed the first Windows compile issues: U++ container copyability, BLITZ helper-name collision, and the deterministic large-model test fixture.

PAN FIX: `05f871e341af528d932a5f92350d93536e1addb1` removes unsupported Ctrl mouse capture from middle-button panning and terminates an in-view pan on `MiddleUp` or `MouseLeave`.

TOUCHED FOR CURRENT RETEST: `Ui/UiGraph/UiNodeGraphInteraction.cpp`, `docs/ACTIVE_WORK.md`.

VALIDATED AT `91d4316`: `UiGraphTest` Debug 90/90; Release 90/90; `UiDataModelsTest` Debug 7535 checks / 0 fails; `UiGraphDemo` Debug builds and launches; `git diff --check` passed; working tree clean.

OPEN RUNTIME ITEM: Gary reproduced an apparent crash while middle-button panning. The pan implementation was taking `Ctrl::SetCapture()` from `MiddleDown`, although U++ documents capture for left/right mouse interactions only. That unsupported capture path is removed in `05f871e3` and requires Windows runtime confirmation.

SECONDARY UX ITEM: first auto-fit appears visually too large in the demo. Do not mix this polish into the pan-stability fix; revisit after the pan retest passes.

STATUS: PUBLISHED TO MAIN — compile/test baseline passed; focused Windows pan retest pending.

NEXT ACTION: Gary pulls current `main`, rebuilds `UiGraphDemo`, repeatedly tests middle-button pan including crossing attached controls and leaving/re-entering the graph area, confirms no crash/capture residue, reruns `UiGraphTest`, and reports exact results. If pan is stable, close runtime acceptance and then consider the initial-fit scale as a separate polish task.
