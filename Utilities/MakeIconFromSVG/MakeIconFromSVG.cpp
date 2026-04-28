#include "MakeIconFromSVG.h"

#include <Painter/Painter.h>

namespace Upp {

static bool IsHelpArg(const String& s)
{
    String t = ToLower(TrimBoth(s));
    return t == "-h" || t == "--help" || t == "help" || t == "/?" || t == "-?";
}

static void PrintHelp()
{
    Cout() << "MakeIconFromSVG\n";
    Cout() << "Convert SVG/PNG (and other StreamRaster formats) to either shared IML append files or UiMakeIcon headers.\n\n";
    Cout() << "Usage:\n";
    Cout() << "  MakeIconFromSVG <input1> [input2 ...] [--format iml|uimakeicon] [--size N|WIDTHxHEIGHT]\n";
    Cout() << "                  [--output-base path_without_extension] [--token-prefix PREFIX]\n\n";
    Cout() << "Options:\n";
    Cout() << "  --format       iml (default) or uimakeicon\n";
    Cout() << "  --size         Target icon size, e.g. 24 or 48x48\n";
    Cout() << "  --output-base  Base path without extension\n";
    Cout() << "                 iml       -> <base>.iml.append + <base>.icons_h.append\n";
    Cout() << "                 uimakeicon-> <base>.h\n";
    Cout() << "  --token-prefix Prefix added ahead of the normalized icon token\n\n";
    Cout() << "Examples:\n";
    Cout() << "  MakeIconFromSVG designs/search.svg\n";
    Cout() << "  MakeIconFromSVG designs/check.svg designs/radio.svg --size 48x48 --output-base Ui/icon_batch\n";
    Cout() << "  MakeIconFromSVG designs/search.svg --format uimakeicon --output-base Ui/newicons/search_icon\n";
}

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

static String DefaultTokenFromPath(const String& path, const Size& sz, const String& prefix = String())
{
    String base = GetFileTitle(path);
    String tok = UpperToken(base);
    if(sz.cx > 0 && sz.cy > 0) {
        int suffix = max(sz.cx, sz.cy);
        tok << "_" << AsString(suffix);
    }
    if(!prefix.IsEmpty())
        return UpperToken(prefix) + "_" + tok;
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

static String ResolveOutputBase(const Vector<String>& input_paths, SvgIconOutputMode mode)
{
    if(input_paths.GetCount() == 1) {
        String dir = GetFileDirectory(input_paths[0]);
        String base = GetFileTitle(input_paths[0]) + "_icon";
        return AppendFileName(dir, base);
    }
    String dir = GetCurrentDirectory();
    return AppendFileName(dir, mode == SvgIconOutputMode::IML ? "icons_batch" : "icons_batch_icon");
}

static bool ParseOutputMode(const String& text, SvgIconOutputMode& mode)
{
    String t = ToLower(TrimBoth(text));
    if(t.IsEmpty() || t == "iml") {
        mode = SvgIconOutputMode::IML;
        return true;
    }
    if(t == "uimakeicon") {
        mode = SvgIconOutputMode::UIMAKEICON;
        return true;
    }
    return false;
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

static void AddLE16(StringBuffer& out, int value)
{
    out.Cat((char)(value & 0xff));
    out.Cat((char)((value >> 8) & 0xff));
}

static String BuildImlPayload(const Image& img)
{
    Size sz = img.GetSize();
    StringBuffer raw;
    raw.Reserve(13 + sz.cx * sz.cy * 4);
    raw.Cat((char)0);
    AddLE16(raw, sz.cx);
    AddLE16(raw, sz.cy);
    AddLE16(raw, 0);
    AddLE16(raw, 0);
    AddLE16(raw, 0);
    AddLE16(raw, 0);

    for(int y = 0; y < sz.cy; y++)
        for(int x = 0; x < sz.cx; x++) {
            RGBA px = img[y][x];
            raw.Cat((char)px.r);
            raw.Cat((char)px.g);
            raw.Cat((char)px.b);
            raw.Cat((char)px.a);
        }

    return ZCompress(String(raw));
}

static String BuildImlEntryText(const String& token, const String& compressed, const String& source_svg, Size sz)
{
    String out;
    out << "// Auto-generated by MakeIconFromSVG\n";
    out << "// Source: " << source_svg << "\n";
    out << "// Size: " << sz.cx << "x" << sz.cy << "\n";
    out << "// Format: U++ IML packed payload\n";
    out << "IMAGE_ID(" << token << ")\n";
    out << "IMAGE_BEGIN_DATA\n";
    for(int i = 0; i < compressed.GetCount(); i += 32) {
        out << "IMAGE_DATA(";
        for(int j = 0; j < 32; j++) {
            int index = i + j;
            byte b = index < compressed.GetCount() ? (byte)compressed[index] : 0;
            if(j)
                out << ",";
            out << Format("0x%02x", (int)b);
        }
        out << ")\n";
    }
    out << "IMAGE_END_DATA(" << compressed.GetCount() << ", 1)\n";
    return out;
}

static String BuildIconsHAppendText(const String& token, const String& source_path, Size sz)
{
    String out;
    out << "// Source: " << source_path << "\n";
    out << "// Size: " << sz.cx << "x" << sz.cy << "\n";
    out << "inline Image " << token << "()\n";
    out << "{\n";
    out << "    return UiIconsImg::" << token << "();\n";
    out << "}\n";
    out << "out.Add(UiIconCatalogEntry(\"" << token << "\", &" << token << "));\n";
    return out;
}

struct ResolvedSvgIcon : Moveable<ResolvedSvgIcon> {
    String input_path;
    String token;
    Size   size;
    Image  image;
};

static bool ResolveIconImage(const SvgIconJob& job, const String& input_path, ResolvedSvgIcon& out, String& error)
{
    if(!FileExists(input_path)) {
        error = "input file not found: " + input_path;
        return false;
    }

    String ext = ToLower(GetFileExt(input_path));
    bool svg_input = (ext == ".svg");

    String svg;
    Size render_sz = job.size;
    Image img;

    if(svg_input) {
        svg = LoadFile(input_path);
        if(svg.IsEmpty()) {
            error = "failed to load input svg or file is empty: " + input_path;
            return false;
        }
        if(!IsSVG(svg)) {
            error = "input .svg file is not valid: " + input_path;
            return false;
        }
        render_sz = ResolveSize(svg, job.size);
        img = RenderSvgExact(render_sz, svg);
    }
    else {
        img = StreamRaster::LoadFileAny(input_path);
        if(IsNull(img)) {
            error = "unsupported image file: " + input_path;
            return false;
        }
        if(render_sz.cx <= 0 || render_sz.cy <= 0)
            render_sz = img.GetSize();
        img = FitImageToTarget(img, render_sz);
    }

    if(render_sz.cx <= 0 || render_sz.cy <= 0 || render_sz.cx > 4096 || render_sz.cy > 4096) {
        error = "render size is invalid/out-of-range for: " + input_path;
        return false;
    }
    if(IsNull(img)) {
        error = "failed to render input image: " + input_path;
        return false;
    }

    out.input_path = input_path;
    out.size = render_sz;
    out.image = img;
    out.token = DefaultTokenFromPath(input_path, render_sz, job.token_prefix);
    return true;
}

bool ParseSvgIconJob(const Vector<String>& args, SvgIconJob& job, String& error)
{
    if(args.GetCount() < 1) {
        error = "usage: MakeIconFromSVG <input1> [input2 ...] [--format iml|uimakeicon] [--size N|WIDTHxHEIGHT] [--output-base path] [--token-prefix PREFIX]";
        return false;
    }

    job = SvgIconJob();
    for(int i = 0; i < args.GetCount(); i++) {
        const String& a = args[i];
        if(a == "--format") {
            if(i + 1 >= args.GetCount() || !ParseOutputMode(args[++i], job.output_mode)) {
                error = "unknown or missing --format value";
                return false;
            }
        }
        else if(a == "--size") {
            if(i + 1 >= args.GetCount()) {
                error = "missing --size value";
                return false;
            }
            String e = ParseSizeText(args[++i], job.size);
            if(!e.IsEmpty()) {
                error = e;
                return false;
            }
        }
        else if(a == "--output-base") {
            if(i + 1 >= args.GetCount()) {
                error = "missing --output-base value";
                return false;
            }
            job.output_base = args[++i];
        }
        else if(a == "--token-prefix") {
            if(i + 1 >= args.GetCount()) {
                error = "missing --token-prefix value";
                return false;
            }
            job.token_prefix = args[++i];
        }
        else if(a.StartsWith("--")) {
            error = "unknown option: " + a;
            return false;
        }
        else {
            job.input_paths.Add(a);
        }
    }

    if(job.input_paths.IsEmpty()) {
        error = "at least one input path is required";
        return false;
    }

    if(job.output_base.IsEmpty())
        job.output_base = ResolveOutputBase(job.input_paths, job.output_mode);

    return true;
}

bool BuildIconHeaderFromSvg(const SvgIconJob& job, String& error)
{
    // The shared icon workflow has two explicit insertion points now:
    // UiIcons.iml for data payloads and UiIcons.h for wrappers/catalog lines.
    // Emit them as separate append files so future scripted or AI-assisted
    // merges do not need to infer destinations from mixed output text.
    Vector<ResolvedSvgIcon> icons;
    Index<String> used;
    for(int i = 0; i < job.input_paths.GetCount(); i++) {
        ResolvedSvgIcon icon;
        if(!ResolveIconImage(job, job.input_paths[i], icon, error))
            return false;
        if(used.Find(icon.token) >= 0) {
            error = "duplicate generated token: " + icon.token;
            return false;
        }
        used.Add(icon.token);
        icons.Add(pick(icon));
    }

    if(job.output_mode == SvgIconOutputMode::IML) {
        String iml_append;
        String header_append;
        header_append << "// Auto-generated by MakeIconFromSVG\n";
        header_append << "// Append ICON_* wrappers near the wrapper block and add catalog lines\n";
        header_append << "// inside UiIconCatalog() in Ui/UiIcons.h.\n\n";
        for(int i = 0; i < icons.GetCount(); i++) {
            iml_append << BuildImlEntryText(icons[i].token, BuildImlPayload(icons[i].image), icons[i].input_path, icons[i].size) << "\n";
            header_append << BuildIconsHAppendText(icons[i].token, icons[i].input_path, icons[i].size) << "\n";
        }
        String iml_path = job.output_base + ".iml.append";
        String header_path = job.output_base + ".icons_h.append";
        if(!SaveFile(iml_path, iml_append)) {
            error = "failed to write output file: " + iml_path;
            return false;
        }
        if(!SaveFile(header_path, header_append)) {
            error = "failed to write output file: " + header_path;
            return false;
        }
    }
    else {
        String output_text;
        for(int i = 0; i < icons.GetCount(); i++) {
            Vector<byte> encoded = EncodeRle(icons[i].image);
            if(encoded.IsEmpty()) {
                error = "failed to encode image: " + icons[i].input_path;
                return false;
            }
            output_text << BuildHeaderText(icons[i].token, encoded, icons[i].input_path, icons[i].size) << "\n";
        }
        String header_path = job.output_base + ".h";
        if(!SaveFile(header_path, output_text)) {
            error = "failed to write output file: " + header_path;
            return false;
        }
    }
    return true;
}

}

using namespace Upp;

CONSOLE_APP_MAIN
{
    const Vector<String>& args = CommandLine();

    if(args.IsEmpty() || IsHelpArg(args[0])) {
        PrintHelp();
        return;
    }

    SvgIconJob job;
    String error;

    if(!ParseSvgIconJob(args, job, error)) {
        Cout() << error << "\n";
        Cout() << "\n";
        PrintHelp();
        SetExitCode(1);
        return;
    }

    if(!BuildIconHeaderFromSvg(job, error)) {
        Cout() << "error: " << error << "\n";
        SetExitCode(2);
        return;
    }

    if(job.output_mode == SvgIconOutputMode::IML) {
        Cout() << "ok: wrote " << job.output_base << ".iml.append\n";
        Cout() << "ok: wrote " << job.output_base << ".icons_h.append\n";
    }
    else {
        Cout() << "ok: wrote " << job.output_base << ".h\n";
    }
}
