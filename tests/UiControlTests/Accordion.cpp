#include <Core/Core.h>
#include <Ui/Ui.h>

using namespace Upp;

namespace {
class AccordionMeasuredChild : public Ctrl {
public:
    Size GetMinSize() const override { return Size(80, 32); }
};
}

int RunAccordionSuite()
{
    int checks = 0, failed = 0;
    auto check = [&](bool ok, const char* text) {
        ++checks;
        if(!ok) { ++failed; Cout() << "FAIL: " << text << '\n'; }
    };
    UiAccordion accordion;
    accordion.SetAnimation(false).SetSingleOpen(false).SetEnforceOne(false);
    auto style = accordion.GetStyle();
    style.body_style.metrics.frame_width = 3;
    style.body_style.metrics.content_margin = Rect(8, 8, 8, 8);
    accordion.SetCustomStyle(style);
    ParentCtrl empty;
    AccordionMeasuredChild measured, absolute;
    int first = accordion.AddSection("Empty filling host", true);
    int second = accordion.AddSection("Measured filling control", true);
    int third = accordion.AddSection("Explicitly positioned control", true);
    accordion.GetSectionContent(first).Add(empty.SizePos());
    accordion.GetSectionContent(second).Add(measured.HSizePos(0, 0).VSizePos(4, 6));
    accordion.GetSectionContent(third).Add(absolute.LeftPos(0, 80).TopPos(10, 90));
    const int natural = accordion.GetMinSize().cy;
    for(int height : {400, 800, 250, 600}) {
        accordion.SetRect(0, 0, 400, height);
        for(int pass = 0; pass < 4; ++pass) {
            accordion.Layout();
            check(accordion.GetMinSize().cy == natural,
                  "filling content does not feed its allocated height into intrinsic measurement");
            for(int section = 0; section < accordion.GetCount(); ++section) {
                auto& body = accordion.GetSectionBody(section);
                const auto& body_style = body.GetStyle();
                check(accordion.GetSectionContent(section).GetRect() ==
                      UiStyledInnerRect(Rect(body.GetSize()), body_style.metrics, body_style.skin),
                      "section content uses the body's inset and chrome bounds");
            }
        }
    }
    check(accordion.GetSectionBodyHeight(second) >= 32 + 4 + 6,
          "filling content reserves its intrinsic height and anchor insets");
    check(accordion.GetSectionBodyHeight(third) >= 100,
          "absolute content retains its authored offset and height");
    Cout() << "ACCORDION checks=" << checks << " failed=" << failed << '\n';
    return failed ? 1 : 0;
}
