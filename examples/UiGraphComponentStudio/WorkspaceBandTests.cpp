#include "WorkspaceWindow.h"
#include <Ui/UiGraph/UiGraphNodeComponent.h>

namespace Upp {
namespace GraphWorkspace {
// Integration, not another allocator. This gate uses the real inspector setters,
// retained production presentation, shared paint clip and production JSON/export.
bool RunWorkspaceBandTests(String& error)
{
    error.Clear();
    int checks = 0, failed = 0;
    auto expect = [&](bool pass, const char* message) {
        checks++;
        if(!pass) { failed++; if(error.IsEmpty()) error = message; }
        LOG((pass ? "PASS: " : "FAIL: ") << message);
    };
    NodeWorkspace w;
    w.SetRect(0, 0, DPI(1560), DPI(980));
    w.Layout();
    w.document_ = MakeDocument(UiGraphNodeTemplateKind::Media);
    w.document_.shape = 1; // Ellipse preview; Base remains the editing scope.
    w.preview_.SetZoom(1.0, Point(0, 0));
    w.ApplyDocument();
    w.SelectPage(1);
    auto commit = [&](const char* id, const Value& value) {
        w.ApplyProperty(id, value, true);
        Ctrl::ProcessEvents();
    };
    expect(w.EffectiveLayout().ellipse_bands
           && !UiGraphBuiltinNodeTemplate(UiGraphNodeTemplateKind::Media).ellipse_bands
           && !MakeDocument(UiGraphNodeTemplateKind::Minimal).family.base_layout.ellipse_bands,
           "new identified Media opts in without changing legacy production templates or Minimal");
    const auto* flag = w.properties_.Find("ellipse_bands");
    const auto* width = w.properties_.Find("ellipse_width");
    expect(flag && width && flag->enabled && width->enabled,
           "Template inspector exposes editable band policy in Base scope");
    const double zoom = w.preview_.GetZoom();
    const Pointf pan = w.preview_.GetPan();
    const Size node_size = w.document_.size;
    w.inspector_.SelectProperty("ellipse_bands");
    const int structure = w.properties_.GetStructureRevision();
    commit("ellipse_bands", false);
    const UiGraphNodePresentation conservative = w.snapshot_;
    commit("ellipse_bands", true);
    const UiGraphNodePresentation banded = w.snapshot_;
    expect(!banded.header.IsEmpty() && !banded.footer.IsEmpty()
           && banded.header.top < conservative.header.top
           && banded.footer.bottom > conservative.footer.bottom
           && banded.header.GetWidth() < conservative.header.GetWidth()
           && banded.header.GetHeight() == conservative.header.GetHeight()
           && banded.footer.GetHeight() == conservative.footer.GetHeight()
           && banded.safe == conservative.safe,
           "inspector toggle moves real ellipse bands outward without inflating safe or their heights");
    expect(w.preview_.GetZoom() == zoom && w.preview_.GetPan() == pan
           && w.document_.size == node_size
           && w.inspector_.GetSelectedPropertyId() == "ellipse_bands"
           && w.properties_.GetStructureRevision() == structure,
           "band edit preserves camera, authored size and inspector structure/selection");
    commit("ellipse_width", 65);
    expect(w.EffectiveLayout().ellipse_band_width_percent == 65
           && w.snapshot_.header.GetWidth() < banded.header.GetWidth()
           && w.snapshot_.header.top <= banded.header.top,
           "band percentage controls production width and outward placement");
    w.Undo();
    expect(w.EffectiveLayout().ellipse_band_width_percent == 80
           && w.snapshot_.header == banded.header && w.snapshot_.footer == banded.footer,
           "band edit uses the existing undo transaction");

    // There must be a pickable painted footprint in the newly available area,
    // not only a red diagram moved beyond the old safe clipping rectangle.
    bool found_outer = false;
    Point outer;
    String outer_id;
    int outer_region = -1;
    for(const auto& c : w.snapshot_.components) {
        if(c.region != UiGraphNodeSlotRegion::Header
           || c.representation == UiGraphNodeComponentRepresentation::Hidden) continue;
        Rect painted = c.footprint & UiNodeGraphDetail::NodeComponentClip(w.snapshot_, c);
        Rect outside = painted;
        outside.bottom = min(outside.bottom, w.snapshot_.safe.top);
        if(outside.IsEmpty()) continue;
        outer = outside.CenterPoint(); outer_id = c.id; outer_region = (int)c.region;
        found_outer = true; break;
    }
    expect(found_outer, "Media has painted header content outside the former safe rectangle");
    if(found_outer) {
        w.selection_.id.Clear();
        w.preview_.LeftDown(outer, 0);
        expect(w.selection_.id == outer_id && w.selection_.region == outer_region,
               "outer ellipse header picks the same identified component as production paint");
    }

    w.document_.edit_base = false;
    w.SelectPage(1);
    commit("ellipse_width", 60);
    expect(!w.properties_.Find("ellipse_width")->enabled
           && !w.document_.family.layout_override[1]
           && w.document_.family.base_layout.ellipse_band_width_percent == 80,
           "inherited shape rejects band edits without silently detaching or changing Base");
    w.document_.family.DetachLayout(1); w.Changed();
    commit("ellipse_width", 70);
    expect(w.document_.family.Layout(1).ellipse_band_width_percent == 70
           && w.document_.family.base_layout.ellipse_band_width_percent == 80
           && !w.document_.family.style_override[1],
           "explicit shape-layout override isolates band edits from Base and appearance");
    Document restored;
    String message;
    String json = AsJSON(Encode(w.document_));
    expect(Load(json, restored, message) && AsJSON(Encode(restored)) == json
           && restored.family.Layout(1).ellipse_band_width_percent == 70,
           "inspector-authored band policy survives the real JSON round trip");
    String code = GenerateCpp(w.document_, message);
    expect(message.IsEmpty() && code.Find("t.ellipse_bands = true;") >= 0
           && code.Find("t.ellipse_band_width_percent = 70;") >= 0
           && code.Find("MakeEllipseLayout") >= 0,
           "inspector-authored shape policy reaches the production C++ factory");

    // Keep the previously failing title case independent of the new band default.
    // It must pass with bands DISABLED as well as enabled: no changed header size,
    // node dimensions, font floor or masks are allowed to conceal the failure.
    w.document_ = MakeDocument(UiGraphNodeTemplateKind::Media);
    w.document_.edit_base = true;
    String title_id, subtitle_id;
    for(int i = 0; i < w.document_.family.base_layout.slot_count; i++) {
        auto& r = w.document_.family.base_layout.slots[i];
        if(r.region != UiGraphNodeSlotRegion::Header) continue;
        if(r.feature == UiGraphNodeSlotFeature::Title) { title_id = r.id; r.font_height = DPI(20); }
        if(r.feature == UiGraphNodeSlotFeature::Subtitle) {
            subtitle_id = r.id; r.font_height = DPI(18); r.component_style.bold = 1;
        }
    }
    for(int i = 0; i < 2; i++) {
        auto icon = NewComponent(UiGraphNodeComponentKind::Icon, w.EffectiveLayout());
        expect(PlaceComponent(w.document_, -1, w.document_.revision, icon, false,
                              UiGraphNodeSlotRegion::Header, String(), message),
               "readable-row fixture inserts both icons through the real authoring transaction");
    }
    for(bool bands : {false, true}) for(int shape : {0, 1}) {
        w.document_.family.base_layout.ellipse_bands = bands;
        w.document_.shape = shape;
        w.preview_.SetZoom(1.0, Point(0, 0)); w.ApplyDocument();
        bool readable = w.snapshot_.level == UiGraphPresentationLevel::Normal;
        for(const String& id : {title_id, subtitle_id}) {
            const auto* c = w.snapshot_.FindComponent(id);
            bool text = c && c->representation == UiGraphNodeComponentRepresentation::Text
                       && !c->text.IsEmpty() && c->font.GetCy() <= c->content.GetHeight();
            readable &= text;
            if(!text) {
                LOG("UIGRAPH_TEXT_CAPACITY shape=" << shape << " bands=" << bands
                    << " id=" << id << " header=" << w.snapshot_.header
                    << " outcome=" << ComponentOutcome(c));
                if(c) LOG("slot=" << c->slot << " content=" << c->content
                          << " font-height=" << c->font.GetHeight() << " line=" << c->font.GetCy());
            }
        }
        expect(readable, "both Media text rows remain readable at Normal with and without ellipse bands");
    }

    // The C++ page must own the whole content host, even after repeated page and
    // window layout passes. Test all four modes, not just their Boolean setting.
    for(int page : {3, 0, 2, 1, 3}) {
        w.SelectPage(page); w.Layout(); w.rail_.Layout(); w.rail_content_.Layout();
        const bool coding = page == 3;
        bool exclusive = w.inspector_.IsShown() != coding && w.code_.IsShown() == coding
                      && w.code_tools_.IsShown() == coding;
        if(coding) exclusive &= w.code_.GetRect().top == DPI(32)
                            && w.code_.GetRect().bottom == w.rail_content_.GetSize().cy;
        for(int i = 0; i < 4; i++) exclusive &= w.mode_[i].IsChecked() == (i == page);
        expect(exclusive, "rail mode remains exclusive, selected and full-height after relayout");
    }
    LOG("UIGRAPH_WORKSPACE_BAND_UI_SUMMARY checks=" << checks << " failed=" << failed);
    return failed == 0;
}
} // namespace GraphWorkspace
} // namespace Upp
