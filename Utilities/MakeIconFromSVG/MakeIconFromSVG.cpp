#include "MakeIconFromSVG.h"

#include <Painter/Painter.h>

namespace Upp {

static String UpperToken(String s)
{
    String out;
    out.Reserve(s.GetCount() + 8);
    bool prev_us = false;
    for(int i = 0; i < s.GetCount(); i++) {
        int c = (byte)s[i];
        bool ok = IsAlpha(c) || IsDigit(c);
        if(ok) {
            out.Cat((char)ToUpper(c));
            prev_us = false;
        }
        else if(!prev_us) {
            out.Cat('_');
            prev_us = true;
        }
    }
    while(out.GetCount() && out[0] == '_')
        out.Remove(0);
    while(out.GetCount() && out[out.GetCount() - 1] == '_')
        out.Trim(out.GetCount() - 1);
    if(out.IsEmpty() || IsDigit((byte)out[0]))
        out = String("ICON_") + out;
    return out;
}

static String DefaultTokenFromPath(const String& path, const Size& sz)
{
    String base = GetFileTitle(path);
    String tok = UpperToken(base);
    if(sz.cx > 0 && sz.cy > 0) {
        int suffix = max(sz.cx, sz.cy);
        tok << "_" << AsString(suffix);
    }
    return tok;
}

static String ParseSizeText(const String& s, Size& out)
{
    String t = TrimBoth(s);
    if(t.IsEmpty())
        return "size is empty";

    int x = t.Find('x');
    if(x < 0)
        x = t.Find('X');

    if(x >= 0) {
        int w = ScanInt(t.Left(x));
        int h = ScanInt(t.Mid(x + 1));
        if(w <= 0 || h <= 0)
            return "size must be WIDTHxHEIGHT with positive values";
        out = Size(w, h);
        return String();
    }

    int side = ScanInt(t);
    if(side <= 0)
        return "size must be positive";
    out = Size(side, side);
    return String();
}

static String ResolveOutputPath(const String& svg_path)
{
    String dir = GetFileDirectory(svg_path);
    String base = GetFileTitle(svg_path);
    return AppendFileName(dir, base + "_icon.h");
}

static Image FitImageToTarget(const Image& src, Size target)
{
    if(IsNull(src) || target.cx <= 0 || target.cy <= 0)
        return Image();

    Size isz = src.GetSize();
    if(isz.cx <= 0 || isz.cy <= 0)
        return Image();

    double sx = (double)target.cx / isz.cx;
    double sy = (double)target.cy / isz.cy;
    double s = min(sx, sy);

    int w = max(1, (int)floor(isz.cx * s + 0.5));
    int h = max(1, (int)floor(isz.cy * s + 0.5));
    Image scaled = (w == isz.cx && h == isz.cy) ? src : CachedRescale(src, Size(w, h));

    ImageBuffer ib(target);
    ib.SetKind(IMAGE_ALPHA);
    Fill(~ib, RGBAZero(), ib.GetLength());

    int ox = (target.cx - w) / 2;
    int oy = (target.cy - h) / 2;
    Copy(ib, Point(ox, oy), scaled, scaled.GetSize());
    return Image(ib);
}

static Size ResolveSize(const String& svg, Size requested)
{
    if(requested.cx > 0 && requested.cy > 0)
        return requested;

    Sizef dim;
    Rectf viewbox;
    GetSVGDimensions(svg, dim, viewbox);

    int w = fround(dim.cx);
    int h = fround(dim.cy);

    if(w <= 0 || h <= 0) {
        if(!IsNull(viewbox) && viewbox.GetWidth() > 0 && viewbox.GetHeight() > 0) {
            w = fround(viewbox.GetWidth());
            h = fround(viewbox.GetHeight());
        }
    }

    if(w <= 0 || h <= 0)
        return Size(24, 24);
    return Size(w, h);
}

static Image RenderSvgExact(Size target, const String& svg)
{
    if(target.cx <= 0 || target.cy <= 0)
        return Image();

    Rectf bb = GetSVGBoundingBox(svg);
    if(IsNull(bb) || bb.GetWidth() <= 0 || bb.GetHeight() <= 0)
        return Image();

    ImageBuffer ib(target);
    ib.SetKind(IMAGE_ALPHA);
    Fill(~ib, RGBAZero(), ib.GetLength());

    BufferPainter p(ib, MODE_ANTIALIASED);

    double sx = (double)target.cx / bb.GetWidth();
    double sy = (double)target.cy / bb.GetHeight();
    double s = min(sx, sy);

    double ox = (target.cx - bb.GetWidth() * s) * 0.5;
    double oy = (target.cy - bb.GetHeight() * s) * 0.5;

    p.Translate(ox, oy);
    p.Scale(s);
    p.Translate(-bb.left, -bb.top);
    RenderSVG(p, svg);

    return Image(ib);
}

static Vector<byte> EncodeRle(const Image& img)
{
    Vector<byte> out;
    Size sz = img.GetSize();
    if(sz.cx <= 0 || sz.cy <= 0)
        return out;

    uint16 w = (uint16)sz.cx;
    uint16 h = (uint16)sz.cy;
    w = (uint16)(w | 0x8000);

    out.Add((byte)(w & 0xff));
    out.Add((byte)((w >> 8) & 0xff));
    out.Add((byte)(h & 0xff));
    out.Add((byte)((h >> 8) & 0xff));

    RGBA prev = img[0][0];
    int run = 0;
    int total = sz.cx * sz.cy;

    auto Flush = [&](const RGBA& px, int cnt) {
        while(cnt > 0) {
            int piece = min(cnt, 65535);
            out.Add((byte)(piece & 0xff));
            out.Add((byte)((piece >> 8) & 0xff));
            out.Add(px.r);
            out.Add(px.g);
            out.Add(px.b);
            out.Add(px.a);
            cnt -= piece;
        }
    };

    for(int i = 0; i < total; i++) {
        int y = i / sz.cx;
        int x = i % sz.cx;
        RGBA px = img[y][x];
        if(i == 0) {
            prev = px;
            run = 1;
            continue;
        }
        if(px.r == prev.r && px.g == prev.g && px.b == prev.b && px.a == prev.a && run < 65535) {
            run++;
        }
        else {
            Flush(prev, run);
            prev = px;
            run = 1;
        }
    }

    if(run > 0)
        Flush(prev, run);

    return out;
}

static String BytesToCppArray(const Vector<byte>& data)
{
    String out;
    for(int i = 0; i < data.GetCount(); i++) {
        if(i % 12 == 0)
            out << (i == 0 ? "    " : "\n    , ");
        else
            out << ", ";
        out << Format("0x%02x", (int)data[i]);
    }
    if(!data.IsEmpty())
        out << '\n';
    return out;
}

static String BuildHeaderText(const String& token, const Vector<byte>& data, const String& source_svg, Size sz)
{
    String guard = String("_GENERATED_") + token + "_h_";
    String data_name = "DATA_" + token;
    String icon_name = "ICON_" + token;

    String out;
    out << "#ifndef " << guard << "\n";
    out << "#define " << guard << "\n\n";
    out << "#include <CtrlCore/CtrlCore.h>\n\n";
    out << "// Auto-generated by MakeIconFromSVG\n";
    out << "// Source: " << source_svg << "\n";
    out << "// Size: " << sz.cx << "x" << sz.cy << "\n";
    out << "// Format: UiMakeIcon RLE (uint16 run + RGBA)\n";
    out << "// Include <Ui/UiDraw.h> before using " << icon_name << "().\n\n";
    out << "static const unsigned char " << data_name << "[] = {\n";
    out << BytesToCppArray(data);
    out << "};\n\n";
    out << "inline Upp::Image " << icon_name << "()\n";
    out << "{\n";
    out << "    return Upp::UiMakeIcon(" << data_name << ");\n";
    out << "}\n\n";
    out << "#endif\n";
    return out;
}

bool ParseSvgIconJob(const Vector<String>& args, SvgIconJob& job, String& error)
{
    if(args.GetCount() < 1) {
        error = "usage: MakeIconFromSVG <input.(svg|png|...)> [output.h] [symbol_token] [size|WIDTHxHEIGHT]";
        return false;
    }

    job = SvgIconJob();
    job.input_path = args[0];

    if(args.GetCount() >= 2)
        job.output_header = args[1];
    else
        job.output_header = ResolveOutputPath(job.input_path);

    if(args.GetCount() >= 4) {
        String e = ParseSizeText(args[3], job.size);
        if(!e.IsEmpty()) {
            error = e;
            return false;
        }
    }

    if(args.GetCount() >= 3)
        job.symbol_token = UpperToken(args[2]);

    return true;
}

bool BuildIconHeaderFromSvg(const SvgIconJob& job, String& error)
{
    if(!FileExists(job.input_path)) {
        error = "input file not found: " + job.input_path;
        return false;
    }

    String ext = ToLower(GetFileExt(job.input_path));
    bool svg_input = (ext == ".svg");

    String svg;
    Size render_sz = job.size;
    Image img;

    if(svg_input) {
        svg = LoadFile(job.input_path);
        if(svg.IsEmpty()) {
            error = "failed to load input svg or file is empty";
            return false;
        }
        if(!IsSVG(svg)) {
            error = "input .svg file is not valid";
            return false;
        }
        render_sz = ResolveSize(svg, job.size);
        img = RenderSvgExact(render_sz, svg);
    }
    else {
        img = StreamRaster::LoadFileAny(job.input_path);
        if(IsNull(img)) {
            error = "unsupported image file (supported: .svg, and raster via StreamRaster e.g. .png)";
            return false;
        }
        if(render_sz.cx <= 0 || render_sz.cy <= 0)
            render_sz = img.GetSize();
        img = FitImageToTarget(img, render_sz);
    }

    if(render_sz.cx <= 0 || render_sz.cy <= 0 || render_sz.cx > 4096 || render_sz.cy > 4096) {
        error = "render size is invalid/out-of-range";
        return false;
    }

    if(IsNull(img)) {
        error = "failed to render input image";
        return false;
    }

    Vector<byte> encoded = EncodeRle(img);
    if(encoded.IsEmpty()) {
        error = "failed to encode image";
        return false;
    }

    String token = job.symbol_token;
    if(token.IsEmpty())
        token = DefaultTokenFromPath(job.input_path, render_sz);

    String header = BuildHeaderText(token, encoded, job.input_path, render_sz);
    if(!SaveFile(job.output_header, header)) {
        error = "failed to write output header: " + job.output_header;
        return false;
    }

    return true;
}

}

using namespace Upp;

CONSOLE_APP_MAIN
{
    const Vector<String>& args = CommandLine();
    SvgIconJob job;
    String error;

    if(!ParseSvgIconJob(args, job, error)) {
        Cout() << "MakeIconFromSVG\n";
        Cout() << error << "\n";
        Cout() << "example: MakeIconFromSVG designs/drag_indicator.svg Ui/GeneratedDragIcon.h NAVIGATION_OUTLINED_DRAG_INDICATOR_48 24x24\n";
        SetExitCode(1);
        return;
    }

    if(!BuildIconHeaderFromSvg(job, error)) {
        Cout() << "error: " << error << "\n";
        SetExitCode(2);
        return;
    }

    Cout() << "ok: wrote " << job.output_header << "\n";
}
