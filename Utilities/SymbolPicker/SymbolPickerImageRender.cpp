#include "SymbolPickerImageRender.h"

#include "SymbolPickerGeneratedCatalog.h"

namespace Upp {

static Image TintRenderedImage(const Image& src, Color col)
{
	if(src.IsEmpty() || IsNull(col))
		return src;

	Size sz = src.GetSize();
	ImageBuffer ib(sz);
	ib.SetKind(src.GetKind());

	for(int y = 0; y < sz.cy; ++y) {
		const RGBA* srow = src[y];
		RGBA* drow = ib[y];
		for(int x = 0; x < sz.cx; ++x) {
			const RGBA& s = srow[x];
			RGBA& d = drow[x];
			if(s.a == 0) {
				d = RGBAZero();
				continue;
			}

			int lum = (int)((54 * s.r + 183 * s.g + 19 * s.b + 128) >> 8);
			int darkness = 255 - lum;
			int coverage = max<int>(s.a, (s.a * darkness + 127) / 255);
			int a = clamp(coverage, 0, 255);

			d.r = (byte)((col.GetR() * a + 127) / 255);
			d.g = (byte)((col.GetG() * a + 127) / 255);
			d.b = (byte)((col.GetB() * a + 127) / 255);
			d.a = a;
		}
	}

	return Image(ib);
}

static Image RenderTransparentSvg(Size sz, const String& svg_xml)
{
	Rectf bounds = GetSVGBoundingBox(svg_xml);
	if(bounds.IsEmpty())
		return Null;

	Sizef fitted = GetFitSize(bounds.GetSize(), Sizef(sz.cx, sz.cy));
	if(fitted.cx <= 0.0 || fitted.cy <= 0.0)
		return Null;

	ImageBuffer ib(sz);
	ib.SetKind(IMAGE_ALPHA);
	Fill(~ib, RGBAZero(), ib.GetLength());

	BufferPainter sw(ib, MODE_ANTIALIASED);
	sw.Clear(RGBAZero());
	double scale = min(fitted.cx / bounds.GetWidth(), fitted.cy / bounds.GetHeight());
	double ox = (sz.cx - fitted.cx) * 0.5;
	double oy = (sz.cy - fitted.cy) * 0.5;
	sw.Scale(scale);
	sw.Translate(ox - bounds.left * scale, oy - bounds.top * scale);
	RenderSVG(sw, svg_xml, Black());
	return Image(ib);
}

Image RenderSymbolPickerIconImage(const SymbolPickerIconEntry& entry, int pixel_size, Color tint, String* error)
{
	int size = max(1, pixel_size);
	String svg_xml;
	if(!DecodeGeneratedSymbolPickerSvg(entry.catalog_id, svg_xml) || svg_xml.IsEmpty()) {
		if(error)
			*error = Format("Could not decode SVG for %s.", entry.catalog_id);
		return Null;
	}

	Image base = RenderTransparentSvg(Size(size, size), svg_xml);
	if(base.IsEmpty()) {
		if(error)
			*error = Format("Could not render SVG for %s.", entry.catalog_id);
		return Null;
	}

	return TintRenderedImage(base, tint);
}

}
