#ifndef _Utilities_IconExportCore_IconExportCore_h_
#define _Utilities_IconExportCore_IconExportCore_h_

#include <Draw/Draw.h>

namespace Upp {

bool ValidateUppImlImageSize(Size sz, String* error = nullptr);
bool BuildUppImlPayload(const Image& img, String& payload, String* error = nullptr);
bool BuildUppImlEntryText(const String& token, const String& compressed, const String& source_svg, Size sz, String& out, String* error = nullptr);
bool BuildUppImlEntryText(const String& token, const Image& img, const String& source_svg, String& out, String* error = nullptr);

}

#endif
