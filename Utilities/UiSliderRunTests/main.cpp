#include <Core/Core.h>
#include <Ui/Ui.h>
#include <cmath>

using namespace Upp;

namespace {

struct TestCtx {
    int checks = 0;
    int fails = 0;

    void Expect(bool ok, const String& text)
    {
        checks++;
        if(!ok) {
            fails++;
            Cout() << "FAIL: " << text << '\n';
        }
    }
};

bool Near(double a, double b)
{
    return std::fabs(a - b) < 1e-12;
}

}

CONSOLE_APP_MAIN
{
    TestCtx t;

    UiSlider slider;
    t.Expect(Near(slider.GetMin(), 0.0) && Near(slider.GetMax(), 100.0),
             "default domain is 0..100");
    t.Expect(Near(slider.GetValue(), 0.0), "default value is zero");

    slider.SetRange(10, -10).SetStep(2).SetValue(5.1);
    t.Expect(Near(slider.GetMin(), -10.0) && Near(slider.GetMax(), 10.0),
             "reversed domain is normalized");
    t.Expect(Near(slider.GetValue(), 6.0), "value is clamped and step-quantized");

    slider.SetData(-3.1);
    t.Expect(Near(slider.GetValue(), -4.0), "SetData uses the authoritative numeric contract");
    t.Expect(Near((double)slider.GetData(), -4.0), "GetData reports the current value");

    int changing = 0;
    int actions = 0;
    slider.WhenChanging = [&] { changing++; };
    slider.WhenAction = [&] { actions++; };

    slider.Disable();
    const double disabled_value = slider.GetValue();
    t.Expect(!slider.Key(K_RIGHT, 1), "disabled slider rejects keyboard editing");
    slider.MouseWheel(Point(0, 0), 120, 0);
    t.Expect(Near(slider.GetValue(), disabled_value),
             "disabled slider rejects wheel editing");
    t.Expect(changing == 0 && actions == 0,
             "disabled input emits no value events");

    slider.Enable();
    t.Expect(slider.Key(K_RIGHT, 1), "enabled slider accepts keyboard editing");
    t.Expect(Near(slider.GetValue(), disabled_value + 2.0),
             "keyboard editing advances by the configured step");
    t.Expect(changing == 1 && actions == 1,
             "keyboard edit emits changing and action once");
    slider.MouseWheel(Point(0, 0), -120, 0);
    t.Expect(Near(slider.GetValue(), disabled_value),
             "wheel editing follows the configured step");
    t.Expect(changing == 2 && actions == 2,
             "wheel edit emits changing and action once");

    UiSlider geometry;
    geometry.SetTrackSize(Size(DPI(150), DPI(5))).SetThumbSize(Size(DPI(20), DPI(20)));
    geometry.SetDirection(UiDirection::H);
    Size horizontal = geometry.GetMinSize();
    geometry.SetDirection(UiDirection::V);
    Size vertical = geometry.GetMinSize();
    t.Expect(horizontal.cx > horizontal.cy,
             "horizontal natural size uses track length on the major axis");
    t.Expect(vertical.cy > vertical.cx,
             "vertical natural size uses track length on the major axis");

    geometry.SetRect(0, 0, DPI(50), DPI(260));
    Rect vertical_track = geometry.GetTrackGeometry();
    t.Expect(vertical_track.GetHeight() > vertical_track.GetWidth() * 8,
             "vertical track is tall and narrow rather than rotating length into thickness");

    geometry.ExpandTrack(true).SetDirection(UiDirection::H);
    geometry.SetRect(0, 0, DPI(220), DPI(50));
    Rect short_h = geometry.GetTrackGeometry();
    geometry.SetRect(0, 0, DPI(520), DPI(50));
    Rect long_h = geometry.GetTrackGeometry();
    t.Expect(long_h.GetWidth() > short_h.GetWidth() + DPI(250),
             "expanded horizontal track consumes additional allocated width");

    geometry.SetDirection(UiDirection::V);
    geometry.SetRect(0, 0, DPI(50), DPI(220));
    Rect short_v = geometry.GetTrackGeometry();
    geometry.SetRect(0, 0, DPI(50), DPI(520));
    Rect long_v = geometry.GetTrackGeometry();
    t.Expect(long_v.GetHeight() > short_v.GetHeight() + DPI(250),
             "expanded vertical track consumes additional allocated height");

    UiSlider no_ticks;
    Size without_ticks = no_ticks.GetMinSize();
    no_ticks.SetTicks(true, 5, 0);
    Size with_ticks = no_ticks.GetMinSize();
    t.Expect(with_ticks.cy > without_ticks.cy,
             "horizontal ticks participate in natural cross-axis size");

    Cout() << Format("UISLIDER_SUMMARY checks=%d failed=%d\n", t.checks, t.fails);
    SetExitCode(t.fails ? 1 : 0);
}
