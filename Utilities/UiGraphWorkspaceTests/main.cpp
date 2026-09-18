#include <Utilities/UiGraphWorkspace/UiGraphWorkspace.h>
using namespace Upp;
using namespace Upp::GraphWorkspace;

CONSOLE_APP_MAIN
{
    int checks = 0, failed = 0;
    auto expect = [&](bool pass, const char* text) {
        checks++; if(!pass) failed++;
        Cout() << (pass ? "PASS: " : "FAIL: ") << text << '\n';
    };
    Document d = MakeDocument(); String error;
    expect(Validate(d, error), "default family validates");
    for(int i = 1; i <= 7; i++) {
        auto preset = MakeDocument((UiGraphNodeTemplateKind)i);
        expect(Validate(preset, error), "every built-in is an identified production template");
    }
    d.family.DetachLayout(1);
    d.family.shape_layout[1].header_height = DPI(17);
    expect(d.family.Layout(0).header_height == d.family.base_layout.header_height && d.family.Layout(1).header_height == DPI(17), "layout override isolates shape");
    expect(!d.family.style_override[1], "layout override does not detach appearance");
    d.family.DetachStyle(2); d.family.shape_style[2].face[0] = Color(5, 10, 15);
    expect(!d.family.layout_override[2] && d.family.Style(2).face[0] == Color(5, 10, 15), "style override is independent");
    Document copy = d; copy.family.base_layout.slots[0].label = "Independent clone";
    expect(copy.family.base_layout.slots[0].label != d.family.base_layout.slots[0].label, "cloning does not alias component edits");
    auto r = NewComponent(UiGraphNodeComponentKind::Text, d.family.base_layout);
    r.Literal("Repeated footer");
    int revision = d.revision;
    expect(PlaceComponent(d, -1, revision, r, false, UiGraphNodeSlotRegion::Footer, String(), error), "palette drop commits");
    expect(!PlaceComponent(d, -1, revision, r, true, UiGraphNodeSlotRegion::Header, String(), error), "stale drag rejected");
    expect(!PlaceComponent(d, 0, d.revision, r, true, UiGraphNodeSlotRegion::Header, String(), error), "inherited target read-only");
    expect(PlaceComponent(d, -1, d.revision, r, true, UiGraphNodeSlotRegion::Header, String(), error), "move transaction");
    int idx = d.family.base_layout.FindComponent(r.id);
    expect(idx >= 0 && d.family.base_layout.slots[idx].literal == r.literal, "move preserves identity and data");
    String before = AsJSON(Encode(d));
    expect(!PlaceComponent(d, -1, d.revision, r, false, UiGraphNodeSlotRegion::Footer, String(), error) && before == AsJSON(Encode(d)), "rejected duplicate is atomic");
    expect(!PlaceComponent(d, -1, d.revision, r, true, (UiGraphNodeSlotRegion)255, String(), error), "invalid region rejected");
    expect(RemoveComponent(d, -1, r.id, error), "remove identified component");
    d.family.base_layout.slots[0].label = "Quote \" slash \\ newline\nUTF8: \303\251";
    String json = AsJSON(Encode(d)); Document loaded;
    expect(Load(json, loaded, error) && AsJSON(Encode(loaded)) == json, "JSON round trip preserves family, independent variants and preview");
    int saved_revision = loaded.revision;
    expect(!Load("{invalid", loaded, error) && loaded.revision == saved_revision, "bad JSON leaves document intact");
    ValueMap bad = Encode(d); bad.Set("version", 99);
    expect(!Decode(bad, loaded, error), "unknown version rejected");
    bad = Encode(d); bad.Add("future_field", 1);
    expect(!Decode(bad, loaded, error), "unknown fields rejected rather than lost");
    expect(!Load(String('[', 40) + String(']', 40), loaded, error), "nesting bound enforced before parse");
    String code = GenerateCpp(d, error);
    expect(!code.IsEmpty() && error.IsEmpty(), "C++ emitted");
    expect(code.Find("MakeBaseLayout") >= 0 && code.Find("MakeBaseStyle") >= 0 && code.Find("bool Register") >= 0, "separate layout style and real registration");
    expect(code.Find("ParseJSON") < 0 && code.Find("UiGraphWorkspace.h") < 0, "generated runtime code has no authoring/JSON dependency");
    expect(QuoteCpp("x\"\\\n") == "\"x\\\"\\\\\\012\"", "C++ string escaping");
    // Test save failure without disturbing an existing good file.
    String path = AppendFileName(GetTempPath(), "uigraph-workspace-test-" + AsString(Random()) + ".json");
    expect(SaveAtomic(path, json, error), "atomic first save");
    expect(SaveAtomic(path, "replacement", error) && LoadFile(path) == "replacement", "atomic overwrite");
    expect(!SaveAtomic(AppendFileName(path, "missing/file.json"), json, error) && LoadFile(path) == "replacement", "failed save preserves prior good file");
    FileDelete(path);
    for(const String& arg : CommandLine())
        if(arg.StartsWith("--export=")) expect(SaveFile(arg.Mid(9), code), "write generated compilation fixture");
    Cout() << "UIGRAPH_WORKSPACE_SUMMARY checks=" << checks << " failed=" << failed << '\n';
    SetExitCode(failed ? 1 : 0);
}
