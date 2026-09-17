#include <Ui/Ui.h>

using namespace Upp;

namespace {
using Feature = UiGraphNodeSlotFeature;
using Region = UiGraphNodeSlotRegion;
using Place = UiGraphNodeSlotPlacement;
using Override = UiGraphNodeLodOverride;

const char* regions[] = { "Header", "Content.Left", "Content.Main", "Content.Right",
                         "Overlay.Left", "Overlay.Main", "Overlay.Right", "Footer" };
const char* placements[] = { "Fill", "Top", "Bottom", "Left", "Right", "Center" };
const char* representations[] = { "Hidden", "Text", "Icon", "Bar", "Dot" };
const char* reasons[] = { "", "LOD policy", "missing data", "wrong data type", "no space", "too small" };
const UiAlign horizontal[] = { UiAlign::LEFT, UiAlign::CENTER, UiAlign::RIGHT };

class ComponentTable : public Ctrl {
    UiGraphNodeTemplate* spec_ = nullptr;
    String selected_;
    int StartColumns() const { return max(DPI(180), GetSize().cx - DPI(244)); }
public:
    Event<String> WhenSelect;
    Event<String, int> WhenToggle;
    void Bind(UiGraphNodeTemplate& spec) { spec_ = &spec; Refresh(); }
    void Select(const String& id) { selected_ = id; Refresh(); }
    void Paint(Draw& w) override
    {
        w.DrawRect(GetSize(), SColorPaper());
        if(!spec_) return;
        const int line = DPI(20), columns = StartColumns();
        Font font = StdFont().Height(DPI(12));
        w.DrawText(DPI(8), DPI(5), "Structure / component     (click to select)", font.Bold(), SColorText());
        for(int level = 0; level < 4; level++)
            w.DrawText(columns + level * DPI(60), DPI(5), level ? "LOD " + AsString(level) : String("Normal"),
                       font.Bold(), SColorText());
        int y = DPI(28);
        for(int region = 0; region < 8; region++) {
            w.DrawText(DPI(8), y + DPI(2), regions[region], font.Bold(), SColorText());
            y += line;
            for(int i = 0; i < spec_->slot_count; i++) {
                const auto& r = spec_->slots[i];
                if(!r.IsComponent() || (int)r.region != region) continue;
                if(r.id == selected_) w.DrawRect(0, y, GetSize().cx, line, Color(221, 236, 249));
                String label = "    " + r.id + "  / " + placements[(int)r.placement];
                w.Clip(0, y, columns - DPI(6), line);
                w.DrawText(DPI(8), y + DPI(2), label, font, SColorText());
                w.End();
                for(int level = 0; level < 4; level++) {
                    byte bit = byte(1u << level);
                    String text = (r.force_off & bit) ? "Off" : (r.force_on & bit) ? "On"
                                : r.Allows((UiGraphPresentationLevel)level) ? "Auto +" : "Auto -";
                    Color ink = r.Allows((UiGraphPresentationLevel)level) ? Color(24, 126, 91) : Color(174, 69, 55);
                    w.DrawText(columns + level * DPI(60), y + DPI(2), text, font, ink);
                }
                y += line;
            }
        }
    }
    void LeftDown(Point p, dword) override
    {
        if(!spec_) return;
        int y = DPI(28);
        for(int region = 0; region < 8; region++) {
            y += DPI(20);
            for(int i = 0; i < spec_->slot_count; i++) {
                const auto& r = spec_->slots[i];
                if(!r.IsComponent() || (int)r.region != region) continue;
                if(p.y >= y && p.y < y + DPI(20)) {
                    String id = r.id; // events may edit the shared template
                    WhenSelect(id);
                    int level = (p.x - StartColumns()) / DPI(60);
                    if(p.x >= StartColumns() && level >= 0 && level < 4) WhenToggle(id, level);
                    return;
                }
                y += DPI(20);
            }
        }
    }
};

class Preview : public Ctrl {
    UiBoxLayout column_ { UiDirection::V };
    UiLabel heading_, detail_;
public:
    UiNodeGraph graph;
    Preview()
    {
        Add(column_.SizePos());
        column_.SetGap(DPI(4)).SetInset(DPI(4)).SetAlignItems(UiCrossAlign::Stretch);
        column_.Add(heading_).Fixed(DPI(22));
        column_.Add(graph).Expand(1);
        column_.Add(detail_).Fixed(DPI(48));
        graph.SetAutoFitOnFirstPaint(false).SetEditable(false);
        auto style = graph.GetStyle();
        style.min_zoom = 0.02;
        style.node.title_font = StdFont().Height(DPI(18)).Bold();
        style.node.subtitle_font = StdFont().Height(DPI(14));
        style.node.description_font = StdFont().Height(DPI(14));
        style.node.metrics.shadow.enabled = false;
        style.show_grid = false;
        graph.SetCustomStyle(style);
    }
    void Heading(const String& s) { heading_.SetText(s); }
    void Report(UiGraphNodeRef ref, const String& selected)
    {
        UiGraphNodePresentation p;
        if(!graph.GetNodePresentation(ref, p)) { detail_.SetText("Waiting for layout"); return; }
        String text = Format("Actual LOD %d  |  zoom %.2f", (int)p.level, graph.GetZoom());
        if(!p.template_error.IsEmpty()) text << "\n" << p.template_error;
        else if(p.safe.IsEmpty()) text << "\nNo rich capacity / Micro hints pending";
        else if(auto c = p.FindComponent(selected)) {
            text << "\n" << selected << ": " << representations[(int)c->representation];
            if(c->reason != UiGraphNodeComponentReason::None) text << " / " << reasons[(int)c->reason];
        }
        else text << "\nNo selected component";
        detail_.SetText(text);
    }
};

class ComponentStudio : public TopWindow {
    // Shared definitions and model outlive all preview controls.
    UiGraphNodeTemplate spec_;
    UiGraphModel model_;
    UiGraphNodeRef node_;
    String selected_ = "name";
    bool syncing_ = true, started_ = false, expanded_ = false, ready_ = false;

    UiBoxLayout root_ { UiDirection::V }, toolbar_ { UiDirection::H };
    UiBoxLayout previews_ { UiDirection::H }, lower_ { UiDirection::H }, inspector_ { UiDirection::V };
    UiBoxLayout rows_[7], order_actions_ { UiDirection::H };
    UiLabel labels_[7], title_, note_, selected_label_, message_;
    UiDropdown shape_, region_, placement_, alignment_, small_, flow_, ink_, font_;
    UiLineEdit sample_;
    UiButton expand_, reset_, earlier_, later_;
    ComponentTable table_;
    Preview views_[4];

    void Reports()
    {
        for(auto& v : views_) v.Report(node_, selected_);
        table_.Refresh();
    }
    void ApplyTemplate(const UiGraphNodeTemplate& candidate)
    {
        String error;
        if(!candidate.Validate(error)) { message_.SetText(error); SyncInspector(); return; }
        spec_ = candidate;
        for(auto& v : views_) v.graph.InvalidateNodePresentation();
        message_.SetText("Template updated. Cameras unchanged. LOD cells cycle Inherit / On / Off.");
        SyncInspector(); Reports();
    }
    void Edit(int property, int index)
    {
        if(syncing_ || index < 0) return;
        auto candidate = spec_;
        int i = candidate.FindComponent(selected_);
        if(i < 0) return;
        auto& r = candidate.slots[i];
        switch(property) {
        case 0: r.region = (Region)index; break;
        case 1: r.placement = (Place)index; r.extent = 0; break;
        case 2: r.align_h = horizontal[minmax(index, 0, 2)]; break;
        case 3: r.small = (UiGraphNodeSmallMode)index; break;
        case 4: r.flow = (UiGraphNodeSlotFlow)index; break;
        case 5: r.ink = index == 1 ? Color(204, 50, 45) : index == 2 ? Color(26, 110, 190)
                       : index == 3 ? Color(28, 143, 91) : Color(Null); break;
        case 6: r.font_height = index ? DPI(10 + index * 4) : 0; break;
        }
        ApplyTemplate(candidate);
    }
    void SyncInspector()
    {
        int i = spec_.FindComponent(selected_);
        if(i < 0) return;
        syncing_ = true;
        const auto& r = spec_.slots[i];
        selected_label_.SetText(selected_ + "  |  " + (r.data_key.IsEmpty() ? "node field" : "data." + r.data_key));
        region_.Select((int)r.region); placement_.Select((int)r.placement);
        int align = r.align_h == UiAlign::RIGHT ? 2 : r.align_h == UiAlign::CENTER ? 1 : 0;
        alignment_.Select(align); small_.Select((int)r.small); flow_.Select((int)r.flow);
        int ink = IsNull(r.ink) ? 0 : r.ink == Color(204, 50, 45) ? 1 : r.ink == Color(26, 110, 190) ? 2 : 3;
        ink_.Select(ink);
        int height = 0;
        for(int f = 1; f <= 4; f++) if(r.font_height == DPI(10 + f * 4)) height = f;
        font_.Select(height); font_.Enable(r.IsText());
        syncing_ = false;
    }
    void ResetCameras()
    {
        const double zooms[] = { 0.95, 0.40, 0.22, 0.075 };
        for(int i = 0; i < 4; i++) {
            views_[i].graph.SetZoom(zooms[i], Point(0, 0));
            views_[i].graph.CenterOnNode(node_);
        }
        Reports();
    }
    void BuildSpecimen()
    {
        spec_.SetKind(UiGraphNodeTemplateKind::Summary).SetHeaderHeight(DPI(42))
             .SetFooterHeight(DPI(34)).SetOverlayColumns(0, DPI(65));
        auto add = [&](const char* id, Feature feature, Region region, Place place,
                       int extent, const char* binding = "") {
            UiGraphNodeSlotRule r;
            r.id = id; r.feature = feature; r.region = region; r.placement = place; r.extent = extent;
            r.data_key = binding;
            r.small = feature == Feature::Icon ? UiGraphNodeSmallMode::Dot : UiGraphNodeSmallMode::BarThenDot;
            if(place == Place::Right) r.align_h = UiAlign::RIGHT;
            String error;
            if(!spec_.AddComponent(r, error)) Panic(~error);
        };
        add("icon", Feature::Icon, Region::Header, Place::Left, DPI(30));
        add("name", Feature::Title, Region::Header, Place::Fill, 0);
        add("description", Feature::Description, Region::ContentMain, Place::Fill, 0);
        add("state", Feature::Subtitle, Region::OverlayRight, Place::Top, DPI(22), "state");
        add("percentage", Feature::Subtitle, Region::Footer, Place::Right, DPI(58), "percentage");
        add("format", Feature::Subtitle, Region::Footer, Place::Fill, 0, "format");
        spec_.slots[spec_.FindComponent("description")].Align(UiAlign::CENTER).Lod(true, true, false, false);
        spec_.slots[spec_.FindComponent("format")].Lod(true, true, false, false);

        UiGraphNode n;
        n.title = "Convert EXR";
        n.description = "Image processing";
        n.icon = ICON_DESIGN_WIDGETS_48();
        n.size = Sizef(DPI(400), DPI(260));
        ValueMap data;
        data.Add("state", "Ready"); data.Add("format", "EXR / JPEG"); data.Add("percentage", "87%");
        n.data = data;
        UiGraphPort in; in.id = "in"; in.direction = UiGraphPortDirection::Input;
        UiGraphPort out; out.id = "out"; out.direction = UiGraphPortDirection::Output;
        n.ports.Add(in); n.ports.Add(out);
        node_ = model_.AddNode(n);
    }
public:
    ComponentStudio()
    {
        Title("UiGraph / Component Studio — first production slice");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(1360), DPI(880));
        BuildSpecimen();
        Add(root_);
        root_.SetInset(DPI(8)).SetGap(DPI(6)).SetAlignItems(UiCrossAlign::Stretch);
        root_.Add(toolbar_).Fixed(DPI(38));
        root_.Add(note_).Fixed(DPI(24));
        root_.Add(previews_).Expand(1);
        root_.Add(lower_).Fixed(DPI(326));
        root_.Add(message_).Fixed(DPI(26));
        toolbar_.SetGap(DPI(6));
        title_.SetText("Node components"); toolbar_.Add(title_).Fixed(DPI(150));
        for(const char* shape : { "Rectangle", "Ellipse", "Diamond", "Triangle", "Hexagon", "Cloud", "Document", "Database" })
            shape_.Add(shape);
        shape_.Select(0); toolbar_.Add(shape_).Fixed(DPI(136));
        sample_.SetTextUtf8("Convert EXR"); toolbar_.Add(sample_).Expand(1);
        expand_.SetText("Expand node"); toolbar_.Add(expand_).Fixed(DPI(120));
        reset_.SetText("Reset cameras"); toolbar_.Add(reset_).Fixed(DPI(130));
        note_.SetText("Same model and production template in four independent cameras. Wheel to zoom; middle-drag to pan.");
        previews_.SetGap(DPI(8)).SetAlignItems(UiCrossAlign::Stretch);
        const char* headings[] = { "Normal reference", "LOD 1 reference", "LOD 2 reference", "LOD 3 reference" };
        for(int i = 0; i < 4; i++) {
            auto& v = views_[i];
            v.Heading(headings[i]);
            v.graph.WhenResolveNodePresentation = [this](const UiGraphNode&, const UiGraphNodeStyle&,
                                                        UiGraphPresentationRequest& r) { r.node_template = &spec_; };
            v.graph.SetModel(model_);
            v.graph.WhenViewport = [this] { Reports(); };
            previews_.Add(v).Expand(i == 0 ? 2 : 1);
        }
        lower_.SetGap(DPI(12)).SetAlignItems(UiCrossAlign::Stretch);
        table_.Bind(spec_); table_.Select(selected_);
        lower_.Add(table_).Expand(2); lower_.Add(inspector_).Expand(1);
        inspector_.SetGap(DPI(4)).SetAlignItems(UiCrossAlign::Stretch); inspector_.Add(selected_label_).Fixed(DPI(26));
        UiDropdown* fields[] = { &region_, &placement_, &alignment_, &small_, &flow_, &ink_, &font_ };
        const char* names[] = { "Region", "Placement", "Align", "Small form", "Hidden flow", "Ink", "Font height" };
        for(int i = 0; i < 7; i++) {
            // Explicit row layout; no coordinates in the window's resize handler.
            labels_[i].SetText(names[i]);
            rows_[i].SetDirection(UiDirection::H).SetGap(DPI(6));
            rows_[i].Add(labels_[i]).Fixed(DPI(92)); rows_[i].Add(*fields[i]).Expand(1);
            inspector_.Add(rows_[i]).Fixed(DPI(32));
            fields[i]->WhenSelect = [this, i](int index) { Edit(i, index); };
        }
        for(auto name : regions) region_.Add(name);
        for(auto name : placements) placement_.Add(name);
        alignment_.Add("Left").Add("Center").Add("Right");
        small_.Add("Hide when unreadable").Add("Bar").Add("Bar then dot").Add("Dot");
        flow_.Add("Stable").Add("Reflow");
        ink_.Add("Inherit role ink").Add("Red").Add("Blue").Add("Green");
        font_.Add("Inherit").Add("14").Add("18").Add("22").Add("26");
        earlier_.SetText("Earlier in order"); later_.SetText("Later in order");
        order_actions_.SetGap(DPI(4));
        order_actions_.Add(earlier_).Expand(1); order_actions_.Add(later_).Expand(1);
        inspector_.Add(order_actions_).Fixed(DPI(28));
        earlier_.WhenAction = [this] {
            auto candidate = spec_; int i = candidate.FindComponent(selected_);
            if(i > 0) { Swap(candidate.slots[i], candidate.slots[i - 1]); ApplyTemplate(candidate); }
        };
        later_.WhenAction = [this] {
            auto candidate = spec_; int i = candidate.FindComponent(selected_);
            if(i >= 0 && i + 1 < candidate.slot_count) {
                Swap(candidate.slots[i], candidate.slots[i + 1]); ApplyTemplate(candidate);
            }
        };
        table_.WhenSelect = [this](String id) { selected_ = id; table_.Select(id); SyncInspector(); Reports(); };
        table_.WhenToggle = [this](String id, int level) {
            auto candidate = spec_;
            int i = candidate.FindComponent(id);
            if(i < 0) return;
            auto& r = candidate.slots[i]; byte bit = byte(1u << level);
            Override next = (r.force_off & bit) ? Override::Inherit : (r.force_on & bit) ? Override::Off : Override::On;
            r.Override((UiGraphPresentationLevel)level, next);
            ApplyTemplate(candidate);
        };
        sample_.WhenChange = [this] {
            const auto* old = model_.FindNode(node_);
            if(!old) return;
            UiGraphNode n = clone(*old); n.title = sample_.GetTextUtf8(); model_.UpdateNode(node_, n); Reports();
        };
        shape_.WhenSelect = [this](int i) {
            const UiGraphNodeShape shapes[] = { UiGraphNodeShape::Rectangle, UiGraphNodeShape::Ellipse,
                UiGraphNodeShape::Diamond, UiGraphNodeShape::Triangle, UiGraphNodeShape::Hexagon,
                UiGraphNodeShape::Cloud, UiGraphNodeShape::Document, UiGraphNodeShape::Database };
            if(i < 0 || i >= 8) return;
            const auto* old = model_.FindNode(node_);
            if(!old) return;
            UiGraphNode n = clone(*old); n.shape = shapes[i]; model_.UpdateNode(node_, n); Reports();
        };
        expand_.WhenAction = [this] {
            expanded_ = !expanded_;
            model_.SetNodeSize(node_, expanded_ ? Sizef(DPI(600), DPI(400)) : Sizef(DPI(400), DPI(260)));
            expand_.SetText(expanded_ ? "Compact node" : "Expand node"); Reports();
        };
        reset_.WhenAction = [this] { ResetCameras(); };
        message_.SetText("First slice: text/icon editing. Native Micro hints, drag/drop and save/export are not implemented yet.");
        SyncInspector();
        ready_ = true;
        Layout();
    }
    void Layout() override
    {
        root_.SetRect(0, 0, GetSize().cx, GetSize().cy);
        if(ready_ && !started_) { started_ = true; ResetCameras(); }
    }
};
}

GUI_APP_MAIN
{
    ComponentStudio().Run();
}
