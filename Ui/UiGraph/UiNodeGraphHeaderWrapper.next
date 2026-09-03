#ifndef _Ui_UiGraph_UiNodeGraph_H2_wrapper_h_
#define _Ui_UiGraph_UiNodeGraph_H2_wrapper_h_

#include "UiNodeGraphHierarchyApi.h"

// H2 recovery wrapper. UiNodeGraphBase.h is the exact validated pre-H2 header
// blob. Inject only the hierarchy declarations while this tranche is under
// Windows acceptance, then undefine every staging macro immediately.
#define WhenViewport WhenViewport; UIGRAPH_HIERARCHY_PUBLIC_DECLS
#define drag_preview_positions_ drag_preview_positions_; UIGRAPH_HIERARCHY_PRIVATE_DECLS; int uigraph_h2_injection_dummy_
#include "UiNodeGraphBase.h"
#undef drag_preview_positions_
#undef WhenViewport
#undef UIGRAPH_HIERARCHY_PRIVATE_DECLS
#undef UIGRAPH_HIERARCHY_PUBLIC_DECLS

#endif
