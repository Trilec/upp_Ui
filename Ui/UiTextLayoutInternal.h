#ifndef _Ui_UiTextLayoutInternal_h_
#define _Ui_UiTextLayoutInternal_h_
#include <Core/Core.h>

namespace Upp {
namespace UiTextInternal {
// One checked scalar/UTF-16 mapping shared by layout and Unicode boundaries.
struct Encoding {
	bool valid=false;
	WString text;
	Vector<char16_t> utf16; // NUL-terminated; Length16 excludes the terminator.
	Vector<int> scalar_to_utf16, utf16_to_scalar;
	bool Build(const WString& source,String& error);
	int Length16() const { return max(0,utf16.GetCount()-1); }
};
bool GraphemeBreaks(const Encoding& text,const String& locale,Vector<int>& out,String& error);
bool WordBreaks(const Encoding& text,const String& locale,Vector<int>& out,String& error);
bool BaseDirection(const Encoding& text,bool& right_to_left,String& error);
bool IsWhitespace(int scalar);
String UnicodeVersion();
}
}
#endif
