#include "WorkspaceWindow.h"

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
    workspace.document_.edit_base = false;
    workspace.RebuildInspector();
    int height = workspace.EffectiveLayout().slots[workspace.EffectiveLayout().FindComponent(title_id)].font_height;
    workspace.ApplyProperty("font_height", 40, true);
    expect(!workspace.properties_.Find("font_face")->enabled
           && workspace.EffectiveLayout().slots[workspace.EffectiveLayout().FindComponent(title_id)].font_height == height,
           "inherited component rejects edits rather than mutating Base implicitly");
    LOG("UIGRAPH_WORKSPACE_VIEW_SUMMARY checks=" << checks << " failed=" << failed);
    return failed == 0;
}
} // namespace GraphWorkspace
} // namespace Upp
