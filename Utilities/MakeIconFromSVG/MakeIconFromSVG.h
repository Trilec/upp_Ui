#ifndef _utilities_MakeIconFromSVG_h_
#define _utilities_MakeIconFromSVG_h_

#include <Core/Core.h>
#include <Draw/Draw.h>

namespace Upp {

enum class SvgIconOutputMode : byte {
    IML = 0,
    UIMAKEICON,
};

// MakeIconFromSVG
// ---------------
// Small CLI utility that converts SVG or raster assets into either:
// - shared icon append files for the IML-backed Ui icon system
// - UiMakeIcon inline header output for local/legacy usage
//
// IML output writes two companion append files:
// - <base>.iml.append
//   contains IMAGE_ID / IMAGE_BEGIN_DATA / IMAGE_DATA / IMAGE_END_DATA blocks
//   for merge into UiIcons.iml
// - <base>.icons_h.append
//   contains ICON_* wrapper and UiIconCatalogEntry append fragments
//   for merge into UiIcons.h
//
// UiMakeIcon output contains:
// - DATA_<TOKEN> : RLE-encoded RGBA icon bytes for UiMakeIcon(...)
// - ICON_<TOKEN>(): inline image factory
//
// Input formats:
// - SVG: rendered via Painter (exact-fit into requested size)
// - Raster: loaded via StreamRaster, then fit/centered into requested size
//
// CLI format:
//   MakeIconFromSVG <input1> [input2 ...] [--format iml|uimakeicon] [--size N|WIDTHxHEIGHT]
//                   [--output-base path_without_extension] [--token-prefix PREFIX]
//
// Examples:
//   MakeIconFromSVG designs/search.svg
//   MakeIconFromSVG designs/check.svg designs/radio.svg --size 48x48 --output-base Ui/icon_batch
//   MakeIconFromSVG designs/search.svg --format uimakeicon --output-base Ui/newicons/search_icon
//
// Changelog:
// - 2026-04: default output switched to the shared IML append workflow and
//   batch input now emits explicit companion append files for UiIcons.iml
//   and UiIcons.h.
//
// Help:
//   MakeIconFromSVG --help
//   MakeIconFromSVG -h

struct SvgIconJob {
    Vector<String> input_paths;
    String output_base;
    String token_prefix;
    Size   size = Size(0, 0);
    SvgIconOutputMode output_mode = SvgIconOutputMode::IML;
};

bool ParseSvgIconJob(const Vector<String>& args, SvgIconJob& job, String& error);
bool BuildIconHeaderFromSvg(const SvgIconJob& job, String& error);

}

#endif
