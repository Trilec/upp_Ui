#ifndef _utilities_MakeIconFromSVG_h_
#define _utilities_MakeIconFromSVG_h_

#include <Core/Core.h>
#include <Draw/Draw.h>

namespace Upp {

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
