#ifndef _Ui_UiTextLayout_h_
#define _Ui_UiTextLayout_h_

// Author: C Edwards (dodobar). Apache License 2.0; see repository LICENSE.
// Ui-owned text boundary for GPUUI hosts. Windows uses DirectWrite and system
// ICU; ordinary Ui assemblies retain their existing native text helpers.
// Controls use this on the GUI thread. Immutable rendering snapshots own their
// font resources and may outlive the originating control through queued frames.
// Cache limits count declared text payload, not native font/driver residency.
#include <Draw/Draw.h>
#include <memory>
#ifdef flagGPUUI
#include <RenderCore/GpuTextLayout.h>
#endif

namespace Upp {
enum class UiTextDirection { Auto, LeftToRight, RightToLeft };

struct UiTextLayoutOptions {
	UiTextDirection direction=UiTextDirection::Auto;
	double width=1000000;
	bool wrap=false;
	int tab_spaces=4;
	String locale;
};

struct UiTextHit {
	int position=0; // U++ WString scalar index, never a UTF-16 surrogate interior.
	bool trailing=false, inside=false; // trailing means upstream affinity at position.
};

// Shared immutable layout. Drawing, width, caret, hit-testing and selection
// must consume this same result. Native handles stay inside the implementation.
// GUI controls retain layouts only while their text/font/layout inputs match.
class UiTextLayout {
public:
	UiTextLayout() = default;
	static UiTextLayout Create(const WString& text, Font font,
	                           const UiTextLayoutOptions& options=UiTextLayoutOptions());
	bool IsValid() const;
	String GetError() const;
	Sizef GetSize() const;
	int GetLength() const;
	int GetGlyphCount() const;
	// Multi-grapheme ligatures retain evenly distributed editing stops within
	// DirectWrite's native cluster advance; one grapheme is never split.
	Pointf GetCaret(int position, bool upstream=false) const;
	UiTextHit HitTest(Pointf point) const;
	Vector<Rectf> GetSelection(int from,int to) const;
	int NextGrapheme(int position,int direction) const;
	UiTextHit MoveVisual(UiTextHit position,int direction) const;
	bool Draw(Draw& draw,Pointf origin,Color ink) const;
#ifdef flagGPUUI
	std::shared_ptr<const GpuShapedText> GetGpuText() const;
#endif
private:
	struct Data;
	std::shared_ptr<Data> data;
};

struct UiTextLayoutStats {
	int cache_entries=0, cache_entry_limit=256;
	int64 cache_payload_bytes=0, cache_payload_limit=4*1024*1024;
	uint64 hits=0, misses=0;
};
UiTextLayoutStats GetUiTextLayoutStats();
void ClearUiTextLayoutCache();
// Ui-owned text entry points. GPUUI builds use one layout for measuring and
// drawing complete strings; ordinary assemblies retain their native Draw path.
Size UiGetTextSize(const WString& text,Font font=StdFont());
WString UiFitText(const WString& text,Font font,int width,const WString& tail=WString("..."));
void UiDrawTextEllipsis(Draw& draw,int x,int y,int width,const WString& text,
                        const char *tail,Font font,Color ink);
inline void UiDrawTextEllipsis(Draw& draw,int x,int y,int width,const String& text,const char *tail,Font font,Color ink) {
	UiDrawTextEllipsis(draw,x,y,width,text.ToWString(),tail,font,ink);
}
inline Size UiGetTextSize(const String& text,Font font=StdFont()) { return UiGetTextSize(text.ToWString(),font); }
inline Size UiGetTextSize(const char *text,Font font=StdFont()) { return UiGetTextSize(String(text),font); }
void UiDrawText(Draw& draw,int x,int y,const WString& text,Font font,Color ink);
inline void UiDrawText(Draw& draw,int x,int y,const String& text,Font font,Color ink) { UiDrawText(draw,x,y,text.ToWString(),font,ink); }
inline void UiDrawText(Draw& draw,int x,int y,const char *text,Font font,Color ink) { UiDrawText(draw,x,y,String(text),font,ink); }
bool UiDrawTextSelection(Draw& draw,Point origin,const UiTextLayout& layout,
                         Color ink,int from,int to,Color background,Color selected_ink);

}
#endif
