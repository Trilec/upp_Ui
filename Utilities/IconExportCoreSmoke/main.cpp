#include "../IconExportCore/IconExportCore.h"

using namespace Upp;

static Image MakeKnownImage()
{
	ImageBuffer ib(Size(4, 4));
	for(int y = 0; y < ib.GetSize().cy; ++y) {
		RGBA* row = ib[y];
		for(int x = 0; x < ib.GetSize().cx; ++x) {
			row[x].r = (byte)(16 + x * 7 + y * 5);
			row[x].g = (byte)(24 + x * 5 + y * 7);
			row[x].b = (byte)(32 + x * 3 + y * 9);
			row[x].a = (byte)(64 + x * 11 + y * 13);
		}
	}
	return Image(ib);
}

static void AddLE16(StringBuffer& out, int value)
{
	out.Cat((char)(value & 0xff));
	out.Cat((char)((value >> 8) & 0xff));
}

static String BuildExpectedRawBytes(const Image& img)
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
	for(int y = 0; y < sz.cy; ++y) {
		const RGBA* row = img[y];
		for(int x = 0; x < sz.cx; ++x) {
			const RGBA& px = row[x];
			raw.Cat((char)px.r);
			raw.Cat((char)px.g);
			raw.Cat((char)px.b);
			raw.Cat((char)px.a);
		}
	}
	return String(raw);
}

#define FAIL(msg) do { Cout() << msg << '\n'; SetExitCode(1); return; } while(0)

CONSOLE_APP_MAIN
{
	String error;
	Image img = MakeKnownImage();
	if(img.IsEmpty())
		FAIL("Codec smoke image is empty.");

	String payload1;
	String payload2;
	if(!BuildUppImlPayload(img, payload1, &error))
		FAIL(error);
	if(!BuildUppImlPayload(img, payload2, &error))
		FAIL(error);
	if(payload1 != payload2)
		FAIL("U++ IML payload is not deterministic.");

	String raw = ZDecompress(payload1);
	String expected_raw = BuildExpectedRawBytes(img);
	if(raw != expected_raw)
		FAIL("Decompressed U++ IML payload does not match expected bytes.");

	Size sz = img.GetSize();
	if(raw.GetCount() != 13 + sz.cx * sz.cy * 4)
		FAIL("Decompressed U++ IML payload has the wrong length.");
	if((byte)raw[0] != 0)
		FAIL("U++ IML marker byte is wrong.");
	if((byte)raw[1] != (byte)(sz.cx & 0xff) || (byte)raw[2] != (byte)((sz.cx >> 8) & 0xff))
		FAIL("U++ IML width is not little-endian.");
	if((byte)raw[3] != (byte)(sz.cy & 0xff) || (byte)raw[4] != (byte)((sz.cy >> 8) & 0xff))
		FAIL("U++ IML height is not little-endian.");

	for(int y = 0; y < sz.cy; ++y) {
		const RGBA* row = img[y];
		for(int x = 0; x < sz.cx; ++x) {
			int offset = 13 + (y * sz.cx + x) * 4;
			if((byte)raw[offset + 0] != row[x].r || (byte)raw[offset + 1] != row[x].g || (byte)raw[offset + 2] != row[x].b || (byte)raw[offset + 3] != row[x].a)
				FAIL("U++ IML RGBA bytes were not preserved.");
		}
	}

	String entry1;
	String entry2;
	if(!BuildUppImlEntryText("ICON_SMOKE_SAMPLE", payload1, "sample.svg", sz, entry1, &error))
		FAIL(error);
	if(!BuildUppImlEntryText("ICON_SMOKE_SAMPLE", payload1, "sample.svg", sz, entry2, &error))
		FAIL(error);
	if(entry1 != entry2)
		FAIL("U++ IML entry text is not deterministic.");

	Vector<String> lines = Split(entry1, '\n');
	int data_rows = 0;
	int end_count = -1;
	int row_index = 0;
	for(const String& line : lines) {
		if(line.StartsWith("IMAGE_DATA(")) {
			++data_rows;
			String body = line.Mid(11, line.GetCount() - 12);
			Vector<String> parts = Split(body, ',');
			if(parts.GetCount() != 32)
				FAIL("IMAGE_DATA rows are not 32 bytes wide.");
			for(int i = 0; i < parts.GetCount(); ++i) {
				String t = TrimBoth(parts[i]);
				int payload_index = row_index * 32 + i;
				if(t.GetCount() != 4 || !t.StartsWith("0x"))
					FAIL("IMAGE_DATA rows contain malformed byte literals.");
				if(payload_index >= payload1.GetCount() && t != "0x00")
					FAIL("Final IMAGE_DATA row padding was not applied.");
			}
			++row_index;
		}
		else if(line.StartsWith("IMAGE_END_DATA(")) {
			String body = line.Mid(15, line.GetCount() - 16);
			Vector<String> parts = Split(body, ',');
			if(parts.GetCount() != 2)
				FAIL("IMAGE_END_DATA has the wrong arity.");
			end_count = ScanInt(TrimBoth(parts[0]));
		}
	}
	if(data_rows == 0 || end_count != payload1.GetCount())
		FAIL("IMAGE_DATA rows or IMAGE_END_DATA count are wrong.");

	String empty_payload;
	if(BuildUppImlPayload(Image(), empty_payload, &error))
		FAIL("Empty images should be rejected.");
	if(ValidateUppImlImageSize(Size(0, 1), &error))
		FAIL("Zero width images should be rejected.");
	if(ValidateUppImlImageSize(Size(65536, 1), &error))
		FAIL("Oversized images should be rejected.");

	String failed_text = "sentinel";
	String failed_error;
	if(BuildUppImlEntryText("ICON_SMOKE_FAIL", empty_payload, "bad.svg", Size(4, 4), failed_text, &failed_error))
		FAIL("A failed IML entry unexpectedly succeeded.");
	if(!failed_text.IsEmpty())
		FAIL("A failed IML entry left partial output behind.");

	Cout() << "Shared IML codec smoke OK\n";
}
