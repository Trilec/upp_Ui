#include "DesignerTemplates.h"
#include "DesignerDefaults.h"

// DesignerTemplates.cpp - starter model seeds for common layout shapes.
// Templates create ordinary DesignerNode trees, so they exercise the same
// command/preview/codegen paths as hand-built layouts.

namespace Upp {

static void InitTemplateNode(DesignerModel& model, const DesignerRegistry& registry, DesignerNodeId id)
{
	DesignerNode* n = model.Find(id);
	const DesignerType* t = n ? registry.Find(n->type_id) : nullptr;
	if(n && t && t->init_defaults)
		t->init_defaults(*n);
}

static DesignerNode* TemplateNode(DesignerModel& model, DesignerNodeId id)
{
	return model.Find(id);
}

static void SetTemplateProperty(DesignerModel& model, DesignerNodeId id, const char* key, const Value& value)
{
	if(DesignerNode* n = TemplateNode(model, id))
		n->properties.Set(key, value);
}

static void SetTemplateName(DesignerModel& model, DesignerNodeId id, const String& name)
{
	if(DesignerNode* n = TemplateNode(model, id))
		n->name = name;
}

static void SetTemplateText(DesignerModel& model, DesignerNodeId id, const String& text)
{
	SetTemplateProperty(model, id, "text", text);
}

static void SetTemplateSubtitle(DesignerModel& model, DesignerNodeId id, const String& subtitle)
{
	SetTemplateProperty(model, id, "subtitle", subtitle);
}

static void SetTemplateRole(DesignerModel& model, DesignerNodeId id, const char* role)
{
	SetTemplateProperty(model, id, "role", role);
}

static void SetTemplateDirection(DesignerModel& model, DesignerNodeId id, const char* dir)
{
	SetTemplateProperty(model, id, "direction", dir);
}

static void SetTemplateSizing(DesignerModel& model, DesignerNodeId id, const char* h, const char* v)
{
	SetTemplateProperty(model, id, "h_sizing", h);
	SetTemplateProperty(model, id, "v_sizing", v);
}

static void SetTemplateFixedSize(DesignerModel& model, DesignerNodeId id, int w, int h)
{
	if(w > 0)
		SetTemplateProperty(model, id, "width", w);
	if(h > 0)
		SetTemplateProperty(model, id, "height", h);
}

static void SetTemplateMinSize(DesignerModel& model, DesignerNodeId id, int w, int h)
{
	if(w > 0)
		SetTemplateProperty(model, id, "min_width", w);
	if(h > 0)
		SetTemplateProperty(model, id, "min_height", h);
}

static void SetTemplateGap(DesignerModel& model, DesignerNodeId id, int gap)
{
	SetTemplateProperty(model, id, "gap", gap);
}

static void SetTemplateInset(DesignerModel& model, DesignerNodeId id, int inset)
{
	SetTemplateProperty(model, id, "inset", inset);
}

static void SetTemplateColumns(DesignerModel& model, DesignerNodeId id, int cols)
{
	SetTemplateProperty(model, id, "columns", cols);
}

static void SetTemplateIcon(DesignerModel& model, DesignerNodeId id, const char* icon)
{
	SetTemplateProperty(model, id, "icon", icon);
}

static DesignerNodeId AddTemplateNode(DesignerModel& model, const DesignerRegistry& registry,
                                      const String& type, DesignerNodeId parent, const String& name)
{
	DesignerNodeId id = model.AddNode(type, parent);
	InitTemplateNode(model, registry, id);
	SetTemplateName(model, id, name);
	SetTemplateText(model, id, name);
	return id;
}

static void ClearDesignerModel(DesignerModel& model)
{
	const DesignerNode* root = model.Find(Designer_ROOT);
	if(!root)
		return;
	Vector<DesignerNodeId> children = clone(root->children);
	for(DesignerNodeId id : children)
		model.RemoveNode(id);
	model.SetVirtualSize(DesignerWindowSize());
}

static void BuildHolyGrailTemplate(DesignerModel& model, const DesignerRegistry& registry)
{
	DesignerNodeId root = AddTemplateNode(model, registry, "BoxLayout", Designer_ROOT, "holyColumn");
	SetTemplateSizing(model, root, "Expand", "Expand");
	SetTemplateGap(model, root, 10);
	SetTemplateInset(model, root, 12);

	DesignerNodeId header = AddTemplateNode(model, registry, "UiTitleCard", root, "Header");
	SetTemplateRole(model, header, "Accent");
	SetTemplateSizing(model, header, "Expand", "Fixed");
	SetTemplateFixedSize(model, header, 0, 76);
	SetTemplateSubtitle(model, header, "Workspace summary and global actions");
	SetTemplateIcon(model, header, "ICON_DESIGN_SPACE_DASHBOARD_48");
	SetTemplateProperty(model, header, "content_inset", 10);
	SetTemplateProperty(model, header, "media_gap", 10);
	SetTemplateProperty(model, header, "media_reserve", 28);

	DesignerNodeId body = AddTemplateNode(model, registry, "BoxLayout", root, "bodyRow");
	SetTemplateDirection(model, body, "H");
	SetTemplateSizing(model, body, "Expand", "Expand");
	SetTemplateGap(model, body, 10);

	DesignerNodeId left = AddTemplateNode(model, registry, "UiGroupPanel", body, "Navigation");
	SetTemplateRole(model, left, "Subtle");
	SetTemplateSizing(model, left, "Fixed", "Expand");
	SetTemplateFixedSize(model, left, 180, 0);
	SetTemplateMinSize(model, left, 160, 0);
	SetTemplateSubtitle(model, left, "Sections and saved views");
	SetTemplateProperty(model, left, "line", true);
	SetTemplateProperty(model, left, "header_band", true);
	SetTemplateIcon(model, left, "ICON_DESIGN_LEFT_PANEL_OPEN_48");

	DesignerNodeId left_col = AddTemplateNode(model, registry, "BoxLayout", left, "navColumn");
	SetTemplateSizing(model, left_col, "Expand", "Expand");
	SetTemplateGap(model, left_col, 6);
	SetTemplateInset(model, left_col, 6);
	static const char* nav_items[] = {"Overview", "Content", "Library", "Settings"};
	for(const char* item : nav_items) {
		DesignerNodeId btn = AddTemplateNode(model, registry, "UiButton", left_col, item);
		SetTemplateRole(model, btn, item == nav_items[0] ? "Accent" : "Subtle");
		SetTemplateSizing(model, btn, "Expand", "Fixed");
		SetTemplateFixedSize(model, btn, 0, 34);
	}

	DesignerNodeId main = AddTemplateNode(model, registry, "UiPanel", body, "Main content");
	SetTemplateRole(model, main, "Standard");
	SetTemplateSizing(model, main, "Expand", "Expand");
	SetTemplateInset(model, main, 12);

	DesignerNodeId main_col = AddTemplateNode(model, registry, "BoxLayout", main, "mainColumn");
	SetTemplateSizing(model, main_col, "Expand", "Expand");
	SetTemplateGap(model, main_col, 10);
	SetTemplateInset(model, main_col, 4);

	DesignerNodeId hero = AddTemplateNode(model, registry, "UiTitleCard", main_col, "Primary Article");
	SetTemplateRole(model, hero, "Standard");
	SetTemplateSizing(model, hero, "Expand", "Fixed");
	SetTemplateFixedSize(model, hero, 0, 92);
	SetTemplateSubtitle(model, hero, "Lead content, summary, and next actions");
	SetTemplateIcon(model, hero, "ICON_DESIGN_DESCRIPTION_48");
	SetTemplateProperty(model, hero, "card_line", true);

	DesignerNodeId feed = AddTemplateNode(model, registry, "GridLayout", main_col, "contentGrid");
	SetTemplateSizing(model, feed, "Expand", "Expand");
	SetTemplateColumns(model, feed, 2);
	SetTemplateGap(model, feed, 8);
	SetTemplateInset(model, feed, 2);
	for(int i = 1; i <= 4; ++i) {
		DesignerNodeId card = AddTemplateNode(model, registry, "UiTitleCard", feed, Format("Story %d", i));
		SetTemplateRole(model, card, (i % 2) ? "Standard" : "Subtle");
		SetTemplateSizing(model, card, "Expand", "Fixed");
		SetTemplateFixedSize(model, card, 0, 74);
		SetTemplateSubtitle(model, card, "Brief summary and supporting detail");
	}

	DesignerNodeId right = AddTemplateNode(model, registry, "UiGroupPanel", body, "Widgets");
	SetTemplateRole(model, right, "Subtle");
	SetTemplateSizing(model, right, "Fixed", "Expand");
	SetTemplateFixedSize(model, right, 200, 0);
	SetTemplateMinSize(model, right, 180, 0);
	SetTemplateSubtitle(model, right, "Status, notes, and alerts");
	SetTemplateIcon(model, right, "ICON_DESIGN_RIGHT_PANEL_OPEN_48");

	DesignerNodeId right_col = AddTemplateNode(model, registry, "BoxLayout", right, "widgetColumn");
	SetTemplateSizing(model, right_col, "Expand", "Expand");
	SetTemplateGap(model, right_col, 8);
	SetTemplateInset(model, right_col, 6);
	for(int i = 0; i < 3; ++i) {
		DesignerNodeId card = AddTemplateNode(model, registry, "UiTitleCard", right_col, i == 1 ? "Attention" : Format("Widget %d", i + 1));
		SetTemplateRole(model, card, i == 1 ? "Alert" : "Standard");
		SetTemplateSizing(model, card, "Expand", "Fixed");
		SetTemplateFixedSize(model, card, 0, 66);
		SetTemplateSubtitle(model, card, i == 1 ? "Escalated item in the side rail" : "Secondary supporting block");
	}

	DesignerNodeId footer = AddTemplateNode(model, registry, "UiTitleCard", root, "Footer");
	SetTemplateRole(model, footer, "Subtle");
	SetTemplateSizing(model, footer, "Expand", "Fixed");
	SetTemplateFixedSize(model, footer, 0, 58);
	SetTemplateSubtitle(model, footer, "Secondary status, notes, or legal copy");
	SetTemplateProperty(model, footer, "media_reserve", 0);
	SetTemplateProperty(model, footer, "media_gap", 0);
	model.SelectOne(root);
}

static void BuildMagazineTemplate(DesignerModel& model, const DesignerRegistry& registry)
{
	DesignerNodeId root = AddTemplateNode(model, registry, "BoxLayout", Designer_ROOT, "magazineColumn");
	SetTemplateSizing(model, root, "Expand", "Expand");
	SetTemplateGap(model, root, 10);
	SetTemplateInset(model, root, 12);

	DesignerNodeId masthead = AddTemplateNode(model, registry, "UiTitleCard", root, "Magazine Header");
	SetTemplateRole(model, masthead, "Standard");
	SetTemplateSizing(model, masthead, "Expand", "Fixed");
	SetTemplateFixedSize(model, masthead, 0, 68);
	SetTemplateSubtitle(model, masthead, "Editorial layout with feature hero and supporting stories");
	SetTemplateIcon(model, masthead, "ICON_DESIGN_CUSTOM_TYPOGRAPHY_48");

	DesignerNodeId hero = AddTemplateNode(model, registry, "UiTitleCard", root, "Featured Hero");
	SetTemplateRole(model, hero, "Accent");
	SetTemplateSizing(model, hero, "Expand", "Fixed");
	SetTemplateFixedSize(model, hero, 0, 104);
	SetTemplateSubtitle(model, hero, "Top story with deck, byline, and callout");
	SetTemplateIcon(model, hero, "ICON_DESIGN_INFO_48");

	DesignerNodeId body = AddTemplateNode(model, registry, "BoxLayout", root, "magazineBody");
	SetTemplateDirection(model, body, "H");
	SetTemplateSizing(model, body, "Expand", "Expand");
	SetTemplateGap(model, body, 10);

	DesignerNodeId column = AddTemplateNode(model, registry, "BoxLayout", body, "storyColumn");
	SetTemplateSizing(model, column, "Expand", "Expand");
	SetTemplateGap(model, column, 8);

	DesignerNodeId story_grid = AddTemplateNode(model, registry, "GridLayout", column, "storyGrid");
	SetTemplateSizing(model, story_grid, "Expand", "Expand");
	SetTemplateColumns(model, story_grid, 2);
	SetTemplateGap(model, story_grid, 8);
	for(int i = 1; i <= 4; ++i) {
		DesignerNodeId card = AddTemplateNode(model, registry, "UiTitleCard", story_grid, Format("Story %d", i));
		SetTemplateRole(model, card, i == 1 ? "Standard" : "Subtle");
		SetTemplateSizing(model, card, "Expand", "Fixed");
		SetTemplateFixedSize(model, card, 0, 82);
		SetTemplateSubtitle(model, card, "Compact summary card with supporting copy");
	}

	DesignerNodeId rail = AddTemplateNode(model, registry, "UiGroupPanel", body, "Side notes");
	SetTemplateRole(model, rail, "Subtle");
	SetTemplateSizing(model, rail, "Fixed", "Expand");
	SetTemplateFixedSize(model, rail, 210, 0);
	SetTemplateSubtitle(model, rail, "Quotes, facts, and promoted links");
	DesignerNodeId rail_col = AddTemplateNode(model, registry, "BoxLayout", rail, "railColumn");
	SetTemplateSizing(model, rail_col, "Expand", "Expand");
	SetTemplateGap(model, rail_col, 8);
	SetTemplateInset(model, rail_col, 6);
	for(int i = 0; i < 3; ++i) {
		DesignerNodeId note = AddTemplateNode(model, registry, "UiTitleCard", rail_col, i == 0 ? "Quote" : Format("Side card %d", i));
		SetTemplateRole(model, note, i == 0 ? "Accent" : "Standard");
		SetTemplateSizing(model, note, "Expand", "Fixed");
		SetTemplateFixedSize(model, note, 0, 68);
		SetTemplateSubtitle(model, note, "Editorial aside or related reading");
	}
	model.SelectOne(root);
}

static void BuildSpaTemplate(DesignerModel& model, const DesignerRegistry& registry)
{
	DesignerNodeId root = AddTemplateNode(model, registry, "BoxLayout", Designer_ROOT, "spaColumn");
	SetTemplateSizing(model, root, "Expand", "Expand");
	SetTemplateGap(model, root, 10);
	SetTemplateInset(model, root, 12);

	DesignerNodeId top = AddTemplateNode(model, registry, "BoxLayout", root, "topBar");
	SetTemplateDirection(model, top, "H");
	SetTemplateSizing(model, top, "Expand", "Fixed");
	SetTemplateFixedSize(model, top, 0, 44);
	SetTemplateGap(model, top, 8);

	DesignerNodeId brand = AddTemplateNode(model, registry, "UiTitleCard", top, "Workspace");
	SetTemplateRole(model, brand, "Standard");
	SetTemplateSizing(model, brand, "Expand", "Fixed");
	SetTemplateFixedSize(model, brand, 0, 44);
	SetTemplateSubtitle(model, brand, "SPA navigation and content shell");
	SetTemplateIcon(model, brand, "ICON_DESIGN_DESKTOP_MAC_48");
	SetTemplateProperty(model, brand, "media_reserve", 22);
	SetTemplateProperty(model, brand, "media_gap", 8);

	DesignerNodeId save = AddTemplateNode(model, registry, "UiSplitButton", top, "Save");
	SetTemplateRole(model, save, "Accent");
	SetTemplateSizing(model, save, "Fixed", "Fixed");
	SetTemplateFixedSize(model, save, 110, 34);

	DesignerNodeId body = AddTemplateNode(model, registry, "BoxLayout", root, "spaBody");
	SetTemplateDirection(model, body, "H");
	SetTemplateSizing(model, body, "Expand", "Expand");
	SetTemplateGap(model, body, 10);

	DesignerNodeId nav = AddTemplateNode(model, registry, "UiGroupPanel", body, "Navigation");
	SetTemplateRole(model, nav, "Subtle");
	SetTemplateSizing(model, nav, "Fixed", "Expand");
	SetTemplateFixedSize(model, nav, 190, 0);
	SetTemplateSubtitle(model, nav, "Primary routes and filters");
	DesignerNodeId nav_col = AddTemplateNode(model, registry, "BoxLayout", nav, "navStack");
	SetTemplateSizing(model, nav_col, "Expand", "Expand");
	SetTemplateGap(model, nav_col, 6);
	SetTemplateInset(model, nav_col, 6);
	static const char* spa_items[] = {"Dashboard", "Projects", "Activity", "Settings"};
	for(const char* item : spa_items) {
		DesignerNodeId btn = AddTemplateNode(model, registry, "UiButton", nav_col, item);
		SetTemplateRole(model, btn, item == spa_items[0] ? "Accent" : "Subtle");
		SetTemplateSizing(model, btn, "Expand", "Fixed");
		SetTemplateFixedSize(model, btn, 0, 34);
	}

	DesignerNodeId content = AddTemplateNode(model, registry, "BoxLayout", body, "contentStack");
	SetTemplateSizing(model, content, "Expand", "Expand");
	SetTemplateGap(model, content, 10);

	DesignerNodeId page = AddTemplateNode(model, registry, "UiTitleCard", content, "Overview");
	SetTemplateRole(model, page, "Standard");
	SetTemplateSizing(model, page, "Expand", "Fixed");
	SetTemplateFixedSize(model, page, 0, 90);
	SetTemplateSubtitle(model, page, "Primary page summary and actions");
	SetTemplateIcon(model, page, "ICON_DESIGN_SPLITSCREEN_LANDSCAPE_48");

	DesignerNodeId section_grid = AddTemplateNode(model, registry, "GridLayout", content, "sectionGrid");
	SetTemplateSizing(model, section_grid, "Expand", "Expand");
	SetTemplateColumns(model, section_grid, 2);
	SetTemplateGap(model, section_grid, 8);
	for(const char* section : {"Home", "Profile", "Settings", "Billing"}) {
		DesignerNodeId card = AddTemplateNode(model, registry, "UiTitleCard", section_grid, section);
		SetTemplateRole(model, card, section == String("Billing") ? "Accent" : "Standard");
		SetTemplateSizing(model, card, "Expand", "Fixed");
		SetTemplateFixedSize(model, card, 0, 92);
		SetTemplateSubtitle(model, card, "Feature area or route-level content");
	}
	model.SelectOne(root);
}

static void BuildCardGridTemplate(DesignerModel& model, const DesignerRegistry& registry)
{
	DesignerNodeId root = AddTemplateNode(model, registry, "BoxLayout", Designer_ROOT, "cardGridColumn");
	SetTemplateSizing(model, root, "Expand", "Expand");
	SetTemplateGap(model, root, 10);
	SetTemplateInset(model, root, 12);

	DesignerNodeId header = AddTemplateNode(model, registry, "UiTitleCard", root, "Card Grid");
	SetTemplateRole(model, header, "Standard");
	SetTemplateSizing(model, header, "Expand", "Fixed");
	SetTemplateFixedSize(model, header, 0, 68);
	SetTemplateSubtitle(model, header, "Repeated cards with clean grid sizing");
	SetTemplateIcon(model, header, "ICON_DESIGN_CODE_BLOCKS_48");

	DesignerNodeId grid = AddTemplateNode(model, registry, "GridLayout", root, "cards");
	SetTemplateSizing(model, grid, "Expand", "Expand");
	SetTemplateColumns(model, grid, 3);
	SetTemplateGap(model, grid, 8);
	SetTemplateInset(model, grid, 2);
	for(int i = 1; i <= 6; ++i) {
		DesignerNodeId card = AddTemplateNode(model, registry, "UiTitleCard", grid, Format("Card %d", i));
		SetTemplateRole(model, card, i == 1 ? "Accent" : (i % 2 ? "Standard" : "Subtle"));
		SetTemplateSizing(model, card, "Expand", "Fixed");
		SetTemplateFixedSize(model, card, 0, 84);
		SetTemplateSubtitle(model, card, "Metric, status, or summary block");
	}
	model.SelectOne(root);
}

static void BuildSplitScreenTemplate(DesignerModel& model, const DesignerRegistry& registry)
{
	DesignerNodeId root = AddTemplateNode(model, registry, "BoxLayout", Designer_ROOT, "splitColumn");
	SetTemplateSizing(model, root, "Expand", "Expand");
	SetTemplateGap(model, root, 10);
	SetTemplateInset(model, root, 12);

	DesignerNodeId header = AddTemplateNode(model, registry, "UiTitleCard", root, "Shared Header");
	SetTemplateRole(model, header, "Standard");
	SetTemplateSizing(model, header, "Expand", "Fixed");
	SetTemplateFixedSize(model, header, 0, 68);
	SetTemplateSubtitle(model, header, "Two-pane layout for side-by-side comparison");
	SetTemplateIcon(model, header, "ICON_DESIGN_SPLITSCREEN_PORTRAIT_48");

	DesignerNodeId row = AddTemplateNode(model, registry, "BoxLayout", root, "splitBody");
	SetTemplateDirection(model, row, "H");
	SetTemplateSizing(model, row, "Expand", "Expand");
	SetTemplateGap(model, row, 10);

	DesignerNodeId left = AddTemplateNode(model, registry, "UiPanel", row, "Left Panel");
	SetTemplateRole(model, left, "Standard");
	SetTemplateSizing(model, left, "Expand", "Expand");
	SetTemplateInset(model, left, 12);
	DesignerNodeId left_card = AddTemplateNode(model, registry, "UiTitleCard", left, "Primary View");
	SetTemplateRole(model, left_card, "Standard");
	SetTemplateSizing(model, left_card, "Expand", "Fixed");
	SetTemplateFixedSize(model, left_card, 0, 88);
	SetTemplateSubtitle(model, left_card, "Main working surface");

	DesignerNodeId right = AddTemplateNode(model, registry, "UiPanel", row, "Right Panel");
	SetTemplateRole(model, right, "Subtle");
	SetTemplateSizing(model, right, "Expand", "Expand");
	SetTemplateInset(model, right, 12);
	DesignerNodeId right_card = AddTemplateNode(model, registry, "UiTitleCard", right, "Reference View");
	SetTemplateRole(model, right_card, "Subtle");
	SetTemplateSizing(model, right_card, "Expand", "Fixed");
	SetTemplateFixedSize(model, right_card, 0, 88);
	SetTemplateSubtitle(model, right_card, "Secondary side-by-side context");
	model.SelectOne(root);
}

static void BuildFPatternTemplate(DesignerModel& model, const DesignerRegistry& registry)
{
	DesignerNodeId root = AddTemplateNode(model, registry, "BoxLayout", Designer_ROOT, "fPatternColumn");
	SetTemplateSizing(model, root, "Expand", "Expand");
	SetTemplateGap(model, root, 10);
	SetTemplateInset(model, root, 12);

	DesignerNodeId top = AddTemplateNode(model, registry, "UiTitleCard", root, "Top Navigation Bar");
	SetTemplateRole(model, top, "Standard");
	SetTemplateSizing(model, top, "Expand", "Fixed");
	SetTemplateFixedSize(model, top, 0, 64);
	SetTemplateSubtitle(model, top, "F-pattern dashboard with metrics and primary work area");
	SetTemplateIcon(model, top, "ICON_DESIGN_FORMAT_PAINT_48");

	DesignerNodeId metrics = AddTemplateNode(model, registry, "GridLayout", root, "metricRow");
	SetTemplateSizing(model, metrics, "Expand", "Fixed");
	SetTemplateFixedSize(model, metrics, 0, 92);
	SetTemplateColumns(model, metrics, 3);
	SetTemplateGap(model, metrics, 8);
	for(int i = 0; i < 3; ++i) {
		DesignerNodeId card = AddTemplateNode(model, registry, "UiTitleCard", metrics,
			i == 2 ? "Primary CTA" : Format("Metric %d", i + 1));
		SetTemplateRole(model, card, i == 2 ? "Accent" : "Standard");
		SetTemplateSizing(model, card, "Expand", "Expand");
		SetTemplateSubtitle(model, card, i == 2 ? "Call to action or highlighted state" : "Headline figure and context");
	}

	DesignerNodeId body = AddTemplateNode(model, registry, "BoxLayout", root, "bodyRow");
	SetTemplateDirection(model, body, "H");
	SetTemplateSizing(model, body, "Expand", "Expand");
	SetTemplateGap(model, body, 10);

	DesignerNodeId primary = AddTemplateNode(model, registry, "UiGroupPanel", body, "Primary List");
	SetTemplateRole(model, primary, "Subtle");
	SetTemplateSizing(model, primary, "Fixed", "Expand");
	SetTemplateFixedSize(model, primary, 220, 0);
	SetTemplateSubtitle(model, primary, "Persistent left-hand scan path");

	DesignerNodeId secondary = AddTemplateNode(model, registry, "UiPanel", body, "Secondary Content");
	SetTemplateRole(model, secondary, "Standard");
	SetTemplateSizing(model, secondary, "Expand", "Expand");
	SetTemplateInset(model, secondary, 12);
	DesignerNodeId secondary_col = AddTemplateNode(model, registry, "BoxLayout", secondary, "secondaryColumn");
	SetTemplateSizing(model, secondary_col, "Expand", "Expand");
	SetTemplateGap(model, secondary_col, 8);
	for(int i = 0; i < 3; ++i) {
		DesignerNodeId card = AddTemplateNode(model, registry, "UiTitleCard", secondary_col,
			i == 0 ? "Primary Story" : Format("Support %d", i));
		SetTemplateRole(model, card, i == 0 ? "Standard" : "Subtle");
		SetTemplateSizing(model, card, "Expand", "Fixed");
		SetTemplateFixedSize(model, card, 0, i == 0 ? 88 : 68);
		SetTemplateSubtitle(model, card, i == 0 ? "Longer body area for the main reading path" : "Supporting content block");
	}
	model.SelectOne(root);
}

static void BuildDesignerWorkbenchTemplate(DesignerModel& model, const DesignerRegistry& registry)
{
	DesignerNodeId root = AddTemplateNode(model, registry, "BoxLayout", Designer_ROOT, "main_box");
	SetTemplateDirection(model, root, "V");
	SetTemplateSizing(model, root, "Expand", "Expand");
	SetTemplateGap(model, root, 8);
	SetTemplateInset(model, root, 8);

	DesignerNodeId top = AddTemplateNode(model, registry, "BoxLayout", root, "top_box");
	SetTemplateDirection(model, top, "H");
	SetTemplateSizing(model, top, "Expand", "Fit");
	SetTemplateGap(model, top, 8);
	SetTemplateInset(model, top, 6);
	SetTemplateProperty(model, top, "wrap", "Flow");

	DesignerNodeId title = AddTemplateNode(model, registry, "UiTitleCard", top, "titlecard");
	SetTemplateText(model, title, "Designer");
	SetTemplateSubtitle(model, title, "Workbench shell preset");
	SetTemplateRole(model, title, "Standard");
	SetTemplateSizing(model, title, "Fit", "Fit");
	SetTemplateFixedSize(model, title, 220, 72);
	SetTemplateIcon(model, title, "ICON_BRAND_NEWLOGO_V5_48");
	SetTemplateProperty(model, title, "media_reserve", 28);
	SetTemplateProperty(model, title, "media_gap", 8);

	DesignerNodeId spacer_a = AddTemplateNode(model, registry, "Spacer", top, "spacer");
	SetTemplateSizing(model, spacer_a, "Fit", "Expand");
	SetTemplateMinSize(model, spacer_a, 10, 10);
	SetTemplateFixedSize(model, spacer_a, 24, 24);

	DesignerNodeId save = AddTemplateNode(model, registry, "UiSplitButton", top, "save_button");
	SetTemplateText(model, save, "Save");
	SetTemplateRole(model, save, "Accent");
	SetTemplateSizing(model, save, "Fit", "Fit");
	SetTemplateFixedSize(model, save, 112, 34);
	SetTemplateMinSize(model, save, 1, 24);

	DesignerNodeId load = AddTemplateNode(model, registry, "UiSplitButton", top, "load_button");
	SetTemplateText(model, load, "Load");
	SetTemplateRole(model, load, "Accent");
	SetTemplateSizing(model, load, "Fit", "Fit");
	SetTemplateFixedSize(model, load, 112, 34);
	SetTemplateMinSize(model, load, 1, 24);

	DesignerNodeId version = AddTemplateNode(model, registry, "UiLabel", top, "version_badge");
	SetTemplateText(model, version, "v1.0.1 Alpha");
	SetTemplateRole(model, version, "Accent");
	SetTemplateSizing(model, version, "Fit", "Fixed");
	SetTemplateFixedSize(model, version, 120, 24);
	SetTemplateMinSize(model, version, 76, 28);
	SetTemplateIcon(model, version, "ICON_DESIGN_ADJUST_48");
	SetTemplateProperty(model, version, "content_gap", 6);

	DesignerNodeId grow = AddTemplateNode(model, registry, "Spacer", top, "spacer_grow");
	SetTemplateSizing(model, grow, "Expand", "Expand");
	SetTemplateMinSize(model, grow, 10, 10);
	SetTemplateFixedSize(model, grow, 24, 24);
	SetTemplateProperty(model, grow, "weight", 1);

	DesignerNodeId theme = AddTemplateNode(model, registry, "UiDropdown", top, "theme_dropdown");
	SetTemplateText(model, theme, "Theme");
	SetTemplateRole(model, theme, "Accent");
	SetTemplateSizing(model, theme, "Fit", "Fit");
	SetTemplateFixedSize(model, theme, 180, 32);
	SetTemplateProperty(model, theme, "item_count", 2);
	SetTemplateProperty(model, theme, "item_0_text", "Minimal");
	SetTemplateProperty(model, theme, "item_1_text", "Pill");
	SetTemplateProperty(model, theme, "selected_index", 0);

	DesignerNodeId dark = AddTemplateNode(model, registry, "UiToolButton", top, "dark_toggle");
	SetTemplateText(model, dark, "Dark");
	SetTemplateRole(model, dark, "Accent");
	SetTemplateSizing(model, dark, "Fit", "Fit");
	SetTemplateFixedSize(model, dark, 72, 34);
	SetTemplateMinSize(model, dark, 65, 24);
	SetTemplateIcon(model, dark, "ICON_ACTION_DARK_MODE_48");

	DesignerNodeId help = AddTemplateNode(model, registry, "UiToolButton", top, "help_button");
	SetTemplateText(model, help, "Help");
	SetTemplateRole(model, help, "Accent");
	SetTemplateSizing(model, help, "Fit", "Fit");
	SetTemplateFixedSize(model, help, 72, 34);
	SetTemplateMinSize(model, help, 65, 24);
	SetTemplateIcon(model, help, "ICON_DESIGN_HELP_48");

	DesignerNodeId exit = AddTemplateNode(model, registry, "UiButton", top, "exit_button");
	SetTemplateText(model, exit, "Exit");
	SetTemplateRole(model, exit, "Alert");
	SetTemplateSizing(model, exit, "Fit", "Fit");
	SetTemplateFixedSize(model, exit, 120, 32);
	SetTemplateMinSize(model, exit, 1, 1);
	SetTemplateIcon(model, exit, "ICON_ACTION_CANCEL_48");

	DesignerNodeId center = AddTemplateNode(model, registry, "BoxLayout", root, "center_box");
	SetTemplateDirection(model, center, "H");
	SetTemplateSizing(model, center, "Expand", "Expand");
	SetTemplateGap(model, center, 8);

	DesignerNodeId left = AddTemplateNode(model, registry, "BoxLayout", center, "leftlayout");
	SetTemplateDirection(model, left, "V");
	SetTemplateSizing(model, left, "Fit", "Expand");
	SetTemplateMinSize(model, left, 56, 1);
	SetTemplateProperty(model, left, "wrap", "Flow");
	SetTemplateGap(model, left, 8);

	DesignerNodeId left_tools_panel = AddTemplateNode(model, registry, "UiPanel", left, "left_tools_panel");
	SetTemplateRole(model, left_tools_panel, "Subtle");
	SetTemplateSizing(model, left_tools_panel, "Fit", "Fit");
	SetTemplateFixedSize(model, left_tools_panel, 240, 140);
	DesignerNodeId left_tools = AddTemplateNode(model, registry, "BoxLayout", left_tools_panel, "left_tools");
	SetTemplateDirection(model, left_tools, "H");
	SetTemplateSizing(model, left_tools, "Expand", "Expand");
	SetTemplateGap(model, left_tools, 8);
	SetTemplateInset(model, left_tools, 8);
	SetTemplateProperty(model, left_tools, "wrap", "Flow");
	for(const char* icon_name : {
		"ICON_DESIGN_LAYOUTS_CATEGORY_48",
		"ICON_DESIGN_TAB_GROUP_48",
		"ICON_DESIGN_WIDGETS_48",
		"ICON_DESIGN_DYNAMIC_FORM_48",
		"ICON_DESIGN_DASHBOARD_EDIT_48"
	}) {
		DesignerNodeId b = AddTemplateNode(model, registry, "UiToolButton", left_tools, "tool");
		SetTemplateRole(model, b, "Standard");
		SetTemplateSizing(model, b, "Fit", "Fit");
		SetTemplateFixedSize(model, b, 40, 34);
		SetTemplateIcon(model, b, icon_name);
	}
	DesignerNodeId left_gap = AddTemplateNode(model, registry, "Spacer", left_tools, "left_panel_spacer");
	SetTemplateSizing(model, left_gap, "Fit", "Expand");
	SetTemplateFixedSize(model, left_gap, 24, 24);
	SetTemplateMinSize(model, left_gap, 10, 10);
	DesignerNodeId left_toggle = AddTemplateNode(model, registry, "UiToolButton", left_tools, "close_left");
	SetTemplateText(model, left_toggle, "Close");
	SetTemplateRole(model, left_toggle, "Standard");
	SetTemplateSizing(model, left_toggle, "Fit", "Fit");
	SetTemplateFixedSize(model, left_toggle, 72, 34);
	SetTemplateIcon(model, left_toggle, "ICON_DESIGN_LEFT_PANEL_CLOSE_48");

	DesignerNodeId left_scroll = AddTemplateNode(model, registry, "UiScrollPanel", left, "scrollpanel");
	SetTemplateRole(model, left_scroll, "Subtle");
	SetTemplateSizing(model, left_scroll, "Expand", "Expand");
	SetTemplateFixedSize(model, left_scroll, 260, 160);
	SetTemplateMinSize(model, left_scroll, 1, 1);
	DesignerNodeId left_scroll_col = AddTemplateNode(model, registry, "BoxLayout", left_scroll, "left_scroll_content");
	SetTemplateSizing(model, left_scroll_col, "Expand", "Expand");
	SetTemplateGap(model, left_scroll_col, 8);
	SetTemplateInset(model, left_scroll_col, 8);
	for(int i = 0; i < 4; ++i) {
		DesignerNodeId card = AddTemplateNode(model, registry, "UiTitleCard", left_scroll_col,
			i == 0 ? "Toolbox" : Format("Library %d", i));
		SetTemplateRole(model, card, i == 0 ? "Accent" : "Standard");
		SetTemplateSizing(model, card, "Expand", "Fixed");
		SetTemplateFixedSize(model, card, 0, 68);
		SetTemplateSubtitle(model, card, i == 0 ? "Pinned tools and categories" : "Reusable assets and fragments");
	}

	DesignerNodeId preview = AddTemplateNode(model, registry, "BoxLayout", center, "previewlayout");
	SetTemplateDirection(model, preview, "V");
	SetTemplateSizing(model, preview, "Expand", "Expand");
	SetTemplateGap(model, preview, 8);
	DesignerNodeId preview_card = AddTemplateNode(model, registry, "UiTitleCard", preview, "Preview");
	SetTemplateRole(model, preview_card, "Standard");
	SetTemplateSizing(model, preview_card, "Expand", "Fixed");
	SetTemplateFixedSize(model, preview_card, 0, 72);
	SetTemplateSubtitle(model, preview_card, "Central work area and generated output surface");
	SetTemplateIcon(model, preview_card, "ICON_DESIGN_PANEL_48");
	DesignerNodeId preview_surface = AddTemplateNode(model, registry, "UiPanel", preview, "preview_surface");
	SetTemplateRole(model, preview_surface, "Standard");
	SetTemplateSizing(model, preview_surface, "Expand", "Expand");
	SetTemplateInset(model, preview_surface, 12);
	DesignerNodeId preview_stack = AddTemplateNode(model, registry, "BoxLayout", preview_surface, "preview_stack");
	SetTemplateSizing(model, preview_stack, "Expand", "Expand");
	SetTemplateGap(model, preview_stack, 10);
	DesignerNodeId canvas = AddTemplateNode(model, registry, "UiTitleCard", preview_stack, "Canvas");
	SetTemplateRole(model, canvas, "Subtle");
	SetTemplateSizing(model, canvas, "Expand", "Expand");
	SetTemplateFixedSize(model, canvas, 0, 260);
	SetTemplateSubtitle(model, canvas, "Preview frame, selection overlays, and generated shell");
	DesignerNodeId notes = AddTemplateNode(model, registry, "UiLabel", preview_stack, "notes");
	SetTemplateText(model, notes, "Resize, theme, save/load, and codegen are all exercised by this preset.");
	SetTemplateRole(model, notes, "Subtle");
	SetTemplateSizing(model, notes, "Expand", "Fit");

	DesignerNodeId right = AddTemplateNode(model, registry, "BoxLayout", center, "rightlayout");
	SetTemplateDirection(model, right, "V");
	SetTemplateSizing(model, right, "Fit", "Expand");
	SetTemplateGap(model, right, 8);
	SetTemplateProperty(model, right, "wrap", "Flow");

	DesignerNodeId right_tools_panel = AddTemplateNode(model, registry, "UiPanel", right, "right_tools_panel");
	SetTemplateRole(model, right_tools_panel, "Subtle");
	SetTemplateSizing(model, right_tools_panel, "Fit", "Fit");
	SetTemplateFixedSize(model, right_tools_panel, 240, 140);
	DesignerNodeId right_tools = AddTemplateNode(model, registry, "BoxLayout", right_tools_panel, "right_tools");
	SetTemplateDirection(model, right_tools, "H");
	SetTemplateSizing(model, right_tools, "Expand", "Expand");
	SetTemplateGap(model, right_tools, 8);
	SetTemplateInset(model, right_tools, 8);
	SetTemplateProperty(model, right_tools, "wrap", "Flow");
	DesignerNodeId right_toggle = AddTemplateNode(model, registry, "UiToolButton", right_tools, "close_right");
	SetTemplateText(model, right_toggle, "Close");
	SetTemplateRole(model, right_toggle, "Standard");
	SetTemplateSizing(model, right_toggle, "Fit", "Fit");
	SetTemplateFixedSize(model, right_toggle, 72, 34);
	SetTemplateIcon(model, right_toggle, "ICON_DESIGN_RIGHT_PANEL_CLOSE_48");
	for(const char* icon_name : {
		"ICON_DESIGN_CODE_BLOCKS_48",
		"ICON_DESIGN_FORMAT_PAINT_48",
		"ICON_DESIGN_TUNE_48",
		"ICON_DESIGN_ACCOUNT_TREE_48"
	}) {
		DesignerNodeId b = AddTemplateNode(model, registry, "UiToolButton", right_tools, "tool");
		SetTemplateRole(model, b, "Standard");
		SetTemplateSizing(model, b, "Fit", "Fit");
		SetTemplateFixedSize(model, b, 40, 34);
		SetTemplateIcon(model, b, icon_name);
	}
	DesignerNodeId right_gap = AddTemplateNode(model, registry, "Spacer", right_tools, "right_panel_spacer");
	SetTemplateSizing(model, right_gap, "Fit", "Expand");
	SetTemplateFixedSize(model, right_gap, 24, 24);
	SetTemplateMinSize(model, right_gap, 10, 10);

	DesignerNodeId right_scroll = AddTemplateNode(model, registry, "UiScrollPanel", right, "scrollpanel_02");
	SetTemplateRole(model, right_scroll, "Subtle");
	SetTemplateSizing(model, right_scroll, "Expand", "Expand");
	SetTemplateFixedSize(model, right_scroll, 260, 160);
	SetTemplateMinSize(model, right_scroll, 1, 1);
	DesignerNodeId right_scroll_col = AddTemplateNode(model, registry, "BoxLayout", right_scroll, "right_scroll_content");
	SetTemplateSizing(model, right_scroll_col, "Expand", "Expand");
	SetTemplateGap(model, right_scroll_col, 8);
	SetTemplateInset(model, right_scroll_col, 8);
	for(int i = 0; i < 4; ++i) {
		DesignerNodeId card = AddTemplateNode(model, registry, "UiTitleCard", right_scroll_col,
			i == 0 ? "Inspector" : Format("Panel %d", i));
		SetTemplateRole(model, card, i == 0 ? "Accent" : "Standard");
		SetTemplateSizing(model, card, "Expand", "Fixed");
		SetTemplateFixedSize(model, card, 0, 68);
		SetTemplateSubtitle(model, card, i == 0 ? "Properties, code, hierarchy, and theme overrides" : "Supporting tool or output panel");
	}

	DesignerNodeId bottom = AddTemplateNode(model, registry, "BoxLayout", root, "lowerlayout");
	SetTemplateDirection(model, bottom, "V");
	SetTemplateSizing(model, bottom, "Expand", "Fit");
	SetTemplateGap(model, bottom, 6);
	SetTemplateInset(model, bottom, 6);

	DesignerNodeId status = AddTemplateNode(model, registry, "UiLabel", bottom, "status_label");
	SetTemplateText(model, status, "Ready. Preset covers split buttons, dropdown, tool buttons, scroll panels, spacers, and role surfaces.");
	SetTemplateRole(model, status, "Standard");
	SetTemplateSizing(model, status, "Fit", "Fit");

	model.SelectOne(root);
}

bool ApplyDesignerTemplate(DesignerModel& model, const DesignerRegistry& registry, const String& id)
{
	if(id.IsEmpty() || id == "Current")
		return false;
	ClearDesignerModel(model);
	if(id == "HolyGrail")
		BuildHolyGrailTemplate(model, registry);
	else if(id == "Magazine")
		BuildMagazineTemplate(model, registry);
	else if(id == "SPA")
		BuildSpaTemplate(model, registry);
	else if(id == "CardGrid")
		BuildCardGridTemplate(model, registry);
	else if(id == "SplitScreen")
		BuildSplitScreenTemplate(model, registry);
	else if(id == "FPattern")
		BuildFPatternTemplate(model, registry);
	else if(id == "DesignerWorkbench")
		BuildDesignerWorkbenchTemplate(model, registry);
	else
		return false;
	return true;
}

}
