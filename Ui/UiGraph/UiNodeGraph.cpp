#include <Ui/UiGraph/UiNodeGraph.h>
#include <Ui/Ui.h>
#include <Ui/UiRenderLayer.h>

// Preserve the validated retained implementation in this translation unit,
// but keep active render/hierarchy/performance policy separate. H2 aliases only
// the methods whose semantics must become scope-local; the performance slice
// additionally aliases public camera mutations so composite view changes can be
// coalesced without rewriting the retained implementation.
#define RefreshLayout() do { if(auto_fit_first_paint_ && !first_paint_done_) Ctrl::RefreshLayout(); } while(0)
#define Rectangle                    LegacyRectangle
#define UsesRectangularStyledSurface UsesRectangularStyledSurfaceLegacy
#define Paint                         PaintLegacy
#define PaintGraphGeometry            PaintGraphGeometryLegacy
#define PaintNodeDetails              PaintNodeDetailsLegacy
#define SetModel                      SetModelLegacy
#define UseInternalModel              UseInternalModelLegacy
#define SetZoom                       SetZoomLegacy
#define SetPan                        SetPanLegacy
#define PanBy                         PanByLegacy
#define ResetView                     ResetViewLegacy
#define FitToGraph                    FitToGraphLegacy
#define CenterOnNode                  CenterOnNodeLegacy
#define SelectNode                    SelectNodeLegacy
#define SelectEdge                    SelectEdgeLegacy
#define SetData                       SetDataLegacy
#define GetData                       GetDataLegacy
#define Layout                        LayoutLegacy
#include "UiNodeGraphBase.inc"
#undef Layout
#undef GetData
#undef SetData
#undef SelectEdge
#undef SelectNode
#undef CenterOnNode
#undef FitToGraph
#undef ResetView
#undef PanBy
#undef SetPan
#undef SetZoom
#undef UseInternalModel
#undef SetModel
#undef PaintNodeDetails
#undef PaintGraphGeometry
#undef Paint
#undef UsesRectangularStyledSurface
#undef Rectangle
#undef RefreshLayout

// Keep the accepted R9/R10 software render policy intact as the rich/detail
// fallback. P2 gives the active name to a projected-micro direct Draw path while
// retaining this exact renderer for ordinary/reference nodes and custom paint.
#define Paint PaintRenderBase
#define PaintGraphGeometry PaintGraphGeometryRenderBase
#include "UiNodeGraphRender.inc"
#undef PaintGraphGeometry
#undef Paint

// The retained recovery source uses wire-0 as its historical flat Rectangle.
// Keep that mapping while compiling the new overview path so canonical Rectangle
// (wire 13) continues to own its authored corner radius exactly as the active
// renderer does.
#define Rectangle LegacyRectangle
#include "UiNodeGraphPerformance.inc"
#undef Rectangle

// All active hierarchy/view operations should settle through the overview-aware
// preparer. The retained PrepareGeometry remains available to legacy/recovery
// internals but cannot accidentally turn an extreme overview back into rich
// per-node geometry on SetScope/Fit/selection/Layout.
#define PrepareGeometry PrepareViewGeometry
#include "UiNodeGraphHierarchy.inc"
#undef PrepareGeometry

#include "UiNodeGraphHierarchyModelSwitch.inc"
#include "UiNodeGraphHierarchyPaint.inc"
