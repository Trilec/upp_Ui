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

void TestDropdownAuthority(TestCtx& t)
{
    UiListModel model;
    UiModelItem first("First", 1);
    first.description = "External model item";
    first.right_text = "A";
    model.Add(first);
    model.Add(UiModelItem("Second", 2));

    UiDropdown drop;
    drop.SetModel(model);
    t.Expect(drop.GetCount() == 2 && drop.GetItem(0).text == "First",
             "Dropdown reads count and rows directly from the bound UiListModel");

    UiModelItem changed = model.Get(0);
    changed.text = "Changed externally";
    changed.description = "No mirror refresh required";
    model.Set(0, changed);
    t.Expect(drop.GetItem(0).text == "Changed externally"
             && drop.GetItemDescription(0) == "No mirror refresh required",
             "external model update is immediately authoritative in Dropdown");

    drop.SelectByData(1);
    drop.SetRect(0, 0, 260, 34);
    drop.Layout();
    t.Expect(drop.GetSelectedText() == "Changed externally" && drop.GetLiveItemRenderCount() == 1,
             "collapsed Dropdown selection uses one prepared renderer instance");

    drop.Layout();
    t.Expect(drop.GetLastRenderLayoutCount() == 0,
             "unchanged collapsed Dropdown layout reuses prepared renderer geometry");

    UiItemRenderImage image;
    drop.SetItemRender(image);
    drop.Layout();
    t.Expect(drop.GetLiveItemRenderCount() == 1 && drop.GetLastRenderLayoutCount() == 1,
             "Dropdown renderer prototype can be replaced without changing model state");

    UiDropdown::Item spelling("Alias spelling", 3);
    drop.UseInternalModel().Clear().Add(spelling);
    t.Expect(drop.GetCount() == 1 && drop.GetItem(0).data == Value(3),
             "UiDropdown::Item is only a direct UiModelItem spelling, not separate state");
}

void TestMenuRenderData(TestCtx& t)
{
    UiMenuItem item("Open", "payload");
    item.description = "Open a document";
    item.right_text = "Right";
    item.shortcut_text = "Ctrl+O";
    item.command_id = "file.open";
    item.default_item = true;
    item.enabled = true;

    UiItemRenderData data = UiMakeItemRenderData(item);
    t.Expect(data.title == "Open" && data.description == "Open a document"
             && data.right_text == "Ctrl+O",
             "Menu item maps ordinary title/description/right content into UiItemRenderData");
    t.Expect(data.data == Value("payload") && data.value == Value("file.open") && data.emphasized,
             "Menu render data preserves payload, command id and default-item emphasis");

    UiItemRenderBasic render;
    render.SetData(data);
    t.Expect(render.PrepareLayout(RectC(0, 0, 280, 30), UiDirection::H),
             "Menu presentation data prepares through the common item renderer");
    int serial = render.GetLayoutSerial();
    UiItemRenderState state;
    state.hot = true;
    ImageDraw draw(280, 30);
    render.Paint(draw, state);
    t.Expect(render.GetLayoutSerial() == serial,
             "Menu renderer Paint consumes prepared geometry without relayout");

    UiMenu menu;
    UiMenuModel model;
    model.AddChild(model.Root(), item);
    menu.SetModel(model);
    UiItemRenderImage image;
    menu.SetItemRender(image);
    t.Expect(dynamic_cast<const UiItemRenderImage *>(&menu.GetItemRender()) != nullptr
             && menu.GetLiveItemRenderCount() == 0,
             "Menu accepts a shared renderer prototype while closed without per-model renderer allocation");
}

} // namespace

CONSOLE_APP_MAIN
{
    TestCtx t;
    TestDropdownAuthority(t);
    TestMenuRenderData(t);
    Cout() << "\nChecks: " << t.checks << ", Fails: " << t.fails << '\n';
    SetExitCode(t.fails ? 1 : 0);
}
