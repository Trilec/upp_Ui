#include <Ui/Ui.h>

using namespace Upp;

namespace {

struct TestCtx {
    int checks = 0;
    int fails = 0;

    void Expect(bool ok, const String& text)
    {
        checks++;
        Cout() << (ok ? "PASS: " : "FAIL: ") << text << '\n';
        if(!ok)
            fails++;
    }
};

struct ProbeButton : UiButton {
    bool IsMouseOverProbe() const { return mouse_over_; }
};

void TestInteractionGeometry(TestCtx& t)
{
    ProbeButton button;
    button.SetRect(0, 0, 100, 40);

    t.Expect(button.GetInteractionRect() == Rect(0, 0, 100, 40),
             "button without outer shadow uses the full control surface for pointer interaction");

    UiButton::Style style = button.GetStyle();
    style.metrics.shadow.enabled = true;
    style.metrics.shadow.inset = false;
    style.metrics.shadow.distance = 5;
    style.metrics.shadow.offset_x = 2;
    style.metrics.shadow.offset_y = 3;
    style.metrics.shadow.alpha = 90;
    button.SetCustomStyle(style);

    t.Expect(button.GetInteractionRect() == Rect(3, 2, 93, 32),
             "procedural outer shadow margins are excluded from the pointer interaction surface");
    t.Expect(!button.IsInteractionPoint(Point(1, 1)) && button.IsInteractionPoint(Point(10, 10)),
             "shadow-only pixels are non-interactive while styled-surface pixels remain interactive");

    button.MouseMove(Point(1, 1), 0);
    t.Expect(!button.IsMouseOverProbe(),
             "hover does not activate over procedural shadow-only pixels");
    button.MouseMove(Point(10, 10), 0);
    t.Expect(button.IsMouseOverProbe(),
             "hover activates when the pointer enters the styled interaction surface");

    button.SetInteractionInset(Rect(2, 3, 4, 5));
    t.Expect(button.GetInteractionRect() == Rect(5, 5, 89, 27),
             "explicit interaction inset further removes baked decorative pixels from the styled surface");
    t.Expect(button.GetInteractionInset() == Rect(2, 3, 4, 5),
             "interaction inset remains explicit control state");

    button.SetInteractionInset(Rect(-4, 2, -7, 3));
    t.Expect(button.GetInteractionInset() == Rect(0, 2, 0, 3),
             "interaction inset clamps negative thickness to zero");

    style.metrics.shadow.inset = true;
    style.skin.content_inset = Rect(9, 8, 7, 6);
    button.SetCustomStyle(style);
    button.SetInteractionInset(0);
    t.Expect(button.GetInteractionRect() == Rect(0, 0, 100, 40),
             "inset shadow and skin content inset do not silently shrink pointer interaction");

    button.SetPressedContentOffset(2, 1);
    t.Expect(button.GetPressedContentOffset() == Point(2, 1),
             "pressed-content displacement has an explicit semantic API separate from hit geometry");

    UiSplitButton split;
    split.SetInteractionInset(2)
         .SetPressedContentOffset(1, 1)
         .SetSplitWidth(DPI(28))
         .SetPopupMinWidth(DPI(160));
    t.Expect(split.GetInteractionInset() == Rect(2, 2, 2, 2)
             && split.GetPressedContentOffset() == Point(1, 1),
             "split button preserves fluent access to the shared interaction and pressed-content APIs");
}

void TestIconSizingContract(TestCtx& t)
{
    UiButton button;
    button.SetText(String())
          .SetIcon(ICON_DESIGN_WIDGETS_48())
          .SetIconSize(31, 17)
          .SetIconScaleToContent(false)
          .SetIconRenderMode(UiIconRenderMode::MonoTint);

    t.Expect(button.GetIconSize() == Size(31, 17),
             "explicit button icon width and height remain independent authored dimensions");
    t.Expect(!button.IsIconScaleToContent(),
             "explicit button icon sizing is not silently converted to scale-to-content");
    t.Expect(button.GetIconRenderMode() == UiIconRenderMode::MonoTint,
             "button keeps the shared explicit icon render-mode contract");

    const Size exact_min = button.GetMinSize();
    t.Expect(exact_min.cx >= 31 && exact_min.cy >= 17,
             "button natural sizing reserves the explicit icon dimensions");

    button.SetIconScaleToContent(true);
    t.Expect(button.IsIconScaleToContent(),
             "scale-to-content remains an explicit independent presentation mode");

    button.SetIconScaleToContent(false).SetIconSize(18, 29);
    t.Expect(button.GetIconSize() == Size(18, 29),
             "changing explicit icon dimensions preserves the requested non-square size");
}

} // namespace

CONSOLE_APP_MAIN
{
    TestCtx t;
    TestInteractionGeometry(t);
    TestIconSizingContract(t);
    Cout() << "\nChecks: " << t.checks << ", Fails: " << t.fails << '\n';
    SetExitCode(t.fails ? 1 : 0);
}
