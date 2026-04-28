# MakeIconFromSVG

Small CLI utility that converts SVG and raster image files into either:

- shared IML append files for the library icon pack
- U++ icon headers (`DATA_*` + `ICON_*()`), using `UiMakeIcon` RLE data for local or legacy usage

## What it does

- Accepts `.svg` (Painter render path).
- Accepts raster formats supported by `StreamRaster` (e.g. `.png`).
- In default `iml` mode, writes two companion append files:
  - `<base>.iml.append`
    - merge into [UiIcons.iml](E:\apps\github\upp_Ui\Ui\UiIcons.iml)
  - `<base>.icons_h.append`
    - merge into [UiIcons.h](E:\apps\github\upp_Ui\Ui\UiIcons.h)
- In `uimakeicon` mode, writes one C++ header with:
  - `static const unsigned char DATA_<TOKEN>[]`
  - `inline Upp::Image ICON_<TOKEN>()`

## Build

From repo root:

```bat
"E:\upp-18468\umk.exe" "E:\apps\github\upp_Ui\Utilities,E:\apps\github\upp_Ui,E:\upp-18468\uppsrc" MakeIconFromSVG CLANGx64 -br "E:\apps\github\upp_Ui\build\MakeIconFromSVG.exe"
```

Output tool:

- `build/MakeIconFromSVG.exe`

## Usage

```text
MakeIconFromSVG <input1> [input2 ...] [--format iml|uimakeicon] [--size N|WIDTHxHEIGHT]
                [--output-base path_without_extension] [--token-prefix PREFIX]
```

Arguments:

- `input1 [input2 ...]`: one or more source image paths.
- `--format`: `iml` (default) or `uimakeicon`.
- `--size`: icon size (`24` or `48x48`).
- `--output-base`: output base path without extension.
- `--token-prefix`: optional prefix prepended to generated `ICON_*` tokens.

Default outputs:

- single input + `iml`:
  - `<input_dir>/<input_name>_icon.iml.append`
  - `<input_dir>/<input_name>_icon.icons_h.append`
- multiple inputs + `iml`:
  - `<cwd>/icons_batch.iml.append`
  - `<cwd>/icons_batch.icons_h.append`
- `uimakeicon`:
  - `<base>.h`

## Examples

Generate default shared-icon append files from SVG:

```bat
build\MakeIconFromSVG.exe designs\search.svg
```

Generate a batch append bundle:

```bat
build\MakeIconFromSVG.exe designs\check.svg designs\radio.svg --size 48x48 --output-base Ui\icon_batch
```

Generate legacy/local `UiMakeIcon` output:

```bat
build\MakeIconFromSVG.exe designs\search.svg --format uimakeicon --output-base Ui\newicons\search_icon
```

Show help:

```bat
build\MakeIconFromSVG.exe --help
```

## Recommended workflow

- For shared library icons:
  - use the default `iml` mode
  - append `<base>.iml.append` into [UiIcons.iml](E:\apps\github\upp_Ui\Ui\UiIcons.iml)
  - append `<base>.icons_h.append` into [UiIcons.h](E:\apps\github\upp_Ui\Ui\UiIcons.h)
- For local or legacy inline icon work:
  - generate `uimakeicon` output
  - keep it in a local staging header if needed

This keeps the shared icon pack on the IML-backed storage path while making the two insertion points explicit enough for scripted or AI-assisted merges.
