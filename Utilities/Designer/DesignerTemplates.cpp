#include "DesignerTemplates.h"

namespace Upp {

static void InitTemplateNode(DesignerModel& model, const DesignerRegistry& registry, DesignerNodeId id)
{
	DesignerNode* n = model.Find(id);
	const DesignerType* t = n ? registry.Find(n->type_id) : nullptr;
	if(n && t && t->init_defaults)
		t->init_defaults(*n);
}

static DesignerNodeId AddTemplateNode(DesignerModel& model, const DesignerRegistry& registry,
                                        const String& type, DesignerNodeId parent, const String& name,
                                        Color face = Null, Color frame = Null, const String& sizing = String(),
                                        int width = 0, int height = 0)
{
	DesignerNodeId id = model.AddNode(type, parent);
	InitTemplateNode(model, registry, id);
	DesignerNode* n = model.Find(id);
	if(!n)
		return id;
	n->name = name;
	if(!IsNull(face))
		n->properties.Set("face", face);
	if(!IsNull(frame))
		n->properties.Set("frame", frame);
	if(!sizing.IsEmpty())
		n->properties.Set("sizing", sizing);
	if(width > 0)
		n->properties.Set("width", width);
	if(height > 0)
		n->properties.Set("height", height);
	n->properties.Set("text", name);
	n->properties.Set("radius", 6);
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
	model.SetVirtualSize(Size(760, 460));
}

static void BuildHolyGrailTemplate(DesignerModel& model, const DesignerRegistry& registry)
{
	DesignerNodeId root = AddTemplateNode(model, registry, "BoxLayout", Designer_ROOT, "holyColumn");
	model.Find(root)->properties.Set("sizing", "Expand");
	AddTemplateNode(model, registry, "UiTitleCard", root, "Header", Color(79, 70, 229), Color(79, 70, 229), "Fixed", 600, 48);
	DesignerNodeId body = AddTemplateNode(model, registry, "BoxLayout", root, "bodyRow", Color(248, 250, 252), Color(226, 232, 240), "Expand");
	model.Find(body)->properties.Set("direction", "H");
	AddTemplateNode(model, registry, "UiLabel", body, "Left nav", Color(248, 250, 252), Color(226, 232, 240), "Fixed", 120, 160);
	AddTemplateNode(model, registry, "UiLabel", body, "Main content", White(), Color(226, 232, 240), "Expand");
	AddTemplateNode(model, registry, "UiLabel", body, "Right widgets", Color(248, 250, 252), Color(226, 232, 240), "Fixed", 120, 160);
	AddTemplateNode(model, registry, "UiLabel", root, "Footer", Color(124, 58, 237), Color(124, 58, 237), "Fixed", 600, 40);
	model.SelectOne(root);
}

static void BuildMagazineTemplate(DesignerModel& model, const DesignerRegistry& registry)
{
	DesignerNodeId root = AddTemplateNode(model, registry, "BoxLayout", Designer_ROOT, "magazineColumn");
	AddTemplateNode(model, registry, "UiTitleCard", root, "Magazine Header", Color(6, 182, 212), Color(8, 145, 178), "Fixed", 600, 48);
	AddTemplateNode(model, registry, "UiTitleCard", root, "Featured Hero", Color(245, 158, 11), Color(217, 119, 6), "Fixed", 600, 70);
	DesignerNodeId grid = AddTemplateNode(model, registry, "GridLayout", root, "storyGrid", Color(248, 250, 252), Color(226, 232, 240), "Expand");
	for(int i = 1; i <= 4; i++)
		AddTemplateNode(model, registry, "UiLabel", grid, Format("Card %d", i), White(), Color(226, 232, 240), "Fit", 120, 60);
	AddTemplateNode(model, registry, "UiLabel", root, "Footer", Color(139, 92, 246), Color(168, 85, 247), "Fixed", 600, 40);
	model.SelectOne(root);
}

static void BuildSpaTemplate(DesignerModel& model, const DesignerRegistry& registry)
{
	DesignerNodeId root = AddTemplateNode(model, registry, "BoxLayout", Designer_ROOT, "spaColumn");
	AddTemplateNode(model, registry, "UiTitleCard", root, "Navigation", Color(239, 68, 68), Color(220, 38, 38), "Fixed", 600, 48);
	for(String section : { "Home", "Profile", "Settings" })
		AddTemplateNode(model, registry, "UiTitleCard", root, section, White(), Color(226, 232, 240), "Fit", 500, 70);
	model.SelectOne(root);
}

static void BuildCardGridTemplate(DesignerModel& model, const DesignerRegistry& registry)
{
	DesignerNodeId root = AddTemplateNode(model, registry, "BoxLayout", Designer_ROOT, "cardGridColumn");
	AddTemplateNode(model, registry, "UiTitleCard", root, "Header", Color(71, 85, 105), Color(51, 65, 85), "Fixed", 600, 48);
	DesignerNodeId grid = AddTemplateNode(model, registry, "GridLayout", root, "cards", Color(248, 250, 252), Color(226, 232, 240), "Expand");
	model.Find(grid)->properties.Set("columns", 3);
	for(int i = 1; i <= 6; i++)
		AddTemplateNode(model, registry, "UiLabel", grid, Format("Card %d", i), White(), Color(226, 232, 240), "Fit", 110, 70);
	AddTemplateNode(model, registry, "UiLabel", root, "Footer", Color(146, 64, 14), Color(180, 83, 9), "Fixed", 600, 40);
	model.SelectOne(root);
}

static void BuildSplitScreenTemplate(DesignerModel& model, const DesignerRegistry& registry)
{
	DesignerNodeId root = AddTemplateNode(model, registry, "BoxLayout", Designer_ROOT, "splitColumn");
	AddTemplateNode(model, registry, "UiTitleCard", root, "Shared Header", Color(16, 185, 129), Color(5, 150, 105), "Fixed", 600, 48);
	DesignerNodeId row = AddTemplateNode(model, registry, "BoxLayout", root, "splitBody", Color(248, 250, 252), Color(226, 232, 240), "Expand");
	model.Find(row)->properties.Set("direction", "H");
	AddTemplateNode(model, registry, "UiLabel", row, "Left Panel", Color(240, 253, 244), Color(226, 232, 240), "Expand");
	AddTemplateNode(model, registry, "UiLabel", row, "Right Panel", Color(254, 243, 199), Color(226, 232, 240), "Expand");
	AddTemplateNode(model, registry, "UiLabel", root, "Footer", Color(16, 185, 129), Color(5, 150, 105), "Fixed", 600, 40);
	model.SelectOne(root);
}

static void BuildFPatternTemplate(DesignerModel& model, const DesignerRegistry& registry)
{
	DesignerNodeId root = AddTemplateNode(model, registry, "BoxLayout", Designer_ROOT, "fPatternColumn");
	AddTemplateNode(model, registry, "UiTitleCard", root, "Top Navigation Bar", Color(59, 130, 246), Color(29, 78, 216), "Fixed", 600, 48);
	DesignerNodeId metrics = AddTemplateNode(model, registry, "BoxLayout", root, "metricRow", Color(219, 234, 254), Color(219, 234, 254), "Fixed", 600, 48);
	model.Find(metrics)->properties.Set("direction", "H");
	for(String item : { "Key Metric 1", "Key Metric 2", "CTA Button" })
		AddTemplateNode(model, registry, "UiLabel", metrics, item, White(), Color(219, 234, 254), "Expand");
	DesignerNodeId body = AddTemplateNode(model, registry, "BoxLayout", root, "bodyRow", White(), Color(226, 232, 240), "Expand");
	model.Find(body)->properties.Set("direction", "H");
	AddTemplateNode(model, registry, "UiLabel", body, "Primary List", Color(248, 250, 252), Color(226, 232, 240), "Fixed", 200, 180);
	AddTemplateNode(model, registry, "UiLabel", body, "Secondary Content", White(), Color(226, 232, 240), "Expand");
	AddTemplateNode(model, registry, "UiLabel", root, "Footer", Color(59, 130, 246), Color(29, 78, 216), "Fixed", 600, 40);
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
	else
		return false;
	return true;
}

}
