#include "WorkspaceWindow.h"
#include <Ui/UiGraph/UiGraphNodeComponent.h>

namespace Upp {
namespace GraphWorkspace {
// Tests the actual view coordinate/selection contract, not a second allocator.
// OS drag negotiation and capture during a physical drag remain manual checks.
bool RunWorkspaceViewTests(String& error)
{
    error.Clear(); int checks = 0, failed = 0;
    auto expect = [&](bool ok, const char* message) {
        checks++;
        if(!ok) { failed++; if(error.IsEmpty()) error = message; }
        LOG((ok ? "PASS: " : "FAIL: ") << message);
    };
    Document document = MakeDocument();
    const UiGraphNodeTemplate& spec = document.family.base_layout;
    UiGraphNodePresentation p;
    Vector<Pointf> outline;
    outline << Pointf(0, 0) << Pointf(100, 0) << Pointf(100, 80) << Pointf(0, 80);
    WorkspaceSelection selection;
    RegionView region;
    for(int width : {160, 220, 320}) {
        region.SetRect(0, 0, DPI(width), DPI(210));
        region.Set(spec, p, outline, RectC(0, 0, 100, 80), selection);
        bool inside = region.targets_.GetCount() == 5, disjoint = true, reachable = true;
        for(int i = 0; i < region.targets_.GetCount(); i++) {
            const Rect& box = region.targets_[i].rect;
            inside &= !box.IsEmpty() && (box & Rect(region.GetSize())) == box;
            reachable &= region.Hit(box.CenterPoint()) == i;
            for(int j = 0; j < i; j++) disjoint &= (box & region.targets_[j].rect).IsEmpty();
        }
        expect(inside && disjoint && reachable, "all empty-region targets wrap inside a narrow diagram");
    }
    expect(region.Hit(Point(-1, 0)) < 0 && region.Hit(Point(region.GetSize().cx, 1)) < 0,
           "diagram rejects outside-client points");
    p.safe = RectC(0, 0, 100, 80); p.header = RectC(0, 0, 100, 24);
    p.body = p.content = p.overlay = RectC(0, 24, 100, 56);
    p.content_main = p.overlay_main = p.body;
    p.port_lanes[0] = RectC(0, 24, 20, 56);
    auto& visible = p.components.Add();
    visible.id = "visible"; visible.region = UiGraphNodeSlotRegion::Header;
    visible.slot = RectC(0, 0, 48, 24); visible.footprint = visible.slot;
    visible.representation = UiGraphNodeComponentRepresentation::Text;
    auto& hidden = p.components.Add();
    hidden.id = "hidden"; hidden.region = UiGraphNodeSlotRegion::Header;
    hidden.slot = RectC(52, 0, 48, 24);
    hidden.representation = UiGraphNodeComponentRepresentation::Hidden;
    region.Set(spec, p, outline, RectC(0, 0, 100, 80), selection);
    expect(region.Hit(region.Project(p.port_lanes[0]).CenterPoint()) < 0,
           "shared body-side port lane is not a component drop target");
    bool visible_active = false, hidden_inactive = false;
    int hidden_target = -1;
    for(int i = 0; i < region.targets_.GetCount(); i++) {
        const auto& target = region.targets_[i];
        if(target.id == "visible") visible_active = region.Active(target);
        if(target.id == "hidden") { hidden_inactive = !region.Active(target); hidden_target = i; }
    }
    expect(visible_active && hidden_inactive,
           "hidden component stays grey even when its region contains a visible sibling");
    String picked;
    region.WhenSelect = [&](String id, int) { picked = id; };
    if(hidden_target >= 0) region.LeftDown(region.targets_[hidden_target].rect.CenterPoint(), 0);
    expect(picked == "hidden", "hidden Stable component remains selectable in its diagram");
    region.hover_ = 3; region.Set(spec, p, outline, RectC(0, 0, 100, 80), selection);
    expect(region.hover_ < 0, "diagram refresh clears stale drop feedback");

    StructureView table;
    table.SetRect(0, 0, DPI(360), DPI(240)); table.Set(spec, p, selection);
    table.horizontal_.Set(table.CanvasWidth() - table.GetSize().cx);
    int row = -1;
    for(int i = 0; i < table.rows_.GetCount(); i++)
        if(!table.rows_[i].id.IsEmpty()) { row = i; break; }
    expect(row >= 0, "production family exposes a component row");
    if(row >= 0) {
        const String id = table.rows_[row].id;
        int lod = -1, drags = 0;
        table.WhenSelect = [&](String, int) { table.Set(spec, p, selection); }; // re-entrant rebuild
        table.WhenLod = [&](String selected, int l) { if(selected == id) lod = l; };
        table.WhenDrag = [&](String) { drags++; };
        Point cell(table.Columns() + DPI(62) * 3 + DPI(20) - table.horizontal_.Get(),
                   DPI(30) + row * table.RowHeight() + DPI(10));
        expect(Rect(table.GetSize()).Contains(cell), "horizontal scroll makes the LOD3 cell reachable");
        table.LeftDown(cell, 0);
        expect(lod == 3, "scrolled LOD click keeps the intended component and level across selection rebuild");
        table.LeftDrag(cell, 0);
        expect(drags == 0, "LOD cells cannot start a component move");
        lod = -1; table.LeftDown(Point(-1, cell.y), 0);
        table.LeftDown(Point(cell.x, table.GetSize().cy), 0);
        expect(lod < 0, "structure ignores coordinates outside its client area");
        table.drop_row_ = row; table.Set(spec, p, selection);
        expect(table.drop_row_ < 0, "table refresh clears stale insertion feedback");
    }
    UiGraphNodeSlotRule styled;
    styled.data_key = "status"; styled.component_style.face[0] = Color(12, 127, 211);
    expect(PlacementSummary(styled).Find("status") >= 0 && PlacementSummary(styled).Find("styled") >= 0,
           "placement summary includes data binding and component face overrides");

    DragTile tile; int drags = 0, clicks = 0;
    tile.WhenDrag = [&] { drags++; };
    tile.WhenAction = [&] { clicks++; };
    tile.Disable(); tile.LeftDrag(Point(), 0);
    expect(drags == 0, "disabled palette item cannot start a drag");
    tile.Enable(); tile.LeftDrag(Point(), 0); tile.LeftUp(Point(), 0);
    expect(drags == 1 && clicks == 0, "drag path is disarmed before a later button release");
    tile.Key(K_SPACE, 1);
    expect(clicks == 1, "palette keyboard activation still works after drag cancellation");
    // User regression: two icons dropped into Media's existing header must not
    // appear only when the subtitle reflows away at LOD1. Use real authoring
    // transactions and the production renderer, not a hand-made allocation.
    NodeWorkspace workspace;
    workspace.document_ = MakeDocument(UiGraphNodeTemplateKind::Media);
    workspace.document_.edit_base = true;
    String icons[2];
    for(int i = 0; i < 2; i++) {
        auto icon = NewComponent(UiGraphNodeComponentKind::Icon, workspace.EffectiveLayout());
        icons[i] = icon.id;
        expect(PlaceComponent(workspace.document_, -1, workspace.document_.revision,
                              icon, false, UiGraphNodeSlotRegion::Header, String(), error),
               "Media header accepts a new identified icon");
    }
    workspace.ApplyDocument();
    bool icons_visible = true;
    for(double zoom : {1.0, 0.38, 1.0}) {
        workspace.preview_.SetZoom(zoom, Point(0, 0));
        workspace.preview_.CenterOnNode(workspace.node_);
        workspace.Reports();
        for(const String& id : icons) {
            const auto* c = workspace.snapshot_.FindComponent(id);
            icons_visible &= c && !c->slot.IsEmpty()
                          && c->representation != UiGraphNodeComponentRepresentation::Hidden;
        }
    }
    expect(icons_visible, "both header icons survive Normal / LOD1 / Normal without mask changes");
    workspace.page_ = 1; workspace.selection_.id.Clear(); workspace.RebuildInspector();
    workspace.inspector_.SelectProperty("preview_labels");
    workspace.inspector_.SetPropertyExpanded("preview_data", true);
    const int revision = workspace.properties_.GetStructureRevision();
    for(bool labels : {true, false, true}) {
        workspace.ApplyProperty("preview_labels", labels, true);
        Ctrl::ProcessEvents();
        expect(workspace.inspector_.GetSelectedPropertyId() == "preview_labels"
               && workspace.inspector_.IsPropertyExpanded("preview_data")
               && workspace.properties_.GetStructureRevision() == revision,
               "port-label commit preserves property selection, expansion and model structure");
    }
    String title_id;
    for(int i = 0; i < workspace.EffectiveLayout().slot_count; i++)
        if(workspace.EffectiveLayout().slots[i].feature == UiGraphNodeSlotFeature::Title)
            title_id = workspace.EffectiveLayout().slots[i].id;
    workspace.Select(title_id, (int)UiGraphNodeSlotRegion::Header);
    auto* font = workspace.properties_.Find("font_face");
    expect(font && font->enabled && workspace.properties_.FindIndex("font_face") < workspace.properties_.FindIndex("placement"),
           "selected text exposes editable typography before detailed layout");
    workspace.inspector_.SelectProperty("font_height");
    workspace.ApplyProperty("font_height", 20, true);
    Ctrl::ProcessEvents();
    expect(workspace.inspector_.GetSelectedPropertyId() == "font_height"
           && workspace.EffectiveLayout().slots[workspace.EffectiveLayout().FindComponent(title_id)].font_height == DPI(20),
           "component typography commits without losing the edited property");
    for(int i = 0; i < workspace.document_.family.base_layout.slot_count; i++) {
        auto& rule = workspace.document_.family.base_layout.slots[i];
        if(rule.feature == UiGraphNodeSlotFeature::Subtitle && rule.region == UiGraphNodeSlotRegion::Header) {
            rule.font_height = DPI(18); rule.component_style.bold = 1;
        }
    }
    for(int shape : {0, 1}) {
        workspace.document_.shape = shape;
        workspace.preview_.SetZoom(1.0, Point(0, 0));
        workspace.ApplyDocument();
        const auto* title = workspace.snapshot_.FindComponent(title_id);
        expect(workspace.snapshot_.level == UiGraphPresentationLevel::Normal && title
               && title->representation == UiGraphNodeComponentRepresentation::Text
               && title->font.GetCy() <= title->content.GetHeight(),
               "Media title stays readable with an enlarged subtitle and two icons on Rectangle/Ellipse");
    }
    workspace.document_.edit_base = false;
    workspace.RebuildInspector();
    int height = workspace.EffectiveLayout().slots[workspace.EffectiveLayout().FindComponent(title_id)].font_height;
    workspace.ApplyProperty("font_height", 40, true);
    expect(!workspace.properties_.Find("font_face")->enabled
           && workspace.EffectiveLayout().slots[workspace.EffectiveLayout().FindComponent(title_id)].font_height == height,
           "inherited component rejects edits rather than mutating Base implicitly");
    // The workspace must own Delete only while a component surface has focus.
    // Open a real native window for focus tests; no save/file dialogs are used.
    workspace.SetRect(0, 0, DPI(1100), DPI(760));
    workspace.TopWindow::Open();
    Ctrl::ProcessEvents();
    auto dispatch_key = [&](dword key) {
        // Mirror Ctrl's focus-to-parent Key propagation, including PreviewGraph.
        for(Ctrl* c = Ctrl::GetFocusCtrl(); c; c = c->GetParent())
            if(c->Key(key, 1)) return true;
        return false;
    };
    int slots_before = workspace.EffectiveLayout().slot_count;
    workspace.table_.SetFocus();
    dispatch_key(K_DELETE);
    expect(workspace.EffectiveLayout().slot_count == slots_before,
           "Delete on inherited shape does not mutate Base");
    workspace.document_.edit_base = true;
    workspace.Select(icons[0], (int)UiGraphNodeSlotRegion::Header);
    workspace.inspector_.SetFocus();
    workspace.Key(K_DELETE, 1);
    expect(workspace.EffectiveLayout().FindComponent(icons[0]) >= 0,
           "workspace Delete does not remove a component while inspector owns focus");
    auto click_row = [&](const String& id) {
        auto& table = workspace.table_;
        table.horizontal_.Set(0);
        for(int i = 0; i < table.rows_.GetCount(); i++) if(table.rows_[i].id == id) {
            table.scroll_.Set(i * table.RowHeight());
            Point at(DPI(100), DPI(30) + i * table.RowHeight() - table.scroll_.Get() + DPI(10));
            table.LeftDown(at, 0);
            return;
        }
    };
    click_row(icons[0]);
    expect(workspace.table_.HasFocus() && workspace.selection_.id == icons[0],
           "clicking a structure component takes keyboard focus from inspector");
    dispatch_key(K_DELETE);
    expect(workspace.EffectiveLayout().slot_count == slots_before - 1
           && workspace.EffectiveLayout().FindComponent(icons[0]) < 0
           && workspace.EffectiveLayout().FindComponent(icons[1]) >= 0,
           "Delete removes only the selected identified component");
    dispatch_key(K_CTRL_Z);
    expect(workspace.EffectiveLayout().slot_count == slots_before
           && workspace.EffectiveLayout().FindComponent(icons[0]) >= 0,
           "component Delete uses the existing undo transaction");
    workspace.Select(icons[1], (int)UiGraphNodeSlotRegion::Header);
    workspace.preview_.SetFocus();
    dispatch_key(K_DELETE);
    expect(workspace.EffectiveLayout().FindComponent(icons[1]) < 0
           && workspace.model_.FindNode(workspace.node_) != nullptr,
           "preview Delete removes a component and never the specimen node");
    workspace.TopWindow::Close(); // bypass authoring save prompt for the test fixture

    PreviewGraph proxy;
    proxy.snapshot.safe = RectC(0, 0, 100, 60);
    auto& cue = proxy.snapshot.components.Add();
    cue.id = "thin-title"; cue.region = UiGraphNodeSlotRegion::Header;
    cue.slot = RectC(10, 10, 80, 30); cue.footprint = RectC(15, 22, 50, 1);
    cue.representation = UiGraphNodeComponentRepresentation::Bar;
    String proxy_pick;
    proxy.WhenComponentSelect = [&](String id, int) { proxy_pick = id; };
    proxy.LeftDown(Point(25, 24), 0);
    expect(proxy_pick == "thin-title", "thin preview proxy has a small slot-clipped selection tolerance");
    proxy_pick.Clear();
    proxy.LeftDown(Point(5, 24), 0);
    expect(proxy_pick.IsEmpty(), "proxy hit tolerance does not leak outside its component slot");
    // Direct production preparation with measured heights keeps this test
    // independent of Arial availability, monitor DPI and platform font metrics.
    UiGraphNodeSlotRule text_rule;
    text_rule.id = "height-fit"; text_rule.component_kind = UiGraphNodeComponentKind::Text;
    text_rule.feature = UiGraphNodeSlotFeature::Title;
    text_rule.font_height = 24; text_rule.readable_min_px = 9;
    text_rule.small = UiGraphNodeSmallMode::BarThenDot;
    UiGraphNode text_node;
    text_node.title = "A deliberately long asset name to elide";
    UiGraphNodeStyle text_style = UiNodeGraph::StyleDefault().node;
    auto resolved = UiNodeGraphDetail::ResolveNodeComponent(text_rule, text_node, text_style, false);
    Font small_font = resolved.base_font; small_font.Height(12);
    auto prepare_text = [&](int width, int height, double zoom, bool micro = false) {
        UiGraphNodeComponentPresentation out;
        out.slot = RectC(0, 0, width, height);
        auto data = UiNodeGraphDetail::ResolveNodeComponent(text_rule, text_node, text_style, micro);
        UiNodeGraphDetail::PrepareNodeComponent(text_rule, data, zoom, text_node, text_style, out, micro);
        return out;
    };
    auto fitted = prepare_text(80, small_font.GetCy(), 1.0);
    expect(fitted.representation == UiGraphNodeComponentRepresentation::Text
           && !fitted.text.IsEmpty() && fitted.font.GetHeight() >= text_rule.readable_min_px
           && fitted.font.GetHeight() < resolved.base_font.GetHeight()
           && fitted.font.GetCy() <= fitted.content.GetHeight()
           && (fitted.footprint & fitted.content) == fitted.footprint,
           "short text slot shrinks to a readable line before becoming a bar");
    expect(fitted.text != resolved.text && fitted.text.ToString().EndsWith("...")
           && GetTextSize(fitted.text, fitted.font).cx <= fitted.content.GetWidth(),
           "width overflow remains ellipsis at the fitted readable font");
    auto full = prepare_text(1000, resolved.base_font.GetCy(), 1.0);
    expect(full.representation == UiGraphNodeComponentRepresentation::Text
           && full.font == resolved.base_font && full.text == resolved.text,
           "a text slot with capacity keeps its exact authored font and content");
    Font minimum_font = resolved.base_font; minimum_font.Height(text_rule.readable_min_px);
    auto cramped = prepare_text(100, max(1, minimum_font.GetCy() - 2), 1.0);
    expect(cramped.representation != UiGraphNodeComponentRepresentation::Text
           && cramped.reason == UiGraphNodeComponentReason::NoSpace,
           "no sub-readable font is forced into an insufficient text band");
    auto distant = prepare_text(100, 40, 0.25);
    expect(distant.representation != UiGraphNodeComponentRepresentation::Text
           && distant.reason == UiGraphNodeComponentReason::TooSmall,
           "height fitting does not enlarge a zoomed-out font past its readability policy");
    auto micro_text = prepare_text(12, 8, 0.25, true);
    expect(micro_text.representation != UiGraphNodeComponentRepresentation::Text && micro_text.micro,
           "native Micro remains a proxy path without the text fitting search");
    text_rule.overflow = UiGraphNodeOverflow::Clip;
    auto clipped = prepare_text(80, small_font.GetCy(), 1.0);
    expect(clipped.representation != UiGraphNodeComponentRepresentation::Text,
           "Clip overflow retains authored font sizing instead of automatic height fitting");
    expect(ComponentOutcome(&cramped).Find("no room") >= 0
           && ComponentOutcome(&distant).Find("readable") >= 0
           && ComponentOutcome(&full).Find("Text") >= 0,
           "table outcome explains capacity, pixel simplification and readable text separately from LOD mask");
    LOG("UIGRAPH_WORKSPACE_VIEW_SUMMARY checks=" << checks << " failed=" << failed);
    return failed == 0;
}
} // namespace GraphWorkspace
} // namespace Upp
