#include "UiTextLayout.h"
#include "UiTextLayoutInternal.h"
#include <Painter/Painter.h>
#ifdef flagGPUUI
#include <CtrlCore/CtrlCore.h>
#endif
#include <cmath>
#ifdef flagGPUUI
#include <RenderCore/GpuDrawTarget.h>
#endif
#if defined(PLATFORM_WIN32) && defined(flagGPUUI)
#include <d2d1.h>
#include <dwrite_3.h>
#include <wincodec.h>
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


struct FontBitmapEntry {
 Image image; int64 bytes=0; uint64 used=0;
};
struct FontBitmapCache {
 Mutex mutex; ArrayMap<String,FontBitmapEntry> entries;
 int64 bytes=0; uint64 clock=0,hits=0,misses=0;
};
FontBitmapCache& BitmapCache() { static FontBitmapCache cache; return cache; }
Image DecodeFontBitmap(const DWRITE_GLYPH_IMAGE_DATA& data,DWRITE_GLYPH_IMAGE_FORMATS format)
{
 if(!data.imageData || !data.imageDataSize || data.imageDataSize>4*1024*1024 ||
    !data.pixelSize.width || !data.pixelSize.height || data.pixelSize.width>1024 || data.pixelSize.height>1024) return Image();
 const Size size((int)data.pixelSize.width,(int)data.pixelSize.height);
 ImageBuffer image(size);
 if(format==DWRITE_GLYPH_IMAGE_FORMATS_PREMULTIPLIED_B8G8R8A8) {
  if((int64)size.cx*size.cy*4!=data.imageDataSize) return Image();
  const byte *source=(const byte *)data.imageData;
  for(int i=0;i<image.GetLength();++i) {
   RGBA& p=image.Begin()[i]; p.r=source[4*i+2]; p.g=source[4*i+1]; p.b=source[4*i]; p.a=source[4*i+3];
   if(p.r>p.a || p.g>p.a || p.b>p.a) return Image();
  }
  return Image(image);
 }
 const HRESULT apartment=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
 struct Apartment { bool release; ~Apartment() { if(release) CoUninitialize(); } } guard{SUCCEEDED(apartment)};
 if(FAILED(apartment) && apartment!=RPC_E_CHANGED_MODE) return Image();
 LayoutCom<IWICImagingFactory> factory; LayoutCom<IWICStream> stream;
 LayoutCom<IWICBitmapDecoder> decoder; LayoutCom<IWICBitmapFrameDecode> frame;
 LayoutCom<IWICFormatConverter> converter;
 if(FAILED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(factory.Put()))) ||
    FAILED(factory->CreateStream(stream.Put())) ||
    FAILED(stream->InitializeFromMemory((BYTE *)data.imageData,data.imageDataSize)) ||
    FAILED(factory->CreateDecoderFromStream(stream.p,nullptr,WICDecodeMetadataCacheOnDemand,decoder.Put())) ||
    FAILED(decoder->GetFrame(0,frame.Put()))) return Image();
 UINT width=0,height=0;
 if(FAILED(frame->GetSize(&width,&height)) || width!=data.pixelSize.width || height!=data.pixelSize.height ||
    FAILED(factory->CreateFormatConverter(converter.Put())) ||
    FAILED(converter->Initialize(frame.p,GUID_WICPixelFormat32bppPRGBA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom))) return Image();
 Vector<byte> bytes; bytes.SetCount(size.cx*size.cy*4);
 if(FAILED(converter->CopyPixels(nullptr,size.cx*4,bytes.GetCount(),bytes.Begin()))) return Image();
 for(int i=0;i<image.GetLength();++i) {
  RGBA& p=image.Begin()[i]; p.r=bytes[4*i]; p.g=bytes[4*i+1]; p.b=bytes[4*i+2]; p.a=bytes[4*i+3];
 }
 return Image(image);
}
Image FontBitmap(const String& identity,const DWRITE_GLYPH_IMAGE_DATA& data,DWRITE_GLYPH_IMAGE_FORMATS format)
{
 String key=identity+":"+AsString((int)format)+":"+AsString(data.uniqueDataId);
 auto& cache=BitmapCache();
 {
  Mutex::Lock lock(cache.mutex); int found=cache.entries.Find(key);
  if(found>=0) { ++cache.hits; cache.entries[found].used=++cache.clock; return cache.entries[found].image; }
  ++cache.misses;
 }
 Image image=DecodeFontBitmap(data,format); if(image.IsEmpty()) return image;
 int64 bytes=(int64)image.GetLength()*sizeof(RGBA)+key.GetCount()+sizeof(FontBitmapEntry);
 if(bytes>4*1024*1024) return Image();
 Mutex::Lock lock(cache.mutex); int found=cache.entries.Find(key);
 if(found>=0) return cache.entries[found].image;
 while(!cache.entries.IsEmpty() && (cache.entries.GetCount()>=128 || cache.bytes>4*1024*1024-bytes)) {
  int oldest=0; for(int i=1;i<cache.entries.GetCount();++i) if(cache.entries[i].used<cache.entries[oldest].used) oldest=i;
  cache.bytes-=cache.entries[oldest].bytes; cache.entries.Remove(oldest);
 }
 auto& entry=cache.entries.Add(key); entry.image=image; entry.bytes=bytes; entry.used=++cache.clock; cache.bytes+=bytes;
 return image;
}

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

// Private effects are retained by DirectWrite together with the immutable layout.
class LayoutInk : public IUnknown {
	LONG refs=1;
public:
	Color ink;
	explicit LayoutInk(Color ink):ink(ink) {}
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void **object) override {
		if(!object) return E_POINTER; *object=nullptr;
		if(IsEqualIID(iid,__uuidof(IUnknown))) { *object=this; AddRef(); return S_OK; }
		return E_NOINTERFACE;
	}
	ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&refs); }
	ULONG STDMETHODCALLTYPE Release() override { ULONG left=InterlockedDecrement(&refs); if(!left) delete this; return left; }
};
Color EffectInk(IUnknown *effect) {
	// This provider installs only LayoutInk effects; native Draw returns them.
	auto *ink=dynamic_cast<LayoutInk *>(effect); return ink?ink->ink:Color(Null);
}
class LayoutInline : public IDWriteInlineObject {
	LONG refs=1;
public:
	int position;
	DWRITE_INLINE_OBJECT_METRICS metrics{};
	LayoutInline(const UiTextInlineObject& object,double scale):position(object.position) {
		metrics.width=(FLOAT)(object.size.cx/scale); metrics.height=(FLOAT)object.size.cy;
		metrics.baseline=(FLOAT)(object.baseline<0?object.size.cy:object.baseline);
	}
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void **object) override {
		if(!object) return E_POINTER; *object=nullptr;
		if(IsEqualIID(iid,__uuidof(IUnknown))||IsEqualIID(iid,__uuidof(IDWriteInlineObject))) {
			*object=static_cast<IDWriteInlineObject *>(this); AddRef(); return S_OK;
		} return E_NOINTERFACE;
	}
	ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&refs); }
	ULONG STDMETHODCALLTYPE Release() override { ULONG left=InterlockedDecrement(&refs); if(!left) delete this; return left; }
	HRESULT STDMETHODCALLTYPE Draw(void *,IDWriteTextRenderer *,FLOAT,FLOAT,BOOL,BOOL,IUnknown *) override { return S_OK; }
	HRESULT STDMETHODCALLTYPE GetMetrics(DWRITE_INLINE_OBJECT_METRICS *out) override { if(!out) return E_POINTER; *out=metrics; return S_OK; }
	HRESULT STDMETHODCALLTYPE GetOverhangMetrics(DWRITE_OVERHANG_METRICS *out) override { if(!out) return E_POINTER; *out={}; return S_OK; }
	HRESULT STDMETHODCALLTYPE GetBreakConditions(DWRITE_BREAK_CONDITION *before,DWRITE_BREAK_CONDITION *after) override {
		if(!before||!after) return E_POINTER; *before=*after=DWRITE_BREAK_CONDITION_NEUTRAL; return S_OK;
	}
};

class RunCollector : public IDWriteTextRenderer {
	GpuShapedText& text;
	Vector<UiTextInlinePlacement>& objects;
	const Vector<UiTextLine>& lines;
	Vector<int>& run_lines;
	Vector<int>& decoration_lines;
	double scale,yoffset;
	bool color_fonts;
	ArrayMap<IDWriteFontFace *,std::shared_ptr<LayoutFace>> faces;
	ULONG refs=1;
public:
	RunCollector(GpuShapedText& text,Vector<UiTextInlinePlacement>& objects,const Vector<UiTextLine>& lines,
	             Vector<int>& run_lines,Vector<int>& decoration_lines,double scale,double yoffset,bool color_fonts)
	:text(text),objects(objects),lines(lines),run_lines(run_lines),decoration_lines(decoration_lines),scale(scale),yoffset(yoffset),color_fonts(color_fonts) {}
	int LineAt(double y) const {
		y+=yoffset; int lo=0,hi=lines.GetCount();
		while(lo<hi) { int mid=(lo+hi)/2; if(lines[mid].bounds.top+lines[mid].baseline<y) lo=mid+1; else hi=mid; }
		if(lo==lines.GetCount()) return lo-1;
		if(lo && fabs(lines[lo-1].bounds.top+lines[lo-1].baseline-y)<fabs(lines[lo].bounds.top+lines[lo].baseline-y)) --lo;
		return lo;
	}
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

 HRESULT Collect(FLOAT x,FLOAT y,const DWRITE_GLYPH_RUN *run,Color ink,byte opacity,
                 DWRITE_GLYPH_IMAGE_FORMATS format=DWRITE_GLYPH_IMAGE_FORMATS_TRUETYPE) {
  if(!run || !run->fontFace || run->isSideways || run->glyphCount>65536 ||
     run->glyphCount>(UINT32)(65536-text.GetGlyphCount())) return E_INVALIDARG;
  int face_index=faces.Find(run->fontFace);
  if(face_index<0) { face_index=faces.GetCount(); faces.Add(run->fontFace,std::make_shared<LayoutFace>(run->fontFace,scale)); }
  auto source=faces[face_index];
  if(source->GetIdentity().IsEmpty()) return E_FAIL;
  const bool bitmap=format==DWRITE_GLYPH_IMAGE_FORMATS_PNG || format==DWRITE_GLYPH_IMAGE_FORMATS_JPEG ||
    format==DWRITE_GLYPH_IMAGE_FORMATS_TIFF || format==DWRITE_GLYPH_IMAGE_FORMATS_PREMULTIPLIED_B8G8R8A8;
  LayoutCom<IDWriteFontFace4> face4;
  if(bitmap && FAILED(run->fontFace->QueryInterface(IID_PPV_ARGS(face4.Put())))) return E_NOINTERFACE;
  GpuGlyphRunSnapshot *target=nullptr;
  if(!bitmap) {
   if(text.runs.GetCount()>=1024) return E_OUTOFMEMORY;
   target=&text.runs.Add(); target->face=source; target->em_size=run->fontEmSize; target->ink=ink; target->opacity=opacity;
   run_lines.Add(LineAt(y));
  }
  const bool rtl=(run->bidiLevel&1)!=0; double pen=x;
  for(UINT32 i=0;i<run->glyphCount;++i) {
   const double advance=run->glyphAdvances?run->glyphAdvances[i]:0;
   const DWRITE_GLYPH_OFFSET offset=run->glyphOffsets?run->glyphOffsets[i]:DWRITE_GLYPH_OFFSET{};
   Pointf origin((pen+(rtl?-advance:0)+(rtl?-offset.advanceOffset:offset.advanceOffset))*scale,y-offset.ascenderOffset+yoffset);
   if(bitmap) {
    if(text.runs.GetCount()>=1024) return E_OUTOFMEMORY;
    DWRITE_GLYPH_IMAGE_DATA data{}; void *context=nullptr;
    HRESULT hr=face4->GetGlyphImageData(run->glyphIndices[i],(UINT32)ceil(run->fontEmSize),format,&data,&context);
    struct Guard { IDWriteFontFace4 *face; void *context; ~Guard() { if(context) face->ReleaseGlyphImageData(context); } } guard{face4.p,context};
    if(FAILED(hr) || !data.pixelsPerEm || !data.imageDataSize) return E_FAIL;
    Image image=FontBitmap(source->GetIdentity(),data,format); if(image.IsEmpty()) return E_FAIL;
    target=&text.runs.Add(); target->face=source; target->em_size=run->fontEmSize; target->ink=ink; target->opacity=opacity;
    run_lines.Add(LineAt(y)); target->bitmap=image;
    double ratio=(double)run->fontEmSize/data.pixelsPerEm;
    Pointf top=origin-Pointf(data.horizontalLeftOrigin.x*ratio*scale,data.horizontalLeftOrigin.y*ratio);
    target->bitmap_bounds=Rectf(top,Sizef(image.GetWidth()*ratio*scale,image.GetHeight()*ratio));
   }
   auto& glyph=target->glyphs.Add(); glyph.glyph=run->glyphIndices[i]; glyph.origin=origin;
   pen+=rtl?-advance:advance;
  }
  return text.GetPayloadBytes()<=4*1024*1024?S_OK:E_OUTOFMEMORY;
 }
 HRESULT STDMETHODCALLTYPE DrawGlyphRun(void *,FLOAT x,FLOAT y,DWRITE_MEASURING_MODE measuring,
  const DWRITE_GLYPH_RUN *run,const DWRITE_GLYPH_RUN_DESCRIPTION *description,IUnknown *effect) override {
  if(!run || !run->fontFace) return E_INVALIDARG;
  LayoutCom<IDWriteFontFace2> face2;
  if(!color_fonts || (SUCCEEDED(run->fontFace->QueryInterface(IID_PPV_ARGS(face2.Put()))) && !face2->IsColorFont()))
   return Collect(x,y,run,EffectInk(effect),255);
  LayoutCom<IDWriteFactory4> factory;
  if(FAILED(Factory().factory->QueryInterface(IID_PPV_ARGS(factory.Put())))) return E_NOINTERFACE;
  LayoutCom<IDWriteColorGlyphRunEnumerator1> layers;
  auto formats=(DWRITE_GLYPH_IMAGE_FORMATS)(DWRITE_GLYPH_IMAGE_FORMATS_TRUETYPE|DWRITE_GLYPH_IMAGE_FORMATS_CFF|
    DWRITE_GLYPH_IMAGE_FORMATS_COLR|DWRITE_GLYPH_IMAGE_FORMATS_PNG|DWRITE_GLYPH_IMAGE_FORMATS_JPEG|
    DWRITE_GLYPH_IMAGE_FORMATS_TIFF|DWRITE_GLYPH_IMAGE_FORMATS_PREMULTIPLIED_B8G8R8A8);
  HRESULT hr=factory->TranslateColorGlyphRun(D2D1_POINT_2F{x,y},run,description,formats,measuring,nullptr,0,layers.Put());
  if(hr==DWRITE_E_NOCOLOR) return Collect(x,y,run,EffectInk(effect),255);
  if(FAILED(hr)) return hr;
  for(int count=0;count<1024;++count) {
   BOOL more=FALSE; hr=layers->MoveNext(&more); if(FAILED(hr) || !more) return hr;
   const DWRITE_COLOR_GLYPH_RUN1 *layer=nullptr;
   if(FAILED(hr=layers->GetCurrentRun(&layer)) || !layer) return E_FAIL;
   Color ink=EffectInk(effect); byte alpha=255;
   if(layer->paletteIndex!=0xffff) {
    auto channel=[](float value) { return (int)floor(minmax<double>(value,0,1)*255+.5); };
    if(!Finite(layer->runColor.r)||!Finite(layer->runColor.g)||!Finite(layer->runColor.b)||!Finite(layer->runColor.a)) return E_INVALIDARG;
    ink=Color(channel(layer->runColor.r),channel(layer->runColor.g),channel(layer->runColor.b)); alpha=channel(layer->runColor.a);
   }
   hr=Collect(layer->baselineOriginX,layer->baselineOriginY,&layer->glyphRun,ink,alpha,layer->glyphImageFormat);
   if(FAILED(hr)) return hr;
  }
  return E_OUTOFMEMORY;
 }

	HRESULT Decoration(FLOAT x,FLOAT y,FLOAT width,FLOAT thickness,FLOAT offset,DWRITE_READING_DIRECTION direction,Color ink) {
		if(width<=0 || thickness<=0) return S_OK;
		if(text.decorations.GetCount()>=4096) return E_OUTOFMEMORY;
		double left=direction==DWRITE_READING_DIRECTION_RIGHT_TO_LEFT?x-width:x;
		decoration_lines.Add(LineAt(y));
		text.decorations.Add(GpuTextDecoration(Rectf(left*scale,y+offset+yoffset,(left+width)*scale,y+offset+thickness+yoffset),ink));
		return S_OK;
	}
	HRESULT STDMETHODCALLTYPE DrawUnderline(void *,FLOAT x,FLOAT y,const DWRITE_UNDERLINE *line,IUnknown *effect) override {
		return Decoration(x,y,line->width,line->thickness,line->offset,line->readingDirection,EffectInk(effect));
	}
	HRESULT STDMETHODCALLTYPE DrawStrikethrough(void *,FLOAT x,FLOAT y,const DWRITE_STRIKETHROUGH *line,IUnknown *effect) override {
		return Decoration(x,y,line->width,line->thickness,line->offset,line->readingDirection,EffectInk(effect));
	}
	HRESULT STDMETHODCALLTYPE DrawInlineObject(void *,FLOAT x,FLOAT y,IDWriteInlineObject *object,BOOL sideways,BOOL,IUnknown *) override {
		auto *placed=dynamic_cast<LayoutInline *>(object);
		if(!placed || sideways || objects.GetCount()>=1024) return E_INVALIDARG;
		auto& out=objects.Add(); out.position=placed->position;
		out.bounds=Rectf(x*scale,y+yoffset,(x+placed->metrics.width)*scale,y+yoffset+placed->metrics.height);
		return S_OK;
	}
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
	Vector<UiTextLine> lines;
	Vector<UiTextInlinePlacement> objects;
#if defined(PLATFORM_WIN32) && defined(flagGPUUI)
	struct Paragraph {
		int start=0,end=0,utf16_start=0;
		double y=0,height=0;
		LayoutCom<IDWriteTextLayout> native;
	};
	Array<Paragraph> paragraphs;
	Vector<int> graphemes,words;
	struct VisualStop : Moveable<VisualStop> { Pointf point; UiTextHit hit; };
	Vector<VisualStop> visual_stops;
	std::shared_ptr<GpuShapedText> glyphs=std::make_shared<GpuShapedText>();
	Array<std::shared_ptr<const GpuShapedText>> line_glyphs;
	double scale=1;
	bool Check(HRESULT hr,const char *message) { if(SUCCEEDED(hr)) return true; error=message; return false; }
	bool Build(const WString& text,Font requested,const UiTextLayoutOptions& settings,
	           const Vector<UiTextStyleRange>& styles,const Vector<UiTextInlineObject>& inline_objects) {
		font=requested; font.RealizeStd(); options=settings;
		if(!encoded.Build(text,error)) return false;
		if(!Finite(options.width) || options.width<=0 || options.tab_spaces<1 || options.tab_spaces>64 ||
		   options.locale.GetCount()>256 || (int)options.direction<0 || (int)options.direction>2 || (int)options.alignment<0 || (int)options.alignment>3) {
			error="invalid text layout options"; return false;
		}
		if(!UiTextInternal::GraphemeBreaks(encoded,options.locale,graphemes,error) ||
		   !UiTextInternal::WordBreaks(encoded,options.locale,words,error)) return false;
		double em=font.GetHeight()?abs(font.GetHeight()):max(1,StdFont().GetHeight());
		if(em<=0 || em>512) { error="text size exceeds512pixel glyph contract"; return false; }
		if(font.GetWidth()) {
			Font natural=font; natural.Width(0);
			scale=abs(font.GetWidth())/(double)max(1,natural.GetWidth('M'));
		}
		if(!Finite(scale) || scale<=0) { error="invalid text width scale"; return false; }
		auto *factory=Factory().factory.p;
		if(!factory) { error="DirectWrite text factory unavailable"; return false; }
		UiTextInternal::Encoding family,locale;
		double tab_advance=max(1,font.GetWidth(' '))/scale;
		String locale_name=options.locale;
		if(locale_name.IsEmpty()) {
			wchar_t buffer[LOCALE_NAME_MAX_LENGTH]{};
			if(GetUserDefaultLocaleName(buffer,LOCALE_NAME_MAX_LENGTH)) locale_name=FromSystemCharsetW(buffer);
		}
		if(!family.Build(font.GetFaceName().ToWString(),error) || !locale.Build(locale_name.ToWString(),error)) return false;
		LayoutCom<IDWriteFontCollection1> private_fonts;
		if(!options.font_file.IsEmpty()) {
			String path=GetFullPath(options.font_file);
			const int64 bytes=GetFileLength(path);
			if(bytes<=0 || bytes>256*1024*1024) { error="font file is missing or exceeds256MiB"; return false; }
			UiTextInternal::Encoding file_path;
			if(!file_path.Build(path.ToWString(),error)) return false;
			LayoutCom<IDWriteFactory5> modern; LayoutCom<IDWriteFontFile> file;
			LayoutCom<IDWriteFontSetBuilder1> builder; LayoutCom<IDWriteFontSet> set;
			if(!Check(factory->QueryInterface(IID_PPV_ARGS(modern.Put())),"private fonts need DirectWrite factory5") ||
			   !Check(factory->CreateFontFileReference((const WCHAR *)file_path.utf16.Begin(),nullptr,file.Put()),"cannot open application font file") ||
			   !Check(modern->CreateFontSetBuilder(builder.Put()),"cannot create application font set") ||
			   !Check(builder->AddFontFile(file.p),"unsupported application font file") ||
			   !Check(builder->CreateFontSet(set.Put()),"cannot create application font set") ||
			   !Check(modern->CreateFontCollectionFromFontSet(set.p,private_fonts.Put()),"cannot create application font collection")) return false;
			UINT32 index=0; BOOL found=FALSE;
			if(!Check(private_fonts->FindFamilyName((const WCHAR *)family.utf16.Begin(),&index,&found),"cannot find application font family")) return false;
			if(!found) index=0;
			LayoutCom<IDWriteFontFamily1> first; LayoutCom<IDWriteLocalizedStrings> names;
			if(!Check(private_fonts->GetFontFamily(index,first.Put()),"application font has no family") ||
			   !Check(first->GetFamilyNames(names.Put()),"application font family has no name")) return false;
			UINT32 length=0;
			if(!Check(names->GetStringLength(0,&length),"cannot read application font family") || length>4096) return false;
			Buffer<WCHAR> name(length+1);
			if(!Check(names->GetString(0,~name,length+1),"cannot read application font family") ||
			   !family.Build(FromSystemCharsetW(~name).ToWString(),error)) return false;
			LayoutCom<IDWriteFont> metrics_font; LayoutCom<IDWriteFontFace> metrics_face;
			if(!Check(first->GetFirstMatchingFont(font.IsBold()?DWRITE_FONT_WEIGHT_BOLD:DWRITE_FONT_WEIGHT_NORMAL,
			    DWRITE_FONT_STRETCH_NORMAL,font.IsItalic()?DWRITE_FONT_STYLE_ITALIC:DWRITE_FONT_STYLE_NORMAL,metrics_font.Put()),"cannot measure application font") ||
			   !Check(metrics_font->CreateFontFace(metrics_face.Put()),"cannot measure application font face")) return false;
			DWRITE_FONT_METRICS metrics{}; metrics_face->GetMetrics(&metrics);
			UINT32 scalars[2]={'M',' '}; UINT16 indices[2]{}; DWRITE_GLYPH_METRICS advances[2]{};
			if(!metrics.designUnitsPerEm || !Check(metrics_face->GetGlyphIndices(scalars,2,indices),"cannot find application font metrics") ||
			   !Check(metrics_face->GetDesignGlyphMetrics(indices,2,advances,FALSE),"cannot measure application glyphs")) return false;
			if(font.GetWidth()) scale=abs(font.GetWidth())/max(1.,advances[0].advanceWidth*em/metrics.designUnitsPerEm);
			tab_advance=max(1.,advances[1].advanceWidth*em/metrics.designUnitsPerEm);

		}
		Vector<int> run_lines,decoration_lines;
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
			if(!Check(factory->CreateTextFormat((const WCHAR *)family.utf16.Begin(),private_fonts.p,
				font.IsBold()?DWRITE_FONT_WEIGHT_BOLD:DWRITE_FONT_WEIGHT_NORMAL,
				font.IsItalic()?DWRITE_FONT_STYLE_ITALIC:DWRITE_FONT_STYLE_NORMAL,
				DWRITE_FONT_STRETCH_NORMAL,(FLOAT)em,(const WCHAR *)locale.utf16.Begin(),format.Put()),"cannot create text format")) return false;
			if(!Check(format->SetReadingDirection(rtl?DWRITE_READING_DIRECTION_RIGHT_TO_LEFT:DWRITE_READING_DIRECTION_LEFT_TO_RIGHT),"invalid paragraph direction") ||
			   !Check(format->SetWordWrapping(options.wrap?(options.wrap_at_character?DWRITE_WORD_WRAPPING_CHARACTER:DWRITE_WORD_WRAPPING_WRAP):DWRITE_WORD_WRAPPING_NO_WRAP),"invalid wrapping") ||
			   !Check(format->SetIncrementalTabStop((FLOAT)(tab_advance*options.tab_spaces)),"invalid tab stop")) return false;
			DWRITE_TEXT_ALIGNMENT alignment=DWRITE_TEXT_ALIGNMENT_LEADING;
			if(options.alignment==UiTextAlignment::Center) alignment=DWRITE_TEXT_ALIGNMENT_CENTER;
			else if(options.alignment==UiTextAlignment::Left) alignment=rtl?DWRITE_TEXT_ALIGNMENT_TRAILING:DWRITE_TEXT_ALIGNMENT_LEADING;
			else if(options.alignment==UiTextAlignment::Right) alignment=rtl?DWRITE_TEXT_ALIGNMENT_LEADING:DWRITE_TEXT_ALIGNMENT_TRAILING;
			if(!Check(format->SetTextAlignment(alignment),"invalid text alignment")) return false;
			UINT32 length=encoded.scalar_to_utf16[end]-p.utf16_start;
			if(!Check(factory->CreateTextLayout((const WCHAR *)encoded.utf16.Begin()+p.utf16_start,length,format.p,
				(FLOAT)(options.width/scale),10000000,p.native.Put()),"cannot create text layout")) return false;
			DWRITE_TEXT_RANGE range{0,length};
			if(font.IsUnderline() && !Check(p.native->SetUnderline(TRUE,range),"cannot underline text")) return false;
			if(font.IsStrikeout() && !Check(p.native->SetStrikethrough(TRUE,range),"cannot strike text")) return false;
			for(const auto& style:styles) {
				int first=max(start,style.from),last=min(end,style.from+style.length);
				if(first>=last) continue;
				DWRITE_TEXT_RANGE styled{(UINT32)(encoded.scalar_to_utf16[first]-p.utf16_start),
				                        (UINT32)(encoded.scalar_to_utf16[last]-encoded.scalar_to_utf16[first])};
				if(!IsNull(style.font)) {
					Font f=style.font; f.RealizeStd(); UiTextInternal::Encoding name;
					if(f.GetWidth()!=font.GetWidth()) { error="styled font width differs from paragraph width scale"; return false; }
					if(private_fonts.p && f.GetFace()!=font.GetFace()) {
						LayoutCom<IDWriteFontCollection> system;
						if(!Check(factory->GetSystemFontCollection(system.Put()),"cannot access system font collection") ||
						   !Check(p.native->SetFontCollection(system.p,styled),"cannot set styled system font collection")) return false;
					}
					if(!name.Build(f.GetFaceName().ToWString(),error) ||
					   !Check(p.native->SetFontFamilyName((const WCHAR *)name.utf16.Begin(),styled),"cannot set styled family") ||
					   !Check(p.native->SetFontSize((FLOAT)(f.GetHeight()?abs(f.GetHeight()):em),styled),"cannot set styled size") ||
					   !Check(p.native->SetFontWeight(f.IsBold()?DWRITE_FONT_WEIGHT_BOLD:DWRITE_FONT_WEIGHT_NORMAL,styled),"cannot set styled weight") ||
					   !Check(p.native->SetFontStyle(f.IsItalic()?DWRITE_FONT_STYLE_ITALIC:DWRITE_FONT_STYLE_NORMAL,styled),"cannot set styled slant") ||
					   !Check(p.native->SetUnderline(f.IsUnderline(),styled),"cannot set styled underline") ||
					   !Check(p.native->SetStrikethrough(f.IsStrikeout(),styled),"cannot set styled strike")) return false;
				}
				if(!IsNull(style.ink)) {
					LayoutCom<LayoutInk> effect; *effect.Put()=new LayoutInk(style.ink);
					if(!Check(p.native->SetDrawingEffect(effect.p,styled),"cannot set styled ink")) return false;
				}
				if(style.tracking) {
					LayoutCom<IDWriteTextLayout1> spaced;
					if(!Check(p.native->QueryInterface(__uuidof(IDWriteTextLayout1),reinterpret_cast<void **>(spaced.Put())),"character spacing unavailable") ||
					   !Check(spaced->SetCharacterSpacing(0,(FLOAT)(style.tracking/scale),0,styled),"cannot set character spacing")) return false;
				}
			}
			for(const auto& object:inline_objects) if(object.position>=start && object.position<end) {
				LayoutCom<LayoutInline> native_object; *native_object.Put()=new LayoutInline(object,scale);
				DWRITE_TEXT_RANGE replaced{(UINT32)(encoded.scalar_to_utf16[object.position]-p.utf16_start),1};
				if(!Check(p.native->SetInlineObject(native_object.p,replaced),"cannot set inline metrics")) return false;
			}
			DWRITE_TEXT_METRICS metrics{};
			if(!Check(p.native->GetMetrics(&metrics),"cannot measure text")) return false;
			if(!options.wrap) {
				if(!Check(p.native->SetMaxWidth(max(1.0f,metrics.widthIncludingTrailingWhitespace)),"cannot fit intrinsic paragraph") ||
				   !Check(p.native->GetMetrics(&metrics),"cannot remeasure paragraph")) return false;
			}
			p.height=metrics.height; size.cx=max(size.cx,(double)metrics.widthIncludingTrailingWhitespace*scale); size.cy+=p.height;
			UINT32 line_count=0;
			p.native->GetLineMetrics(nullptr,0,&line_count);
			if(line_count>32769) { error="native line count exceeds text budget"; return false; }
			Buffer<DWRITE_LINE_METRICS> native_lines(line_count);
			if(!Check(p.native->GetLineMetrics(~native_lines,line_count,&line_count),"cannot get native lines")) return false;
			int offset=0; double top=p.y;
			for(UINT32 i=0;i<line_count;++i) {
				const auto& native_line=native_lines[i];
				auto& line=lines.Add();
				line.from=encoded.utf16_to_scalar[p.utf16_start+offset];
				int finish=min<int>(length,offset+native_line.length-native_line.newlineLength);
				line.to=encoded.utf16_to_scalar[p.utf16_start+finish];
				line.bounds=Rectf(0,top,options.wrap?options.width:metrics.widthIncludingTrailingWhitespace*scale,top+native_line.height);
				line.baseline=native_line.baseline; line.right_to_left=rtl;
				top+=native_line.height; offset+=native_line.length;
			}
			RunCollector collector(*glyphs,objects,lines,run_lines,decoration_lines,scale,p.y,options.color_fonts);
			if(!Check(p.native->Draw(nullptr,&collector,0,0),"cannot collect positioned glyphs")) return false;
			if(end==text.GetCount()) break;
			if(text[end]=='\r' && end+1<text.GetCount() && text[end+1]=='\n') ++end;
			start=end+1;
		}
		if(!glyphs->IsValid(&error)) return false;
		int64 line_bytes=0;
		if(lines.GetCount()==1 && lines[0].bounds.top==0) line_glyphs.Add()=glyphs;
		else {
			Array<std::shared_ptr<GpuShapedText>> slices;
			for(int i=0;i<lines.GetCount();++i) slices.Add()=std::make_shared<GpuShapedText>();
			for(int i=0;i<glyphs->runs.GetCount();++i) {
				int line=run_lines[i];
				if(line<0 || line>=slices.GetCount()) { error="glyph run has no native line"; return false; }
				const auto& source=glyphs->runs[i]; auto& target=slices[line]->runs.Add();
				target.face=source.face; target.em_size=source.em_size; target.ink=source.ink; target.opacity=source.opacity; target.glyphs=clone(source.glyphs);
				target.bitmap=source.bitmap; target.bitmap_bounds=source.bitmap_bounds-Pointf(0,lines[line].bounds.top);
				for(auto& glyph:target.glyphs) glyph.origin.y-=lines[line].bounds.top;
			}
			for(int i=0;i<glyphs->decorations.GetCount();++i) {
				int line=decoration_lines[i];
				if(line<0 || line>=slices.GetCount()) { error="decoration has no native line"; return false; }
				auto decoration=glyphs->decorations[i]; decoration.rect-=Pointf(0,lines[line].bounds.top);
				slices[line]->decorations.Add(decoration);
			}
			for(const auto& slice:slices) {
				if(!slice->IsValid(&error)) return false;
				line_bytes+=slice->GetPayloadBytes(); line_glyphs.Add()=slice;
			}
		}
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
		payload=glyphs->GetPayloadBytes()+line_bytes+(int64)line_glyphs.GetCount()*sizeof(std::shared_ptr<const GpuShapedText>)+(int64)encoded.text.GetCount()*4+
		        (int64)encoded.utf16.GetCount()*2+
		        (int64)(encoded.scalar_to_utf16.GetCount()+encoded.utf16_to_scalar.GetCount()+graphemes.GetCount()+words.GetCount())*4+
		        (int64)visual_stops.GetCount()*sizeof(VisualStop)+(int64)paragraphs.GetCount()*sizeof(Paragraph)+
		        (int64)lines.GetCount()*sizeof(UiTextLine)+(int64)objects.GetCount()*sizeof(UiTextInlinePlacement)+
		        options.locale.GetCount()+options.font_file.GetCount();
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
	bool Build(const WString&,Font,const UiTextLayoutOptions&,const Vector<UiTextStyleRange>&,const Vector<UiTextInlineObject>&) { error="full text layout provider unavailable in this assembly"; return false; }
#endif
};

UiTextLayout UiTextLayout::Create(const WString& text,Font font,const UiTextLayoutOptions& options)
{
	return CreateStyled(text,font,Vector<UiTextStyleRange>(),Vector<UiTextInlineObject>(),options);
}
UiTextLayout UiTextLayout::CreateStyled(const WString& text,Font font,
    const Vector<UiTextStyleRange>& styles,const Vector<UiTextInlineObject>& objects,const UiTextLayoutOptions& options)
{
	UiTextLayout result;
	// Validate before serialising an untrusted-size key.
	if(styles.GetCount()>1024 || objects.GetCount()>1024 || text.GetCount()>32768 || options.locale.GetCount()>256 || options.locale.Find('\0')>=0 || options.font_file.GetCount()>4096 || options.font_file.Find('\0')>=0 || !Finite(options.width) ||
	   options.width<=0 || options.tab_spaces<1 || options.tab_spaces>64 ||
	   (int)options.direction<0 || (int)options.direction>2) {
		result.data=std::make_shared<Data>(); result.data->error="invalid text layout input or options"; return result;
	}
	for(int i=0;i<text.GetCount();++i)
		if(text[i]>0x10ffff || (text[i]>=0xd800 && text[i]<=0xdfff)) {
			result.data=std::make_shared<Data>(); result.data->error="invalid Unicode scalar"; return result;
		}
	int previous=0;
	for(const auto& style:styles) {
		Font resolved=style.font; if(!IsNull(resolved)) resolved.RealizeStd();
		if(style.from<previous || style.length<=0 || style.length>text.GetCount()-style.from ||
		   style.from<0 || !Finite(style.tracking) || fabs(style.tracking)>512 ||
		   (!IsNull(resolved) && (abs(resolved.GetHeight())>512))) {
			result.data=std::make_shared<Data>(); result.data->error="invalid or overlapping styled text range"; return result;
		}
		previous=style.from+style.length;
	}
	previous=-1;
	for(const auto& object:objects) {
		if(object.position<=previous || object.position<0 || object.position>=text.GetCount() ||
		   text[object.position]!=0xfffc || !Finite(object.size.cx) || !Finite(object.size.cy) ||
		   object.size.cx<=0 || object.size.cy<=0 || !Finite(object.baseline) ||
		   object.baseline>object.size.cy) {
			result.data=std::make_shared<Data>(); result.data->error="invalid inline object metrics or position"; return result;
		}
		previous=object.position;
	}
	if(IsNull(font)) font=StdFont();
	font.RealizeStd();
	StringStream key_stream;
	int64 font_key=font.AsInt64(); int direction=(int)options.direction;
	double width=options.width; bool wrap=options.wrap; int tabs=options.tab_spaces; String locale=options.locale; WString key_text=text;
	int alignment=(int)options.alignment; bool character_wrap=options.wrap_at_character, color_fonts=options.color_fonts;
	key_stream % font_key % direction % width % wrap % character_wrap % color_fonts % alignment % tabs % locale % key_text;
	int style_count=styles.GetCount(),object_count=objects.GetCount(); key_stream % style_count % object_count;
	for(const auto& style:styles) {
		int from=style.from,length=style.length; Font f=style.font; if(!IsNull(f)) f.RealizeStd();
		int64 id=f.AsInt64(); Color ink=style.ink; double tracking=style.tracking;
		key_stream % from % length % id % ink % tracking;
	}
	for(const auto& object:objects) {
		int position=object.position; Sizef size=object.size; double baseline=object.baseline;
		key_stream % position % size % baseline;
	}
	if(!options.font_file.IsEmpty()) {
		String path=GetFullPath(options.font_file); int64 length=GetFileLength(path);
		key_stream % path % length;
#ifdef PLATFORM_WIN32
		FileTime modified=GetFileTime(path); uint64 stamp=((uint64)modified.dwHighDateTime<<32)|modified.dwLowDateTime;
		key_stream % stamp;
#else
		Time modified=FileGetTime(path); key_stream % modified;
#endif
	}
	String key=key_stream.GetResult();
	auto& cache=Cache();
	{
		Mutex::Lock lock(cache.mutex); int i=cache.entries.Find(key);
		if(i>=0) { ++cache.stats.hits; auto& entry=cache.entries[i]; entry.used=++cache.clock; return entry.layout; }
		++cache.stats.misses;
	}
	result.data=std::make_shared<Data>();
	if(!result.data->Build(text,font,options,styles,objects)) return result;
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
int UiTextLayout::GetLineCount() const { return IsValid()?data->lines.GetCount():0; }
UiTextLine UiTextLayout::GetLine(int index) const {
	return IsValid() && index>=0 && index<data->lines.GetCount()?data->lines[index]:UiTextLine();
}
int UiTextLayout::GetInlineCount() const { return IsValid()?data->objects.GetCount():0; }
UiTextInlinePlacement UiTextLayout::GetInline(int index) const {
	return IsValid() && index>=0 && index<data->objects.GetCount()?data->objects[index]:UiTextInlinePlacement();
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
int UiTextLayout::NextWord(int position,int direction) const {
#if defined(PLATFORM_WIN32) && defined(flagGPUUI)
	if(IsValid()) {
		position=minmax(position,0,GetLength()); const auto& words=data->words;
		if(direction<0) {
			while(position>0 && UiTextInternal::IsWhitespace(data->encoded.text[position-1])) --position;
			int i=FindLowerBound(words,position);
			return data->Snap(words[max(0,i-1)],false);
		}
		if(direction>0) {
			int i=FindUpperBound(words,position);
			position=words[min(i,words.GetCount()-1)];
			while(position<GetLength() && UiTextInternal::IsWhitespace(data->encoded.text[position])) ++position;
			return data->Snap(position,true);
		}
		return data->Snap(position,false);
	}
#endif
	return 0;
}
bool UiTextLayout::GetWord(int position,int& from,int& to) const {
#if defined(PLATFORM_WIN32) && defined(flagGPUUI)
	if(IsValid() && GetLength()>0) {
		position=minmax(position,0,GetLength()-1);
		int i=max(0,FindUpperBound(data->words,position)-1);
		from=data->Snap(data->words[i],false);
		to=data->Snap(data->words[min(i+1,data->words.GetCount()-1)],true); return true;
	}
#endif
	return false;
}
UiTextHit UiTextLayout::MoveVisual(UiTextHit current,int direction,bool cross_lines) const {
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
		if(next.position==current.position && next.trailing==current.trailing && cross_lines) {
			int line=0; while(line+1<data->lines.GetCount() && data->lines[line+1].bounds.top<=point.y+.01) ++line;
			int delta=direction*(data->lines[line].right_to_left?-1:1),target=line+delta;
			if(target>=0 && target<data->lines.GetCount()) {
				const auto& bounds=data->lines[target].bounds;
				return HitTest(Pointf(direction<0?1e7:-1e7,(bounds.top+bounds.bottom)*.5));
			}
		}
		return next;
	}
#endif
	return current;
}
#ifdef flagGPUUI
std::shared_ptr<const GpuShapedText> UiTextLayout::GetLineGpuText(int line) const {
#ifdef PLATFORM_WIN32
	if(IsValid() && line>=0 && line<data->line_glyphs.GetCount()) return data->line_glyphs[line];
#endif
	return {};
}
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
namespace {
#ifdef flagGPUUI
bool DrawLayoutSnapshot(Upp::Draw& draw,Pointf origin,Color ink,bool override_ink,
                        const std::shared_ptr<const GpuShapedText>& glyphs) {
	if(auto *target=dynamic_cast<GpuDrawTarget *>(&draw))
		return target->DrawShapedText(origin,glyphs,ink,override_ink);
	if(auto *target=dynamic_cast<GpuPainterEffects *>(&draw))
		return target->DrawShapedText(origin,glyphs,ink,override_ink);
	if(glyphs) if(auto *painter=dynamic_cast<Painter *>(&draw)) {
		painter->Begin(); painter->EvenOdd(false); String error;
		for(const auto& run:glyphs->runs) {
			painter->Begin(); painter->Opacity(run.opacity/255.);
			if(!run.bitmap.IsEmpty()) {
				Image image=run.bitmap;
				if(override_ink) {
					ImageBuffer pixels(image); RGBA color=ink;
					for(RGBA& p:pixels) {
						p.r=(p.a*color.r+127)/255; p.g=(p.a*color.g+127)/255; p.b=(p.a*color.b+127)/255;
					}
					image=Image(pixels);
				}
				Rectf rect=run.bitmap_bounds+origin;
				painter->Rectangle(rect).Fill(image,Xform2D::Scale(rect.Width()/image.GetWidth(),rect.Height()/image.GetHeight())*
				                            Xform2D::Translation(rect.left,rect.top),FILL_PAD);
			}
			else for(const auto& glyph:run.glyphs) {
				LayoutPainterSink sink(*painter,origin+glyph.origin);
				if(!run.face->GetOutline(glyph.glyph,run.em_size,sink,error)) { painter->End(); painter->End(); return false; }
				if(sink.used) painter->Fill(override_ink || IsNull(run.ink)?ink:run.ink);
			}
			painter->End();
		}
		for(const auto& r:glyphs->decorations) painter->Rectangle(r.rect+origin).Fill(override_ink || IsNull(r.ink)?ink:r.ink);
		painter->End(); return true;
	}
	return false;
}
#endif
}
bool UiTextLayout::Draw(Upp::Draw& draw,Pointf origin,Color ink,bool override_ink) const {
#ifdef flagGPUUI
	return DrawLayoutSnapshot(draw,origin,ink,override_ink,GetGpuText());
#else
	return false;
#endif
}
bool UiTextLayout::DrawLine(Upp::Draw& draw,int line,Pointf origin,Color ink,bool override_ink) const {
#ifdef flagGPUUI
	return DrawLayoutSnapshot(draw,origin,ink,override_ink,GetLineGpuText(line));
#else
	return false;
#endif
}
#ifdef flagGPUUI
struct UiTextComposition::State {
	WString text;
	Vector<byte> attributes;
	int cursor=0;
	mutable Font font_key=Null;
	mutable UiTextLayout layout;
	Ptr<Ctrl> owner,root;
	Color ink=SColorText(),paper=SColorPaper();
	struct View : Ctrl {
		UiTextComposition::State& composition;
		explicit View(UiTextComposition::State& composition):composition(composition) { NoWantFocus(); IgnoreMouse(); }
		void Paint(Draw& draw) override {
			draw.DrawRect(GetSize(),composition.paper);
			draw.DrawRect(0,0,GetSize().cx,1,Blend(composition.paper,composition.ink,70));
			draw.DrawRect(0,GetSize().cy-1,GetSize().cx,1,Blend(composition.paper,composition.ink,70));
			composition.PaintAt(draw,Pointf(4,3),Rect(GetSize()),composition.font_key,max(1,DPI(1)));
		}
	};
	One<View> view;
	const UiTextLayout& Layout(Font font) const {
		if(IsNull(font)) font=StdFont(); font.RealizeStd();
		if(!layout.IsValid() || font_key!=font) { layout=UiTextLayout::Create(text,font); font_key=font; }
		return layout;
	}
	Pointf InlineOrigin(Rect anchor,Rect limits,Font font) const {
		const auto& shaped=Layout(font); Sizef size=shaped.GetSize();
		Pointf origin(anchor.left-shaped.GetCaret(0).x,anchor.top);
		origin.x=max<double>(limits.left,min(origin.x,limits.right-size.cx));
		origin.y=max<double>(limits.top,min(origin.y,limits.bottom-size.cy)); return origin;
	}
	Rect CaretAt(Pointf origin,Font font,int width) const {
		const auto& shaped=Layout(font); if(!shaped.IsValid()) return RectC((int)origin.x,(int)origin.y,width,1);
		Pointf at=shaped.GetCaret(shaped.NextGrapheme(cursor,0)); int line=0;
		while(line+1<shaped.GetLineCount() && shaped.GetLine(line+1).bounds.top<=at.y+.01) ++line;
		return RectC((int)floor(origin.x+at.x+.5),(int)floor(origin.y+at.y+.5),max(1,width),
		             max(1,(int)ceil(shaped.GetLine(line).bounds.Height())));
	}
	bool PositionView(Rect anchor,Font font) {
		if(!view || !owner || !root || !root->IsOpen()) return false;
		const auto& shaped=Layout(font); Sizef size=shaped.GetSize();
		Point offset=owner->GetScreenView().TopLeft()-root->GetScreenView().TopLeft();
		Size available=root->GetView().GetSize(); int width=min(available.cx,max(DPI(24),(int)ceil(size.cx)+8));
		int height=min(available.cy,max(DPI(16),(int)ceil(size.cy)+6));
		int x=offset.x+anchor.left,y=offset.y+anchor.bottom+2;
		if(y+height>available.cy) y=offset.y+anchor.top-height-2;
		x=max(0,min(x,available.cx-width)); y=max(0,min(y,available.cy-height));
		view->SetRect(x,y,width,height); return true;
	}
	void PaintAt(Draw& draw,Pointf origin,Rect limits,Font font,int caret_width) const {
		const auto& shaped=Layout(font);
		if(!shaped.IsValid()) { shaped.Draw(draw,origin,ink); return; }
		draw.Clip(limits); shaped.Draw(draw,origin,ink);
		for(int first=0;first<text.GetCount();) {
			byte attribute=attributes.IsEmpty()?0:attributes[first]; int last=first+1;
			while(last<text.GetCount() && (attributes.IsEmpty()?0:attributes[last])==attribute) ++last;
			int thickness=attribute==1 || attribute==3?2:1;
			for(const auto& rect:shaped.GetSelection(first,last))
				draw.DrawRect((int)floor(origin.x+rect.left),(int)ceil(origin.y+rect.bottom)-thickness,
				              max(1,(int)ceil(rect.right)-(int)floor(rect.left)),thickness,ink);
			first=last;
		}
		draw.DrawRect(CaretAt(origin,font,caret_width),ink); draw.End();
	}
};
UiTextComposition::UiTextComposition() {}
UiTextComposition::~UiTextComposition() {}
bool UiTextComposition::IsActive() const { return state && !state->text.IsEmpty(); }
bool UiTextComposition::Update(const GpuImeComposition& value)
{
	if(!value.IsValid()) return false;
	if(value.text.IsEmpty()) { Clear(); return true; }
	if(!state) state.Create();
	if(state->text!=value.text) { state->layout=UiTextLayout(); state->font_key=Null; }
	state->text=value.text; state->attributes=clone(value.attributes); state->cursor=value.caret; return true;
}
void UiTextComposition::Bind(Ctrl& owner)
{
	if(!IsActive()) return;
	state->owner=&owner; Ctrl *root=owner.GetTopCtrl();
	bool gpu=false;
	int depth=0;
	for(Ctrl *candidate=root;candidate && depth++<32;candidate=candidate->GetOwner()) {
		if(auto *host=dynamic_cast<GpuImeOverlayHost *>(candidate)) { gpu=host->IsGpuImeOverlayReady(); break; }
	}
	if(!gpu || !root || !root->IsOpen()) return;
	if(!state->view || state->root!=root) {
		state->view.Clear(); state->root=root; state->view.Create(*state); root->Add(*state->view);
	}
}
void UiTextComposition::Clear() { state.Clear(); }
Rect UiTextComposition::GetCaret(Rect anchor,Rect limits,Font font) const
{
	if(!IsActive()) return anchor;
	if(state->PositionView(anchor,font)) {
		Rect caret=state->CaretAt(Pointf(4,3),font,max(1,anchor.Width()));
		Point offset=state->view->GetScreenView().TopLeft()-state->owner->GetScreenView().TopLeft();
		return caret.Offseted(offset.x,offset.y);
	}
	return state->CaretAt(state->InlineOrigin(anchor,limits,font),font,max(1,anchor.Width()));
}
void UiTextComposition::Paint(Draw& draw,Rect anchor,Rect limits,Font font,Color ink,Color paper) const
{
	if(!IsActive()) return;
	state->ink=ink; state->paper=paper;
	// The root child shares the existing Vulkan device and avoids native popup
	// contexts, clipping in compact fields and covering the editable source text.
	if(state->PositionView(anchor,font)) return;
	Pointf origin=state->InlineOrigin(anchor,limits,font); Sizef size=state->Layout(font).GetSize();
	draw.Clip(limits);
	draw.DrawRect(Rect((int)floor(origin.x),(int)floor(origin.y),(int)ceil(origin.x+size.cx),(int)ceil(origin.y+size.cy+2)),paper);
	state->PaintAt(draw,origin,limits,font,max(1,anchor.Width())); draw.End();
}

#endif

UiTextLayoutStats GetUiTextLayoutStats() {
	auto& cache=Cache(); Mutex::Lock lock(cache.mutex);
	auto stats=cache.stats; stats.cache_entries=cache.entries.GetCount();
#if defined(PLATFORM_WIN32) && defined(flagGPUUI)
	auto& bitmaps=BitmapCache(); Mutex::Lock bitmap_lock(bitmaps.mutex);
	stats.bitmap_asset_entries=bitmaps.entries.GetCount(); stats.bitmap_asset_bytes=bitmaps.bytes;
	stats.bitmap_asset_hits=bitmaps.hits; stats.bitmap_asset_misses=bitmaps.misses;
#endif
	return stats;
}
void ClearUiTextLayoutCache() {
	auto& cache=Cache(); Mutex::Lock lock(cache.mutex);
	cache.entries.Clear(); cache.stats=UiTextLayoutStats();
#if defined(PLATFORM_WIN32) && defined(flagGPUUI)
	auto& bitmaps=BitmapCache(); Mutex::Lock bitmap_lock(bitmaps.mutex);
	bitmaps.entries.Clear(); bitmaps.bytes=0; bitmaps.hits=bitmaps.misses=0;
#endif
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
		bool ok=layout.Draw(draw,Pointf(origin),selected_ink,true); draw.End();
		if(!ok) return false;
	}
	return true;
}

}
