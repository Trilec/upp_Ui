# UiTypographyDemo

A focused native example for the shared project-font contract. It embeds the
original PT Sans and PT Serif static TrueType fonts (four genuine faces each),
with SIL OFL licence texts and SHA-256/source metadata in `fonts/manifest.json`.

From the Ui checkout, with U++ installed at `E:/upp-18468`:

```powershell
python examples/build_demos.py --umk E:/upp-18468/umk.exe --package UiTypographyDemo
Start-Process bin/windows-x64/UiTypographyDemo.exe
```

The family button changes inherited Body text. Heading stays PT Serif, and Code
stays explicitly monospace. Light/Dark changes the existing theme. Resize to
exercise list/gallery reflow and document wrapping; UiLabel/UiMultiEdit use
explicit line breaks rather than soft wrapping.

Focused automated checks and native renders:

```powershell
# The output folders must already exist; build_demos creates build and bin.
$demo = (Resolve-Path bin/windows-x64/UiTypographyDemo.exe).Path
$report = Join-Path (Get-Location) 'build/font-work/ui-tests.txt'
Start-Process $demo -ArgumentList "--verify=$report" -WindowStyle Hidden -Wait
$images = Join-Path (Get-Location) 'build/font-work/rendered'
Start-Process $demo -ArgumentList "--render=$images" -WindowStyle Hidden -Wait
```

`--verify` returns a nonzero exit code on failures and writes the individual
checks. `--render` creates eight PNGs using actual native controls, two body
families, Light/Dark and two window widths. DPI uses the current display; this is
not multi-monitor/DPI-transition acceptance.

This loader is currently Windows/private GDI memory/static TrueType only. CFF
OTF, collections, variable/color fonts and other platforms return diagnostics.
See `docs/PROJECT_FONTS.md` for public APIs, ownership and Designer integration.
