#include "WorkspaceWindow.h"

namespace Upp {
namespace GraphWorkspace {
namespace {
int Logical(int n) { return n < 0 ? n : fround(n * 1024.0 / DPI(1024)); }
int Metric(const Value& v) { int n = (int)v; return n < 0 ? n : DPI(n); }
const char* states[] = { "Normal", "Hot", "Selected", "Disabled" };
}
void NodeWorkspace::AddSetter(const String& id, Function<void(Document&, const Value&)> setter)
{
    setters_.Add(id, pick(setter));
}
void NodeWorkspace::RebuildInspector()
{
    if(!ready_) return;
    building_ = true; inspector_.SetModel(nullptr); properties_.Clear(false); setters_.Clear();
    for(int i = 0; i < 4; i++) mode_[i].SetChecked(page_ == i);
    inspector_.Show(page_ != 3); code_.Show(page_ == 3); code_tools_.Show(page_ == 3);
    if(page_ == 3) {
        String error; code_.SetTextUtf8(GenerateCpp(document_, error));
        selection_label_.SetText("Generated C++ / separate layout + style factories");
        if(!error.IsEmpty()) status_.SetText(error);
        building_ = false; rail_.Layout(); return;
    }
    auto text = [&](const char* id, const char* label, const String& value, const char* group, Function<void(Document&, const Value&)> setter) {
        properties_.AddText(id, label, value, group); AddSetter(id, pick(setter));
    };
    auto integer = [&](const char* id, const char* label, int value, int low, int high, const char* group, Function<void(Document&, const Value&)> setter) {
        properties_.AddNumericInt(id, label, value, low, high, 1, group); AddSetter(id, pick(setter));
    };
    auto boolean = [&](const char* id, const char* label, bool value, const char* group, Function<void(Document&, const Value&)> setter) {
        properties_.AddBoolean(id, label, value, group); AddSetter(id, pick(setter));
    };
    auto choice = [&](const char* id, const char* label, int value, const char* names, const char* group, Function<void(Document&, const Value&)> setter) {
        auto& p = properties_.AddChoice(id, label, value, group);
        Vector<String> options = Split(names, '|'); for(int i = 0; i < options.GetCount(); i++) p.AddChoice(i, options[i]);
        AddSetter(id, pick(setter));
    };
    auto colour = [&](const String& id, const String& label, Color value, Color inherited, const String& group, Function<void(Document&, const Value&)> setter) {
        auto& p = properties_.AddColor(id, label, IsNull(value) ? inherited : value, group);
        p.overrideable = true; p.override_active = !IsNull(value); p.inherited = IsNull(value);
        p.default_value = inherited; AddSetter(id, pick(setter));
    };
    if(page_ == 2) {
        const auto& s = document_.family.Style(Scope());
        UiGraphNodeStyle theme = UiNodeGraph::StyleForRole(preview_.GetStyle().node, (UiGraphNodeRole)s.role);
        selection_label_.SetText(document_.family.name + " / " + (document_.edit_base ? String("Base style") : String(shape_names[document_.shape]) + " style"));
        bool editable = EditableStyle(document_) != nullptr;
        choice("role", "Role", s.role, "Standard|Subtle|Accent|Alert", "Node role", [this](Document& d, const Value& v) { EditableStyle(d)->role = (int)v; });
        integer("radius", "Corner radius (-1 inherit)", s.radius, -1, 512, "Silhouette", [this](Document& d, const Value& v) { EditableStyle(d)->radius = (int)v; });
        integer("frame_width", "Frame width (-1 inherit)", s.frame_width, -1, 32, "Frame", [this](Document& d, const Value& v) { EditableStyle(d)->frame_width = (int)v; });
        integer("header_band", "Header band (-1 inherit)", s.header_band, -1, 1, "Header", [this](Document& d, const Value& v) { EditableStyle(d)->header_band = (int)v; });
        const char* keys[] = { "face", "frame", "ink", "header" };
        for(int field = 0; field < 4; field++) for(int state = 0; state < 4; state++) {
            const Color* colors = field == 0 ? s.face : field == 1 ? s.frame : field == 2 ? s.ink : s.header;
            String id = String(keys[field]) + AsString(state);
            Color inherited = field == 0 ? (theme.palette.face[state].IsSolid() ? theme.palette.face[state].color : SColorPaper())
                            : field == 1 ? theme.palette.frame[state] : field == 2 ? theme.title_ink[state] : theme.header_face[state];
            colour(id, states[state], colors[state], inherited, keys[field], [this, field, state](Document& d, const Value& v) {
                auto* a = EditableStyle(d); Color* colors = field == 0 ? a->face : field == 1 ? a->frame : field == 2 ? a->ink : a->header;
                colors[state] = IsNull(v) ? Color(Null) : (Color)v;
            });
        }
        integer("shadow", "Enabled (-1 inherit)", s.shadow, -1, 1, "Shadow", [this](Document& d, const Value& v) { EditableStyle(d)->shadow = (int)v; });
        integer("shadow_y", "Offset Y (-1 inherit)", s.shadow_y, -1, 128, "Shadow", [this](Document& d, const Value& v) { EditableStyle(d)->shadow_y = (int)v; });
        integer("shadow_alpha", "Alpha (-1 inherit)", s.shadow_alpha, -1, 255, "Shadow", [this](Document& d, const Value& v) { EditableStyle(d)->shadow_alpha = (int)v; });
        colour("shadow_color", "Colour", s.shadow_color, Color(32, 48, 64), "Shadow", [this](Document& d, const Value& v) { EditableStyle(d)->shadow_color = IsNull(v) ? Color(Null) : (Color)v; });
        for(int i = 0; i < properties_.GetCount(); i++) properties_[i].enabled = editable;
    }
    else if(page_ == 1 || selection_.id.IsEmpty()) {
        const auto& t = EffectiveLayout();
        selection_label_.SetText(document_.family.name + " / " + (page_ == 1 ? String("Template / Layout") : String(region_names[selection_.region])));
        text("family_name", "Family name", document_.family.name, "Family", [](Document& d, const Value& v) { d.family.name = (String)v; });
        choice("body_mode", "Body mode", (int)t.body_mode, "Stack|Centered|Media|Key value|Fields|Port rows|Flow tags", "Layout", [this](Document& d, const Value& v) { EditableLayout(d)->body_mode = (UiGraphNodeBodyMode)(int)v; });
        integer("header", "Header height", Logical(t.header_height), -1, 1024, "Regions", [this](Document& d, const Value& v) { EditableLayout(d)->header_height = Metric(v); });
        integer("footer", "Footer height", Logical(t.footer_height), 0, 1024, "Regions", [this](Document& d, const Value& v) { EditableLayout(d)->footer_height = Metric(v); });
        const int dimensions[] = {t.content_left_width, t.content_right_width, t.overlay_left_width, t.overlay_right_width};
        const char* names[] = { "Content left", "Content right", "Overlay left", "Overlay right" };
        for(int i = 0; i < 4; i++) {
            String id = "column" + AsString(i);
            properties_.AddNumericInt(id, names[i], Logical(dimensions[i]), 0, 1024, 1, "Regions");
            AddSetter(id, [this, i](Document& d, const Value& v) { auto* t = EditableLayout(d); int* n[] = {&t->content_left_width, &t->content_right_width, &t->overlay_left_width, &t->overlay_right_width}; *n[i] = Metric(v); });
        }
        boolean("width_policy", "Use projected-width LOD", t.lod_widths.enabled, "LOD", [this](Document& d, const Value& v) { EditableLayout(d)->lod_widths.enabled = (bool)v; });
        boolean("left_ports", "Left labels in Body", t.left_port_lane_body_only, "Port reservations", [this](Document& d, const Value& v) { EditableLayout(d)->left_port_lane_body_only = (bool)v; });
        boolean("right_ports", "Right labels in Body", t.right_port_lane_body_only, "Port reservations", [this](Document& d, const Value& v) { EditableLayout(d)->right_port_lane_body_only = (bool)v; });
        boolean("micro", "Native Micro hints", t.micro_hints, "LOD", [this](Document& d, const Value& v) { EditableLayout(d)->micro_hints = (bool)v; });
        integer("budget", "Micro operation budget", t.micro_hint_budget, 0, 16, "LOD", [this](Document& d, const Value& v) { EditableLayout(d)->micro_hint_budget = (int)v; });
        for(int i = 0; i < properties_.GetCount(); i++) if(properties_[i].id != "family_name") properties_[i].enabled = EditableLayout(document_) != nullptr;
        text("preview_title", "Title", document_.title, "Preview data (not exported)", [](Document& d, const Value& v) { d.title = (String)v; });
        text("preview_subtitle", "Subtitle", document_.subtitle, "Preview data (not exported)", [](Document& d, const Value& v) { d.subtitle = (String)v; });
        text("preview_description", "Description", document_.description, "Preview data (not exported)", [](Document& d, const Value& v) { d.description = (String)v; });
        properties_.AddMultiline("preview_data", "Data object (JSON)", AsJSON(document_.data), "Preview data (not exported)").SetExpandedRowSpan(4);
        AddSetter("preview_data", [](Document& d, const Value& v) { Value data = ParseJSON((String)v); if(!data.Is<ValueMap>()) throw Exc("Expected JSON object"); d.data = data; });
        int show_labels = document_.data.Find("show_port_labels");
        boolean("preview_labels", "Show port labels", show_labels >= 0 && document_.data.GetValue(show_labels).Is<bool>() && (bool)document_.data.GetValue(show_labels), "Preview data (not exported)", [](Document& d, const Value& v) { d.data.Set("show_port_labels", v); });
        integer("width", "Authored width", document_.size.cx, 32, 2048, "Preview size", [](Document& d, const Value& v) { d.size.cx = (int)v; });
        integer("height", "Authored height", document_.size.cy, 24, 2048, "Preview size", [](Document& d, const Value& v) { d.size.cy = (int)v; });
    }
    else {
        String component_id = selection_.id;
        int n = EffectiveLayout().FindComponent(component_id);
        if(n < 0) { building_ = false; selection_.id.Clear(); RebuildInspector(); return; }
        const auto& r = EffectiveLayout().slots[n];
        const auto* effective = snapshot_.FindComponent(component_id);
        auto mutate = [this, component_id](Function<void(UiGraphNodeSlotRule&, const Value&)> action) -> Function<void(Document&, const Value&)> {
            return [this, component_id, action](Document& d, const Value& v) {
                auto* t = EditableLayout(d); if(!t) throw Exc("Inherited layout is read-only");
                int i = t->FindComponent(component_id); if(i < 0) throw Exc("Stale component selection"); action(t->slots[i], v);
            };
        };
        selection_label_.SetText((r.label.IsEmpty() ? r.id : r.label) + " / " + region_names[(int)r.region]);
        if(!EditableLayout(document_)) {
            selection_label_.SetText((r.label.IsEmpty() ? r.id : r.label) + " / inherited - edit Base or create layout override");
            selection_label_.Tip("Typography and component settings belong to the layout section. Select BASE on the left to edit the family, or Create layout override for this shape.");
        }
        else selection_label_.Tip("Component typography, colour and layout are independently editable here.");
        properties_.AddReadOnly("identity", "Stable ID", r.id, "Component");
        text("label", "Label", r.label, "Component", mutate([](UiGraphNodeSlotRule& r, const Value& v) { r.label = (String)v; }));
        properties_.AddReadOnly("kind", "Renderer", kind_names[(int)r.GetKind()], "Component");
        choice("component_role", "Role", (int)r.component_style.role, "Inherit|Standard|Subtle|Accent|Alert", "Typography", mutate([](UiGraphNodeSlotRule& r, const Value& v) { r.component_style.role = (UiGraphNodeComponentRole)(int)v; }));
        AddPropertyFont(properties_, "font_face", "Font face", r.component_style.font_face, "Typography");
        AddSetter("font_face", mutate([](UiGraphNodeSlotRule& r, const Value& v) { r.component_style.font_face = (String)v; }));
        integer("font_height", "Font height (0 = inherited)", Logical(r.font_height), 0, 256, "Typography", mutate([](UiGraphNodeSlotRule& r, const Value& v) { r.font_height = Metric(v); }));
        const int flags[] = { r.component_style.bold, r.component_style.italic, r.component_style.underline };
        const char* names[] = { "Bold", "Italic", "Underline" };
        for(int i = 0; i < 3; i++) {
            String id = "fontflag" + AsString(i); auto& p = properties_.AddChoice(id, names[i], flags[i], "Typography");
            p.AddChoice(-1, "Inherit").AddChoice(0, "Off").AddChoice(1, "On");
            AddSetter(id, mutate([i](UiGraphNodeSlotRule& r, const Value& v) { int* flags[] = {&r.component_style.bold, &r.component_style.italic, &r.component_style.underline}; *flags[i] = (int)v; }));
        }
        boolean("literal", "Use static content", r.use_literal, "Source", mutate([](UiGraphNodeSlotRule& r, const Value& v) { r.use_literal = (bool)v; }));
        text("binding", "Data key (empty = node field)", r.data_key, "Source", mutate([](UiGraphNodeSlotRule& r, const Value& v) { r.data_key = (String)v; }));
        choice("feature", "Node field / text role", (int)r.feature, "Title|Subtitle|Icon|Badge|Media|Description|Control|Footer", "Source", mutate([](UiGraphNodeSlotRule& r, const Value& v) { r.feature = (UiGraphNodeSlotFeature)(int)v; }));
        if(r.IsText()) text("text", "Static text", r.literal.Is<String>() ? String(r.literal) : String(), "Source", mutate([](UiGraphNodeSlotRule& r, const Value& v) { r.literal = v; r.use_literal = true; }));
        else if(r.GetKind() == UiGraphNodeComponentKind::Icon || r.GetKind() == UiGraphNodeComponentKind::Image) {
            AddPropertyImage(properties_, "asset", "Static image", r.asset, "workspace-image", "Source").SetExpandedRowSpan(3);
            AddSetter("asset", mutate([](UiGraphNodeSlotRule& r, const Value& v) { if(!v.Is<Image>()) throw Exc("Expected Image"); r.asset = (Image)v; r.use_literal = true; }));
            AddPropertyIcon(properties_, "icon", "Icon catalogue", String(), "Source");
            AddSetter("icon", mutate([](UiGraphNodeSlotRule& r, const Value& v) {
                for(const auto& entry : UiIconCatalog()) if(entry.name == String(v) && entry.factory) { r.asset = entry.factory(); r.use_literal = true; return; }
                throw Exc("Icon is unavailable");
            }));
            choice("icon_mode", "Icon colour", (int)r.icon_mode, "Auto|Mono tint|Preserve colour", "Source", mutate([](UiGraphNodeSlotRule& r, const Value& v) { r.icon_mode = (UiIconRenderMode)(int)v; }));
            choice("image_fit", "Fit", (int)r.image_fit, "Contain|Cover", "Source", mutate([](UiGraphNodeSlotRule& r, const Value& v) { r.image_fit = (UiGraphNodeImageFit)(int)v; }));
            text("overview_key", "Tiny-ready image data key", r.overview_data_key, "Source", mutate([](UiGraphNodeSlotRule& r, const Value& v) { r.overview_data_key = (String)v; }));
        }
        else {
            properties_.AddMultiline("value", "Static value (JSON)", AsJSON(r.literal), "Source").SetExpandedRowSpan(3);
            AddSetter("value", mutate([](UiGraphNodeSlotRule& r, const Value& v) { r.literal = ParseJSON((String)v); r.use_literal = true; }));
        }
        choice("region", "Region", (int)r.region, "Header|Content Left|Content Main|Content Right|Overlay Left|Overlay Main|Overlay Right|Footer", "Layout", mutate([](UiGraphNodeSlotRule& r, const Value& v) { r.region = (UiGraphNodeSlotRegion)(int)v; }));
        choice("placement", "Placement", (int)r.placement, "Fill|Top|Bottom|Left|Right|Center", "Layout", mutate([](UiGraphNodeSlotRule& r, const Value& v) { r.placement = (UiGraphNodeSlotPlacement)(int)v; }));
        choice("align", "Horizontal", r.align_h == UiAlign::LEFT ? 0 : r.align_h == UiAlign::CENTER ? 1 : 2, "Left|Center|Right", "Layout", mutate([](UiGraphNodeSlotRule& r, const Value& v) { const UiAlign a[] = {UiAlign::LEFT, UiAlign::CENTER, UiAlign::RIGHT}; r.align_h = a[(int)v]; }));
        choice("vertical", "Vertical", r.align_v == UiAlign::TOP ? 0 : r.align_v == UiAlign::CENTER ? 1 : 2, "Top|Center|Bottom", "Layout", mutate([](UiGraphNodeSlotRule& r, const Value& v) { const UiAlign a[] = {UiAlign::TOP, UiAlign::CENTER, UiAlign::BOTTOM}; r.align_v = a[(int)v]; }));
        integer("extent", "Extent (0 = natural)", Logical(r.extent), 0, 2048, "Layout", mutate([](UiGraphNodeSlotRule& r, const Value& v) { r.extent = Metric(v); }));
        integer("gap", "Gap (-1 = default)", Logical(r.gap_after), -1, 512, "Layout", mutate([](UiGraphNodeSlotRule& r, const Value& v) { r.gap_after = Metric(v); }));
        choice("flow", "When excluded", (int)r.flow, "Stable reservation|Reflow", "Layout", mutate([](UiGraphNodeSlotRule& r, const Value& v) { r.flow = (UiGraphNodeSlotFlow)(int)v; }));
        choice("overflow", "Overflow", (int)r.overflow, "Ellipsis|Clip|Wrap", "Layout", mutate([](UiGraphNodeSlotRule& r, const Value& v) { r.overflow = (UiGraphNodeOverflow)(int)v; }));
        integer("items", "Maximum rows / items", r.max_items, 1, 12, "Layout", mutate([](UiGraphNodeSlotRule& r, const Value& v) { r.max_items = (int)v; }));
        integer("preferred_width", "Preferred width", Logical(r.preferred_size.cx), 0, 2048, "Layout", mutate([](UiGraphNodeSlotRule& r, const Value& v) { r.preferred_size.cx = Metric(v); }));
        integer("preferred_height", "Preferred height", Logical(r.preferred_size.cy), 0, 2048, "Layout", mutate([](UiGraphNodeSlotRule& r, const Value& v) { r.preferred_size.cy = Metric(v); }));
        choice("small", "Small representation", (int)r.small, "Hidden|Bar|Bar then dot|Dot", "LOD representation", mutate([](UiGraphNodeSlotRule& r, const Value& v) { r.small = (UiGraphNodeSmallMode)(int)v; }));
        integer("readable", "Readable minimum (pixels)", r.readable_min_px, 1, 128, "LOD representation", mutate([](UiGraphNodeSlotRule& r, const Value& v) { r.readable_min_px = (int)v; }));
        for(int field = 0; field < 3; field++) for(int state = 0; state < 4; state++) {
            const char* names[] = {"Ink", "Face", "Frame"};
            const Color* colors = field == 0 ? r.component_style.ink : field == 1 ? r.component_style.face : r.component_style.frame;
            String id = "color" + AsString(field) + AsString(state);
            Color inherited = effective ? (field == 0 ? effective->state_ink[state] : field == 1 ? effective->state_face[state] : effective->state_frame[state]) : Color(Null);
            if(IsNull(inherited)) inherited = field == 0 ? SColorText() : SColorPaper();
            colour(id, states[state], colors[state], inherited, names[field], mutate([field, state](UiGraphNodeSlotRule& r, const Value& v) {
                Color* colors = field == 0 ? r.component_style.ink : field == 1 ? r.component_style.face : r.component_style.frame;
                colors[state] = IsNull(v) ? Color(Null) : (Color)v;
            }));
        }
        integer("padding", "Padding", Logical(r.component_style.padding), 0, 128, "Frame", mutate([](UiGraphNodeSlotRule& r, const Value& v) { r.component_style.padding = Metric(v); }));
        integer("component_frame", "Frame width", Logical(r.component_style.frame_width), 0, 32, "Frame", mutate([](UiGraphNodeSlotRule& r, const Value& v) { r.component_style.frame_width = Metric(v); }));
        integer("component_radius", "Radius", Logical(r.component_style.radius), 0, 128, "Frame", mutate([](UiGraphNodeSlotRule& r, const Value& v) { r.component_style.radius = Metric(v); }));
        for(int i = 0; i < properties_.GetCount(); i++) properties_[i].enabled = EditableLayout(document_) != nullptr;
    }
    inspector_.SetModel(&properties_); building_ = false; rail_.Layout(); SyncLeft();
}
void NodeWorkspace::ApplyProperty(const String& id, const Value& value, bool final)
{
    if(building_) return;
    int i = setters_.Find(id); if(i < 0) return;
    const auto* item = properties_.Find(id);
    if(!item || !item->enabled || item->read_only) return;
    // Only a renderer-kind change alters this inspector's schema. Ordinary
    // commits must not detach its model, lose scroll, or select the first row.
    Document next = property_origin_ ? *property_origin_ : document_;
    try {
        setters_[i](next, value);
        String error;
        if(!Validate(next, error)) throw Exc(error);
        // Portable authoring constraints are checked on commit; image/data
        // resources do not get decoded or transformed by PropertyEditor paint.
        if(final) Encode(next);
        if(final) { PushUndo(property_origin_ ? *property_origin_ : document_); next.dirty = true; next.revision = document_.revision + 1; }
        document_ = next;
        ApplyDocument();
        if(final) {
            property_origin_.Clear();
            properties_.SetValue(id, value, false);
            properties_.SetValidationError(id, String(), false);
            // Auto components can change kind during Preview already; deciding
            // from the final value versus the preview would miss that change.
            bool schema_changed = id == "feature";
            const int scope = Scope(), page = page_, revision = document_.revision;
            const String selected = selection_.id;
            // Run after PropertyEditor finishes its own commit transaction. A
            // queued update may not replace a newer selection/edit/document.
            PostCallback([this, scope, page, revision, selected, id, schema_changed] {
                if(property_origin_ || Scope() != scope || page_ != page
                   || document_.revision != revision || selection_.id != selected) return;
                if(schema_changed) {
                    RebuildInspector();
                    inspector_.SelectProperty(id);
                }
                else SyncInspectorValues();
            });
        }
    }
    catch(const Exc& e) { properties_.SetValidationError(id, e); status_.SetText(e); }
}
// Refresh dependent summaries in the SAME model. Selection, expanded rows,
// filter and scroll remain owned by PropertyEditor and are not reconstructed.
void NodeWorkspace::SyncInspectorValues()
{
    auto set = [&](const String& id, const Value& value) {
        if(properties_.Find(id)) {
            properties_.SetValue(id, value, false);
            inspector_.RefreshValue(id);
        }
    };
    auto colour = [&](const String& id, Color authored, Color inherited) {
        auto* p = properties_.Find(id);
        if(!p) return;
        p->default_value = inherited;
        p->override_active = !IsNull(authored);
        p->inherited = IsNull(authored);
        set(id, IsNull(authored) ? inherited : authored);
    };
    if(page_ == 2) {
        const auto& s = document_.family.Style(Scope());
        UiGraphNodeStyle theme = UiNodeGraph::StyleForRole(preview_.GetStyle().node, (UiGraphNodeRole)s.role);
        const char* keys[] = { "face", "frame", "ink", "header" };
        for(int field = 0; field < 4; field++) for(int state = 0; state < 4; state++) {
            const Color* authored = field == 0 ? s.face : field == 1 ? s.frame : field == 2 ? s.ink : s.header;
            Color inherited = field == 0 ? (theme.palette.face[state].IsSolid() ? theme.palette.face[state].color : SColorPaper())
                            : field == 1 ? theme.palette.frame[state] : field == 2 ? theme.title_ink[state] : theme.header_face[state];
            colour(String(keys[field]) + AsString(state), authored[state], inherited);
        }
        colour("shadow_color", s.shadow_color, Color(32, 48, 64));
    }
    else if(page_ == 1 || selection_.id.IsEmpty()) {
        set("preview_data", AsJSON(document_.data));
        int labels = document_.data.Find("show_port_labels");
        set("preview_labels", labels >= 0 && document_.data.GetValue(labels).Is<bool>() && (bool)document_.data.GetValue(labels));
    }
    else {
        int n = EffectiveLayout().FindComponent(selection_.id);
        if(n < 0) return;
        const auto& r = EffectiveLayout().slots[n];
        const auto* c = snapshot_.FindComponent(selection_.id);
        selection_.region = (int)r.region;
        selection_label_.SetText((r.label.IsEmpty() ? r.id : r.label) + " / " + region_names[(int)r.region]);
        set("literal", r.use_literal);
        set("asset", r.asset);
        for(int field = 0; field < 3; field++) for(int state = 0; state < 4; state++) {
            const Color* authored = field == 0 ? r.component_style.ink : field == 1 ? r.component_style.face : r.component_style.frame;
            Color inherited = c ? (field == 0 ? c->state_ink[state] : field == 1 ? c->state_face[state] : c->state_frame[state]) : Color(Null);
            if(IsNull(inherited)) inherited = field == 0 ? SColorText() : SColorPaper();
            colour("color" + AsString(field) + AsString(state), authored[state], inherited);
        }
    }
    // Values not covered by dependency groups already belong to the same model.
    // Refreshing that row does not rebuild the rows or force it back into view.
    String selected = inspector_.GetSelectedPropertyId();
    if(!selected.IsEmpty()) inspector_.RefreshValue(selected);
}
void NodeWorkspace::CancelProperty()
{
    if(!property_origin_) return;
    document_ = *property_origin_; property_origin_.Clear(); ApplyDocument();
}
void NodeWorkspace::FinishProperty()
{
    if(property_origin_) { inspector_.Key(K_ESCAPE, 1); CancelProperty(); }
}
void NodeWorkspace::OverrideProperty(const String& id, bool active)
{
    auto* p = properties_.Find(id); if(!p) return;
    Value v = active ? p->value : Value();
    ApplyProperty(id, v, true);
}
} // namespace GraphWorkspace
} // namespace Upp
