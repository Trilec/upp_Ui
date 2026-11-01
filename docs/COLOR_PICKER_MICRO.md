# UiColorPickerMicro

A small opaque RGB selector in the core `Ui` package. Include `<Ui/Ui.h>` or
`<Ui/UiColorPickerMicro.h>`; it needs no browser, filesystem or Imaging package.
The maintained family example is `examples/UiColorPickerDemo`: select **Use micro
picker** in its inspector to exercise palette views, columns, ramps and RGB.

```cpp
UiColorPickerMicro picker;
picker.SetColor(Color(208, 208, 208)).SetColumns(8);
picker.WhenAction = [&] { UseColour(picker.GetColor()); };
picker.WhenLayoutChange = [&] { picker.SetRect(picker.GetMinSize()); };
```

The default palette is 8×5, with 17px colour squares in 27px hit targets and a
10px layout inset (DPI scaled). The footer contains a larger **Apply** swatch,
the complete hex value, RGB toggle and palette-view icon. RGB labels align with
the painted tracks; their sliders use the shared `UiSlider::SetTrackInset`
option. Other sliders retain their existing automatic spacing.

Palette and valid hex choices commit immediately. RGB and ramp adjustments
preview locally; **Apply**, Enter or Space commits the held colour. Escape bubbles
to the popup/embedding host, which owns cancellation and acceptance policy.
`SetColor`/`SetData` are silent by default. `WhenAction` runs synchronously on the
GUI thread and may destroy the control; do not retain borrowed child pointers.

The palette icon cycles Standard → Greyscale → Spectrum. Greyscale progresses
from black at top-left to white at bottom-right. Spectrum varies hue across
columns and shade down rows. Cycling reuses the cell controls and preserves the
held colour; Standard restores the caller's palette. `SetPalette` accepts 1–64
non-null colours; invalid input leaves the configuration unchanged. Columns are
clamped to 1–16. `ShowRamps(false)` removes the tiny ramps. `ShowEditor(false)`
(also the compatibility `ShowHex(false)`) supplies a palette-only control.

The host owns fonts and theme. A protected `UiPanel::ResolveThemeStyle` hook
lets the composite refine defaults while all native explicit-style setters retain
their normal freeze/clear semantics. Default panel padding is removed because the
layout owns its inset; explicit `SetCustomStyle` remains explicit until
`ClearCustomStyle`. `WhenLayoutChange` asks popup hosts to remeasure after an
editor, column or palette-size change. The picker creates no windows or timers.
Alpha editing remains the responsibility of the full `UiColorPicker`.
