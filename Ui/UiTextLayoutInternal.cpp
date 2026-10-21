#include "UiTextLayoutInternal.h"

#if defined(PLATFORM_WIN32) && defined(flagGPUUI)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace Upp {
namespace UiTextInternal {
bool Encoding::Build(const WString& source,String& error)
{
	valid=false;
	text.Clear(); utf16.Clear(); scalar_to_utf16.Clear(); utf16_to_scalar.Clear();
	if(source.GetCount()>32768) { error="text layout exceeds 32768 scalar values"; return false; }
	for(int i=0;i<source.GetCount();++i) {
		uint32 cp=source[i];
		if(cp>0x10ffff || (cp>=0xd800 && cp<=0xdfff)) {
			error="text layout contains an invalid Unicode scalar"; return false;
		}
		scalar_to_utf16.Add(utf16.GetCount());
		if(cp<0x10000) { utf16.Add((char16_t)cp); utf16_to_scalar.Add(i); }
		else {
			cp-=0x10000;
			utf16.Add((char16_t)(0xd800+(cp>>10))); utf16_to_scalar.Add(i);
			utf16.Add((char16_t)(0xdc00+(cp&1023))); utf16_to_scalar.Add(i);
		}
	}
	scalar_to_utf16.Add(utf16.GetCount()); utf16_to_scalar.Add(source.GetCount()); utf16.Add(0);
	text=source; valid=true; error.Clear(); return true;
}
#if defined(PLATFORM_WIN32) && defined(flagGPUUI)
namespace {
// Small private binding to Windows' stable ICU C ABI. Signatures/constants
// checked against Windows SDK10.0.26100.0 um/icu.h (U_EXPORT2 == __cdecl).
// Do not ship Unicode range tables or require ICU SDK headers in public Ui.
// Combined icu.dll is an OS component on Windows10 1903 and later.
class UnicodeApi {
public:
	using Open = void *(__cdecl *)(int32_t,const char *,const char16_t *,int32_t,int32_t *);
	using Close = void (__cdecl *)(void *);
	using Next = int32_t (__cdecl *)(void *);
	using BidiOpen = void *(__cdecl *)();
	using BidiSet = void (__cdecl *)(void *,const char16_t *,int32_t,uint8_t,uint8_t *,int32_t *);
	using BidiLevel = uint8_t (__cdecl *)(const void *);
	using Version = void (__cdecl *)(uint8_t *);
	using WhiteSpace = int8_t (__cdecl *)(int32_t);
	HMODULE module=nullptr;
	Open open=nullptr;
	Close close=nullptr;
	Next first=nullptr,next=nullptr;
	BidiOpen bidi_open=nullptr;
	Close bidi_close=nullptr;
	BidiSet bidi_set=nullptr;
	BidiLevel bidi_level=nullptr;
	Version version=nullptr;
	WhiteSpace whitespace=nullptr;
	UnicodeApi() {
		module=LoadLibraryExW(L"icu.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
		if(!module) return;
		open=(Open)GetProcAddress(module,"ubrk_open"); close=(Close)GetProcAddress(module,"ubrk_close");
		first=(Next)GetProcAddress(module,"ubrk_first"); next=(Next)GetProcAddress(module,"ubrk_next");
		bidi_open=(BidiOpen)GetProcAddress(module,"ubidi_open");
		bidi_close=(Close)GetProcAddress(module,"ubidi_close");
		bidi_set=(BidiSet)GetProcAddress(module,"ubidi_setPara");
		bidi_level=(BidiLevel)GetProcAddress(module,"ubidi_getParaLevel");
		version=(Version)GetProcAddress(module,"u_getUnicodeVersion");
		whitespace=(WhiteSpace)GetProcAddress(module,"u_isUWhiteSpace");
	}
	~UnicodeApi() { if(module) FreeLibrary(module); }
	bool Ready() const { return open && close && first && next && bidi_open && bidi_close && bidi_set && bidi_level && version && whitespace; }
};
UnicodeApi& Api() { static UnicodeApi api; return api; }
bool Breaks(const Encoding& text,const String& locale,int kind,Vector<int>& out,String& error)
{
	Vector<int> result;
	if(!Api().Ready()) { error="full text layout requires Windows10 1903+ system ICU"; return false; }
	if(!text.valid || text.utf16.GetCount()!=text.utf16_to_scalar.GetCount() || text.utf16.IsEmpty() ||
	   text.scalar_to_utf16.GetCount()!=text.text.GetCount()+1) {
		error="Unicode boundary input has no validated scalar mapping"; return false;
	}
	if(locale.GetCount()>256 || locale.Find('\0')>=0) { error="invalid text locale"; return false; }
	int32_t status=0;
	void *iterator=Api().open(kind,locale.IsEmpty()?nullptr:locale.Begin(),
	                          text.utf16.Begin(),text.Length16(),&status);
	struct Guard { void *p; ~Guard() { if(p) Api().close(p); } } guard{iterator};
	if(status>0 || !iterator) { error="system Unicode iterator could not open"; return false; }
	int previous=-1;
	for(int pos=Api().first(iterator);pos!=-1;pos=Api().next(iterator)) {
		if(pos<0 || pos>text.Length16() || pos<=previous) {
			error="invalid system Unicode boundary"; return false;
		}
		int scalar=text.utf16_to_scalar[pos];
		if(text.scalar_to_utf16[scalar]!=pos) { error="Unicode boundary splits a surrogate pair"; return false; }
		result.Add(scalar); previous=pos;
	}
	if(result.IsEmpty() || result[0]!=0 || result.Top()!=text.text.GetCount()) {
		error="incomplete system Unicode boundaries"; return false;
	}
	out=pick(result); error.Clear(); return true;
}
}
bool GraphemeBreaks(const Encoding& text,const String& locale,Vector<int>& out,String& error)
{ return Breaks(text,locale,0,out,error); } // UBRK_CHARACTER
bool WordBreaks(const Encoding& text,const String& locale,Vector<int>& out,String& error)
{ return Breaks(text,locale,1,out,error); } // UBRK_WORD
bool BaseDirection(const Encoding& text,bool& right_to_left,String& error)
{
	if(!Api().Ready()) { error="system Unicode direction service unavailable"; return false; }
	if(!text.valid) { error="Unicode direction needs validated text"; return false; }
	void *bidi=Api().bidi_open();
	struct Guard { void *p; ~Guard() { if(p) Api().bidi_close(p); } } guard{bidi};
	if(!bidi) { error="system bidi allocation failed"; return false; }
	int32_t status=0;
	// Full paragraph analysis implements UAX9 P2/P3, including isolate controls.
	// The lightweight first-strong-character API does not provide that contract.
	Api().bidi_set(bidi,text.utf16.Begin(),text.Length16(),0xfe,nullptr,&status);
	if(status>0) { error="system paragraph direction analysis failed"; return false; }
	right_to_left=(Api().bidi_level(bidi)&1)!=0; error.Clear(); return true;
}
bool IsWhitespace(int scalar) { return Api().Ready() && scalar>=0 && scalar<=0x10ffff && Api().whitespace(scalar)!=0; }
String UnicodeVersion()
{
	if(!Api().Ready()) return String();
	uint8_t version[4]{}; Api().version(version);
	return Format("%d.%d.%d.%d",(int)version[0],(int)version[1],(int)version[2],(int)version[3]);
}
#else
bool GraphemeBreaks(const Encoding&,const String&,Vector<int>&,String& error)
{ error="full text layout provider is not available in this assembly"; return false; }
bool WordBreaks(const Encoding&,const String&,Vector<int>&,String& error)
{ error="full text layout provider is not available in this assembly"; return false; }
bool BaseDirection(const Encoding&,bool&,String& error)
{ error="full text layout provider is not available in this assembly"; return false; }
bool IsWhitespace(int scalar) { return IsSpace(scalar); }
String UnicodeVersion() { return String(); }
#endif
}
}
