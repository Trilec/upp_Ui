#include <CtrlLib/CtrlLib.h>

#include "GeneratedRawHeader.h"
#include "GeneratedRleHeader.h"

using namespace Upp;

GUI_APP_MAIN
{
	Image raw = ICON_SMOKE_RAW_SAVE();
	Image rle = ICON_SMOKE_RLE_COPY();
	if(raw.IsEmpty() || rle.IsEmpty())
		Exclamation("Smoke headers returned empty images.");
}
