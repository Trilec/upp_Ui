from pathlib import Path
import re

ROOT = Path('.')

def read(path):
    return (ROOT / path).read_text(encoding='utf-8')

def write(path, text):
    p = ROOT / path
    p.parent.mkdir(parents=True, exist_ok=True)
    p.write_text(text, encoding='utf-8')

def replace_once(path, old, new):
    text = read(path)
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f'{path}: expected one match, found {count}')
    write(path, text.replace(old, new))

preset_h = r'''#ifndef _Utilities_UiDesigner_Services_UiDesignerPresets_h_
#define _Utilities_UiDesigner_Services_UiDesignerPresets_h_

#include <Utilities/UiDesigner/Catalog/UiDesignerCatalog.h>

namespace Upp {

class UiDesignerPresetLibrary {
public:
    static bool Build(const String& id, const UiDesignerCatalog& catalog,
                      UiDesignerDocument& fragment,
                      UiDesignerNodeId& fragment_root, String& error);
};

}

#endif
'''

preset_cpp = r'''#include "UiDesignerPresets.h"

namespace Upp {

class UiDesignerPresetBuilder {
public:
    UiDesignerPresetBuilder(UiDesignerDocument& document,
                            const UiDesignerCatalog& catalog)
        : document_(document), catalog_(catalog) {}

    UiDesignerNodeId Add(const String& type, const String& name,
                         UiDesignerNodeId parent)
    {
        const UiDesignerControlSpec *spec = catalog_.Find(type);
        if(!spec) {
            error_ = "Preset requires unavailable control type " + type;
            return 0;
        }
        UiDesignerNodeId id = document_.AddNode(type, name, parent,
                                                spec->node_flags);
        UiDesignerNode *node = document_.Find(id);
        if(!node) {
            error_ = "Unable to create preset node " + name;
            return 0;
        }
        node->properties = spec->defaults;
        node->data = spec->data_defaults;
        return id;
    }

    UiDesignerPresetBuilder& P(UiDesignerNodeId id, const char *key,
                               const Value& value)
    {
        if(UiDesignerNode *node = document_.Find(id))
            node->properties.Set(key, value);
        return *this;
    }

    UiDesignerPresetBuilder& Text(UiDesignerNodeId id, const String& value)
    {
        return P(id, "text", value).P(id, "title", value);
    }

    UiDesignerPresetBuilder& Size(UiDesignerNodeId id, const char *w,
                                  const char *h, int fixed_w = 0,
                                  int fixed_h = 0)
    {
        P(id, "width_mode", w).P(id, "height_mode", h);
        if(fixed_w > 0) P(id, "fixed_width", fixed_w);
        if(fixed_h > 0) P(id, "fixed_height", fixed_h);
        return *this;
    }

    UiDesignerPresetBuilder& Box(UiDesignerNodeId id, const char *dir,
                                 int gap = 8, int inset = 0,
                                 const char *wrap = "None")
    {
        return P(id, "direction", dir).P(id, "gap", gap)
               .P(id, "inset", inset).P(id, "wrap", wrap);
    }

    String GetError() const { return error_; }

private:
    UiDesignerDocument& document_;
    const UiDesignerCatalog& catalog_;
    String error_;
};

static UiDesignerNodeId BuildHolyGrail(UiDesignerPresetBuilder& b,
                                      UiDesignerNodeId parent)
{
    UiDesignerNodeId root = b.Add("UiBoxLayout", "holy_grail", parent);
    b.Box(root, "V", 10, 12).Size(root, "Expand", "Expand");
    UiDesignerNodeId header = b.Add("UiTitleCard", "header", root);
    b.Text(header, "Header").Size(header, "Expand", "Fixed", 0, 72)
     .P(header, "subtitle", "Workspace summary and global actions")
     .P(header, "role", "Accent");
    UiDesignerNodeId body = b.Add("UiBoxLayout", "body", root);
    b.Box(body, "H", 10).Size(body, "Expand", "Expand");
    UiDesignerNodeId nav = b.Add("UiGroupPanel", "navigation", body);
    b.Text(nav, "Navigation").Size(nav, "Fixed", "Expand", 180, 0)
     .P(nav, "role", "Subtle");
    UiDesignerNodeId main = b.Add("UiPanel", "main_content", body);
    b.Size(main, "Expand", "Expand");
    UiDesignerNodeId main_col = b.Add("UiBoxLayout", "main_column", main);
    b.Box(main_col, "V", 8, 8).Size(main_col, "Expand", "Expand");
    UiDesignerNodeId hero = b.Add("UiTitleCard", "primary_article", main_col);
    b.Text(hero, "Primary article").Size(hero, "Expand", "Fixed", 0, 88)
     .P(hero, "subtitle", "Lead content and supporting actions");
    UiDesignerNodeId grid = b.Add("UiGridLayout", "content_grid", main_col);
    b.Size(grid, "Expand", "Expand").P(grid, "columns", 2).P(grid, "gap", 8);
    for(int i = 1; i <= 4; i++) {
        UiDesignerNodeId card = b.Add("UiTitleCard", Format("story_%d", i), grid);
        b.Text(card, Format("Story %d", i)).Size(card, "Expand", "Fixed", 0, 72);
    }
    UiDesignerNodeId rail = b.Add("UiGroupPanel", "widgets", body);
    b.Text(rail, "Widgets").Size(rail, "Fixed", "Expand", 200, 0)
     .P(rail, "role", "Subtle");
    UiDesignerNodeId footer = b.Add("UiTitleCard", "footer", root);
    b.Text(footer, "Footer").Size(footer, "Expand", "Fixed", 0, 56)
     .P(footer, "role", "Subtle");
    return root;
}

static UiDesignerNodeId BuildMagazine(UiDesignerPresetBuilder& b,
                                      UiDesignerNodeId parent)
{
    UiDesignerNodeId root = b.Add("UiBoxLayout", "magazine", parent);
    b.Box(root, "V", 10, 12).Size(root, "Expand", "Expand");
    UiDesignerNodeId masthead = b.Add("UiTitleCard", "magazine_header", root);
    b.Text(masthead, "Magazine").Size(masthead, "Expand", "Fixed", 0, 64);
    UiDesignerNodeId hero = b.Add("UiTitleCard", "featured_hero", root);
    b.Text(hero, "Featured story").Size(hero, "Expand", "Fixed", 0, 104)
     .P(hero, "role", "Accent");
    UiDesignerNodeId body = b.Add("UiBoxLayout", "magazine_body", root);
    b.Box(body, "H", 10).Size(body, "Expand", "Expand");
    UiDesignerNodeId stories = b.Add("UiGridLayout", "story_grid", body);
    b.Size(stories, "Expand", "Expand").P(stories, "columns", 2).P(stories, "gap", 8);
    for(int i = 1; i <= 4; i++) {
        UiDesignerNodeId card = b.Add("UiTitleCard", Format("magazine_story_%d", i), stories);
        b.Text(card, Format("Story %d", i)).Size(card, "Expand", "Fixed", 0, 80);
    }
    UiDesignerNodeId rail = b.Add("UiGroupPanel", "side_notes", body);
    b.Text(rail, "Side notes").Size(rail, "Fixed", "Expand", 210, 0)
     .P(rail, "role", "Subtle");
    return root;
}

static UiDesignerNodeId BuildSpa(UiDesignerPresetBuilder& b,
                                 UiDesignerNodeId parent)
{
    UiDesignerNodeId root = b.Add("UiBoxLayout", "spa", parent);
    b.Box(root, "V", 10, 12).Size(root, "Expand", "Expand");
    UiDesignerNodeId top = b.Add("UiBoxLayout", "top_bar", root);
    b.Box(top, "H", 8).Size(top, "Expand", "Fixed", 0, 44);
    UiDesignerNodeId brand = b.Add("UiTitleCard", "workspace", top);
    b.Text(brand, "Workspace").Size(brand, "Expand", "Fixed", 0, 44);
    UiDesignerNodeId save = b.Add("UiSplitButton", "save", top);
    b.Text(save, "Save").Size(save, "Fixed", "Fixed", 96, 34)
     .P(save, "role", "Accent");
    UiDesignerNodeId body = b.Add("UiBoxLayout", "spa_body", root);
    b.Box(body, "H", 10).Size(body, "Expand", "Expand");
    UiDesignerNodeId nav = b.Add("UiGroupPanel", "navigation", body);
    b.Text(nav, "Navigation").Size(nav, "Fixed", "Expand", 190, 0);
    UiDesignerNodeId content = b.Add("UiStack", "content_stack", body);
    b.Size(content, "Expand", "Expand");
    UiDesignerNodeId page = b.Add("UiPanel", "active_page", content);
    b.Size(page, "Expand", "Expand");
    UiDesignerNodeId card = b.Add("UiTitleCard", "page_heading", page);
    b.Text(card, "Single-page application").Size(card, "Expand", "Fixed", 0, 88);
    return root;
}

static UiDesignerNodeId BuildCardGrid(UiDesignerPresetBuilder& b,
                                      UiDesignerNodeId parent)
{
    UiDesignerNodeId root = b.Add("UiGridLayout", "card_grid", parent);
    b.Size(root, "Expand", "Expand").P(root, "columns", 3)
     .P(root, "gap", 10).P(root, "inset", 10);
    for(int i = 1; i <= 6; i++) {
        UiDesignerNodeId card = b.Add("UiTitleCard", Format("card_%d", i), root);
        b.Text(card, Format("Card %d", i)).Size(card, "Expand", "Fixed", 0, 96)
         .P(card, "subtitle", "Reusable summary content");
    }
    return root;
}

static UiDesignerNodeId BuildSplitScreen(UiDesignerPresetBuilder& b,
                                         UiDesignerNodeId parent)
{
    UiDesignerNodeId root = b.Add("UiBoxLayout", "split_screen", parent);
    b.Box(root, "H", 10, 10).Size(root, "Expand", "Expand");
    UiDesignerNodeId left = b.Add("UiPanel", "left_screen", root);
    UiDesignerNodeId right = b.Add("UiPanel", "right_screen", root);
    b.Size(left, "Expand", "Expand").P(left, "role", "Standard");
    b.Size(right, "Expand", "Expand").P(right, "role", "Subtle");
    UiDesignerNodeId ltitle = b.Add("UiTitleCard", "left_title", left);
    UiDesignerNodeId rtitle = b.Add("UiTitleCard", "right_title", right);
    b.Text(ltitle, "Primary view").Size(ltitle, "Expand", "Fixed", 0, 72);
    b.Text(rtitle, "Secondary view").Size(rtitle, "Expand", "Fixed", 0, 72);
    return root;
}

static UiDesignerNodeId BuildFPattern(UiDesignerPresetBuilder& b,
                                      UiDesignerNodeId parent)
{
    UiDesignerNodeId root = b.Add("UiBoxLayout", "f_pattern", parent);
    b.Box(root, "V", 10, 12).Size(root, "Expand", "Expand");
    UiDesignerNodeId title = b.Add("UiTitleCard", "f_heading", root);
    b.Text(title, "F-pattern page").Size(title, "Expand", "Fixed", 0, 72)
     .P(title, "role", "Accent");
    UiDesignerNodeId lead = b.Add("UiLabel", "lead_copy", root);
    b.Text(lead, "Strong opening line and supporting context")
     .Size(lead, "Expand", "Fit");
    UiDesignerNodeId body = b.Add("UiBoxLayout", "f_body", root);
    b.Box(body, "H", 10).Size(body, "Expand", "Expand");
    UiDesignerNodeId content = b.Add("UiBoxLayout", "f_content", body);
    b.Box(content, "V", 8).Size(content, "Expand", "Expand");
    for(int i = 1; i <= 3; i++) {
        UiDesignerNodeId row = b.Add("UiTitleCard", Format("section_%d", i), content);
        b.Text(row, Format("Section %d", i)).Size(row, "Expand", "Fixed", 0, 70);
    }
    UiDesignerNodeId rail = b.Add("UiGroupPanel", "f_rail", body);
    b.Text(rail, "Related").Size(rail, "Fixed", "Expand", 220, 0);
    return root;
}

static UiDesignerNodeId BuildHeaderWithActions(UiDesignerPresetBuilder& b,
                                               UiDesignerNodeId parent)
{
    UiDesignerNodeId root = b.Add("UiPanel", "header_shell", parent);
    b.Size(root, "Expand", "Fit").P(root, "inset", 10);
    UiDesignerNodeId row = b.Add("UiBoxLayout", "header_row", root);
    b.Box(row, "H", 8).Size(row, "Expand", "Fit");
    UiDesignerNodeId title = b.Add("UiTitleCard", "page_header", row);
    b.Text(title, "Project overview").Size(title, "Expand", "Fit")
     .P(title, "subtitle", "Summary and primary page context");
    UiDesignerNodeId refresh = b.Add("UiToolButton", "refresh", row);
    b.Text(refresh, "Refresh").Size(refresh, "Fixed", "Fixed", 80, 34);
    UiDesignerNodeId publish = b.Add("UiButton", "publish", row);
    b.Text(publish, "Publish").Size(publish, "Fixed", "Fixed", 96, 34)
     .P(publish, "role", "Accent");
    UiDesignerNodeId more = b.Add("UiDropdown", "more_actions", row);
    b.Text(more, "More").Size(more, "Fixed", "Fixed", 110, 34);
    return root;
}

static UiDesignerNodeId BuildWorkbench(UiDesignerPresetBuilder& b,
                                       UiDesignerNodeId parent)
{
    UiDesignerNodeId root = b.Add("UiBoxLayout", "designer_workbench", parent);
    b.Box(root, "V", 8, 8).Size(root, "Expand", "Expand");
    UiDesignerNodeId header = b.Add("UiTitleCard", "workbench_header", root);
    b.Text(header, "Designer workbench").Size(header, "Expand", "Fixed", 0, 60)
     .P(header, "role", "Accent");
    UiDesignerNodeId center = b.Add("UiBoxLayout", "workbench_center", root);
    b.Box(center, "H", 8).Size(center, "Expand", "Expand");
    UiDesignerNodeId left = b.Add("UiGroupPanel", "catalog", center);
    b.Text(left, "Catalog").Size(left, "Fixed", "Expand", 220, 0);
    UiDesignerNodeId preview = b.Add("UiPanel", "preview", center);
    b.Size(preview, "Expand", "Expand");
    UiDesignerNodeId canvas = b.Add("UiTitleCard", "canvas", preview);
    b.Text(canvas, "Canvas").Size(canvas, "Expand", "Expand")
     .P(canvas, "subtitle", "Central preview and generated output surface");
    UiDesignerNodeId right = b.Add("UiGroupPanel", "inspector", center);
    b.Text(right, "Inspector").Size(right, "Fixed", "Expand", 260, 0);
    UiDesignerNodeId footer = b.Add("UiLabel", "status", root);
    b.Text(footer, "Ready").Size(footer, "Expand", "Fixed", 0, 28);
    return root;
}

bool UiDesignerPresetLibrary::Build(const String& id,
                                    const UiDesignerCatalog& catalog,
                                    UiDesignerDocument& fragment,
                                    UiDesignerNodeId& fragment_root,
                                    String& error)
{
    fragment.NewDocument(Size(1020, 668));
    UiDesignerPresetBuilder b(fragment, catalog);
    const UiDesignerNodeId parent = fragment.GetRootId();
    if(id == "HolyGrail") fragment_root = BuildHolyGrail(b, parent);
    else if(id == "Magazine") fragment_root = BuildMagazine(b, parent);
    else if(id == "SPA") fragment_root = BuildSpa(b, parent);
    else if(id == "CardGrid") fragment_root = BuildCardGrid(b, parent);
    else if(id == "SplitScreen") fragment_root = BuildSplitScreen(b, parent);
    else if(id == "FPattern") fragment_root = BuildFPattern(b, parent);
    else if(id == "HeaderWithActions") fragment_root = BuildHeaderWithActions(b, parent);
    else if(id == "DesignerWorkbench") fragment_root = BuildWorkbench(b, parent);
    else {
        error = "Unknown preset " + id;
        return false;
    }
    if(!fragment_root || !b.GetError().IsEmpty()) {
        error = b.GetError().IsEmpty() ? "Preset construction failed" : b.GetError();
        return false;
    }
    error.Clear();
    return true;
}

}
'''

write('Utilities/UiDesigner/Services/UiDesignerPresets.h', preset_h)
write('Utilities/UiDesigner/Services/UiDesignerPresets.cpp', preset_cpp)

replace_once('Utilities/UiDesigner/Services/UiDesignerServices.h',
             '#include "UiDesignerSession.h"\n',
             '#include "UiDesignerSession.h"\n#include "UiDesignerPresets.h"\n')

services_upp = read('Utilities/UiDesigner/Services/Services.upp')
services_upp = services_upp.replace('    UiDesignerSession.h,\n    UiDesignerSession.cpp,\n',
                                    '    UiDesignerPresets.h,\n    UiDesignerPresets.cpp,\n    UiDesignerSession.h,\n    UiDesignerSession.cpp,\n')
write('Utilities/UiDesigner/Services/Services.upp', services_upp)

session_h = read('Utilities/UiDesigner/Services/UiDesignerSession.h')
session_h = session_h.replace('#include "UiDesignerDrop.h"\n',
                              '#include "UiDesignerDrop.h"\n#include "UiDesignerPresets.h"\n')
session_h = session_h.replace('    bool ExecuteDrop(const UiDesignerDropPlan& plan,\n                     UiDesignerNodeId *created, String& error);\n',
'''    bool ExecuteDrop(const UiDesignerDropPlan& plan,
                     UiDesignerNodeId *created, String& error);
    bool InsertPreset(const String& preset_id, UiDesignerNodeId target,
                      int index, UiDesignerNodeId *created, String& error);
    const Vector<String>& GetRecentPaths() const { return recent_paths_; }
''')
session_h = session_h.replace('    void ApplyPresetDialog();\n',
                              '    void ApplyPresetDialog();\n    void LoadRecentPaths();\n    void AddRecentPath(const String& path);\n')
session_h = session_h.replace('    String current_path_;\n',
                              '    String current_path_;\n    Vector<String> recent_paths_;\n')
write('Utilities/UiDesigner/Services/UiDesignerSession.h', session_h)

session_cpp = read('Utilities/UiDesigner/Services/UiDesignerSession.cpp')
session_cpp = session_cpp.replace('    RegisterUiDesignerBuiltins(catalog_);\n',
                                  '    RegisterUiDesignerBuiltins(catalog_);\n    LoadRecentPaths();\n')
insert_before = 'void UiDesignerSession::WireEvents()\n'
recent_code = r'''void UiDesignerSession::LoadRecentPaths()
{
    recent_paths_.Clear();
    Value parsed = ParseJSON(LoadFile(ConfigFile("uidesigner-recent.json")));
    if(!parsed.Is<ValueArray>())
        return;
    for(const Value& item : (ValueArray)parsed) {
        const String path = item;
        if(!path.IsEmpty() && FileExists(path) && recent_paths_.GetCount() < 10)
            recent_paths_.Add(path);
    }
}

void UiDesignerSession::AddRecentPath(const String& path)
{
    if(path.IsEmpty())
        return;
    for(int i = recent_paths_.GetCount() - 1; i >= 0; --i)
        if(recent_paths_[i] == path)
            recent_paths_.Remove(i);
    recent_paths_.Insert(0, path);
    while(recent_paths_.GetCount() > 10)
        recent_paths_.Drop();
    ValueArray encoded;
    for(const String& item : recent_paths_)
        encoded.Add(item);
    SaveFile(ConfigFile("uidesigner-recent.json"), AsJSON(encoded, true));
}

'''
if insert_before not in session_cpp:
    raise RuntimeError('session WireEvents marker missing')
session_cpp = session_cpp.replace(insert_before, recent_code + insert_before, 1)
session_cpp = session_cpp.replace('    current_path_ = path;\n    state_.selection.Clear();\n',
                                  '    current_path_ = path;\n    AddRecentPath(path);\n    state_.selection.Clear();\n', 1)
session_cpp = session_cpp.replace('    current_path_ = path;\n    commands_.MarkSaved();\n',
                                  '    current_path_ = path;\n    AddRecentPath(path);\n    commands_.MarkSaved();\n', 1)

marker = 'UiDesignerNodeId UiDesignerSession::ResolveInsertParent() const\n'
insert_preset = r'''static String UniquePresetName(const UiDesignerDocument& document,
                               const String& base)
{
    String candidate = base;
    int suffix = 2;
    for(;;) {
        bool exists = false;
        for(const UiDesignerNode& node : document.GetNodes())
            if(node.name == candidate) {
                exists = true;
                break;
            }
        if(!exists)
            return candidate;
        candidate = base + "_" + AsString(suffix++);
    }
}

bool UiDesignerSession::InsertPreset(const String& preset_id,
                                     UiDesignerNodeId target, int index,
                                     UiDesignerNodeId *created, String& error)
{
    if(!catalog_.FindPreset(preset_id)) {
        error = "Unknown preset " + preset_id;
        return false;
    }
    if(!target)
        target = ResolveInsertParent();
    const UiDesignerNode *target_node = document_.Find(target);
    const UiDesignerControlSpec *target_spec = target_node
        ? catalog_.Find(target_node->type) : nullptr;
    if(!target_node || !target_spec ||
       (target_spec->content_host == UiDesignerContentHostKind::None &&
        !HasUiDesignerCapability(target_spec->capabilities,
                                 UiDesignerCapabilityContainer))) {
        error = "Select a container or layout before inserting a preset";
        return false;
    }

    UiDesignerDocument fragment;
    UiDesignerNodeId fragment_root = 0;
    if(!UiDesignerPresetLibrary::Build(preset_id, catalog_, fragment,
                                       fragment_root, error))
        return false;
    const UiDesignerNode *fragment_node = fragment.Find(fragment_root);
    if(!fragment_node)
        return false;
    String reason;
    if(!catalog_.CanInsert(document_, fragment_node->type, target, index, reason)) {
        error = reason;
        return false;
    }

    UiDesignerDocument updated;
    if(!UiDesignerDeserialize(UiDesignerSerialize(document_, false), updated, error))
        return false;
    VectorMap<UiDesignerNodeId, UiDesignerNodeId> id_map;
    Function<UiDesignerNodeId(UiDesignerNodeId, UiDesignerNodeId, int)> clone_node;
    clone_node = [&](UiDesignerNodeId source_id, UiDesignerNodeId parent,
                     int insert_index) -> UiDesignerNodeId {
        const UiDesignerNode *source = fragment.Find(source_id);
        if(!source)
            return 0;
        const String name = UniquePresetName(updated, source->name);
        UiDesignerNodeId destination = updated.AddNode(
            source->type, name, parent, source->flags, insert_index);
        UiDesignerNode *copy = updated.Find(destination);
        if(!copy)
            return 0;
        copy->properties = source->properties;
        copy->data = source->data;
        copy->theme_overrides = source->theme_overrides;
        copy->theme_override_saved = source->theme_override_saved;
        id_map.Add(source_id, destination);
        for(int i = 0; i < source->children.GetCount(); ++i)
            if(!clone_node(source->children[i], destination, i))
                return 0;
        return destination;
    };

    UiDesignerNodeId inserted = clone_node(fragment_root, target, index);
    if(!inserted) {
        error = "Unable to clone preset subtree";
        return false;
    }
    for(int i = 0; i < id_map.GetCount(); ++i) {
        const UiDesignerNode *source = fragment.Find(id_map.GetKey(i));
        UiDesignerNode *destination = updated.Find(id_map[i]);
        if(!source || !destination)
            continue;
        destination->actions = clone(source->actions);
        for(UiDesignerActionBinding& action : destination->actions)
            if(action.target) {
                const int q = id_map.Find(action.target);
                action.target = q >= 0 ? id_map[q] : 0;
            }
    }
    if(!catalog_.ValidateDocument(updated, error))
        return false;
    if(!commands_.ReplaceDocument(updated, "Insert preset " + preset_id)) {
        error = commands_.GetLastError();
        return false;
    }
    Select(inserted);
    if(created)
        *created = inserted;
    WhenStatus("Inserted preset " + preset_id);
    error.Clear();
    return true;
}

'''
if marker not in session_cpp:
    raise RuntimeError('ResolveInsertParent marker missing')
session_cpp = session_cpp.replace(marker, insert_preset + marker, 1)
write('Utilities/UiDesigner/Services/UiDesignerSession.cpp', session_cpp)

builtins = read('Utilities/UiDesigner/Catalog/UiDesignerBuiltins.cpp')
preset_block = '''
    catalog.RegisterPreset({"HolyGrail", "Holy Grail", "Header, three-column body, and footer", "ICON_DESIGN_DASHBOARD_EDIT_48"});
    catalog.RegisterPreset({"Magazine", "Magazine", "Editorial hero, stories, and side rail", "ICON_DESIGN_DESCRIPTION_48"});
    catalog.RegisterPreset({"SPA", "SPA", "Single-page application workspace shell", "ICON_DESIGN_DESKTOP_MAC_48"});
    catalog.RegisterPreset({"CardGrid", "Card Grid", "Responsive reusable card collection", "ICON_DESIGN_GRID_VIEW_48"});
    catalog.RegisterPreset({"SplitScreen", "Split Screen", "Two equal working surfaces", "ICON_DESIGN_SPLIT_SCENE_48"});
    catalog.RegisterPreset({"FPattern", "F Pattern", "Reading-led content hierarchy", "ICON_DESIGN_VIEW_STREAM_48"});
    catalog.RegisterPreset({"HeaderWithActions", "Header with Actions", "Page title and compact action cluster", "ICON_DESIGN_TITLE_48"});
    catalog.RegisterPreset({"DesignerWorkbench", "Designer Workbench", "Catalog, preview, inspector, and status shell", "ICON_DESIGN_DASHBOARD_CUSTOMIZE_48"});
'''
pattern = re.compile(r'(?:\s*catalog\.RegisterPreset\(UiDesignerPreset\{.*?\}\);)+', re.S)
builtins, count = pattern.subn(preset_block.rstrip(), builtins, count=1)
if count != 1:
    raise RuntimeError(f'expected preset registration block, found {count}')
write('Utilities/UiDesigner/Catalog/UiDesignerBuiltins.cpp', builtins)

window_h = read('Utilities/UiDesigner/UiDesigner/UiDesignerWindow.h')
window_h = window_h.replace('    void BuildHeader();\n',
                            '    void BuildHeader();\n    void RefreshLoadMenu();\n')
write('Utilities/UiDesigner/UiDesigner/UiDesignerWindow.h', window_h)

window_cpp = read('Utilities/UiDesigner/UiDesigner/UiDesignerWindow.cpp')
old_load = '''    load_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Accent));
    load_.SetText("Load").SetSplitWidth(DPI(30));
    load_.Add("Open", "open").Add("New blank", "blank")
         .Add("New three pane", "three_pane")
         .Add("New settings", "settings");
'''
new_load = '''    load_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Accent));
    load_.SetText("Load").SetSplitWidth(DPI(30)).SetPopupMinWidth(DPI(260));
    RefreshLoadMenu();
    load_.WhenOpen = [=] { RefreshLoadMenu(); };
'''
if old_load not in window_cpp:
    raise RuntimeError('Load menu block missing')
window_cpp = window_cpp.replace(old_load, new_load, 1)
window_cpp = window_cpp.replace('''        if(action == "open") LoadDocument();
        else if(action == "blank") session_.NewDocument("blank");
        else if(action == "settings") session_.NewDocument("settings");
        else session_.NewDocument("three_pane");
''', '''        if(action == "open") LoadDocument();
        else if(action == "blank") session_.NewDocument("blank");
        else if(action == "dialog") session_.NewDocument("dialog");
        else if(action == "three_pane") session_.NewDocument("three_pane");
        else if(action.StartsWith("recent:")) {
            String error;
            if(!session_.Load(action.Mid(7), error))
                RefreshStatus(error);
        }
''', 1)
refresh_load = r'''void UiDesignerWindow::RefreshLoadMenu()
{
    load_.ClearItems();
    load_.Add("Open…", "open");
    load_.AddSeparator();
    load_.Add("Blank form", "blank")
         .Add("Three-pane form", "three_pane")
         .Add("Dialog form", "dialog");
    load_.AddSeparator();
    const Vector<String>& recent = session_.GetRecentPaths();
    if(recent.IsEmpty())
        load_.AddGroupHeader("No recent files");
    else {
        load_.AddGroupHeader("Recent files");
        for(const String& path : recent) {
            const int item = load_.GetCount();
            load_.Add(GetFileName(path), "recent:" + path);
            load_.SetItemDescription(item, path);
        }
    }
}

'''
marker = 'void UiDesignerWindow::BuildHeader()\n'
window_cpp = window_cpp.replace(marker, refresh_load + marker, 1)
old_wire = '    wire_list(presets_list_); wire_list(layouts_list_);\n'
new_wire = '''    presets_list_.WhenActivate = [=](const String& id) {
        String error;
        UiDesignerNodeId created = 0;
        if(!session_.InsertPreset(id, 0, -1, &created, error))
            RefreshStatus(error);
    };
    presets_list_.WhenFilter = [=](const String& query) {
        session_.State().toolbox_filter = query;
    };
    presets_list_.WhenToolDrop = [=](const String& id, Point) {
        String error;
        UiDesignerNodeId created = 0;
        if(!session_.InsertPreset(id, session_.State().selection.primary,
                                  -1, &created, error))
            RefreshStatus(error);
    };
    presets_list_.WhenToolCancel = [=] { CancelCatalogDrag(); };
    wire_list(layouts_list_);
'''
if old_wire not in window_cpp:
    raise RuntimeError('preset wire marker missing')
window_cpp = window_cpp.replace(old_wire, new_wire, 1)
write('Utilities/UiDesigner/UiDesigner/UiDesignerWindow.cpp', window_cpp)

serialization = read('Utilities/UiDesigner/Core/UiDesignerSerialization.cpp')
serialization = serialization.replace('    out.Set("schema", 3);\n',
                                      '    out.Set("schema", 4);\n    out.Set("ordering", "explicit-children");\n', 1)
order_code = r'''static bool RestoreExplicitChildOrder(const ValueArray& nodes,
                                      const VectorMap<int64, int64>& id_map,
                                      UiDesignerDocument& loaded,
                                      String& error)
{
    VectorMap<int64, int64> declared_parent;
    for(const Value& item : nodes) {
        if(!item.Is<ValueMap>())
            continue;
        ValueMap encoded = item;
        const int64 id = UiDesignerMapValue(encoded, "id", 0);
        declared_parent.GetAdd(id) = UiDesignerMapValue(encoded, "parent", 0);
    }
    for(const Value& item : nodes) {
        if(!item.Is<ValueMap>())
            continue;
        ValueMap encoded = item;
        const int64 old_parent = UiDesignerMapValue(encoded, "id", 0);
        const int parent_q = id_map.Find(old_parent);
        if(parent_q < 0)
            continue;
        ValueArray children = UiDesignerMapValue(encoded, "children", ValueArray());
        Index<int64> seen;
        for(int i = 0; i < children.GetCount(); ++i) {
            const int64 old_child = children[i];
            if(seen.FindAdd(old_child) >= 0) {
                error = "Parent contains duplicate child " + AsString(old_child);
                return false;
            }
            const int child_q = id_map.Find(old_child);
            const int declared_q = declared_parent.Find(old_child);
            if(child_q < 0 || declared_q < 0 || declared_parent[declared_q] != old_parent) {
                error = "Explicit child order disagrees with parent for " +
                        AsString(old_child);
                return false;
            }
            if(!loaded.MoveNode(id_map[child_q], id_map[parent_q], i)) {
                error = "Unable to restore child order for " + AsString(old_parent);
                return false;
            }
        }
    }
    return true;
}

'''
marker = 'static bool LoadNodes(const ValueArray& nodes, bool legacy,\n'
serialization = serialization.replace(marker, order_code + marker, 1)
call_marker = '''    for(int i = 0; i < pending_actions.GetCount(); i++) {
'''
serialization = serialization.replace(call_marker,
'''    if(!RestoreExplicitChildOrder(nodes, id_map, loaded, error))
        return false;

''' + call_marker, 1)
write('Utilities/UiDesigner/Core/UiDesignerSerialization.cpp', serialization)

for path in ['Utilities/UiDesigner/Services/UiDesignerSession.cpp',
             'Utilities/UiDesigner/Services/UiDesignerExport.cpp']:
    text = read(path)
    text = text.replace('project.Set("schema", 1);', 'project.Set("schema", 2);')
    text = text.replace('source_project.Set("schema", 1);', 'source_project.Set("schema", 2);')
    write(path, text)

reg = read('Utilities/UiDesigner/RegressionTests/main.cpp')
fixture_marker = '    UiDesignerSession session;\n    session.NewDocument("blank");\n'
fixture = r'''    Check(dialog_session.Catalog().GetPresets().GetCount() == 8,
          "Preset catalogue exposes the eight composable layout fragments");
    UiDesignerSession preset_session;
    preset_session.NewDocument("blank");
    UiDesignerNodeId inserted_preset = 0;
    String preset_error;
    Check(preset_session.InsertPreset("HolyGrail",
              preset_session.Document().GetRootId(), -1,
              &inserted_preset, preset_error),
          "Holy Grail inserts into the current document: " + preset_error);
    Check(inserted_preset != 0 && preset_session.Document().GetCount() > 8,
          "Inserted preset creates one ordinary nested node subtree");
    Check(preset_session.Undo() && preset_session.Document().GetCount() == 1 &&
              preset_session.Redo() && preset_session.Document().Find(inserted_preset),
          "Preset insertion is one undoable and redoable command transaction");

    const String explicit_order_json = R"JSON({
      "format":"upp-ui-designer",
      "virtual_size":{"cx":512,"cy":250},
      "nodes":[
        {"id":1,"parent":0,"type":"Window","name":"Window","children":[2],"properties":{}},
        {"id":2,"parent":1,"type":"BoxLayout","name":"row","children":[4,3],"properties":{"direction":{"type":"string","value":"H"}}},
        {"id":3,"parent":2,"type":"Label","name":"second","children":[],"properties":{}},
        {"id":4,"parent":2,"type":"Label","name":"first","children":[],"properties":{}}
      ]
    })JSON";
    UiDesignerDocument ordered_legacy;
    Check(UiDesignerDeserialize(explicit_order_json, ordered_legacy, preset_error),
          "Legacy fixture with explicit child order loads: " + preset_error);
    const UiDesignerNode *ordered_parent = ordered_legacy.Find(2);
    Check(ordered_parent && ordered_parent->children.GetCount() == 2 &&
              ordered_legacy.Find(ordered_parent->children[0])->name == "first" &&
              ordered_legacy.Find(ordered_parent->children[1])->name == "second",
          "Legacy import treats each parent children array as authoritative");

'''
if fixture_marker not in reg:
    raise RuntimeError('regression fixture marker missing')
reg = reg.replace(fixture_marker, fixture + fixture_marker, 1)
write('Utilities/UiDesigner/RegressionTests/main.cpp', reg)

status = read('Utilities/UiDesigner/IMPLEMENTATION_STATUS.md')
status += '''\n\n## Preset and persistence contract\n\n- Load owns whole-document starters: Blank form, Three-pane form and Dialog form.\n- Presets are composable command-backed fragments and can be inserted into compatible selected hosts.\n- The current project format remains the canonical wrapper because it stores document and theme together; document schema 4 makes explicit child arrays authoritative.\n- Legacy imports validate and restore explicit per-parent child order rather than relying on flat node-array order.\n'''
write('Utilities/UiDesigner/IMPLEMENTATION_STATUS.md', status)

for temporary in [
    '.github/uidesigner-presets-legacy-rc.py',
    '.github/workflows/uidesigner-presets-legacy-rc.yml',
]:
    p = Path(temporary)
    if p.exists():
        p.unlink()
