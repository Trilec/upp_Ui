#include <Ui/UiGraph/UiNodeGraph.h>
#include <Ui/UiGraph/UiNodeGraphLod.h>
#include <Ui/Ui.h>
#include <Ui/UiRenderLayer.h>

// One implementation per method. These internal parts share helper linkage and
// compile exactly once here; .inc files are not independent translation units.
// See docs/08_UIGRAPH_GUIDE.md for ownership and the exact/projected paint flow.
#include "UiNodeGraphCore.inc"
#include "UiNodeGraphPaintRich.inc"
#include "UiNodeGraphPaintMicro.inc"
#include "UiNodeGraphPresentation.inc"
#include "UiNodeGraphGeometry.inc"
#include "UiNodeGraphCamera.inc"
#include "UiNodeGraphHierarchy.inc"
#include "UiNodeGraphModelBinding.inc"
#include "UiNodeGraphPaint.inc"
