#include <CtrlLib/CtrlLib.h>

#include "GeneratedRawHeader.h"
#include "GeneratedRleHeader.h"

using namespace Upp;

static bool HasVisiblePixels(const Image& img)
{
	Size sz = img.GetSize();
	for(int y = 0; y < sz.cy; ++y) {
		const RGBA* row = img[y];
		for(int x = 0; x < sz.cx; ++x) {
			if(row[x].a > 0)
				return true;
		}
	}
	return false;
}

GUI_APP_MAIN
{
	Image raw = ICON_SMOKE_RAW_SAVE();
	Image rle = ICON_SMOKE_RLE_COPY();
	if(raw.IsEmpty() || rle.IsEmpty()) {
		Exclamation("Smoke headers returned empty images.");
		return;
	}
	if(raw.GetSize() != Size(24, 24) || rle.GetSize() != Size(24, 24)) {
		Exclamation("Smoke headers returned the wrong image dimensions.");
		return;
	}
	if(!HasVisiblePixels(raw) || !HasVisiblePixels(rle)) {
		Exclamation("Smoke headers returned fully transparent images.");
		return;
	}
}
