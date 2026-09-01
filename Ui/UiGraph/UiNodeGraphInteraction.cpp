#include <Ui/UiGraph/UiNodeGraph.h>

// Preserve the complete validated interaction implementation byte-for-byte in
// UiNodeGraphInteractionBase.inc, but compile the three live-camera entry points
// under legacy names. UiNodeGraphView.inc then supplies the active mouse-move,
// middle-pan and wheel-zoom path without duplicating the rest of interaction.
#define UpdatePan  UpdatePanLegacy
#define MouseMove  MouseMoveLegacy
#define MouseWheel MouseWheelLegacy
#include "UiNodeGraphInteractionBase.inc"
#undef MouseWheel
#undef MouseMove
#undef UpdatePan

#include "UiNodeGraphView.inc"
