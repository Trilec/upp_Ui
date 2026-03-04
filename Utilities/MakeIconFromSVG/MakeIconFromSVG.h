#ifndef _utilities_MakeIconFromSVG_h_
#define _utilities_MakeIconFromSVG_h_

#include <Core/Core.h>
#include <Draw/Draw.h>

namespace Upp {

// MakeIconFromSVG
// ---------------
// Small CLI utility that converts SVG or raster assets into U++ icon headers.
//
// Output header contains:
// - DATA_<TOKEN> : RLE-encoded RGBA icon bytes for UiMakeIcon(...)
// - ICON_<TOKEN>(): inline image factory
//
// Input formats:
// - SVG: rendered via Painter (exact-fit into requested size)
// - Raster: loaded via StreamRaster, then fit/centered into requested size
//
// CLI format:
//   MakeIconFromSVG <input.(svg|png|...)> [output.h] [symbol_token] [size|WIDTHxHEIGHT]
//
// Examples:
//   MakeIconFromSVG designs/search.svg Ui/newicons/search_icon.h CUSTOM_SEARCH_48 48x48
//   MakeIconFromSVG designs/NewLogo_v4.png Ui/newicons/newlogo_v4_icon.h CUSTOM_NEWLOGO_V4_48 48x48
//
// Help:
//   MakeIconFromSVG --help
//   MakeIconFromSVG -h

struct SvgIconJob {
    String input_path;
    String output_header;
    String symbol_token;
    Size   size = Size(0, 0);
};

bool ParseSvgIconJob(const Vector<String>& args, SvgIconJob& job, String& error);
bool BuildIconHeaderFromSvg(const SvgIconJob& job, String& error);

}

#endif
