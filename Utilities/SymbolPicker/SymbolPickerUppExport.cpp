#include "SymbolPickerUppExport.h"
#include "SymbolPickerExport.h"
#include "SymbolPickerImageRender.h"

namespace Upp {

static const char* kUppRawInclude = "#include <CtrlLib/CtrlLib.h>\n#include <Ui/UiDraw.h>\n";

static const char* SymbolPickerUppIconStyleText(SymbolPickerIconStyle style)
{
	switch(style) {
	case SymbolPickerIconStyle::Outlined: return "outlined";
	case SymbolPickerIconStyle::Rounded:  return "rounded";
	case SymbolPickerIconStyle::Sharp:    return "sharp";
	}
	return "outlined";
}

static String SanitizeGeneratedCommentLine(String line)
{
	line.Replace("\t", " ");
	String out;
	out.Reserve(line.GetCount());
	for(int i = 0; i < line.GetCount(); ++i) {
		unsigned char c = (unsigned char)line[i];
		out.Cat(c < 32 || c == 127 ? ' ' : (char)c);
	}
	if(!out.IsEmpty() && out[out.GetCount() - 1] == '\\') {
		out.Trim(out.GetCount() - 1);
		out << " [backslash]";
	}
	return out;
}

static void AppendGeneratedCommentLines(String& out, const String& text)
{
	String norm = text;
	norm.Replace("\r\n", "\n");
	norm.Replace("\r", "\n");

	int start = 0;
	for(;;) {
		int end = norm.Find('\n', start);
		String line = end >= 0 ? norm.Mid(start, end - start) : norm.Mid(start);
		out << "// " << SanitizeGeneratedCommentLine(line) << '\n';
		if(end < 0)
			break;
		start = end + 1;
	}
}

static void AppendGeneratedCommentField(String& out, const char* label, const String& text)
{
	out << "// " << label << ":\n";
	AppendGeneratedCommentLines(out, text);
}

static String GetUppHeaderGuardBase(const SymbolPickerProject& project)
{
	String base = TrimBoth(project.output_base_name);
	if(base.IsEmpty())
		base = TrimBoth(project.project_name);
	if(base.IsEmpty())
		base = "SymbolPicker";

	String guard = MakeSymbolPickerSafeCppIdentifierSegment(base);
	if(guard.IsEmpty())
		guard = "SYMBOL_PICKER";
	return ToUpper(guard);
}

static String MakeUppHeaderGuard(const SymbolPickerProject& project, bool use_rle)
{
	String guard = GetUppHeaderGuardBase(project);
	guard << "_UPP_" << (use_rle ? "RLE" : "RAW") << "_HEADER_H";
	return guard;
}

static String MakeWarningCommentBlock(const Vector<String>& warnings)
{
	if(warnings.IsEmpty())
		return String();
	String out;
	out << "// Export warnings:\n";
	for(const String& warning : warnings)
		AppendGeneratedCommentLines(out, warning);
	out << "\n";
	return out;
}

static String GetCollectionLabel(const SymbolPickerCollection& collection, int index)
{
	if(!TrimBoth(collection.name).IsEmpty())
		return collection.name;
	return Format("Collection %d", index + 1);
}

static String GetItemCategoryLabel(const SymbolPickerProject& project,
	const SymbolPickerCollection& collection,
	const SymbolPickerIconRef& item,
	const SymbolPickerIconEntry* catalog_entry)
{
	return ResolveExportCategory(project, collection, item, catalog_entry);
}

static const SymbolPickerIconEntry* ResolveCatalogEntry(const SymbolPickerCatalog& catalog, const SymbolPickerIconRef& item)
{
	const SymbolPickerIconEntry* entry = catalog.FindByCatalogId(item.catalog_id);
	if(!entry && !item.source_id.IsEmpty())
		entry = catalog.FindBySourceId(item.source_id);
	return entry;
}

static int ResolveExportSize(const SymbolPickerProject& project, const SymbolPickerIconRef& item)
{
	int px = item.size > 0 ? item.size : project.default_size;
	if(px <= 0)
		px = 48;
	return px;
}

static void AppendPayloadByte(Vector<byte>& out, unsigned int value)
{
	out.Add((byte)(value & 0xFFu));
}

static String FormatHeaderBytes(const Vector<byte>& data)
{
	String out;
	bool first = true;
	int col = 0;
	for(int i = 0; i < data.GetCount(); ++i) {
		if(!first)
			out << ", ";
		first = false;
		out << "0x" << FormatIntHex((int)data[i], 2);
		++col;
		if(col >= 16 && i + 1 < data.GetCount()) {
			out << "\n    ";
			col = 0;
		}
	}
	return out;
}

static bool BuildEncodedUppPayload(const Image& img, bool use_rle, Vector<byte>& out)
{
	out.Clear();
	Size sz = img.GetSize();
	int w = sz.cx;
	if(w < 0)
		w = 0;
	if(w > 0x7FFF)
		w = 0x7FFF;
	int h = sz.cy;
	if(h < 0)
		h = 0;
	if(h > 0xFFFF)
		h = 0xFFFF;
	unsigned int w_raw = (unsigned int)w;
	unsigned int h_raw = (unsigned int)h;
	if(use_rle)
		w_raw |= 0x8000u;

	AppendPayloadByte(out, w_raw & 0xFFu);
	AppendPayloadByte(out, (w_raw >> 8) & 0xFFu);
	AppendPayloadByte(out, h_raw & 0xFFu);
	AppendPayloadByte(out, (h_raw >> 8) & 0xFFu);

	if(sz.cx <= 0 || sz.cy <= 0)
		return false;

	if(!use_rle) {
		for(int y = 0; y < sz.cy; ++y) {
			const RGBA* row = img[y];
			for(int x = 0; x < sz.cx; ++x) {
				const RGBA& p = row[x];
				AppendPayloadByte(out, p.r);
				AppendPayloadByte(out, p.g);
				AppendPayloadByte(out, p.b);
				AppendPayloadByte(out, p.a);
			}
		}
		return true;
	}

	const int total_px = sz.cx * sz.cy;
	if(total_px <= 0)
		return false;

	int x = 0;
	int y = 0;
	RGBA prev = img[0][0];
	unsigned int run = 1;

	for(int idx = 1; idx < total_px; ++idx) {
		++x;
		if(x >= sz.cx) {
			x = 0;
			++y;
		}
		if(y >= sz.cy)
			return false;

		const RGBA& p = img[y][x];
		if(p.r == prev.r && p.g == prev.g && p.b == prev.b && p.a == prev.a) {
			if(run == 65535u) {
				AppendPayloadByte(out, run & 0xFFu);
				AppendPayloadByte(out, (run >> 8) & 0xFFu);
				AppendPayloadByte(out, prev.r);
				AppendPayloadByte(out, prev.g);
				AppendPayloadByte(out, prev.b);
				AppendPayloadByte(out, prev.a);
				run = 1;
			}
			else
				++run;
		}
		else {
			AppendPayloadByte(out, run & 0xFFu);
			AppendPayloadByte(out, (run >> 8) & 0xFFu);
			AppendPayloadByte(out, prev.r);
			AppendPayloadByte(out, prev.g);
			AppendPayloadByte(out, prev.b);
			AppendPayloadByte(out, prev.a);
			prev = p;
			run = 1;
		}
	}

	if(run == 0)
		return false;

	AppendPayloadByte(out, run & 0xFFu);
	AppendPayloadByte(out, (run >> 8) & 0xFFu);
	AppendPayloadByte(out, prev.r);
	AppendPayloadByte(out, prev.g);
	AppendPayloadByte(out, prev.b);
	AppendPayloadByte(out, prev.a);
	return true;
}

static bool EmitItemBlock(String& out,
	const SymbolPickerProject& project,
	const SymbolPickerCollection& collection,
	int collection_index,
	const SymbolPickerIconRef& item,
	const SymbolPickerIconEntry* entry,
	bool use_rle,
	Index<String>& used_names,
	Vector<String>& warnings)
{
	if(!entry) {
		warnings.Add(Format("Skipped unresolved icon %s%s%s in collection '%s'.",
			(item.catalog_id.IsEmpty() ? String("(missing catalog_id)") : item.catalog_id),
			(item.source_id.IsEmpty() ? String() : " / "),
			(item.source_id.IsEmpty() ? String() : item.source_id),
			collection.name.IsEmpty() ? GetCollectionLabel(collection, collection_index) : collection.name));
		return false;
	}

	int px = ResolveExportSize(project, item);
	String error;
	Image img = RenderSymbolPickerIconImage(*entry, px, item.tint, &error);
	if(img.IsEmpty()) {
		warnings.Add(error.IsEmpty()
			? Format("Could not render %s.", item.catalog_id)
			: error);
		return false;
	}

	Vector<byte> payload;
	if(!BuildEncodedUppPayload(img, use_rle, payload)) {
		warnings.Add(Format("Could not encode %s for U++ header export.", item.catalog_id));
		return false;
	}

	String symbol_name = MakeSymbolPickerExportSymbolName(project, collection, item, entry, used_names);
	String data_sym = "DATA_";
	data_sym << symbol_name;
	String func_sym = symbol_name;
	String collection_name = GetCollectionLabel(collection, collection_index);
	String category = GetItemCategoryLabel(project, collection, item, entry);
	String display_name = MakeSymbolPickerExportDisplayName(project, collection, item, entry);
	String style_text = SymbolPickerUppIconStyleText(item.has_style_override ? item.style_override : entry->style);

	String item_out;
	AppendGeneratedCommentField(item_out, "Collection", collection_name);
	AppendGeneratedCommentField(item_out, "Category", category);
	AppendGeneratedCommentField(item_out, "Symbol", symbol_name);
	AppendGeneratedCommentField(item_out, "Icon", display_name);
	AppendGeneratedCommentField(item_out, "Source", item.source_id);
	item_out << "// Style: " << style_text << " | Size: " << px << '\n';
	if(!TrimBoth(item.comment).IsEmpty())
		AppendGeneratedCommentField(item_out, "Comment", item.comment);
	item_out << "static const unsigned char " << data_sym << "[] = {\n    ";
	item_out << FormatHeaderBytes(payload);
	item_out << "\n};\n\n";
	item_out << "inline Upp::Image " << func_sym << "()\n";
	item_out << "{\n";
	item_out << "    return Upp::UiMakeIcon(" << data_sym << ");\n";
	item_out << "}\n\n";
	out << item_out;
	return true;
}

static String BuildSymbolPickerUppHeader(const SymbolPickerProject& project,
	const SymbolPickerCatalog& catalog,
	SymbolPickerExportScope scope,
	bool use_rle,
	Vector<String>* warnings)
{
	Vector<String> local_warnings;
	Vector<String>& warn = warnings ? *warnings : local_warnings;

	const String guard = MakeUppHeaderGuard(project, use_rle);
	String body;
	Index<String> used_names;
	bool wrote_any = false;
	for(int ci = 0; ci < project.collections.GetCount(); ++ci) {
		if(scope == SymbolPickerExportScope::ActiveCollection && ci != project.active_collection_index)
			continue;

		const SymbolPickerCollection& collection = project.collections[ci];
		for(const SymbolPickerIconRef& item : collection.items) {
			const SymbolPickerIconEntry* entry = ResolveCatalogEntry(catalog, item);
			if(!entry || item.unresolved) {
				warn.Add(Format("Skipped unresolved icon %s%s%s in collection '%s'.",
					(item.catalog_id.IsEmpty() ? String("(missing catalog_id)") : item.catalog_id),
					(item.source_id.IsEmpty() ? String() : " / "),
					(item.source_id.IsEmpty() ? String() : item.source_id),
					collection.name.IsEmpty() ? GetCollectionLabel(collection, ci) : collection.name));
				continue;
			}

			wrote_any |= EmitItemBlock(body, project, collection, ci, item, entry, use_rle, used_names, warn);
		}
	}

	if(!wrote_any) {
		warn.Add("No exportable icons were resolved for U++ header export.");
		return String();
	}

	String out;
	String warnings_text = MakeWarningCommentBlock(warn);
	if(!warnings_text.IsEmpty())
		out << warnings_text;
	out << "// Generated by SymbolPicker.\n";
	AppendGeneratedCommentField(out, "Project", project.project_name.IsEmpty() ? String("(unnamed project)") : project.project_name);
	if(!project.comment.IsEmpty())
		AppendGeneratedCommentField(out, "Project comment", project.comment);
	out << "// Export format: UiMakeIcon " << (use_rle ? "RLE" : "RAW") << "\n";
	if(use_rle)
		out << "// RLE encoding: uint16 little-endian run length followed by premultiplied RGBA.\n";
	else
		out << "// RAW encoding: row-major premultiplied RGBA bytes.\n";
	out << "// Deterministic export; no timestamps.\n\n";
	out << "#ifndef " << guard << "\n";
	out << "#define " << guard << "\n\n";
	out << kUppRawInclude << "\n";
	out << body;

	out << "#endif // " << guard << "\n";
	return out;
}

String BuildSymbolPickerUppRawHeader(const SymbolPickerProject& project,
	const SymbolPickerCatalog& catalog,
	SymbolPickerExportScope scope,
	Vector<String>* warnings)
{
	return BuildSymbolPickerUppHeader(project, catalog, scope, false, warnings);
}

String BuildSymbolPickerUppRleHeader(const SymbolPickerProject& project,
	const SymbolPickerCatalog& catalog,
	SymbolPickerExportScope scope,
	Vector<String>* warnings)
{
	return BuildSymbolPickerUppHeader(project, catalog, scope, true, warnings);
}

static Image MakeSmokeImage()
{
	ImageBuffer ib(Size(4, 1));
	Fill(~ib, RGBAZero(), ib.GetLength());
	RGBA* row = ib[0];
	row[0] = RGBAZero();
	row[1].r = 32;  row[1].g = 64;  row[1].b = 128; row[1].a = 255;
	row[2].r = 32;  row[2].g = 64;  row[2].b = 128; row[2].a = 255;
	row[3].r = 16;  row[3].g = 32;  row[3].b = 48;  row[3].a = 96;
	return Image(ib);
}

bool RunSymbolPickerUppExportSmokeTests(String& error)
{
	error.Clear();

	Vector<byte> raw;
	Vector<byte> rle;
	Image smoke = MakeSmokeImage();
	if(!BuildEncodedUppPayload(smoke, false, raw) || !BuildEncodedUppPayload(smoke, true, rle)) {
		error = "U++ smoke payload encoder failed.";
		return false;
	}
	static const byte kRawExpected[] = {
		0x04, 0x00, 0x01, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x20, 0x40, 0x80, 0xFF,
		0x20, 0x40, 0x80, 0xFF,
		0x10, 0x20, 0x30, 0x60,
	};
	static const byte kRleExpected[] = {
		0x04, 0x80, 0x01, 0x00,
		0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x02, 0x00, 0x20, 0x40, 0x80, 0xFF,
		0x01, 0x00, 0x10, 0x20, 0x30, 0x60,
	};
	if(raw.GetCount() != (int)sizeof(kRawExpected)
		|| rle.GetCount() != (int)sizeof(kRleExpected)) {
		error = "U++ smoke payload encoder produced too little data.";
		return false;
	}
	for(int i = 0; i < (int)sizeof(kRawExpected); ++i) {
		if(raw[i] != kRawExpected[i]) {
			error = "U++ RAW smoke payload bytes do not match the legacy layout.";
			return false;
		}
	}
	for(int i = 0; i < (int)sizeof(kRleExpected); ++i) {
		if(rle[i] != kRleExpected[i]) {
			error = "U++ RLE smoke payload bytes do not match the legacy layout.";
			return false;
		}
	}
	return true;
}

}
