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
#include <RenderCore/GpuImeClient.h>
#endif

namespace Upp {
enum class UiTextDirection { Auto, LeftToRight, RightToLeft };

enum class UiTextAlignment { Leading, Left, Center, Right };

struct UiTextLayoutOptions {
	UiTextDirection direction=UiTextDirection::Auto;
	double width=1000000;
	bool wrap=false;
	bool wrap_at_character=false;
	UiTextAlignment alignment=UiTextAlignment::Leading;
	int tab_spaces=4;
	String locale;
};

// Sorted, non-overlapping scalar ranges. Font Null inherits the base font.
struct UiTextStyleRange : Moveable<UiTextStyleRange> {
	int from=0, length=0;
	Font font=Null;
	Color ink=Null;
	double tracking=0;
};
// One U+FFFC scalar reserves native inline metrics; the host draws its asset.
struct UiTextInlineObject : Moveable<UiTextInlineObject> {
	int position=0;
	Sizef size=Sizef(0,0);
	double baseline=-1; // Negative uses the bottom edge.
};
struct UiTextInlinePlacement : Moveable<UiTextInlinePlacement> {
	int position=0;
	Rectf bounds=Rectf(0,0,0,0);
};
struct UiTextLine : Moveable<UiTextLine> {
	int from=0,to=0; // Scalar range, excluding a hard paragraph separator.
	Rectf bounds=Rectf(0,0,0,0);
	double baseline=0; // Distance below bounds.top.
	bool right_to_left=false;
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
	static UiTextLayout CreateStyled(const WString& text,Font font,
	                           const Vector<UiTextStyleRange>& styles,
	                           const Vector<UiTextInlineObject>& objects=Vector<UiTextInlineObject>(),
	                           const UiTextLayoutOptions& options=UiTextLayoutOptions());
	int GetLineCount() const;
	UiTextLine GetLine(int index) const;
	int GetInlineCount() const;
	UiTextInlinePlacement GetInline(int index) const;
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
	int NextWord(int position,int direction) const;
	bool GetWord(int position,int& from,int& to) const;
	UiTextHit MoveVisual(UiTextHit position,int direction,bool cross_lines=false) const;
	bool Draw(Draw& draw,Pointf origin,Color ink,bool override_ink=false) const;
	// Line-local immutable snapshot; origin is the line's top-left in the host.
	bool DrawLine(Upp::Draw& draw,int line,Pointf origin,Color ink,bool override_ink=false) const;
#ifdef flagGPUUI
	std::shared_ptr<const GpuShapedText> GetGpuText() const;
	std::shared_ptr<const GpuShapedText> GetLineGpuText(int line) const;
#endif
private:
	struct Data;
	std::shared_ptr<Data> data;
};

#ifdef flagGPUUI
// A small transient overlay shared by Ui text editors. Model text and undo
// remain unchanged until commit; native candidate windows remain OS services.
class Ctrl;
class UiTextComposition {
public:
	UiTextComposition();
	~UiTextComposition();
	bool Update(const GpuImeComposition& value);
	void Bind(Ctrl& owner);
	void Clear();
	bool IsActive() const;
	Rect GetCaret(Rect anchor,Rect limits,Font font) const;
	void Paint(Draw& draw,Rect anchor,Rect limits,Font font,Color ink,Color paper) const;
private:
	struct State;
	mutable One<State> state;
};

#endif

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
