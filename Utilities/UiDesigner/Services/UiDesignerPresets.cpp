#include "UiDesignerPresets.h"

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
        UiDesignerNode *node = document_.Find(id);
        const UiDesignerControlSpec *spec = node ? catalog_.Find(node->type) : nullptr;
        if(node && spec) {
            if(spec->FindProperty("text"))
                node->properties.Set("text", value);
            if(spec->FindProperty("title"))
                node->properties.Set("title", value);
        }
        return *this;
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
        b.Text(card, Format("Story %d", i)).Size(card, "Expand", "Fixed", 0, 72)
         .P(card, "grid_row", (i - 1) / 2).P(card, "grid_column", (i - 1) % 2);
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
        b.Text(card, Format("Story %d", i)).Size(card, "Expand", "Fixed", 0, 80)
         .P(card, "grid_row", (i - 1) / 2).P(card, "grid_column", (i - 1) % 2);
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
         .P(card, "subtitle", "Reusable summary content")
         .P(card, "grid_row", (i - 1) / 3).P(card, "grid_column", (i - 1) % 3);
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
