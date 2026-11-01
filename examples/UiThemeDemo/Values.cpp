#include "UiThemeDemo.h"

ValuesSection::ValuesSection()
    : ThemeSection("Values, ranges and curves", "Bounded, editable values and data colours that keep their meaning")
{
    slider_edit_.SetRange(0,100).SetStep(1).SetValue(64);
    interval_edit_.SetRange(0,100).SetStep(1).SetValues(20,80);
    AddSpecimenPair("Slider + entry",slider_edit_,"Range + entries",interval_edit_,40,true);
    Vector<UiRangeSegment> spans;
    for(int i = 0; i < 3; ++i) { UiRangeSegment span(1, Format("Act %d",i+1)); span.subtitle="12 scenes"; spans.Add(span); }
    segments_.SetRange(0,100).SetSegments(spans).ShowLabels().ShowSegmentValues().ShowBoundaryValues(false).ShowEndpointValues(false);
    segments_.SetValueDisplay(UiRangeSegments::ValueDisplay::Percent);
    AddSpecimen("UiRangeSegments", segments_, 56);
    progress_.Set(64,100).Percent(); AddSpecimen("UiProgressBar", progress_);
    slider_edit_.WhenChanging = [this] { progress_.Set((int)slider_edit_.GetValue(),100); };
    ring_.Set(64,100); chart_.AddSegment(60,"Complete").AddSegment(25,"Review").AddSegment(15,"Remaining");
    rings_.SetGap(DPI(24)); rings_.Add(ring_).Fixed(DPI(120)); rings_.Add(chart_).Fixed(DPI(120));
    curve_field_.Editor().SetYRange(0,1);
    curve_field_.SetShowFormula(true).SetShowCopy(true);
    AddSpecimenPair("Progress / chart rings",rings_,"UiBezierCurveField",curve_field_,130,true);
}
void ValuesSection::Apply(const UiThemeContext& context, UiRole role)
{
    ThemeSection::Apply(context, role);
    auto style = UiTheme::ResolveRangeSegments(context, role);
    style.track_size.cy = DPI(48); style.thumb_size = Size(DPI(6),DPI(28));
    segments_.SetCustomStyle(style);
}

ColourSection::ColourSection()
    : ThemeSection("Colour and spatial input", "The surrounding chrome follows the theme; the selected colours remain data")
{
    anchor_.SelectIndex(4); AddSpecimen("UiMatrixSelector", anchor_, 90);
    swatches_.SetColorCount(4).SetColor(0,Color(76,120,160));
    swatches_.SetColor(1,Color(98,151,133)).SetColor(2,Color(216,169,102)).SetColor(3,Color(167,92,108));
    AddSpecimen("UiColorMatrix", swatches_);
    micro_.SetColor(Color(98,151,133)); AddSpecimen("UiColorPickerMicro", micro_, 230);
    picker_.SetSlotCount(2).SetSlotColor(0,Color(98,151,133),false).SetSlotColor(1,Color(76,120,160),false);
    AddSpecimen("UiColorPicker", picker_, 500);
}
