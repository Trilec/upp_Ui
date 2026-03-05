# MakeIconFromSVG

Small CLI utility that converts SVG and raster image files into U++ icon headers (`DATA_*` + `ICON_*()`), using `UiMakeIcon` RLE data.

## What it does

- Accepts `.svg` (Painter render path).
- Accepts raster formats supported by `StreamRaster` (e.g. `.png`).
- Writes a C++ header with:
  - `static const unsigned char DATA_<TOKEN>[]`
  - `inline Upp::Image ICON_<TOKEN>()`

## Build

From repo root:

```bat
"E:\upp-18182\umk.exe" "E:\apps\github\upp_Ui,E:\upp-18182\uppsrc" Utilities/MakeIconFromSVG CLANGx64 -br "E:\apps\github\upp_Ui\build\MakeIconFromSVG_tools"
```

Output tool:

- `build/MakeIconFromSVG_tools.exe`

## Usage

```text
MakeIconFromSVG <input.(svg|png|...)> [output.h] [symbol_token] [size|WIDTHxHEIGHT]
```

Arguments:

- `input`: source image path.
- `output.h` (optional): destination header path. Default: `<input_dir>/<input_name>_icon.h`.
- `symbol_token` (optional): base token used in generated symbol names.
- `size` (optional): icon size (`24` or `48x48`).

## Examples

Generate from SVG:

```bat
build\MakeIconFromSVG_tools.exe designs\search.svg Ui\newicons\search_icon.h ACTION_SEARCH_48 48x48
```

Generate from PNG:

```bat
build\MakeIconFromSVG_tools.exe designs\NewLogo_v4.png Ui\newicons\upplogo2_icon.h BRAND_UPPLOGO2_48 48x48
```

Show help:

```bat
build\MakeIconFromSVG_tools.exe --help
```

## Recommended workflow

- Generate candidate icon blocks.
- Keep one staging bundle header (for example `Ui/newicons.h`) that contains all generated icons.
- Review/curate by hand.
- Copy/merge finalized content into the single main icon header (`Ui/UiIcons.h`).

This keeps the main icon surface centralized while still using the converter for fast generation.
