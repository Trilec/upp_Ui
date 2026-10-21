#include "UiTextLayout.h"
#include "UiTextLayoutInternal.h"
#include <Painter/Painter.h>
#include <cmath>
#ifdef flagGPUUI
#include <RenderCore/GpuDrawTarget.h>
#endif
#if defined(PLATFORM_WIN32) && defined(flagGPUUI)
#include <d2d1.h>
#include <dwrite_3.h>
#endif

namespace Upp {
namespace {
struct LayoutEntry {
	UiTextLayout layout;
	int64 bytes=0;
	uint64 used=0;
};
struct LayoutCache {
	Mutex mutex;
	ArrayMap<String,LayoutEntry> entries;
	UiTextLayoutStats stats;
	uint64 clock=0;
};
LayoutCache& Cache() { static LayoutCache cache; return cache; }
bool Finite(double value) { return std::isfinite(value) && fabs(value)<=1e7; }

#if defined(PLATFORM_WIN32) && defined(flagGPUUI)
template <class T> struct LayoutCom {
	T *p=nullptr;
	LayoutCom()=default;
	explicit LayoutCom(T *source):p(source) { if(p) p->AddRef(); }
	LayoutCom(const LayoutCom&)=delete;
	LayoutCom& operator=(const LayoutCom&)=delete;
	~LayoutCom() { if(p) p->Release(); }
	T **Put() { ASSERT(!p); return &p; }
	T *operator->() const { return p; }
};
struct LayoutFactory {
	LayoutCom<IDWriteFactory> factory;
	LayoutFactory() {
		DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,__uuidof(IDWriteFactory),
		                   reinterpret_cast<IUnknown **>(factory.Put()));
	}
};
LayoutFactory& Factory() { static LayoutFactory factory; return factory; }

class LayoutOutlineSink : public IDWriteGeometrySink {
	FontGlyphConsumer& out;
	double scale;
	ULONG refs=1;
	int commands=0;
public:
	bool failed=false;
	LayoutOutlineSink(FontGlyphConsumer& out,double scale):out(out),scale(scale) {}
	bool Admit() { if(++commands>4096) failed=true; return !failed; }
	Pointf Point(D2D1_POINT_2F p) const { return Pointf(p.x*scale,p.y); }
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void **object) override {
		static const GUID sink={0x2cd9069e,0x12e2,0x11dc,{0x9f,0xed,0x00,0x11,0x43,0xa0,0x55,0xf9}};
		if(!object) return E_POINTER; *object=nullptr;
		if(IsEqualIID(iid,sink)||IsEqualIID(iid,__uuidof(IUnknown))) {
			*object=static_cast<IDWriteGeometrySink *>(this); AddRef(); return S_OK;
		}
		return E_NOINTERFACE;
	}
	ULONG STDMETHODCALLTYPE AddRef() override { return ++refs; }
	ULONG STDMETHODCALLTYPE Release() override { ASSERT(refs>1); return --refs; }
	void STDMETHODCALLTYPE SetFillMode(D2D1_FILL_MODE) noexcept override {}
	void STDMETHODCALLTYPE SetSegmentFlags(D2D1_PATH_SEGMENT) noexcept override {}
	void STDMETHODCALLTYPE BeginFigure(D2D1_POINT_2F p,D2D1_FIGURE_BEGIN) noexcept override { if(Admit()) out.Move(Point(p)); }
	void STDMETHODCALLTYPE AddLines(const D2D1_POINT_2F *p,UINT count) noexcept override {
		for(UINT i=0;i<count;++i) if(Admit()) out.Line(Point(p[i]));
	}
	void STDMETHODCALLTYPE AddBeziers(const D2D1_BEZIER_SEGMENT *p,UINT count) noexcept override {
		for(UINT i=0;i<count;++i) if(Admit()) out.Cubic(Point(p[i].point1),Point(p[i].point2),Point(p[i].point3));
	}
	void STDMETHODCALLTYPE EndFigure(D2D1_FIGURE_END end) noexcept override {
		if(end==D2D1_FIGURE_END_CLOSED && Admit()) out.Close();
	}
	HRESULT STDMETHODCALLTYPE Close() noexcept override { return failed?E_OUTOFMEMORY:S_OK; }
};
class LayoutFace : public GpuGlyphFaceSource {
	LayoutCom<IDWriteFontFace> face;
	String identity;
	double scale;
public:
	LayoutFace(IDWriteFontFace *source,double scale):face(source),scale(scale) {
		UINT32 count=0;
		if(FAILED(face->GetFiles(&count,nullptr)) || count==0 || count>16) return;
		Buffer<IDWriteFontFile *> files(count); for(UINT32 i=0;i<count;++i) files[i]=nullptr;
		struct Guard { IDWriteFontFile **p; UINT32 count; ~Guard() { for(UINT32 i=0;i<count;++i) if(p[i]) p[i]->Release(); } } guard{~files,count};
		if(FAILED(face->GetFiles(&count,~files))) return;
		identity<<AsString((int)face->GetType())<<":"<<AsString((int)face->GetIndex())
		        <<":"<<AsString((int)face->GetSimulations())<<":";
		auto hex=[&](const void *bytes,int n) {
			const byte *p=(const byte *)bytes;
			for(int i=0;i<n;++i) { identity.Cat("0123456789abcdef"[p[i]>>4]); identity.Cat("0123456789abcdef"[p[i]&15]); }
		};
		hex(&scale,sizeof(scale));
		for(UINT32 i=0;i<count;++i) {
			const void *key=nullptr; UINT32 size=0;
			if(FAILED(files[i]->GetReferenceKey(&key,&size)) || !key || size>4096) { identity.Clear(); return; }
			identity<<":"<<AsString(size)<<":"; hex(key,size);
		}
		LayoutCom<IDWriteFontFace5> variable;
		if(SUCCEEDED(face->QueryInterface(__uuidof(IDWriteFontFace5),reinterpret_cast<void **>(variable.Put())))) {
			UINT32 axes=variable->GetFontAxisValueCount();
			if(axes>64) { identity.Clear(); return; }
			Buffer<DWRITE_FONT_AXIS_VALUE> values(axes);
			if(axes && FAILED(variable->GetFontAxisValues(~values,axes))) { identity.Clear(); return; }
			identity<<":axes:"<<axes<<":";
			for(UINT32 i=0;i<axes;++i) {
				if(!Finite(values[i].value)) { identity.Clear(); return; }
				hex(&values[i].axisTag,sizeof(values[i].axisTag));
				hex(&values[i].value,sizeof(values[i].value));
			}
		}
		if(identity.GetCount()>8192) identity.Clear();
	}
	const String& GetIdentity() const override { return identity; }
	bool GetOutline(uint32 glyph,double em,FontGlyphConsumer& out,String& error) const override {
		if(identity.IsEmpty() || glyph>65535 || !Finite(em) || em<=0 || em>512) {
			error="invalid shaped glyph outline request"; return false;
		}
		UINT16 index=(UINT16)glyph; FLOAT advance=0; LayoutOutlineSink sink(out,scale);
		HRESULT result=face->GetGlyphRunOutline((FLOAT)em,&index,&advance,nullptr,1,FALSE,FALSE,&sink);
		if(FAILED(result)||sink.failed) { error="DirectWrite shaped glyph outline failed or exceeded4096commands"; return false; }
		error.Clear(); return true;
	}
};

class RunCollector : public IDWriteTextRenderer {
	GpuShapedText& text;
	double scale,yoffset;
	ULONG refs=1;
public:
	RunCollector(GpuShapedText& text,double scale,double yoffset):text(text),scale(scale),yoffset(yoffset) {}
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void **object) override {
		if(!object) return E_POINTER; *object=nullptr;
		if(IsEqualIID(iid,__uuidof(IUnknown)) || IsEqualIID(iid,__uuidof(IDWriteTextRenderer)) ||
		   IsEqualIID(iid,__uuidof(IDWritePixelSnapping))) {
			*object=static_cast<IDWriteTextRenderer *>(this); AddRef(); return S_OK;
		} return E_NOINTERFACE;
	}
	ULONG STDMETHODCALLTYPE AddRef() override { return ++refs; }
	ULONG STDMETHODCALLTYPE Release() override { ASSERT(refs>1); return --refs; }
	HRESULT STDMETHODCALLTYPE IsPixelSnappingDisabled(void *,BOOL *disabled) override { *disabled=TRUE; return S_OK; }
	HRESULT STDMETHODCALLTYPE GetCurrentTransform(void *,DWRITE_MATRIX *matrix) override {
		*matrix={1,0,0,1,0,0}; return S_OK;
	}
	HRESULT STDMETHODCALLTYPE GetPixelsPerDip(void *,FLOAT *pixels) override { *pixels=1; return S_OK; }
	HRESULT STDMETHODCALLTYPE DrawGlyphRun(void *,FLOAT x,FLOAT y,DWRITE_MEASURING_MODE,
		const DWRITE_GLYPH_RUN *run,const DWRITE_GLYPH_RUN_DESCRIPTION *,IUnknown *) override {
		if(!run || !run->fontFace || run->isSideways || text.runs.GetCount()>=1024 ||
		   run->glyphCount>65536 || run->glyphCount>(UINT32)(65536-text.GetGlyphCount())) return E_INVALIDARG;
		auto source=std::make_shared<LayoutFace>(run->fontFace,scale);
		if(source->GetIdentity().IsEmpty()) return E_FAIL;
		auto& target=text.runs.Add(); target.face=source; target.em_size=run->fontEmSize;
		const bool rtl=(run->bidiLevel&1)!=0; double pen=x;
		for(UINT32 i=0;i<run->glyphCount;++i) {
			double advance=run->glyphAdvances?run->glyphAdvances[i]:0;
			DWRITE_GLYPH_OFFSET offset=run->glyphOffsets?run->glyphOffsets[i]:DWRITE_GLYPH_OFFSET{};
			auto& glyph=target.glyphs.Add(); glyph.glyph=run->glyphIndices[i];
			glyph.origin=Pointf((pen+(rtl?-advance:0)+(rtl?-offset.advanceOffset:offset.advanceOffset))*scale,
			                    y-offset.ascenderOffset+yoffset);
			pen+=rtl?-advance:advance;
		}
		return S_OK;
	}
	HRESULT Decoration(FLOAT x,FLOAT y,FLOAT width,FLOAT thickness,FLOAT offset,DWRITE_READING_DIRECTION direction) {
		if(width<=0 || thickness<=0) return S_OK;
		if(text.decorations.GetCount()>=4096) return E_OUTOFMEMORY;
		double left=direction==DWRITE_READING_DIRECTION_RIGHT_TO_LEFT?x-width:x;
		text.decorations.Add(Rectf(left*scale,y+offset+yoffset,(left+width)*scale,y+offset+thickness+yoffset));
		return S_OK;
	}
	HRESULT STDMETHODCALLTYPE DrawUnderline(void *,FLOAT x,FLOAT y,const DWRITE_UNDERLINE *line,IUnknown *) override {
		return Decoration(x,y,line->width,line->thickness,line->offset,line->readingDirection);
	}
	HRESULT STDMETHODCALLTYPE DrawStrikethrough(void *,FLOAT x,FLOAT y,const DWRITE_STRIKETHROUGH *line,IUnknown *) override {
		return Decoration(x,y,line->width,line->thickness,line->offset,line->readingDirection);
	}
	HRESULT STDMETHODCALLTYPE DrawInlineObject(void *,FLOAT,FLOAT,IDWriteInlineObject *,BOOL,BOOL,IUnknown *) override { return E_NOTIMPL; }
};
#endif
}

struct UiTextLayout::Data {
	UiTextInternal::Encoding encoded;
	UiTextLayoutOptions options;
	Font font;
	String error;
	Sizef size=Sizef(0,0);
	int64 payload=0;
	bool valid=false;
#if defined(PLATFORM_WIN32) && defined(flagGPUUI)
	struct Paragraph {
		int start=0,end=0,utf16_start=0;
		double y=0,height=0;
		LayoutCom<IDWriteTextLayout> native;
	};
	Array<Paragraph> paragraphs;
	Vector<int> graphemes;
	struct VisualStop : Moveable<VisualStop> { Pointf point; UiTextHit hit; };
	Vector<VisualStop> visual_stops;
	std::shared_ptr<GpuShapedText> glyphs=std::make_shared<GpuShapedText>();
	double scale=1;
	bool Check(HRESULT hr,const char *message) { if(SUCCEEDED(hr)) return true; error=message; return false; }
	bool Build(const WString& text,Font requested,const UiTextLayoutOptions& settings) {
		font=requested; font.RealizeStd(); options=settings;
		if(!encoded.Build(text,error)) return false;
		if(!Finite(options.width) || options.width<=0 || options.tab_spaces<1 || options.tab_spaces>64 ||
		   options.locale.GetCount()>256 || (int)options.direction<0 || (int)options.direction>2) {
			error="invalid text layout options"; return false;
		}
		if(!UiTextInternal::GraphemeBreaks(encoded,options.locale,graphemes,error)) return false;
		double em=font.GetHeight()?abs(font.GetHeight()):12;
		if(em<=0 || em>512) { error="text size exceeds512pixel glyph contract"; return false; }
		if(font.GetWidth()) {
			Font natural=font; natural.Width(0);
			scale=abs(font.GetWidth())/(double)max(1,natural.GetWidth('M'));
		}
		if(!Finite(scale) || scale<=0) { error="invalid text width scale"; return false; }
		auto *factory=Factory().factory.p;
		if(!factory) { error="DirectWrite text factory unavailable"; return false; }
		UiTextInternal::Encoding family,locale;
		String locale_name=options.locale;
		if(locale_name.IsEmpty()) {
			wchar_t buffer[LOCALE_NAME_MAX_LENGTH]{};
			if(GetUserDefaultLocaleName(buffer,LOCALE_NAME_MAX_LENGTH)) locale_name=FromSystemCharsetW(buffer);
		}
		if(!family.Build(font.GetFaceName().ToWString(),error) || !locale.Build(locale_name.ToWString(),error)) return false;
		int start=0;
		for(int end=0;;++end) {
			bool separator=end<text.GetCount() && (text[end]=='\r'||text[end]=='\n'||text[end]==0x2028||text[end]==0x2029);
			if(end<text.GetCount() && !separator) continue;
			if(paragraphs.GetCount()>=1024) { error="text exceeds 1024 paragraph layout limit"; return false; }
			auto& p=paragraphs.Add(); p.start=start; p.end=end; p.y=size.cy; p.utf16_start=encoded.scalar_to_utf16[start];
			bool rtl=options.direction==UiTextDirection::RightToLeft;
			if(options.direction==UiTextDirection::Auto) {
				UiTextInternal::Encoding part;
				if(!part.Build(text.Mid(start,end-start),error) || !UiTextInternal::BaseDirection(part,rtl,error)) return false;
			}
			LayoutCom<IDWriteTextFormat> format;
			if(!Check(factory->CreateTextFormat((const WCHAR *)family.utf16.Begin(),nullptr,
				font.IsBold()?DWRITE_FONT_WEIGHT_BOLD:DWRITE_FONT_WEIGHT_NORMAL,
				font.IsItalic()?DWRITE_FONT_STYLE_ITALIC:DWRITE_FONT_STYLE_NORMAL,
				DWRITE_FONT_STRETCH_NORMAL,(FLOAT)em,(const WCHAR *)locale.utf16.Begin(),format.Put()),"cannot create text format")) return false;
			if(!Check(format->SetReadingDirection(rtl?DWRITE_READING_DIRECTION_RIGHT_TO_LEFT:DWRITE_READING_DIRECTION_LEFT_TO_RIGHT),"invalid paragraph direction") ||
			   !Check(format->SetWordWrapping(options.wrap?DWRITE_WORD_WRAPPING_WRAP:DWRITE_WORD_WRAPPING_NO_WRAP),"invalid wrapping") ||
			   !Check(format->SetIncrementalTabStop((FLOAT)(max(1,font.GetWidth(' '))*options.tab_spaces/scale)),"invalid tab stop")) return false;
			UINT32 length=encoded.scalar_to_utf16[end]-p.utf16_start;
			if(!Check(factory->CreateTextLayout((const WCHAR *)encoded.utf16.Begin()+p.utf16_start,length,format.p,
				(FLOAT)(options.width/scale),10000000,p.native.Put()),"cannot create text layout")) return false;
			DWRITE_TEXT_RANGE range{0,length};
			if(font.IsUnderline() && !Check(p.native->SetUnderline(TRUE,range),"cannot underline text")) return false;
			if(font.IsStrikeout() && !Check(p.native->SetStrikethrough(TRUE,range),"cannot strike text")) return false;
			DWRITE_TEXT_METRICS metrics{};
			if(!Check(p.native->GetMetrics(&metrics),"cannot measure text")) return false;
			if(!options.wrap) {
				if(!Check(p.native->SetMaxWidth(max(1.0f,metrics.widthIncludingTrailingWhitespace)),"cannot fit intrinsic paragraph") ||
				   !Check(p.native->GetMetrics(&metrics),"cannot remeasure paragraph")) return false;
			}
			p.height=metrics.height; size.cx=max(size.cx,(double)metrics.widthIncludingTrailingWhitespace*scale); size.cy+=p.height;
			RunCollector collector(*glyphs,scale,p.y);
			if(!Check(p.native->Draw(nullptr,&collector,0,0),"cannot collect positioned glyphs")) return false;
			if(end==text.GetCount()) break;
			if(text[end]=='\r' && end+1<text.GetCount() && text[end+1]=='\n') ++end;
			start=end+1;
		}
		if(!glyphs->IsValid(&error)) return false;
		for(int position:graphemes) for(bool upstream:{false,true}) {
			Pointf point=Caret(position,upstream);
			if(!Finite(point.x)||!Finite(point.y)) { error="cannot obtain text caret geometry"; return false; }
			auto& stop=visual_stops.Add(); stop.point=point;
			stop.hit.position=position; stop.hit.trailing=upstream; stop.hit.inside=true;
		}
		Sort(visual_stops,[](const VisualStop& a,const VisualStop& b) {
			if(a.point.y!=b.point.y) return a.point.y<b.point.y;
			if(a.point.x!=b.point.x) return a.point.x<b.point.x;
			if(a.hit.position!=b.hit.position) return a.hit.position<b.hit.position;
			return a.hit.trailing<b.hit.trailing;
		});
		payload=glyphs->GetPayloadBytes()+(int64)encoded.text.GetCount()*4+
		        (int64)encoded.utf16.GetCount()*2+
		        (int64)(encoded.scalar_to_utf16.GetCount()+encoded.utf16_to_scalar.GetCount()+graphemes.GetCount())*4+
		        (int64)visual_stops.GetCount()*sizeof(VisualStop)+(int64)paragraphs.GetCount()*sizeof(Paragraph);
		valid=true; error.Clear(); return true;
	}
	int Snap(int position,bool up) const {
		position=minmax(position,0,encoded.text.GetCount());
		int i=FindLowerBound(graphemes,position);
		if(i<graphemes.GetCount() && graphemes[i]==position) return position;
		return up?graphemes[min(i,graphemes.GetCount()-1)]:graphemes[max(0,i-1)];
	}
	const Paragraph& ParagraphFor(int position) const {
		int i=0; while(i+1<paragraphs.GetCount() && position>=paragraphs[i+1].start) ++i;
		return paragraphs[i];
	}
	Pointf Caret(int position,bool upstream) const {
		position=Snap(position,upstream);
		const auto& p=ParagraphFor(position);
		int index=encoded.scalar_to_utf16[min(position,p.end)]-p.utf16_start;
		bool trailing=upstream && index>0;
		if(trailing) index=encoded.scalar_to_utf16[max(p.start,min(position,p.end)-1)]-p.utf16_start;
		FLOAT x=0,y=0; DWRITE_HIT_TEST_METRICS hit{};
		if(FAILED(p.native->HitTestTextPosition(index,trailing,&x,&y,&hit))) return Pointf(Null,Null);
		int start=encoded.utf16_to_scalar[p.utf16_start+hit.textPosition];
		int end=encoded.utf16_to_scalar[min(p.utf16_start+(int)(hit.textPosition+hit.length),encoded.scalar_to_utf16[p.end])];
		if(position>start && position<end && hit.width>0) {
			int first=FindLowerBound(graphemes,start),last=FindLowerBound(graphemes,end);
			int at=FindLowerBound(graphemes,position);
			if(last>first) {
				double fraction=(double)(at-first)/(last-first);
				x=(FLOAT)(hit.left+hit.width*((hit.bidiLevel&1)?1-fraction:fraction));
			}
		}
		return Pointf(x*scale,y+p.y);
	}
#else
	bool Build(const WString&,Font,const UiTextLayoutOptions&) { error="full text layout provider unavailable in this assembly"; return false; }
#endif
};

UiTextLayout UiTextLayout::Create(const WString& text,Font font,const UiTextLayoutOptions& options)
{
	UiTextLayout result;
	// Validate before serialising an untrusted-size key.
	if(text.GetCount()>32768 || options.locale.GetCount()>256 || options.locale.Find('\0')>=0 || !Finite(options.width) ||
	   options.width<=0 || options.tab_spaces<1 || options.tab_spaces>64 ||
	   (int)options.direction<0 || (int)options.direction>2) {
		result.data=std::make_shared<Data>(); result.data->error="invalid text layout input or options"; return result;
	}
	for(int i=0;i<text.GetCount();++i)
		if(text[i]>0x10ffff || (text[i]>=0xd800 && text[i]<=0xdfff)) {
			result.data=std::make_shared<Data>(); result.data->error="invalid Unicode scalar"; return result;
		}
	if(IsNull(font)) font=StdFont();
	font.RealizeStd();
	StringStream key_stream;
	int64 font_key=font.AsInt64(); int direction=(int)options.direction;
	double width=options.width; bool wrap=options.wrap; int tabs=options.tab_spaces; String locale=options.locale; WString key_text=text;
	key_stream % font_key % direction % width % wrap % tabs % locale % key_text;
	String key=key_stream.GetResult();
	auto& cache=Cache();
	{
		Mutex::Lock lock(cache.mutex); int i=cache.entries.Find(key);
		if(i>=0) { ++cache.stats.hits; auto& entry=cache.entries[i]; entry.used=++cache.clock; return entry.layout; }
		++cache.stats.misses;
	}
	result.data=std::make_shared<Data>();
	if(!result.data->Build(text,font,options)) return result;
	int64 bytes=result.data->payload+key.GetCount();
	Mutex::Lock lock(cache.mutex);
	int existing=cache.entries.Find(key); if(existing>=0) return cache.entries[existing].layout;
	if(bytes<=cache.stats.cache_payload_limit) {
		while(!cache.entries.IsEmpty() && (cache.entries.GetCount()>=cache.stats.cache_entry_limit ||
		      cache.stats.cache_payload_bytes+bytes>cache.stats.cache_payload_limit)) {
			int oldest=0; for(int i=1;i<cache.entries.GetCount();++i)
				if(cache.entries[i].used<cache.entries[oldest].used) oldest=i;
			cache.stats.cache_payload_bytes-=cache.entries[oldest].bytes; cache.entries.Remove(oldest);
		}
		auto& entry=cache.entries.Add(key); entry.layout=result; entry.bytes=bytes; entry.used=++cache.clock;
		cache.stats.cache_payload_bytes+=bytes;
	}
	return result;
}
bool UiTextLayout::IsValid() const { return data && data->valid; }
String UiTextLayout::GetError() const { return data?data->error:String("uninitialised text layout"); }
Sizef UiTextLayout::GetSize() const { return IsValid()?data->size:Sizef(0,0); }
int UiTextLayout::GetLength() const { return IsValid()?data->encoded.text.GetCount():0; }
int UiTextLayout::GetGlyphCount() const {
#if defined(PLATFORM_WIN32) && defined(flagGPUUI)
	return IsValid()?data->glyphs->GetGlyphCount():0;
#else
	return 0;
#endif
}
Pointf UiTextLayout::GetCaret(int position,bool upstream) const {
#if defined(PLATFORM_WIN32) && defined(flagGPUUI)
	if(IsValid()) return data->Caret(position,upstream);
#endif
	return Pointf(Null,Null);
}
UiTextHit UiTextLayout::HitTest(Pointf point) const {
	UiTextHit result;
#if defined(PLATFORM_WIN32) && defined(flagGPUUI)
	if(!IsValid() || !Finite(point.x) || !Finite(point.y)) return result;
	int index=0;
	while(index+1<data->paragraphs.GetCount() && point.y>=data->paragraphs[index+1].y) ++index;
	const auto& p=data->paragraphs[index];
	BOOL trailing=FALSE,inside=FALSE; DWRITE_HIT_TEST_METRICS hit{};
	if(FAILED(p.native->HitTestPoint((FLOAT)(point.x/data->scale),(FLOAT)(point.y-p.y),&trailing,&inside,&hit))) return result;
	int pos=p.utf16_start+min<UINT32>(data->encoded.scalar_to_utf16[p.end]-p.utf16_start,hit.textPosition+(trailing?hit.length:0));
	result.position=data->Snap(data->encoded.utf16_to_scalar[pos],trailing);
	int start=data->encoded.utf16_to_scalar[p.utf16_start+hit.textPosition];
	int end=data->encoded.utf16_to_scalar[min(p.utf16_start+(int)(hit.textPosition+hit.length),data->encoded.scalar_to_utf16[p.end])];
	int first=FindLowerBound(data->graphemes,start),last=FindLowerBound(data->graphemes,end);
	if(last>first+1 && hit.width>0) {
		double fraction=(point.x/data->scale-hit.left)/hit.width;
		if(hit.bidiLevel&1) fraction=1-fraction;
		int at=minmax((int)floor(fraction*(last-first)+.5),0,last-first);
		result.position=data->graphemes[first+at]; trailing=at==last-first;
	}
	result.trailing=trailing; result.inside=inside;
#endif
	return result;
}
Vector<Rectf> UiTextLayout::GetSelection(int from,int to) const {
	Vector<Rectf> result;
#if defined(PLATFORM_WIN32) && defined(flagGPUUI)
	if(!IsValid()) return result;
	if(from>to) Swap(from,to); from=data->Snap(from,false); to=data->Snap(to,true);
	if(from==to) return result;
	for(const auto& p:data->paragraphs) {
		int a=max(from,p.start),b=min(to,p.end); if(a>=b) continue;
		UINT32 count=0,start=data->encoded.scalar_to_utf16[a]-p.utf16_start;
		FLOAT x=0,y=0; DWRITE_HIT_TEST_METRICS first_hit{},last_hit{};
		if(FAILED(p.native->HitTestTextPosition(start,FALSE,&x,&y,&first_hit)) ||
		   FAILED(p.native->HitTestTextPosition(data->encoded.scalar_to_utf16[b-1]-p.utf16_start,TRUE,&x,&y,&last_hit)))
			return Vector<Rectf>();
		// DirectWrite may return an empty rectangle for a range ending inside
		// a ligature. Request complete native clusters, then trim each visual
		// segment to the grapheme-level caret positions below.
		start=first_hit.textPosition;
		UINT32 end=min<UINT32>(data->encoded.scalar_to_utf16[p.end]-p.utf16_start,
		                      last_hit.textPosition+last_hit.length);
		if(end<=start) continue;
		UINT32 length=end-start;
		p.native->HitTestTextRange(start,length,0,0,nullptr,0,&count);
		if(count>65536) return Vector<Rectf>();
		Buffer<DWRITE_HIT_TEST_METRICS> hits(count);
		if(FAILED(p.native->HitTestTextRange(start,length,0,0,~hits,count,&count))) return Vector<Rectf>();
		for(UINT32 i=0;i<count;++i) if(hits[i].length>0) {
			int first=max(a,data->encoded.utf16_to_scalar[p.utf16_start+hits[i].textPosition]);
			int end=min(b,data->encoded.utf16_to_scalar[min(p.utf16_start+(int)(hits[i].textPosition+hits[i].length),data->encoded.scalar_to_utf16[p.end])]);
			if(first>=end) continue;
			double left=data->Caret(first,false).x,right=data->Caret(end,true).x;
			if(left!=right)
				result.Add(Rectf(min(left,right),hits[i].top+p.y,max(left,right),hits[i].top+p.y+hits[i].height));
		}
	}
#endif
	return result;
}
int UiTextLayout::NextGrapheme(int position,int direction) const {
#if defined(PLATFORM_WIN32) && defined(flagGPUUI)
	if(IsValid()) {
		position=minmax(position,0,GetLength()); const auto& boundaries=data->graphemes;
		int i=FindLowerBound(boundaries,position);
		if(direction<0) return boundaries[max(0,i-1)];
		if(direction>0) return boundaries[min(boundaries.GetCount()-1,i+(i<boundaries.GetCount()&&boundaries[i]==position))];
		return data->Snap(position,false);
	}
#endif
	return 0;
}
UiTextHit UiTextLayout::MoveVisual(UiTextHit current,int direction) const {
#if defined(PLATFORM_WIN32) && defined(flagGPUUI)
	if(IsValid() && direction) {
		Pointf point=GetCaret(current.position,current.trailing);
		const auto& stops=data->visual_stops;
		int lo=0,hi=stops.GetCount();
		// Binary-search the retained visual stops. Arrow keys do not rescan all
		// graphemes or repeat native hit-test calls for the whole paragraph.
		while(lo<hi) {
			int mid=lo+(hi-lo)/2; const Pointf& p=stops[mid].point;
			bool before=p.y<point.y-.01 ||
			            (fabs(p.y-point.y)<.01 && (direction<0?p.x<point.x-.01:p.x<=point.x+.01));
			if(before) lo=mid+1; else hi=mid;
		}
		int index=direction<0?lo-1:lo;
		UiTextHit next=current;
		if(index>=0 && index<stops.GetCount() && fabs(stops[index].point.y-point.y)<.01)
			next=stops[index].hit;
		return next;
	}
#endif
	return current;
}
#ifdef flagGPUUI
std::shared_ptr<const GpuShapedText> UiTextLayout::GetGpuText() const {
#ifdef PLATFORM_WIN32
	if(IsValid()) return data->glyphs;
#endif
	return {};
}
#endif
namespace {
#ifdef flagGPUUI
struct LayoutPainterSink : FontGlyphConsumer {
	Painter& out; Pointf origin; bool used=false;
	LayoutPainterSink(Painter& out,Pointf origin):out(out),origin(origin) {}
	void Move(Pointf p) override { out.Move(p+origin); used=true; }
	void Line(Pointf p) override { out.Line(p+origin); }
	void Quadratic(Pointf a,Pointf b) override { out.Quadratic(a+origin,b+origin); }
	void Cubic(Pointf a,Pointf b,Pointf c) override { out.Cubic(a+origin,b+origin,c+origin); }
	void Close() override { out.Close(); }
};
#endif
}
bool UiTextLayout::Draw(Upp::Draw& draw,Pointf origin,Color ink) const {
#ifdef flagGPUUI
	if(auto *target=dynamic_cast<GpuDrawTarget *>(&draw))
		return target->DrawShapedText(origin,GetGpuText(),ink);
	if(auto *target=dynamic_cast<GpuPainterEffects *>(&draw))
		return target->DrawShapedText(origin,GetGpuText(),ink);
	if(IsValid()) if(auto *painter=dynamic_cast<Painter *>(&draw)) {
		painter->Begin(); painter->EvenOdd(false); String error;
		const auto glyphs=GetGpuText();
		for(const auto& run:glyphs->runs) for(const auto& glyph:run.glyphs) {
			LayoutPainterSink sink(*painter,origin+glyph.origin);
			if(!run.face->GetOutline(glyph.glyph,run.em_size,sink,error)) { painter->End(); return false; }
			if(sink.used) painter->Fill(ink);
		}
		for(const auto& r:glyphs->decorations) painter->Rectangle(r+origin).Fill(ink);
		painter->End(); return true;
	}
#endif
	return false;
}
UiTextLayoutStats GetUiTextLayoutStats() {
	auto& cache=Cache(); Mutex::Lock lock(cache.mutex);
	auto stats=cache.stats; stats.cache_entries=cache.entries.GetCount(); return stats;
}
void ClearUiTextLayoutCache() {
	auto& cache=Cache(); Mutex::Lock lock(cache.mutex);
	cache.entries.Clear(); cache.stats=UiTextLayoutStats();
}
Size UiGetTextSize(const WString& text,Font font)
{
#ifdef flagGPUUI
	auto layout=UiTextLayout::Create(text,font);
	Sizef size=layout.GetSize();
	return Size((int)ceil(size.cx),(int)ceil(size.cy));
#else
	return GetTextSize(text,font);
#endif
}
WString UiFitText(const WString& text,Font font,int width,const WString& tail)
{
	if(width<=0 || text.IsEmpty()) return WString();
	if(UiGetTextSize(text,font).cx<=width) return text;
	if(UiGetTextSize(tail,font).cx>width) return WString();
	Vector<int> stops; stops.Add(0);
#ifdef flagGPUUI
	auto layout=UiTextLayout::Create(text,font);
	if(!layout.IsValid()) return text; // The drawing receiver must report the failure.
	for(int pos=0;pos<text.GetCount();) { pos=layout.NextGrapheme(pos,1); stops.Add(pos); }
#else
	for(int i=1;i<=text.GetCount();++i) stops.Add(i);
#endif
	int lo=0,hi=stops.GetCount()-1;
	while(lo<hi) {
		int mid=lo+(hi-lo+1)/2;
		if(UiGetTextSize(text.Left(stops[mid])+tail,font).cx<=width) lo=mid; else hi=mid-1;
	}
	return text.Left(stops[lo])+tail;
}
void UiDrawTextEllipsis(Upp::Draw& draw,int x,int y,int width,const WString& text,
                        const char *tail,Font font,Color ink)
{
	UiDrawText(draw,x,y,UiFitText(text,font,width,String(tail).ToWString()),font,ink);
}
void UiDrawText(Upp::Draw& draw,int x,int y,const WString& text,Font font,Color ink)
{
	if(text.IsEmpty() || IsNull(ink)) return;
#ifdef flagGPUUI
	if(dynamic_cast<GpuDrawTarget *>(&draw) || dynamic_cast<GpuPainterEffects *>(&draw) || dynamic_cast<Painter *>(&draw)) {
		UiTextLayout::Create(text,font).Draw(draw,Pointf(x,y),ink); return;
	}
#endif
	draw.DrawText(x,y,text,font,ink);
}
bool UiDrawTextSelection(Upp::Draw& draw,Point origin,const UiTextLayout& layout,
                         Color ink,int from,int to,Color background,Color selected_ink)
{
	if(!layout.IsValid()) return false;
	auto rectangles=layout.GetSelection(from,to);
	for(const auto& r:rectangles)
		draw.DrawRect((int)floor(origin.x+r.left),(int)floor(origin.y+r.top),
		              (int)ceil(r.right)-(int)floor(r.left),(int)ceil(r.bottom)-(int)floor(r.top),background);
	if(!layout.Draw(draw,Pointf(origin),ink)) return false;
	for(const auto& r:rectangles) {
		draw.Clip(Rect((int)floor(origin.x+r.left),(int)floor(origin.y+r.top),
		               (int)ceil(origin.x+r.right),(int)ceil(origin.y+r.bottom)));
		bool ok=layout.Draw(draw,Pointf(origin),selected_ink); draw.End();
		if(!ok) return false;
	}
	return true;
}

}
